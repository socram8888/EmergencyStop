/*
 * EmergencyStop.c
 *
 * Created: 05/01/2019 15:39:28
 * Author : Marcos
 */ 

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <assert.h>
#include <util/delay.h>
#include <stdbool.h>
#include "vusb/usbconfig.h"
#include "vusb/usbdrv.h"

const PROGMEM char usbHidReportDescriptor[] = {
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,                    // USAGE (Keyboard)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
    0x19, 0xe0,                    //   USAGE_MINIMUM (Keyboard LeftControl)
    0x29, 0xe7,                    //   USAGE_MAXIMUM (Keyboard Right GUI)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x08,                    //   REPORT_COUNT (8)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x81, 0x03,                    //   INPUT (Cnst,Var,Abs)
    0x95, 0x05,                    //   REPORT_COUNT (5)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x05, 0x08,                    //   USAGE_PAGE (LEDs)
    0x19, 0x01,                    //   USAGE_MINIMUM (Num Lock)
    0x29, 0x05,                    //   USAGE_MAXIMUM (Kana)
    0x91, 0x02,                    //   OUTPUT (Data,Var,Abs)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x03,                    //   REPORT_SIZE (3)
    0x91, 0x03,                    //   OUTPUT (Cnst,Var,Abs)
    0x95, 0x06,                    //   REPORT_COUNT (6)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x65,                    //   LOGICAL_MAXIMUM (101)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
    0x19, 0x00,                    //   USAGE_MINIMUM (Reserved (no event indicated))
    0x29, 0x65,                    //   USAGE_MAXIMUM (Keyboard Application)
    0x81, 0x00,                    //   INPUT (Data,Ary,Abs)
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

uint8_t report[6];

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

	// Initialize USB
    usbInit();

	// Force USB re-enumeration
    usbDeviceDisconnect();
	_delay_ms(250);
    usbDeviceConnect();

	// Configure button input
	BUTTON_PORT &= ~_BV(BUTTON_BIT);
	BUTTON_PORT |= _BV(BUTTON_BIT);

	// Setup timer 0
	setupTimer0();

	// Enable interrupts now
	sei();

    while (1) {
        wdt_reset();
        usbPoll();

		if (sendReport && usbInterruptIsReady()) {
			sendReport = false;
			ticksUntilResend = 0;

			// Reserved byte
			report[1] = 0x00;

			// We'll be pressing only a single key, so set the rest of the array to zero
			report[3] = report[4] = report[5] = 0x00;

			if (debounceTicks > 0) {
				report[2] = 0x0F;
			} else {
				report[2] = 0x00;
			}
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
			usbMsgPtr = report;
			return 6;

		case USBRQ_HID_SET_REPORT:
			if (rq->wLength.word == 1) {
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
	idleRate = data[0];
	return 1;
}

ISR(TIMER0_COMPA_vect) {
	// Disable timer 0 interrupts
	disableTimerInt();

	// Enable global interrupts
	sei();

	if (!(BUTTON_PIN & _BV(BUTTON_BIT))) {
		debounceTicks = DEBOUNCE_TICKS;
	} else if (debounceTicks > 0) {
		debounceTicks--;
		if (debounceTicks == 0) {
			sendReport = true;
		}
	}

	if (idleRate > 0 && debounceTicks > 0) {
		// Idle rate considers one tick to be 4ms instead of our 8ms
		ticksUntilResend += 2;
		if (ticksUntilResend >= idleRate) {
			sendReport = true;
		}
	}

	// Re-enable timer interrupts
	enableTimerInt();
}
