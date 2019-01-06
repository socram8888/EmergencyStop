/*
 * EmergencyStop.c
 *
 * Created: 05/01/2019 15:39:28
 * Author : Marcos
 */ 

#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <stdbool.h>
#include <string.h>
#include <util/delay.h>
#include "vusb/usbconfig.h"
#include "vusb/usbdrv.h"

const PROGMEM char usbHidReportDescriptor[] = {
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x04,                    // USAGE (Joystick)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x85, 0x01,                    //     REPORT_ID (1)
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x09, 0x01,                    //     USAGE (Button 1)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x95, 0x07,                    //     REPORT_COUNT (7)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x81, 0x03,                    //     INPUT (Cnst,Var,Abs)
    0x85, 0x02,                    //     REPORT_ID (2)
    0x06, 0x00, 0xff,              //     USAGE_PAGE (Vendor Defined Page 1)
    0x09, 0x01,                    //     USAGE (Vendor Usage 1)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,              //     LOGICAL_MAXIMUM (255)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x04,                    //     REPORT_COUNT (4)
    0xb1, 0x02,                    //   FEATURE (Data,Var,Abs)
    0xc0                           // END_COLLECTION
};

_Static_assert(
		sizeof(usbHidReportDescriptor) == USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH,
		"Invalid HID report descriptor length"
);

uint8_t ledStatus = 0;
uint8_t idleRate = 500 / 4;
uint8_t ticksUntilResend = 0;
uint8_t debounceTicks = 0;
bool sendReport = false;
bool updateSerial = false;

#define REPORT_INPUT  0x0100
#define REPORT_OUTPUT 0x0200

#define REPORT_BUTTON 1
#define REPORT_SERIAL 2

struct {
	uint8_t reportId;
	uint8_t buttons;
} buttonReport;

struct {
	uint8_t reportId;
	uint8_t serial[4];
} serialReport;
uint8_t serialReportPos;

#define DEBOUNCE_TICKS 5

#define BUTTON_DDR PORTB
#define BUTTON_PORT PORTB
#define BUTTON_PIN PINB
#define BUTTON_BIT 0

void inline disableTimerInt() {
	TIMSK &= ~_BV(OCIE0A);
}

void inline enableTimerInt() {
	TIMSK |= _BV(OCIE0A);
}

void setupTimer0() {
	// Clear registers
	TCCR0A = 0;
	TCCR0B = 0;
	TCNT0 = 0;

	// 125.885009765625 Hz (16500000/((127+1)*1024))
	OCR0A = 127;
	// CTC
	TCCR0A |= _BV(WGM01);
	// Prescaler 1024
	TCCR0B |= _BV(CS02) | _BV(CS00);
	// Output Compare Match A Interrupt Enable
	enableTimerInt();
}

int main(void) {
	wdt_enable(WDTO_1S);

	// Configure button input
	BUTTON_DDR &= ~_BV(BUTTON_BIT);
	BUTTON_PORT |= _BV(BUTTON_BIT);

	// Initialize button report
	buttonReport.reportId = REPORT_BUTTON;

	// Initialize serial report
	serialReport.reportId = REPORT_SERIAL;
	eeprom_read_block(serialReport.serial, 0x0000, 4);

	// Initialize USB
    usbInit();

	// Force USB re-enumeration
    usbDeviceDisconnect();
	_delay_ms(250);
    usbDeviceConnect();

	// Setup timer 0
	setupTimer0();

	// Enable interrupts now
	sei();

    while (1) {
        wdt_reset();
        usbPoll();

		if (sendReport && usbInterruptIsReady()) {
			sendReport = false;
			buttonReport.buttons = debounceTicks > 0 ? 0x01 : 0x00;
			usbSetInterrupt((uchar *) &buttonReport, sizeof(buttonReport));
		}

		if (updateSerial && eeprom_is_ready()) {
			updateSerial = false;
			eeprom_update_block(serialReport.serial, 0x0000, 4);
		}
    }
}

usbMsgLen_t usbFunctionSetup(uchar data[8]) {
	usbRequest_t* rq = (void*) data;

	if ((rq->bmRequestType & USBRQ_TYPE_MASK) != USBRQ_TYPE_CLASS) {
		return 0;
	}

	switch (rq->bRequest) {
		case USBRQ_HID_GET_REPORT:
			switch (rq->wValue.bytes[0]) {
				case REPORT_BUTTON:
					if (rq->wLength.word != sizeof(buttonReport)) {
						return 0;
					}
					
					usbMsgPtr = (uchar *) &buttonReport;
					return sizeof(buttonReport);

				case REPORT_SERIAL:
					if (rq->wLength.word != sizeof(serialReport)) {
						return 0;
					}

					usbMsgPtr = (uchar *) &serialReport;
					return sizeof(serialReport);
			}

			return 0;

		case USBRQ_HID_SET_REPORT:
			if (rq->wValue.bytes[0] == REPORT_SERIAL && rq->wLength.word == sizeof(serialReport)) {
				serialReportPos = 0;
				return USB_NO_MSG;
			}

			return 0;

		case USBRQ_HID_GET_IDLE:
			usbMsgPtr = &idleRate;
			return 1;

		case USBRQ_HID_SET_IDLE:
			idleRate = rq->wValue.bytes[1];
			return 0;
	}

	return 0;
}

uchar usbFunctionWrite(uchar *data, uchar len) {
	memcpy((uint8_t *) &serialReport + serialReportPos, data, len);
	serialReportPos += len;

	if (serialReportPos == sizeof(serialReport)) {
		updateSerial = true;
		return 1;
	}

	return 0;
}

ISR(TIMER0_COMPA_vect) {
	// Disable timer 0 interrupts
	disableTimerInt();

	// Enable global interrupts
	sei();

	if (!(BUTTON_PIN & _BV(BUTTON_BIT))) {
		if (debounceTicks == 0) {
			sendReport = true;
		}
		debounceTicks = DEBOUNCE_TICKS;
	} else if (debounceTicks > 0) {
		debounceTicks--;
		if (debounceTicks == 0) {
			sendReport = true;
		}
	}

	if (!sendReport && idleRate > 0) {
		// Idle rate considers one tick to be 4ms instead of our 8ms
		ticksUntilResend += 2;
		if (ticksUntilResend >= idleRate) {
			ticksUntilResend = 0;
			sendReport = true;
		}
	}

	// Re-enable timer interrupts
	enableTimerInt();
}
