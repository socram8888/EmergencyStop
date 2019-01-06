
#ifndef __usbconfig_h_included__
#define __usbconfig_h_included__

/* ---------------------------- Hardware Config ---------------------------- */

// USB pins
#define USB_CFG_IOPORTNAME B
#define USB_CFG_DMINUS_BIT 3
#define USB_CFG_DPLUS_BIT  4

// USB pin interrupts
#define USB_INTR_CFG         PCMSK
#define USB_INTR_CFG_SET     (1 << USB_CFG_DPLUS_BIT)
#define USB_INTR_ENABLE_BIT  PCIE
#define USB_INTR_PENDING_BIT PCIF
#define USB_INTR_VECTOR      SIG_PIN_CHANGE

// 16.5MHz master clock
#define USB_CFG_CLOCK_KHZ (F_CPU / 1000)

/* --------------------------- Functional Range ---------------------------- */

// Enable interrupt in for sending data.
#define USB_CFG_HAVE_INTRIN_ENDPOINT 1

// Set poll speed to 10ms
#define USB_CFG_INTR_POLL_INTERVAL 10

// USB is not self-powered
#define USB_CFG_IS_SELF_POWERED 0

// About 30mA of power consumption
#define USB_CFG_MAX_BUS_POWER 30

// Implement usbFunctionWrite for replying to requests
#define USB_CFG_IMPLEMENT_FN_WRITE 1

/* -------------------------- Device Description --------------------------- */

// Shared USB joystick PID for serial discrimination,
// from Van Ooijen Technische Informatica
#define USB_CFG_VENDOR_ID 0xC0, 0x16
#define USB_CFG_DEVICE_ID 0xDC, 0x27

// Device version: 1.0
#define USB_CFG_DEVICE_VERSION 0x00, 0x01

// Device serial
#define USB_CFG_SERIAL_NUMBER 'o', 'r', 'c', 'a', '.', 'p', 'e', 't', ':', 'E', 'm', 'e', 'r', 'g', 'e', 'n', 'c', 'y', ' ', 's', 't', 'o', 'p'
#define USB_CFG_SERIAL_NUMBER_LEN 23

// No device class - deferred to interface class
#define USB_CFG_DEVICE_CLASS    0
#define USB_CFG_DEVICE_SUBCLASS 0

// HID interface class
#define USB_CFG_INTERFACE_CLASS    0x03
#define USB_CFG_INTERFACE_SUBCLASS 0
#define USB_CFG_INTERFACE_PROTOCOL 0

// HID report descriptor length
#define USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH 47

#endif
