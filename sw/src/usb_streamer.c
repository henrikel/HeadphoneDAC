// Basic setup:
// 3 USB endpoints:
//   - 1 control endpoint
//   - 1 isochronous OUT endpoint (for audio data from host)
//   - 1 isochronous synch IN endpoint (for feedback of sample clock)
//
// Operation:
// Audio samples acquired from host through USB
// USB interrupt for each packet
// Packet data transferred to ring buffer by SW
// Two pointers into ring buffer: 
//   1 write pointer
//   1 read pointer
// After each new transfer from host, update write pointer
// Set up DMA to transfer data from ring buffer to I2S interface

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "arm_math.h"
#include "stm32f4xx.h"
#include "stm32.h"
#ifdef DEBUG
#include "usart.h"
#endif
#include "debounce.h"
#include "audio.h"
#include "usb.h"
#include "usb_hid.h"
#include "usb_audio.h"
#include "usb_streamer.h"

// USB related
#define UAC_EP0_SIZE	64
#define EP_OUT			0x01
#define EP_IN			0x82
#define EP_SIZE			(SAMPLES96000 * 3 * 2 + 6)

#define FB_RATE			2

// HID stuff
#define HID_RIN_EP      0x83
#define HID_RIN_SZ      0x10

#define AUDIO_SAMPLE_FREQ(frq) (uint8_t)(frq), (uint8_t)((frq >> 8)), (uint8_t)((frq >> 16))

typedef struct {
	uint8_t		fbTx;
	uint16_t	sofNum;
	uint32_t	fbDefault, fb, fbAcc;
	int			delta;
	int			rate;
} FeedbackData;

static const uint8_t hid_report_desc[] = {
    HID_USAGE_PAGE(0x0c), /* Consumer page */
    HID_USAGE(0x01),      /* Consumer control */
    HID_COLLECTION(HID_APPLICATION_COLLECTION),
    
    // Buttons
    HID_LOGICAL_MINIMUM(0),
    HID_LOGICAL_MAXIMUM(1),
    HID_REPORT_SIZE(1),
    HID_REPORT_COUNT(4),
    HID_USAGE(0xcd),     /* Usage (Play/Pause) */
    HID_USAGE(0xb5),     /* Usage (Scan Next Track) */
    HID_USAGE(0xb6),     /* Usage (Scan Previous Track) */
    HID_USAGE(0xe2),     /* Usage (Mute) */
    HID_INPUT(HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_RELATIVE | HID_IOF_PREFERRED_STATE),
    HID_REPORT_COUNT(4),     /*  Pad out to a byte */
    HID_INPUT(HID_IOF_CONSTANT),
    HID_END_COLLECTION
};

static struct {
    uint8_t     buttons;
} __attribute__((packed)) hid_report_data;

// Audio device configuration descriptor
struct audio_config {
	struct usb_config_descriptor			config;
	struct usb_iad_descriptor				iad;
	struct usb_interface_descriptor			audio_interface;
	struct usb_audio_header_desc			ac_interface;
	struct usb_audio_input_terminal_desc	input_terminal;
	struct usb_audio_feature_unit_desc		audio_feature;
	struct usb_audio_output_terminal_desc	output_terminal;
	struct usb_audio_as_std_int_desc		as_std_interface0;
	struct usb_audio_as_std_int_desc		as_std_interface1;
	struct usb_audio_as_spec_int_desc		as_spec_interface1;
	struct usb_audio_as_formatI_int_desc	as1;
	struct usb_audio_as_iso_std_endp_desc	ep1;
	struct usb_audio_as_iso_spec_endp_desc	ep1_as;
	struct usb_audio_as_iso_synch_endp_desc	ep2;
	
	struct usb_interface_descriptor			hid_interface;
	struct usb_hid_descriptor				hid_desc;
	struct usb_endpoint_descriptor			hid_epIn;
	
} __attribute__((packed));

volatile FeedbackData	fbData;
usbd_device 			udev;
uint32_t				ubuf[0x20];
uint8_t					tmpBuf[EP_SIZE*2];
int						playing;
#ifdef DEBUG
int						debugCount = 0;
#endif

// Device descriptor
static const struct usb_device_descriptor	device_desc = {
	.bLength			= sizeof(struct usb_device_descriptor),
	.bDescriptorType	= USB_DTYPE_DEVICE,
	.bcdUSB				= VERSION_BCD(2,0,0),
	.bDeviceClass		= USB_CLASS_IAD,//USB_CLASS_PER_INTERFACE,
	.bDeviceSubClass	= USB_SUBCLASS_IAD,//USB_SUBCLASS_NONE,
	.bDeviceProtocol	= USB_PROTO_IAD,//USB_PROTO_NONE,
	.bMaxPacketSize0	= UAC_EP0_SIZE,
	.idVendor			= 0x0483,
	.idProduct			= 0x5730,
	.bcdDevice			= VERSION_BCD(1,0,0),
	.iManufacturer		= 1,
	.iProduct			= 2,
	.iSerialNumber		= INTSERIALNO_DESCRIPTOR,
	.bNumConfigurations	= 1,
};

// Device configuration descriptor
static const struct audio_config	config_desc = {
	.config = { // Configuration 1
		.bLength				= sizeof(struct usb_config_descriptor),
    	.bDescriptorType		= USB_DTYPE_CONFIGURATION,
    	.wTotalLength			= sizeof(struct audio_config),
    	.bNumInterfaces			= 3,
    	.bConfigurationValue	= 1,
    	.iConfiguration			= NO_DESCRIPTOR,
    	.bmAttributes			= USB_CFG_ATTR_RESERVED | USB_CFG_ATTR_SELFPOWERED,
    	.bMaxPower				= USB_CFG_POWER_MA(500),
    },
    .iad = {
    	.bLength				= sizeof(struct usb_iad_descriptor),
    	.bDescriptorType		= USB_DTYPE_INTERFACEASSOC,
    	.bFirstInterface		= 0,
    	.bInterfaceCount		= 3,
    	.bFunctionClass			= USB_CLASS_AUDIO,
    	.bFunctionSubClass		= USB_AUDIO_SUBCLASS_AUDIOCONTROL,
    	.bFunctionProtocol		= USB_AUDIO_PROTO_UNDEFINED,
    	.iFunction				= 0,
    },
    .audio_interface = { // Standard Audio Control interface descriptor
    	.bLength				= sizeof(struct usb_interface_descriptor),
    	.bDescriptorType		= USB_DTYPE_INTERFACE,
    	.bInterfaceNumber		= 0,
    	.bAlternateSetting		= 0,
    	.bNumEndpoints			= 0,
    	.bInterfaceClass		= USB_CLASS_AUDIO,
    	.bInterfaceSubClass		= USB_AUDIO_SUBCLASS_AUDIOCONTROL,
    	.bInterfaceProtocol		= USB_AUDIO_PROTO_UNDEFINED,
    	.iInterface				= 0,
    },
    .ac_interface = { // Class-specific Audio Control interface descriptor 
    	.bLength				= sizeof(struct usb_audio_header_desc),
		.bDescriptorType		= USB_AUDIO_CS_INTERFACE,
		.bDescriptorSubtype		= USB_AUDIO_HEADER,
		.bcdADC					= VERSION_BCD(1,0,0),
		.wTotalLength			= sizeof(struct usb_audio_header_desc) +
								  sizeof(struct usb_audio_input_terminal_desc) +
								  sizeof(struct usb_audio_feature_unit_desc) +
								  sizeof(struct usb_audio_output_terminal_desc),
		.bInCollection			= 1,
		.baInterFaceNr			= {1},
    },
    .input_terminal = { // USB speaker input terminal descriptor
    	.bLength				= sizeof(struct usb_audio_input_terminal_desc),
    	.bDescriptorType		= USB_AUDIO_CS_INTERFACE,
    	.bDescriptorSubtype		= USB_AUDIO_INPUT_TERMINAL,
    	.bTerminalID			= 1,
    	.wTerminalType			= USB_AUDIO_TERMINAL_TYPE_STREAMING,
    	.bAssocTerminal			= 0,
    	.bNrChannels			= 2,
    	.wChannelConfig			= 3,
    	.iChannelNames			= 0,
    	.iTerminal				= 0,
    },
    .audio_feature = { // Audio feature unit descriptor
    	.bLength				= sizeof(struct usb_audio_feature_unit_desc),
    	.bDescriptorType		= USB_AUDIO_CS_INTERFACE,
    	.bDescriptorSubtype		= USB_AUDIO_FEATURE_UNIT,
    	.bUnitID				= 2,
    	.bSourceID				= 1,
    	.bControlSize			= 2,
    	.bmaControls			= {1, 0, 0},
    	.iFeature				= 0,
    },
    .output_terminal = { // USB speaker output terminal descriptor
    	.bLength				= sizeof(struct usb_audio_output_terminal_desc),
    	.bDescriptorType		= USB_AUDIO_CS_INTERFACE,
    	.bDescriptorSubtype		= USB_AUDIO_OUTPUT_TERMINAL,
    	.bTerminalID			= 3,
    	.wTerminalType			= USB_AUDIO_TERMINAL_TYPE_HEADPHONE,
    	.bAssocTerminal			= 0,
    	.bSourceID				= 2,
    	.iTerminal				= 0,
    },
	.as_std_interface0 = { // Standard AS interface descriptor, interface 1, alternate setting 0
	                   // Zero bandwidth, zero endpoints. Used when no audio is playing
		.bLength				= sizeof(struct usb_audio_as_std_int_desc),
		.bDescriptorType		= USB_DTYPE_INTERFACE,
		.bInterfaceNumber		= 1,
		.bAlternateSetting		= 0,
		.bNumEndpoints			= 0,
		.bInterfaceClass		= USB_CLASS_AUDIO,
		.bInterfaceSubClass		= USB_AUDIO_SUBCLASS_AUDIOSTREAMING,
		.bInterfaceProtocol		= USB_AUDIO_PROTO_UNDEFINED,
		.iInterface				= 0,
	},
	.as_std_interface1 = { // Standard AS interface descriptor, alternate setting 1
	                   // Used when audio is playing
		.bLength				= sizeof(struct usb_audio_as_std_int_desc),
		.bDescriptorType		= USB_DTYPE_INTERFACE,
		.bInterfaceNumber		= 1,
		.bAlternateSetting		= 1,
		.bNumEndpoints			= 2,
		.bInterfaceClass		= USB_CLASS_AUDIO,
		.bInterfaceSubClass		= USB_AUDIO_SUBCLASS_AUDIOSTREAMING,
		.bInterfaceProtocol		= USB_AUDIO_PROTO_UNDEFINED,
		.iInterface				= 0,
	},
	.as_spec_interface1 = { // USB speaker Audio streaming interface descriptor
		.bLength				= sizeof(struct usb_audio_as_spec_int_desc),
		.bDescriptorType		= USB_AUDIO_CS_INTERFACE,
		.bDescriptorSubtype		= USB_AUDIO_AS_GENERAL,
		.bTerminalLink			= 1,
		.bDelay					= 1,
		.wFormatTag				= USB_AUDIO_DATA_FORMAT_PCM,
	},
	.as1 = { // USB speaker Audio type I format interface descriptor
		.bLength				= sizeof(struct usb_audio_as_formatI_int_desc),
		.bDescriptorType		= USB_AUDIO_CS_INTERFACE,
		.bDescriptorSubtype		= USB_AUDIO_FORMAT_TYPE,
		.bFormatType			= USB_AUDIO_FORMAT_TYPE_I,
		.bNrChannels			= 2,
		.bSubFrameSize			= 3,
		.bBitResolution			= 24,
		.bSamFreqType			= 4,
		.tSamFreq				= {{AUDIO_SAMPLE_FREQ(44100)},
								   {AUDIO_SAMPLE_FREQ(48000)},
								   {AUDIO_SAMPLE_FREQ(88200)},
								   {AUDIO_SAMPLE_FREQ(96000)}},
	},
	.ep1 = { // Endpoint 1 standard descriptor
		.bLength				= sizeof(struct usb_audio_as_iso_std_endp_desc),
		.bDescriptorType		= USB_DTYPE_ENDPOINT,
		.bEndpointAddress		= EP_OUT,
		.bmAttributes			= 0x05, // 0b00000101, asynchronous isochronous
		.wMaxPacketSize			= 582, // 2 * 3 * 96 + 6 = 582. 3 bytes per channel, 96 kHz plus one extra sample
		.bInterval				= 1,
		.bRefresh				= 0,
		.bSynchAddress			= EP_IN,
	},
	.ep1_as = { // Endpoint, Audio class specific streaming descriptor
		.bLength				= sizeof(struct usb_audio_as_iso_spec_endp_desc),
		.bDescriptorType		= USB_AUDIO_CS_ENDPOINT,
		.bDescriptorSubtype		= USB_AUDIO_EP_GENERAL,
		.bmAttributes			= 0x01,
		.bLockDelayUnits		= 0x00,
		.wLockDelay				= 0x0000,
	},
	.ep2 = { // Standard AS isochronous synch endpoint
		.bLength				= sizeof(struct usb_audio_as_iso_synch_endp_desc),
		.bDescriptorType		= USB_DTYPE_ENDPOINT,
		.bEndpointAddress		= EP_IN,
		.bmAttributes			= 0x11, // Isochronous feedback endpoint
		.wMaxPacketSize			= 0x0003,
		.bInterval				= 0x01,
		.bRefresh				= FB_RATE, // Power of 2. Refresh rate is 2^(10-8) = 2^2 = 4
		.bSynchAddress			= 0,
	},
	
	.hid_interface = { // Standard HID Control interface descriptor
    	.bLength				= sizeof(struct usb_interface_descriptor),
    	.bDescriptorType		= USB_DTYPE_INTERFACE,
    	.bInterfaceNumber		= 2,
    	.bAlternateSetting		= 0,
    	.bNumEndpoints			= 1,
    	.bInterfaceClass		= USB_CLASS_HID,
    	.bInterfaceSubClass		= USB_HID_SUBCLASS_NONBOOT,
    	.bInterfaceProtocol		= USB_HID_PROTO_NONBOOT,
    	.iInterface				= NO_DESCRIPTOR,
    },
    .hid_desc = { // HID descriptor
		.bLength				= sizeof(struct usb_hid_descriptor),
		.bDescriptorType		= USB_DTYPE_HID,
		.bcdHID					= VERSION_BCD(1,0,0),
		.bCountryCode			= 0,
		.bNumDescriptors		= 1,
		.bDescriptorType0		= USB_DTYPE_HID_REPORT,
		.wDescriptorLength0		= sizeof(hid_report_desc),
	},
	.hid_epIn = {
		.bLength				= sizeof(struct usb_endpoint_descriptor),
		.bDescriptorType		= USB_DTYPE_ENDPOINT,
		.bEndpointAddress		= HID_RIN_EP,
		.bmAttributes			= USB_EPTYPE_INTERRUPT,
		.wMaxPacketSize			= HID_RIN_SZ,
		.bInterval				= 10,
	},
};

static const struct usb_string_descriptor	lang_desc = USB_ARRAY_DESC(USB_LANGID_ENG_US);
static const struct usb_string_descriptor	manuf_desc_en = USB_STRING_DESC("Henrik Eliasson");
static const struct usb_string_descriptor	prod_desc_en = USB_STRING_DESC("USB DAC");
static const struct usb_string_descriptor	*const dtable[] = {
	&lang_desc,
	&manuf_desc_en,
	&prod_desc_en,
};

uint8_t set_current(usbd_device *dev, usbd_ctlreq *req);
uint8_t get_current(usbd_device *dev, usbd_ctlreq *req);
uint8_t get_max(usbd_device *dev, usbd_ctlreq *req);
uint8_t get_min(usbd_device *dev, usbd_ctlreq *req);
uint8_t get_res(usbd_device *dev, usbd_ctlreq *req);

void SetFsLED(void) {
	
	switch(audioSettings.sampling_frequency) {
		case 44100:
			GPIOB->BSRR |= GPIO_BSRR_BR6;
			GPIOB->BSRR |= GPIO_BSRR_BS5;
			break;
		case 48000:
			GPIOB->BSRR |= GPIO_BSRR_BS6;
			GPIOB->BSRR |= GPIO_BSRR_BS5;
			break;
		case 88200:
			GPIOB->BSRR |= GPIO_BSRR_BS6;
			GPIOB->BSRR |= GPIO_BSRR_BR5;
			break;
		case 96000:
			GPIOB->BSRR |= GPIO_BSRR_BR6;
			GPIOB->BSRR |= GPIO_BSRR_BR5;
			break;
	}
}

inline static uint32_t clamp(uint32_t val, uint32_t lower, uint32_t upper) {
		
	return val < lower ? lower : (val > upper ? upper : val);
}

void reset_fb_data(AudioSettings audioSettings) {
	
	switch(audioSettings.sampling_frequency) {
		case 44100:
			fbData.fbDefault = (uint32_t) round(44.108 * 16384);
			break;
		case 48000:
			fbData.fbDefault = (uint32_t) round(47.991 * 16384);
			break;
		case 88200:
			fbData.fbDefault = (uint32_t) round(88.216 * 16384);
			break;
		case 96000:	
			fbData.fbDefault = (uint32_t) round(95.982 * 16384);
			break;
		default:
			;
	}
	fbData.fbTx = 0;
	fbData.sofNum = 0;
	fbData.fb = fbData.fbDefault;
	fbData.fbAcc = 0;
	fbData.delta = 0;
	fbData.rate = 1 << FB_RATE;
}

void OTG_FS_IRQHandler(void) {
    usbd_poll(&udev);
}

//static USB_OTG_DeviceTypeDef * const OTGD = (void*)(USB_OTG_FS_PERIPH_BASE + USB_OTG_DEVICE_BASE);
inline static USB_OTG_INEndpointTypeDef* EPIN(uint32_t ep) {
    return (void *)(USB_OTG_FS_PERIPH_BASE + USB_OTG_IN_ENDPOINT_BASE + (ep << 5));
}

static void send_feedback(usbd_device *dev, uint32_t fb, int correction) {
	
	uint8_t		fbD[3];
	
	// Clamp feedback value to something appropriate 
	fb = clamp(fb + correction, fbData.fbDefault - 1024, fbData.fbDefault + 1024);
	
	fbD[0] = fb & 0xff;
	fbD[1] = (fb >> 8) & 0xff;
	fbD[2] = (fb >> 16) & 0xff;
			
	EPIN(EP_IN & 0x7f)->DIEPCTL |= (USB_OTG_DIEPCTL_EPDIS | USB_OTG_DIEPCTL_SNAK);
	(void) usbd_ep_write(dev, EP_IN, fbD, 3);
}

static void ept1_callback(usbd_device *dev, __attribute__((unused)) uint8_t event, uint8_t ep) {
	
	int			len, numSamples, i;
	uint32_t	sampleR, sampleL;
	
	if((ep == EP_OUT) && audioSettings.active) {
	
		usbd_toggle_sof(dev, EP_OUT);
		len = usbd_ep_read(dev, ep, tmpBuf, EP_SIZE); // Returns number of bytes read
		if(len <= EP_SIZE) {
			
			numSamples = len / 6; // 2 channels, 3 bytes in each. Total 6 bytes per sample
			
			for(i = 0; i < numSamples; ++i) {
			
				// The USB delivers 8 bit samples, so one audio sample consists of
				// 3 USB samples
				// Left first
				sampleL = audioSettings.mute ? 0 : tmpBuf[i * 6] | 
				                                   ((uint32_t)tmpBuf[i * 6 + 1] << 8) | 
				                                   ((uint32_t)tmpBuf[i * 6 + 2] << 16);
				sampleR = audioSettings.mute ? 0 : tmpBuf[i * 6 + 3] | 
				                                   ((uint32_t)tmpBuf[i * 6 + 4] << 8) | 
				                                   ((uint32_t)tmpBuf[i * 6 + 5] << 16);
			
				// The audio buffer consists of 16 bit samples
				// The I2S interface is a 16-bit register, so needs to be loaded
				// twice for samples larger than 16 bits. MSB first
				audio_buffer[(4 * i + audio_status.writePtr) % BUF_SIZE] = (sampleL >> 8) & 0xffff;
				audio_buffer[(4 * i + 1 + audio_status.writePtr) % BUF_SIZE] = (sampleL << 8) & 0xff00;
				audio_buffer[(4 * i + 2 + audio_status.writePtr) % BUF_SIZE] = (sampleR >> 8) & 0xffff;
				audio_buffer[(4 * i + 3 + audio_status.writePtr) % BUF_SIZE] = (sampleR << 8) & 0xff00;
			}
			audio_status.writePtr = (audio_status.writePtr + numSamples * 4) % BUF_SIZE;
			
			// Start playing after half the buffer is filled
			if(!audioSettings.playing && (audio_status.writePtr >= BUF_SIZE/2)) {
				audioSettings.playing = 1;
				SPI2->I2SCFGR |= SPI_I2SCFGR_I2SE;
			}
		}
	}
}

static void ept2_callback(__attribute__((unused)) usbd_device *dev, 
		                  __attribute__((unused))uint8_t event, uint8_t ep) {
	
	if(ep == EP_IN) {
		fbData.fbTx = 0;
		fbData.sofNum = 0;
	}
}

static void event_sof(usbd_device *dev, uint8_t event, 
                      __attribute__((unused)) uint8_t ep) {
	
	int			diff;
	
	if(event == usbd_evt_sof) {
		
		//frame = usbd_getframe(dev);
	
		if(audioSettings.active) {
		
			audio_status.readPtr = BUF_SIZE - (DMA1_Stream4->NDTR & 0xffff);
			if(audioSettings.playing)
				diff = audio_status.writePtr > audio_status.readPtr ? 
			           audio_status.writePtr - audio_status.readPtr :
			           audio_status.writePtr - audio_status.readPtr + BUF_SIZE;
			else
				diff = BUF_SIZE / 2;
			
			fbData.delta = (BUF_SIZE / 2 - diff) * 4;
			// if diff < half the buffer size, the I2S consumes less data
			// than the USB interface provides. We need to report a lower 
			// sampling frequency to the host
			
			// Feedback indicator LED
			if(audioSettings.playing && (abs(diff - BUF_SIZE / 2) > 16))
				GPIOC->BSRR |= GPIO_BSRR_BR13;
			else
				GPIOC->BSRR |= GPIO_BSRR_BS13;
		
			if(++fbData.sofNum == fbData.rate) {//if(++fbData.sofNum == (1<<FB_RATE)) {
				fbData.fb = (fbData.fbAcc + TIM2->CCR1) << fbData.rate;//fbData.fb = (fbData.fbAcc + TIM2->CCR1) << 4;
				send_feedback(dev, fbData.fb, fbData.delta);
				fbData.sofNum = 0;
				fbData.fbAcc = 0;
				fbData.fbTx = 1;
			}
			else
				fbData.fbAcc += TIM2->CCR1;
		}
	}
}

static void event_incompl(usbd_device *dev, uint8_t event, 
                          __attribute__((unused)) uint8_t ep) {

	switch(event) {
		case usbd_evt_incomplOUT:
			break;
		case usbd_evt_incomplIN:
			if(audioSettings.active && fbData.fbTx) {
				usbd_flush_tx(dev, EP_IN & 0x7f);
				send_feedback(dev, fbData.fb, fbData.delta);
				fbData.sofNum = 0;
				fbData.fbTx = 0;
			}
			break;
		default:
			;
	}

}

/* HID IN endpoint callback */
static void hid_eptIn(usbd_device *dev, __attribute__((unused)) uint8_t event, uint8_t ep) {
	
	hid_report_data.buttons = 0;
	
	switch(buttonReleased()) {
		case PLAY_BUTTON:
			// Play/pause
			hid_report_data.buttons |= 1;
			if(playing) {
				GPIOB->BSRR |= GPIO_BSRR_BR4;
				playing = 0;
			}
			else {
				GPIOB->BSRR |= GPIO_BSRR_BS4;
				playing = 1;
			}
			break;
		case SCANFORWARD_BUTTON:
			// Scan forward
			hid_report_data.buttons |= 2;
			break;
		case SCANBACK_BUTTON:
			// Scan backward
			hid_report_data.buttons |= 4;
			break;
		case MUTE_BUTTON:
			// Mute
			hid_report_data.buttons |= 8;
			break;
	}

    usbd_ep_write(dev, ep, &hid_report_data, sizeof(hid_report_data));
}

/*
Handle GET_DESC requests
*/
static usbd_respond uac_getdesc(usbd_ctlreq *req, void **address, uint16_t *length) {

	const uint8_t	dtype = req->wValue >> 8,
					dnumber = req->wValue & 0xff;
	const void		*desc;
	uint16_t		len = 0;
	int				result = usbd_fail;

	switch(dtype) {
		case USB_DTYPE_DEVICE:
			desc = &device_desc;
			len = sizeof(device_desc);
			result = usbd_ack;
			break;
		case USB_DTYPE_CONFIGURATION:
			desc = (void *)&config_desc;
			len = sizeof(struct audio_config);
			result = usbd_ack;
			break;
		case USB_DTYPE_STRING:
			if(dnumber < 3) {
				desc = dtable[dnumber];
				len = ((struct usb_string_descriptor *)desc)->bLength;
				result = usbd_ack;
			}
			break;
		case USB_AUDIO_CS_DEVICE:
			desc = (void *)&config_desc.ac_interface;
			len = config_desc.ac_interface.wTotalLength;
			result = usbd_ack;
			break;
		default:			
			;
	}
	if(len == 0)
		len = ((struct usb_header_descriptor *)desc)->bLength;
	
	*address = (void *)desc;
	*length = len;

	return result;
}

/*
Handle CONTROL requests
*/
static usbd_respond uac_control(usbd_device *dev, usbd_ctlreq *req, 
                                __attribute__((unused)) usbd_rqc_callback *callback) {
	
	uint8_t	result = usbd_fail;
	
	if(((USB_REQ_RECIPIENT | USB_REQ_TYPE) & req->bmRequestType) == (USB_REQ_INTERFACE | USB_REQ_CLASS)) {
		if(req->wIndex == 2) {
			// HID
			switch(req->bRequest) {
        		case USB_HID_SETIDLE:
            		result = usbd_ack;
            		break;
        		case USB_HID_GETREPORT:
            		dev->status.data_ptr = &hid_report_data;
            		dev->status.data_count = sizeof(hid_report_data);
            		result = usbd_ack;
            		break;
        		default:
            		;
        	}
		}
		else {
			// Audio
			switch(req->bRequest) {
				case GET_CUR:
					result = get_current(dev, req);
					break;
				case GET_MAX:
					result = get_max(dev, req);
					break;
				case GET_MIN:
					result = get_min(dev, req);
					break;
				case GET_RES:
					result = get_res(dev, req);
					break;
				case SET_CUR:
					result = set_current(dev, req);
					break;
				default:
					;
			}
		}
		
	}
	else if(((USB_REQ_RECIPIENT | USB_REQ_TYPE) & req->bmRequestType) == (USB_REQ_ENDPOINT | USB_REQ_CLASS)) {
		if(req->wIndex == EP_OUT) {
			switch(req->bRequest) {
				case GET_CUR:
				case GET_MAX:
				case GET_MIN:
				case GET_RES:
					break;
				case SET_CUR:
					result = set_current(dev, req);
					break;
				default:
					;
			}
		}
	}
	else if(((USB_REQ_RECIPIENT | USB_REQ_TYPE) & req->bmRequestType) == (USB_REQ_INTERFACE | USB_REQ_STANDARD)) {
		
		if((req->wIndex == 2) && (req->bRequest == USB_STD_GET_DESCRIPTOR)) {
			// HID
			switch((req->wValue >> 8) & 0xff) {
				case USB_DTYPE_HID:
            		dev->status.data_ptr = (uint8_t*)&(config_desc.hid_desc);
            		dev->status.data_count = sizeof(config_desc.hid_desc);
            		result = usbd_ack;
        		case USB_DTYPE_HID_REPORT:
            		dev->status.data_ptr = (uint8_t*)hid_report_desc;
            		dev->status.data_count = sizeof(hid_report_desc);
            		result = usbd_ack;
        		default:
            		;
        	}
		}
		else {
			// Audio
			switch(req->bRequest) {
				case USB_STD_SET_ADDRESS:
				case USB_STD_SET_CONFIG:
				case USB_STD_GET_STATUS:
				case USB_STD_GET_DESCRIPTOR:
					break;
				case USB_STD_GET_INTERFACE:
					if(req->wIndex == 1) {
						((uint8_t *)dev->status.data_ptr)[0] = (uint8_t)audioSettings.active;
						dev->status.data_count = 1;
						result = usbd_ack;
					}
					break;
				case USB_STD_SET_INTERFACE:
					// wIndex is interface number, wValue the alternate setting number
					if((req->wIndex == 1) && ((req->wValue == 0) || (req->wValue == 1))) {
						//usbd_ep_activate(dev, 1);
						if(req->wValue == 0) {
							DisableAudio();
							GPIOB->BSRR |= GPIO_BSRR_BR4;
							playing = 0;
						}
						else if(req->wValue == 1) {
							EnableAudio();
							GPIOB->BSRR |= GPIO_BSRR_BS4;
							playing = 1;
							//Test this: //send_feedback(dev, fbData.fb, 0);
						}
						audioSettings.active = req->wValue;
						audioSettings.playing = 0;
						usbd_flush_rx(dev);
						usbd_flush_tx(dev, EP_IN & 0x7f);
						reset_fb_data(audioSettings);
						
						// Feedback indicator light reset
						GPIOC->BSRR |= GPIO_BSRR_BS13;
						
						result = usbd_ack;
					}
					
					break;
				default:
					;
			}
		}
	}

	return result;
}

static usbd_respond uac_setconf(usbd_device *dev, uint8_t cfg) {
	
	int		result = usbd_fail;
	bool	res;

	switch(cfg) {
		case 0:
			// Deconfigure EP1 OUT
			usbd_ep_deconfig(dev, EP_OUT);
			usbd_reg_endpoint(dev, EP_OUT, 0);
			
			usbd_ep_deconfig(dev, EP_IN);
			usbd_ep_write(dev, EP_IN, 0, 0);
			
			usbd_ep_deconfig(dev, HID_RIN_EP);
        	usbd_reg_endpoint(dev, HID_RIN_EP, 0);
			
			result = usbd_ack;
			break;
		case 1:
			// Config
			// Configure EP1 OUT
    		res = usbd_ep_config(dev, EP_OUT, USB_EPTYPE_ISOCHRONOUS, EP_SIZE);
    		if(res)
    			usbd_reg_endpoint(dev, EP_OUT, ept1_callback);
    		
    		// Configure EP2 IN
    		if(res)
    			res = usbd_ep_config(dev, EP_IN, USB_EPTYPE_ISOCHRONOUS, 3);
    		if(res)
    			usbd_reg_endpoint(dev, EP_IN, ept2_callback);
    		
    		if(res)
    			usbd_ep_write(dev, EP_IN, 0, 0);
    		
    		if(res)	
    			res = usbd_ep_config(dev, HID_RIN_EP, USB_EPTYPE_INTERRUPT, HID_RIN_SZ);
    		if(res)
        		usbd_reg_endpoint(dev, HID_RIN_EP, hid_eptIn);
        	if(res)
        		usbd_ep_write(dev, HID_RIN_EP, 0, 0);
    		
    		if(res)
				result = usbd_ack;
			break;
		default:
			;
	}

	return result;
}

uint8_t set_current(usbd_device *dev, usbd_ctlreq *req) {
	
	uint8_t		cs = (req->wValue >> 8) & 0xff;
	int			result = usbd_fail, tmp;
	
	if(req->bmRequestType == 0x21) {
		// Feature unit SET
		switch(cs) {
			case 1:
				// Mute
				audioSettings.mute = req->data[0];
				if(audioSettings.mute)
					GPIOB->BSRR |= GPIO_BSRR_BS3;
				else
					GPIOB->BSRR |= GPIO_BSRR_BR3;
				result = usbd_ack;
				break;
			default:
				usbd_ep_stall(dev, 0);
		}
	}
	else if(req->bmRequestType == 0x22) {
		// Endpoint SET
		switch(cs) {
			case 1:
				// Sampling frequency control
				if(req->wIndex == 1) {
					tmp = (req->data[0]) | (req->data[1] << 8) | (req->data[2] << 16);
					if(audioSettings.sampling_frequency != tmp) {
						audioSettings.sampling_frequency = tmp;
						if(AudioReconfigure(audioSettings.sampling_frequency)) {
							SetFsLED();
							reset_fb_data(audioSettings);
							result = usbd_ack;
						}
					}
					else
						result = usbd_ack;
				}
				else
					usbd_ep_stall(dev, 0);
				break;
			default:
				;
		}
	}
	return result;
}

uint8_t get_current(usbd_device *dev, usbd_ctlreq *req) {
	
	uint8_t	cs = (req->wValue >> 8) & 0xff;
	int		result = usbd_fail;
	
	if(req->bmRequestType == 0xa1) {
		// Feature unit GET
		switch(cs) {
			case 1:
				// Mute
				dev->status.data_ptr = (uint8_t *)&audioSettings.mute;
				dev->status.data_count = 1;
				result = usbd_ack;
				break;
			default:
				;
		}
	}
	
	return result;
}

uint8_t get_max(__attribute__((unused)) usbd_device *dev, 
                __attribute__((unused)) usbd_ctlreq *req) {
	
	int			result = usbd_fail;

	return result;
}

uint8_t get_min(__attribute__((unused)) usbd_device *dev, 
                __attribute__((unused)) usbd_ctlreq *req) {
	
	int			result = usbd_fail;

	return result;
}

uint8_t get_res(__attribute__((unused)) usbd_device *dev, 
                __attribute__((unused)) usbd_ctlreq *req) {
	
	int			result = usbd_fail;

	return result;
}

void USBDeviceEnable(int enable) {
	
	usbd_connect(&udev, enable);
}

void USBDeviceInit(void) {

	int		tmpReg;
	
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
	
	TIM2->CR1 &= ~TIM_CR1_CEN;
	TIM2->CR1 &= ~(TIM_CR1_CKD | TIM_CR1_CMS | TIM_CR1_DIR);
	TIM2->CCER &= ~TIM_CCER_CC1E;
	
	tmpReg = TIM2->SMCR;
	
	tmpReg &= ~TIM_SMCR_SMS;
	tmpReg |= 4 << TIM_SMCR_SMS_Pos;
	
	tmpReg &= ~TIM_SMCR_TS;
	tmpReg |= 1 << TIM_SMCR_TS_Pos;
	
	tmpReg |= TIM_SMCR_ECE;
	TIM2->SMCR = tmpReg;
	
	tmpReg = TIM2->CCMR1;
	tmpReg &= ~TIM_CCMR1_IC1F;
	tmpReg &= ~TIM_CCMR1_IC1PSC;
	tmpReg &= ~TIM_CCMR1_CC1S;
	tmpReg |= (3 << TIM_CCMR1_CC1S_Pos);
	TIM2->CCMR1 = tmpReg;
	
	tmpReg = TIM2->CCER;
	tmpReg &= ~TIM_CCER_CC1P;
	tmpReg |= TIM_CCER_CC1E;
	TIM2->CCER = tmpReg;
	
	TIM2->OR &= ~TIM_OR_ITR1_RMP;
	TIM2->OR |= 2 << TIM_OR_ITR1_RMP_Pos;
	
	TIM2->CR1 |= TIM_CR1_CEN;
	
	audioSettings.sampling_frequency = 96000;
	audioSettings.bit_depth = 24;
	audioSettings.mute = 0;
	audioSettings.playing = 0;
	audioSettings.active = 0;
	
	playing = 0;
	
	reset_fb_data(audioSettings);
	
	usbd_init(&udev, &usbd_hw, UAC_EP0_SIZE, ubuf, sizeof(ubuf));
    usbd_reg_config(&udev, uac_setconf);
    usbd_reg_control(&udev, uac_control);
    usbd_reg_descr(&udev, uac_getdesc);
    
    usbd_reg_event(&udev, usbd_evt_sof, event_sof);
    usbd_reg_event(&udev, usbd_evt_incomplOUT, event_incompl);
    usbd_reg_event(&udev, usbd_evt_incomplIN, event_incompl);
    
    NVIC_SetPriority(OTG_FS_IRQn, 1);
    NVIC_EnableIRQ(OTG_FS_IRQn);
    usbd_enable(&udev, true);
    usbd_connect(&udev, true);
}