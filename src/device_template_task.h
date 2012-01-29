/*This file is prepared for Doxygen automatic documentation generation.*/
/*! \file ******************************************************************
 *
 * \brief Management of the USB high-level applicative device task.
 *
 * This file manages the USB high-level applicative device task.
 *
 * - Compiler:           IAR EWAVR32 and GNU GCC for AVR32
 * - Supported devices:  All AVR32 devices with a USB module can be used.
 * - AppNote:
 *
 * \author               Atmel Corporation: http://www.atmel.com \n
 *                       Support and FAQ: http://support.atmel.no/
 *
 ***************************************************************************/

/* Copyright (c) 2009 Atmel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an Atmel
 * AVR product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE
 *
 */

#ifndef _DEVICE_TEMPLATE_TASK_H_
#define _DEVICE_TEMPLATE_TASK_H_


//_____ I N C L U D E S ____________________________________________________

#include "conf_usb.h"
#include "usb_descriptors.h"


//_____ D E F I N I T I O N S ______________________________________________
extern struct tDataflashOTP dataflashOTP;


//_____ M A C R O S ________________________________________________________


//_____ D E C L A R A T I O N S ____________________________________________

extern void device_template_task_init(void);
extern void device_template_task(void *pvParameters);

// Module info commands
#define USB_CMD_REQ_APPL_VER			0x00
#define USB_CMD_REQ_HARDWARE_VER		0x01
#define USB_CMD_REQ_SERIAL_NUMBER		0x02
#define USB_CMD_REQ_TESTER_ID			0x03
#define USB_CMD_REQ_BATTINFO			0x04

// Commands to interact with the dataflash
#define USB_CMD_READ_USERPREFS			0x10
#define USB_CMD_READ_SAVEDTRACKS		0x11
#define USB_CMD_READ_RECORDTABLE		0x12
#define USB_CMD_READ_RECORDDATA			0x13
#define USB_CMD_ERASE_USERPREFS			0x14
#define USB_CMD_ERASE_SAVEDTRACKS		0x15
#define USB_CMD_ERASE_RECORDTABLE		0x16
#define USB_CMD_ERASE_RECORDDATA		0x17
#define USB_CMD_WRITE_USERPREFS			0x18
#define USB_CMD_WRITE_SAVEDTRACKS		0x19
#define USB_CMD_WRITE_RECORDTABLE		0x1A
#define USB_CMD_WRITE_RECORDDATA		0x1B
#define USB_CMD_READ_OTP				0x1C
#define USB_CMD_WRITE_OTP				0x1D

// Debug Commands
#define USB_DBG_SEND_DF_CMD				0x30	// Send a command to the dataflash
#define USB_DBG_READ_PM_PGOOD1			0x31	// Read the PM_PGOOD1 for the power supply
#define USB_DBG_READ_PM_PGOOD3			0x32	// Read the PM_PGOOD3 for the power supply
#define USB_DBG_READ_ADC_VCC			0x33	// Read the ADC value for VCC
#define USB_DBG_READ_ADC_VEE			0x34	// Read the ADC value for VEE
#define USB_DBG_READ_ADC_3V3			0x35	// Read the ADC value for 3V3
#define USB_DBG_SEND_GPS_CMD			0x36	// Send a command to the GPS
#define USB_DBG_READ_CHG_STAT			0x37	// Read the status for the charging
#define USB_DBG_SEND_FUEL_CMD			0x38	// Send a command to the fuel gauge


#define INDEX_CMD						0
#define INDEX_LENGTH					1
#define INDEX_DATA						2




#endif  // _DEVICE_TEMPLATE_TASK_H_
