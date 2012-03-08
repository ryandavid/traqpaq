/******************************************************************************
 *
 * GPS Interface
 *
 * - Compiler:          GNU GCC for AVR32
 * - Supported devices: traq|paq hardware version 1.1
 * - AppNote:			N/A
 *
 * - Last Author:		Ryan David ( ryan.david@redline-electronics.com )
 *
 *
 * Copyright (c) 2012 Redline Electronics LLC.
 *
 * This file is part of traq|paq.
 *
 * traq|paq is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * traq|paq is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with traq|paq. If not, see http://www.gnu.org/licenses/.
 *
 ******************************************************************************/
#include "asf.h"
#include "hal.h"
#include "dataflash/dataflash_manager_request.h"
#include "math.h"

xQueueHandle gpsRxdQueue;
xQueueHandle gpsManagerQueue;

unsigned char rxBuffer[GPS_MSG_MAX_STRLEN];
unsigned char gpsTokens[MAX_SIGNALS_SENTENCE];

xTimerHandle xMessageTimer;

__attribute__((__interrupt__)) static void ISR_gps_rxd(void){
	int rxd;
	usart_read_char(GPS_USART, &rxd);
	xQueueSendFromISR(gpsRxdQueue, &rxd, pdFALSE);
}


void gps_task_init( void ){
	struct tGPSRequest request;
	
	gpsRxdQueue		= xQueueCreate( GPS_RXD_QUEUE_SIZE,     sizeof(int)     );
	gpsManagerQueue = xQueueCreate( GPS_MANAGER_QUEUE_SIZE, sizeof(request) );

	INTC_register_interrupt(&ISR_gps_rxd, AVR32_USART3_IRQ, AVR32_INTC_INT0);
	
	xTaskCreate(gps_task, configTSK_GPS_TASK_NAME, configTSK_GPS_TASK_STACK_SIZE, NULL, configTSK_GPS_TASK_PRIORITY, configTSK_GPS_TASK_HANDLE);
}


void gps_task( void *pvParameters ){
	unsigned int rxdChar;										// Temporary storage for received character queue
	unsigned char rxIndex = 0;									// Index in received character buffer
	
	unsigned char processedRMC = FALSE, processedGGA = FALSE;	// Flags for processed NMEA messages
	
	unsigned char recordFlag = FALSE;
	
	unsigned char processChecksum = FALSE;
	unsigned short calculatedChecksum = 0;
	
	unsigned char recordIndex = 0;								// Index in formatted data struct
	
	signed int oldLatitude = 0;									// Previous position update latitude and longitude
	signed int oldLongitude = 0;
	
	unsigned char oldMode = 0;
	
	unsigned int lapTime, oldLapTime;
	
	struct tRecordDataPage gpsData;							// Formatted GPS Data
	struct tGPSLine finishLine;								// Formatted coordinate pairs for "finish line"
	struct tTracklist trackList;
	
	struct tGPSRequest request;
	
	debug_log(DEBUG_PRIORITY_INFO, DEBUG_SENDER_GPS, "Task Started");

	xMessageTimer = xTimerCreate( "gpsMessageTimer", GPS_MSG_TIMEOUT * portTICK_RATE_MS, pdFALSE, NULL, gps_messageTimeout );
	
	// Pull the GPS out of reset and enable the ISR
	gps_enable_interrupts();
	gps_reset();
	
	while(TRUE){
		// Check for pending requests
		if( xQueueReceive(gpsManagerQueue, &request, pdFALSE) == pdTRUE ){
			switch(request.command){
				case(GPS_REQUEST_START_RECORDING):
					lapTime = 0;
					oldLapTime = 0xFFFFFFFF;
					recordFlag = TRUE;
					break;
					
				case(GPS_REQUEST_STOP_RECORDING):
					recordFlag = FALSE;
					recordIndex = 0;
					dataflash_send_request(DFMAN_REQUEST_END_CURRENT_RECORD, NULL, NULL, NULL, FALSE, 20);
					break;
					
				case(GPS_SET_FINISH_POINT):
					// Load the track, and then tell the dataflash that we are using it
					dataflash_send_request(DFMAN_REQUEST_READ_TRACK, &trackList, sizeof(trackList), request.data, TRUE, 20);
					dataflash_send_request(DFMAN_REQUEST_SET_TRACK, NULL, NULL, request.data, FALSE, 20);
					finishLine = gps_find_finish_line(trackList.latitude, trackList.longitude, trackList.course);
					break;
			}
		}
		
		// Service the received characters
		xQueueReceive(gpsRxdQueue, &rxdChar, portMAX_DELAY);
		
		if( rxdChar == GPS_MSG_END_CHAR ){
			// Reset the time since receiving a message
			xTimerReset(xMessageTimer, pdFALSE);
				
			if( gps_received_checksum() == calculatedChecksum ){
					
				gps_buffer_tokenize();
				
				//--------------------------
				// GGA Message Received
				//--------------------------
				if( (rxBuffer[gpsTokens[TOKEN_MESSAGE_ID] + MESSAGE_OFFSET_ID0] == ID_GGA_ID0) &
					(rxBuffer[gpsTokens[TOKEN_MESSAGE_ID] + MESSAGE_OFFSET_ID1] == ID_GGA_ID1) &
					(rxBuffer[gpsTokens[TOKEN_MESSAGE_ID] + MESSAGE_OFFSET_ID2] == ID_GGA_ID2) ){
					
					// Convert Time!
					gpsData.data[recordIndex].latitude	= atoi( &(	rxBuffer[gpsTokens[TOKEN_GGA_LATITUDE		]]) );
					gpsData.data[recordIndex].latitude	= gps_convert_to_decimal_degrees(gpsData.data[recordIndex].latitude);
					
					if( rxBuffer[gpsTokens[TOKEN_GGA_NORS]] == GPS_SOUTH){
						gpsData.data[recordIndex].latitude *= -1;
					}
					
					gpsData.data[recordIndex].longitude	= atoi( &(	rxBuffer[gpsTokens[TOKEN_GGA_LONGITUDE		]]) );
					gpsData.data[recordIndex].longitude = gps_convert_to_decimal_degrees(gpsData.data[recordIndex].longitude);
					
					if( rxBuffer[gpsTokens[TOKEN_GGA_EORW]] == GPS_WEST){
						gpsData.data[recordIndex].longitude *= -1;
					}
					
					gpsData.currentMode					= atoi( &(	rxBuffer[gpsTokens[TOKEN_GGA_QUALITY		]]) ) & 0xFFFF;
					gpsData.satellites					= atoi( &(	rxBuffer[gpsTokens[TOKEN_GGA_NUM_SATELLITES	]]) ) & 0xFF;
					gpsData.hdop						= atoi( &(	rxBuffer[gpsTokens[TOKEN_GGA_HDOP			]]) ) & 0xFFFF;
					gpsData.data[recordIndex].altitude	= atoi( &(	rxBuffer[gpsTokens[TOKEN_GGA_ALTITUDE		]]) ) & 0xFFFF;
					
					// Determine if a lap was detected!
					gpsData.data[recordIndex].lapDetected = gps_intersection(oldLongitude,							oldLatitude,
																			gpsData.data[recordIndex].longitude,    gpsData.data[recordIndex].latitude,
																			finishLine.startLongitude,				finishLine.startLatitude,
																			finishLine.endLongitude,				finishLine.endLatitude);
																			 
					if( gpsData.data[recordIndex].lapDetected && recordFlag){
						lcd_sendWidgetRequest(LCD_REQUEST_UPDATE_OLDLAPTIME, lapTime, pdFALSE);
						
						if(lapTime <= oldLapTime){
							lcd_sendWidgetRequest(LCD_REQUEST_UPDATE_PERIPHERIAL, LCD_PERIPHERIAL_FASTER, pdFALSE);
						}else{
							lcd_sendWidgetRequest(LCD_REQUEST_UPDATE_PERIPHERIAL, LCD_PERIPHERIAL_SLOWER, pdFALSE);
						}
						
						oldLapTime = lapTime;
						lapTime = 0;
					}
					
					// Update the antenna widget
					if(oldMode != gpsData.currentMode){
						lcd_sendWidgetRequest(LCD_REQUEST_UPDATE_ANTENNA, gpsData.currentMode, pdFALSE);
						oldMode = gpsData.currentMode;
					}
					
					// Save the last coordinates for detecting the intersection
					oldLongitude = gpsData.data[recordIndex].longitude;
					oldLatitude = gpsData.data[recordIndex].latitude;
						
					processedGGA = TRUE;
					
				}
				//--------------------------
				// RMC Message Received
				//--------------------------
				if( (rxBuffer[gpsTokens[TOKEN_MESSAGE_ID] + MESSAGE_OFFSET_ID0] == ID_RMC_ID0) &
					(rxBuffer[gpsTokens[TOKEN_MESSAGE_ID] + MESSAGE_OFFSET_ID1] == ID_RMC_ID1) &
					(rxBuffer[gpsTokens[TOKEN_MESSAGE_ID] + MESSAGE_OFFSET_ID2] == ID_RMC_ID2) ){
						
					// More Converting!!
					gpsData.data[recordIndex].utc		= atoi( &(	rxBuffer[gpsTokens[TOKEN_RMC_UTC	]]) );
					gpsData.data[recordIndex].speed		= atoi( &(	rxBuffer[gpsTokens[TOKEN_RMC_SPEED	]]) ) & 0xFFFF;
					gpsData.data[recordIndex].course	= atoi( &(	rxBuffer[gpsTokens[TOKEN_RMC_TRACK	]]) ) & 0xFFFF;
					
					gpsData.date						= atoi( &(	rxBuffer[gpsTokens[TOKEN_RMC_DATE	]]) );
					
					processedRMC = TRUE;

				
				}else
				//--------------------------
				// PMTK001 Message Received
				//--------------------------
				if( (rxBuffer[gpsTokens[TOKEN_MESSAGE_ID] + MESSAGE_OFFSET_ID0] == ID_MTK001_ID0) &
					(rxBuffer[gpsTokens[TOKEN_MESSAGE_ID] + MESSAGE_OFFSET_ID1] == ID_MTK001_ID1) &
					(rxBuffer[gpsTokens[TOKEN_MESSAGE_ID] + MESSAGE_OFFSET_ID2] == ID_MTK001_ID2) &
					(rxBuffer[gpsTokens[TOKEN_MESSAGE_ID] + MESSAGE_OFFSET_ID3] == ID_MTK001_ID3) &
					(rxBuffer[gpsTokens[TOKEN_MESSAGE_ID] + MESSAGE_OFFSET_ID4] == ID_MTK001_ID4) ){
						
					switch( rxBuffer[gpsTokens[TOKEN_PMTK001_FLAG]] ){
						case(PMTK001_INVALID_CMD):
							debug_log(DEBUG_PRIORITY_WARNING, DEBUG_SENDER_GPS, "Invalid Command");
							break;
							
						case(PMTK001_UNSUPPORTED_CMD):
							debug_log(DEBUG_PRIORITY_WARNING, DEBUG_SENDER_GPS, "Unsupported Command");
							break;
							
						case(PMTK001_VALID_CMD_FAILED):
							debug_log(DEBUG_PRIORITY_WARNING, DEBUG_SENDER_GPS, "Command Failed");
							break;
							
						case(PMTK001_VALID_CMD):
							debug_log(DEBUG_PRIORITY_INFO, DEBUG_SENDER_GPS, "Command Succeeded");
							break;
					}
					
				}
				
				
				if(processedGGA && processedRMC){
					processedGGA = FALSE;
					processedRMC = FALSE;
						
					if(recordFlag){
						// Update lap time counter;
						lcd_sendWidgetRequest(LCD_REQUEST_UPDATE_LAPTIME, lapTime++, pdFALSE);
						
						recordIndex++;
						if(recordIndex == RECORD_DATA_PER_PAGE){
							dataflash_send_request(DFMAN_REQUEST_ADD_RECORDDATA, &gpsData, sizeof(gpsData), NULL, TRUE, 20);
							recordIndex = 0;
						}
						
					}
				}  // ProcessedGGA and ProcessedRMC
				
			}else{
				// Invalid CRC received!
				debug_log(DEBUG_PRIORITY_WARNING, DEBUG_SENDER_GPS, "Invalid Checksum Received");
			}				
			
			// Reset the buffer and checksum
			rxIndex = 0;
			calculatedChecksum = 0;

		}else{
			
			//--------------------------
			// Keep a running CRC calculation
			//--------------------------
			if( rxdChar == GPS_CHECKSUM_CHAR ){
				processChecksum = FALSE;
			}
			
			if( processChecksum ){
				calculatedChecksum ^= rxdChar;
			}
			
			if( rxdChar == GPS_MSG_START_CHAR ){
				processChecksum = TRUE;
			}
			
			//--------------------------
			// Store the received character (skip periods!)
			//--------------------------
			if( (rxIndex < GPS_MSG_MAX_STRLEN) && (rxdChar != GPS_PERIOD) ){
				rxBuffer[rxIndex++] = (rxdChar & 0xFF);
			}
						
		}
									
	}		
}


void gps_reset( void ){
	gpio_clr_gpio_pin(GPS_RESET);
	vTaskDelay( (portTickType)TASK_DELAY_MS(GPS_RESET_TIME) );
	gpio_set_gpio_pin(GPS_RESET);
	vTaskDelay( (portTickType)TASK_DELAY_MS(GPS_RESET_TIME) );
}


void gps_buffer_tokenize( void ){
	unsigned char index = 0;
	unsigned char signalPosIndex = 0;

	// Find the start of the NMEA sentence
	while( (index < GPS_MSG_MAX_STRLEN) && (rxBuffer[index++] != GPS_MSG_START_CHAR) );
	
	// Store the location!
	gpsTokens[signalPosIndex++] = index;
				
	// Replace commas with null!
	while(index <= GPS_MSG_MAX_STRLEN){
		if(rxBuffer[index] == GPS_DELIMITER_CHAR){
			rxBuffer[index] = GPS_NULL;
			gpsTokens[signalPosIndex++] = index + 1;
		}
		index++;
	}
	
}


unsigned short gps_received_checksum( void ){
	unsigned char index = 0;
	
	// Find the start of the NMEA checksum
	while( (index < GPS_MSG_MAX_STRLEN) && (rxBuffer[index] != GPS_CHECKSUM_CHAR) ){
		index++;
	}
	
	// Skip the GPS_CHECKSUM_CHAR
	index++;
				
	// Convert the received checksum
	if(rxBuffer[index] >= 'A'){
		rxBuffer[index] += 10 - 'A';
	}else{
		rxBuffer[index] -= '0';
	}
				
	if(rxBuffer[index+1] >= 'A'){
		rxBuffer[index+1] += 10 - 'A';
	}else{
		rxBuffer[index+1] -= '0';
	}
				
	return ((rxBuffer[index] & 0xF) << 4) + (rxBuffer[index+1] & 0xF);
}


unsigned char gps_intersection(signed int x1, signed int y1, signed int x2, signed int y2, signed int x3, signed int y3, signed int x4, signed int y4){
	// (x1, y1) and (x2, y2) are points for line along traveled path
    // (x3, y3) and (y4, x3) are points for the threshold line
	
	// Temporary storage
	float ua, ub, denominator;
	
	// Calculate the denominator
    denominator = (x2 - x1)*(y4 - y3) - (y2 - y1)*(x4 - x3);
    
    ua = ((x4 - x3) * (y1 - y3) - (y4 - y3) * (x1 - x3)) / denominator;
    ub = ((x2 - x1) * (y1 - y3) - (y2 - y1) * (x1 - x3)) / denominator;

    // Calulate the point of intersection
    //long = x1+ua*(x2 - x1);
    //lat = y1+ua*(y2 - y1);
	
	if( (ua >= 0) && (ua <= 1) && (ub >= 0) && (ub <= 1) ){
		return true;
	}
	
	return false;
}

signed int gps_convert_to_decimal_degrees(signed int coordinate){
	unsigned int minutes;
	signed int degrees;
	
	minutes = (coordinate % 1000000);
	degrees = coordinate - minutes;
	
	degrees += ((minutes * 10) / 6);
	
	return degrees;
}

struct tGPSLine gps_find_finish_line(signed int latitude, signed int longitude, unsigned short heading){
	float angle;
	int	vect;
	
	struct tGPSLine finish;
	
	// Add 90 degrees to heading, make sure it is between 0 and 360,
	// and finally shift the decimal back in
	angle = deg2rad(((heading + 900) % 3600) / 10);
	
	// Copy the heading on the point to the heading for the line
	finish.heading = heading;
	
	// Project the THRESHOLD_DISTANCE along the perpendicular angle
	vect = sin(angle) * THRESHOLD_DISTANCE * 60 * 1000000;
	finish.startLongitude	= longitude + vect;
	finish.endLongitude		= longitude - vect;
	
	vect = cos(angle) * THRESHOLD_DISTANCE * 60 * 1000000;
	finish.startLatitude	= latitude + vect;
	finish.endLatitude		= latitude - vect;
	
	return finish;
}

void gps_set_messaging_rate(unsigned char rate){
	
	switch(rate){
		case(GPS_MESSAGING_100MS):
			usart_write_line(GPS_USART, "$PMTK300,100,0,0,0,0*2C");
			break;
			
		case(GPS_MESSAGING_200MS):
			usart_write_line(GPS_USART, "$PMTK300,200,0,0,0,0*2F");
			break;
			
		case(GPS_MESSAGING_500MS):
			usart_write_line(GPS_USART, "$PMTK300,500,0,0,0,0*28");
			break;
			
		case(GPS_MESSAGING_1000MS):
			usart_write_line(GPS_USART, "$PMTK300,1000,0,0,0,0*1C");
			break;
	}
	
	usart_putchar(GPS_USART, GPS_MSG_CR);
	usart_putchar(GPS_USART, GPS_MSG_END_CHAR);
	
}

void gps_set_messages( void ){
	// Enable GGA and RMC messages only
	usart_write_line(GPS_USART, "$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28");
	usart_putchar(GPS_USART, GPS_MSG_CR);
	usart_putchar(GPS_USART, GPS_MSG_END_CHAR);
}

void gps_send_request(unsigned char command, unsigned int *pointer, unsigned char data, unsigned char delay){
	struct tGPSRequest request;
	request.command = command;
	request.pointer = pointer;
	request.data = data;
	
	xQueueSend(gpsManagerQueue, &request, delay);
}

void gps_messageTimeout( void ){
	debug_log(DEBUG_PRIORITY_CRITICAL, DEBUG_SENDER_GPS, "Message Timer Expired");
	debug_log(DEBUG_PRIORITY_WARNING, DEBUG_SENDER_GPS, "Setting Message Rate");
	gps_set_messaging_rate(GPS_MESSAGING_100MS);
}