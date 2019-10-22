#include <stddef.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>

#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/hid.h>

#include "usb.h"

static usbd_device *usbd_dev;

const struct usb_device_descriptor dev_descr = {
	.bLength = USB_DT_DEVICE_SIZE,
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = 0x0200,
	.bDeviceClass = 0,
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	.bMaxPacketSize0 = 64,
	.idVendor = 0x0483,
	.idProduct = 0x5710,
	.bcdDevice = 0x0200,
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 3,
	.bNumConfigurations = 1,
};

static const uint8_t hid_report_descriptor[] = {
    0x05, 0x01,     /* USAGE_PAGE (Generic Desktop)           */
    0x09, 0x06,     /* USAGE (Keyboard)                       */
    0xa1, 0x01,     /* COLLECTION (Application)               */
    0x05, 0x07,     /*   USAGE_PAGE (Keyboard)                */
    0x19, 0xe0,     /*   USAGE_MINIMUM (Keyboard LeftControl) */
    0x29, 0xe7,     /*   USAGE_MAXIMUM (Keyboard Right GUI)   */
    0x15, 0x00,     /*   LOGICAL_MINIMUM (0)                  */
    0x25, 0x01,     /*   LOGICAL_MAXIMUM (1)                  */
    0x75, 0x01,     /*   REPORT_SIZE (1)                      */
    0x95, 0x08,     /*   REPORT_COUNT (8)                     */
    0x81, 0x02,     /*   INPUT (Data,Var,Abs)                 */
    0x95, 0x01,     /*   REPORT_COUNT (1)                     */
    0x75, 0x08,     /*   REPORT_SIZE (8)                      */
    0x81, 0x03,     /*   INPUT (Cnst,Var,Abs)                 */
    0x95, 0x05,     /*   REPORT_COUNT (5)                     */
    0x75, 0x01,     /*   REPORT_SIZE (1)                      */
    0x05, 0x08,     /*   USAGE_PAGE (LEDs)                    */
    0x19, 0x01,     /*   USAGE_MINIMUM (Num Lock)             */
    0x29, 0x05,     /*   USAGE_MAXIMUM (Kana)                 */
    0x91, 0x02,     /*   OUTPUT (Data,Var,Abs)                */
    0x95, 0x01,     /*   REPORT_COUNT (1)                     */
    0x75, 0x03,     /*   REPORT_SIZE (3)                      */
    0x91, 0x03,     /*   OUTPUT (Cnst,Var,Abs)                */
    0x95, 0x06,     /*   REPORT_COUNT (6)                     */
    0x75, 0x08,     /*   REPORT_SIZE (8)                      */
    0x15, 0x00,     /*   LOGICAL_MINIMUM (0)                  */
    0x25, 0x65,     /*   LOGICAL_MAXIMUM (101)                */
    0x05, 0x07,     /*   USAGE_PAGE (Keyboard)                */
    0x19, 0x00,     /*   USAGE_MINIMUM (Reserved)             */
    0x29, 0x65,     /*   USAGE_MAXIMUM (Keyboard Application) */
    0x81, 0x00,     /*   INPUT (Data,Ary,Abs)                 */
    0xc0            /* END_COLLECTION                         */
};

static const struct {
	struct usb_hid_descriptor hid_descriptor;
	struct {
		uint8_t bReportDescriptorType;
		uint16_t wDescriptorLength;
	} __attribute__((packed)) hid_report;
} __attribute__((packed)) hid_function = {
	.hid_descriptor = {
		.bLength = sizeof(hid_function),
		.bDescriptorType = USB_DT_HID,
		.bcdHID = 0x0100,
		.bCountryCode = 0,
		.bNumDescriptors = 1,
	},
	.hid_report = {
		.bReportDescriptorType = USB_DT_REPORT,
		.wDescriptorLength = sizeof(hid_report_descriptor),
	}
};

const struct usb_endpoint_descriptor hid_endpoint = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x81,
	.bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
	.wMaxPacketSize = 8,
	.bInterval = 0x20,
};

const struct usb_interface_descriptor hid_iface = {
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 0,
	.bAlternateSetting = 0,
	.bNumEndpoints = 1,
	.bInterfaceClass = USB_CLASS_HID,
	.bInterfaceSubClass = 1, /* boot */
	.bInterfaceProtocol = 1, /* keyboard */
	.iInterface = 0,

	.endpoint = &hid_endpoint,

	.extra = &hid_function,
	.extralen = sizeof(hid_function),
};


const struct usb_interface ifaces[] = {{
	.num_altsetting = 1,
	.altsetting = &hid_iface,
}};

const struct usb_config_descriptor config = {
	.bLength = USB_DT_CONFIGURATION_SIZE,
	.bDescriptorType = USB_DT_CONFIGURATION,
	.wTotalLength = 0,
	.bNumInterfaces = 1,
	.bConfigurationValue = 1,
	.iConfiguration = 0,
	.bmAttributes = 0xC0,
	.bMaxPower = 0x32,

	.interface = ifaces,
};

static const char *usb_strings[] = {
	"TinyPilot",
	"Keyboard",
	"v1.0",
};


/* Buffer to be used for control requests. */
uint8_t usbd_control_buffer[128];

static enum usbd_request_return_codes hid_control_request(usbd_device *dev, struct usb_setup_data *req, uint8_t **buf, uint16_t *len,
			void (**complete)(usbd_device *, struct usb_setup_data *))
{
	(void)complete;
	(void)dev;

	if((req->bmRequestType != 0x81) ||
	   (req->bRequest != USB_REQ_GET_DESCRIPTOR) ||
	   (req->wValue != 0x2200))
		return USBD_REQ_NOTSUPP;

	/* Handle the HID report descriptor. */
	*buf = (uint8_t *)hid_report_descriptor;
	*len = sizeof(hid_report_descriptor);

	return USBD_REQ_HANDLED;
}



static void hid_set_config(usbd_device *dev, uint16_t wValue)
{
	(void)wValue;
	(void)dev;

	usbd_ep_setup(dev, 0x81, USB_ENDPOINT_ATTR_INTERRUPT, 4, NULL);

	usbd_register_control_callback(
				dev,
				USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_INTERFACE,
				USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
				hid_control_request);
}


void usb_init(void) {

	rcc_periph_clock_enable(RCC_GPIOA);
        rcc_periph_clock_enable(RCC_USB);

	gpio_clear(GPIOA, GPIO12);
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ,
		GPIO_CNF_OUTPUT_OPENDRAIN, GPIO12);
	/*
	 * This is a somewhat common cheap hack to trigger device re-enumeration
	 * on startup.  Assuming a fixed external pullup on D+, (For USB-FS)
	 * setting the pin to output, and driving it explicitly low effectively
	 * "removes" the pullup.  The subsequent USB init will "take over" the
	 * pin, and it will appear as a proper pullup to the host.
	 * The magic delay is somewhat arbitrary, no guarantees on USBIF
	 * compliance here, but "it works" in most places.
	 */
	for (unsigned i = 0; i < 800000; i++) {
	 	__asm__("nop");
        }

	usbd_dev = usbd_init(&st_usbfs_v1_usb_driver, &dev_descr, &config,
                             usb_strings, 3, usbd_control_buffer,
                             sizeof(usbd_control_buffer));
	usbd_register_set_config_callback(usbd_dev, hid_set_config);

        for (;;) {
            usbd_poll(usbd_dev);
        }
}


void usb_write_key(usb_kbd_packet_t *packet) {
    usbd_ep_write_packet(usbd_dev, 0x81, packet->data, sizeof(packet->data));
}
