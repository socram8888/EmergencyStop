/*
 * EmergencyStop.c
 *
 * Created: 05/01/2019 15:39:28
 * Author : Marcos
 */ 

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <stdbool.h>
#include <util/delay.h>
#include "vusb/usbconfig.h"
#include "vusb/usbdrv.h"

const PROGMEM char usbHidReportDescriptor[] = {
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x04,                    // USAGE (Joystick)
    0xa1, 0x01,                    // COLLECTION (Application)
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

uint8_t buttonReport[1];

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
			buttonReport[0] = debounceTicks > 0 ? 0x01 : 0x00;
			usbSetInterrupt((uchar *) &buttonReport, sizeof(buttonReport));
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
			if (rq->wLength.word == sizeof(buttonReport)) {
				usbMsgPtr = (uchar *) &buttonReport;
				return sizeof(buttonReport);
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
