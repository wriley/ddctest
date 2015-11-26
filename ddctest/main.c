/* Name: main.c
 * Project: hid-custom-rq example
 * Author: Christian Starkjohann
 * Creation Date: 2008-04-07
 * Tabsize: 4
 * Copyright: (c) 2008 by OBJECTIVE DEVELOPMENT Software GmbH
 * License: GNU GPL v2 (see License.txt), GNU GPL v3 or proprietary (CommercialLicense.txt)
 */

/*
This example should run on most AVRs with only little changes. No special
hardware resources except INT0 are used. You may have to change usbconfig.h for
different I/O pins for USB. Please note that USB D+ must be the INT0 pin, or
at least be connected to INT0 as well.
We assume that an LED is connected to port B bit 0. If you connect it to a
different port or bit, change the macros below:
*/
#define LED_PORT_DDR        DDRB
#define LED_PORT_OUTPUT     PORTB
#define LED_BIT             0

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>  /* for sei() */
#include <util/delay.h>     /* for _delay_ms() */

#include <avr/pgmspace.h>   /* required by usbdrv.h */
#include "usbdrv.h"
#include "oddebug.h"        /* This is also an example for using debug macros */
#include "requests.h"       /* The custom request numbers we use */

/* ------------------------------------------------------------------------- */
/* ----------------------------- USB interface ----------------------------- */
/* ------------------------------------------------------------------------- */

PROGMEM const char usbHidReportDescriptor[54] = {   /* USB report descriptor */
    0x06, 0x00, 0xFF,  // Usage Page (Vendor Defined 0xFF00)
    0x09, 0x01,        // Usage (0x01)
    0xA1, 0x01,        // Collection (Application)
    0x05, 0x08,        //   Usage Page (LEDs)
    0x09, 0x52,        //   Usage (0x52)
    0xA1, 0x02,        //   Collection (Logical)
    0x19, 0x01,        //     Usage Minimum (Num Lock)
    0x29, 0x14,        //     Usage Maximum (CAV)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x01,        //     Logical Maximum (1)
    0x85, 0x01,        //     Report ID (1)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x08,        //     Report Count (8)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              //   End Collection
    0x05, 0x08,        //   Usage Page (LEDs)
    0x09, 0x52,        //   Usage (0x52)
    0xA1, 0x02,        //   Collection (Logical)
    0x19, 0x01,        //     Usage Minimum (Num Lock)
    0x29, 0x14,        //     Usage Maximum (CAV)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x01,        //     Logical Maximum (1)
    0x85, 0x02,        //     Report ID (2)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x08,        //     Report Count (8)
    0x91, 0x02,        //     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,              //   End Collection
    0xC0              // End Collection
};

static uchar	reportOK[8] = { 0x10, 0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00 };
static uchar	reportVer[8] = { 0x30, 0x30, 0x30, 0x32, 0x30, 0x32, 0x30, 0x30 };
static uchar	currentCmd[9];
static uchar    currentAddress;
static uchar    bytesRemaining;

/* ------------------------------------------------------------------------- */

/* usbFunctionRead() is called when the host requests a chunk of data from
 * the device. For more information see the documentation in usbdrv/usbdrv.h.
 */
uchar   usbFunctionRead(uchar *data, uchar len)
{
	//TODO
		
	uchar i;
	DBG1(0x80, currentCmd, 9);
	
	if(len == 1){
		data[0] = 0x00;
		return 1;
	}
	
	if(currentCmd[0] == 0x02){
		switch(currentCmd[1]){
			case 0x0b:
				for(i = 0; i < 8; i++){
					data[i] = reportVer[i];
				}
				break;
			default:
				for(i = 0; i < 8; i++){
					data[i] = reportOK[i];
				}
				break;
		}
	}
	DBG1(0x81, data, len);
    return len;
}

/* usbFunctionWrite() is called when the host sends a chunk of data to the
 * device. For more information see the documentation in usbdrv/usbdrv.h.
 */
uchar   usbFunctionWrite(uchar *data, uchar len)
{
	uchar i;
	
	//TODO
	DBG1(0x90, data, len);
	for(i = 0; i < len; i++){
		currentCmd[currentAddress] = data[i];
		currentAddress++;
		bytesRemaining--;
	}
	return bytesRemaining == 0; /* return 1 if this was the last chunk */
}

usbMsgLen_t usbFunctionSetup(uchar data[8])
{
usbRequest_t    *rq = (void *)data;
	DBG1(0x40, data, 8);

    if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS){
        if(rq->bRequest == USBRQ_HID_GET_REPORT){
			return USB_NO_MSG;  /* use usbFunctionRead() to obtain data */
			//usbMsgPtr = (void *)&reportOK;
			//return sizeof(reportOK);
        }else if(rq->bRequest == USBRQ_HID_SET_REPORT){
			currentAddress = 0;
			bytesRemaining = rq->wLength.bytes[0];
			return USB_NO_MSG;  /* use usbFunctionWrite() to receive data from host */
        }
    }else{
		DBG1(0x60, 0, 1);
    }
    return 0;   /* default for not implemented requests: return no data back to host */
}

/* ------------------------------------------------------------------------- */

int main(void)
{
uchar   i;

    wdt_enable(WDTO_1S);
    /* Even if you don't use the watchdog, turn it off here. On newer devices,
     * the status of the watchdog (on/off, period) is PRESERVED OVER RESET!
     */
    /* RESET status: all port bits are inputs without pull-up.
     * That's the way we need D+ and D-. Therefore we don't need any
     * additional hardware initialization.
     */
    odDebugInit();
    DBG1(0x00, 0, 0);       /* debug output: main starts */
    usbInit();
    usbDeviceDisconnect();  /* enforce re-enumeration, do this while interrupts are disabled! */
    i = 0;
    while(--i){             /* fake USB disconnect for > 250 ms */
        wdt_reset();
        _delay_ms(1);
    }
    usbDeviceConnect();
    LED_PORT_DDR |= _BV(LED_BIT);   /* make the LED bit an output */
    sei();
    DBG1(0x01, 0, 0);       /* debug output: main loop starts */
    for(;;){                /* main event loop */
#if 0   /* this is a bit too aggressive for a debug output */
        DBG2(0x02, 0, 0);   /* debug output: main loop iterates */
#endif
        wdt_reset();
        usbPoll();
    }
}

/* ------------------------------------------------------------------------- */
