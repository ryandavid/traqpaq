/**
 * \file
 *
 * \brief USB configuration file
 *
 * Copyright (c) 2011 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an Atmel
 *    AVR product.
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
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

#ifndef _CONF_USB_H_
#define _CONF_USB_H_

#include "compiler.h"
#include "hal.h"

/**
 * USB Device Configuration
 * @{
 */

//! Device definition (mandatory)
#define  USB_DEVICE_VENDOR_ID             0xAAAA
#define  USB_DEVICE_PRODUCT_ID            0x1000
#define  USB_DEVICE_MAJOR_VERSION         TRAQPAQ_SW_LEVEL_MAJOR
#define  USB_DEVICE_MINOR_VERSION         TRAQPAQ_SW_LEVEL_MINOR
#define  USB_DEVICE_POWER                 500 // Consumption on Vbus line (mA)
#define  USB_DEVICE_ATTR                  USB_CONFIG_ATTR_SELF_POWERED


//! USB Device string definitions (Optional)
#define  USB_DEVICE_MANUFACTURE_NAME			"Redline Electronics"
#define  USB_DEVICE_PRODUCT_NAME				"traq|paq"
#define  USB_DEVICE_SERIAL_NAME					// Leave Empty
#define	 USB_DEVICE_GET_SERIAL_NAME_POINTER		dataflashOTP.serial
#define  USB_DEVICE_GET_SERIAL_NAME_LENGTH		OTP_SERIAL_LENGTH


/**
 * Device speeds support
 * Low speed not supported by this vendor class
 * @{
 */
//! To authorize the High speed
#if (UC3A3||UC3A4)
# define  USB_DEVICE_HS_SUPPORT
#endif
//@}


/**
 * USB Device Callbacks definitions (Optional)
 * @{
 */
#define  UDC_VBUS_EVENT(b_vbus_high)      main_vbus_action(b_vbus_high)
//#define  UDC_SOF_EVENT()                  main_sof_action()
#define  UDC_SUSPEND_EVENT()              main_suspend_action()
#define  UDC_RESUME_EVENT()               main_resume_action()
//! Mandatory when USB_DEVICE_ATTR authorizes remote wakeup feature
// #define  UDC_REMOTEWAKEUP_ENABLE()        user_callback_remotewakeup_enable()
// #define  UDC_REMOTEWAKEUP_DISABLE()       user_callback_remotewakeup_disable()
//! When a extra string descriptor must be supported
//! other than manufacturer, product and serial string
// #define  UDC_GET_EXTRA_STRING()
//@}

//@}


/**
 * USB Interface Configuration
 * @{
 */

/**
 * Configuration of vendor interface
 * @{
 */
//! Interface callback definition
#define UDI_VENDOR_ENABLE_EXT()           main_vendor_enable()
#define UDI_VENDOR_DISABLE_EXT()          main_vendor_disable()
#define UDI_VENDOR_SETUP_OUT_RECEIVED()   main_setup_out_received()
#define UDI_VENDOR_SETUP_IN_RECEIVED()    main_setup_in_received()

//! Enpoints size for full speed
//! Note: Disable the enpoints of a type, if size equal 0
#define UDI_VENDOR_EPS_SIZE_INT_FS    64
#define UDI_VENDOR_EPS_SIZE_BULK_FS   64
#define UDI_VENDOR_EPS_SIZE_ISO_FS   64

//! Enpoints size for high speed
#define UDI_VENDOR_EPS_SIZE_INT_HS    64
#define UDI_VENDOR_EPS_SIZE_BULK_HS  512
#define UDI_VENDOR_EPS_SIZE_ISO_HS    64

//@}

//@}


/**
 * USB Device Driver Configuration
 * @{
 */
#define UDD_ISOCHRONOUS_NB_BANK 1
//@}

//! The includes of classes and other headers must be done
//! at the end of this file to avoid compile error
#include "udi_vendor_conf.h"

#endif // _CONF_USB_H_
