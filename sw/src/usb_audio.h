#ifndef _USB_AUDIO_H_
#define _USB_AUDIO_H_

#ifdef __cplusplus
    extern "C" {
#endif

#define USB_AUDIO_SUBCLASS_UNDEFINED		0x00
#define USB_AUDIO_SUBCLASS_AUDIOCONTROL		0x01
#define USB_AUDIO_SUBCLASS_AUDIOSTREAMING	0x02
#define USB_AUDIO_SUBCLASS_MIDISTREAMING	0x03

#define USB_AUDIO_PROTO_UNDEFINED			0x00

// USB Audio class-specific descriptors
#define USB_AUDIO_CS_UNDEFINED				0x20
#define USB_AUDIO_CS_DEVICE					0x21
#define USB_AUDIO_CS_CONFIGURATION			0x22
#define USB_AUDIO_CS_STRING					0x23
#define USB_AUDIO_CS_INTERFACE				0x24
#define USB_AUDIO_CS_ENDPOINT				0x25

// USB Audio class-specific interface descriptor subtypes
#define USB_AUDIO_AC_DESCRIPTOR_UNDEFINED	0x00
#define USB_AUDIO_HEADER					0x01
#define USB_AUDIO_INPUT_TERMINAL			0x02
#define USB_AUDIO_OUTPUT_TERMINAL			0x03
#define USB_AUDIO_MIXER_UNIT				0x04
#define USB_AUDIO_SELECTOR_UNIT				0x05
#define USB_AUDIO_FEATURE_UNIT				0x06
#define USB_AUDIO_PROCESSING_UNIT			0x07
#define USB_AUDIO_EXTENSION_UNIT			0x08

// USB Audio class-specific AS interface descriptor subtypes
#define USB_AUDIO_AS_DESCRIPTOR_UNDEFINED	0x00
#define USB_AUDIO_AS_GENERAL				0x01
#define USB_AUDIO_FORMAT_TYPE				0x02
#define USB_AUDIO_FORMAT_SPECIFIC			0x03

// Processing unit process types
#define USB_AUDIO_PROCESS_UNDEFINED			0x00
#define USB_AUDIO_UPDOWNMIX_PROCESS			0x01
#define USB_AUDIO_DOLBY_PROLOGIC_PROCESS	0x02
#define USB_AUDIO_3D_STERO_EXTENDER_PROCESS	0x03
#define USB_AUDIO_REVERBERATION_PROCESS		0x04
#define USB_AUDIO_CHORUS_PROCESS			0x05
#define USB_AUDIO_DYN_RANGE_COMP_PROCESS	0x06

// USB Audio class-specific endpoint descriptor subtypes
#define USB_AUDIO_DESCRIPTOR_UNDEFINED		0x00
#define USB_AUDIO_EP_GENERAL				0x01

// USB Audio class-specific request codes
#define USB_AUDIO_REQUEST_CODE_UNDEFINED	0x00
#define USB_AUDIO_SET_CUR					0x01
#define USB_AUDIO_GET_CUR					0x81
#define USB_AUDIO_SET_MIN					0x02
#define USB_AUDIO_GET_MIN					0x82
#define USB_AUDIO_SET_MAX					0x03
#define USB_AUDIO_GET_MAX					0x83
#define USB_AUDIO_SET_RES					0x04
#define USB_AUDIO_GET_RES					0x84
#define USB_AUDIO_SET_MEM					0x05
#define USB_AUDIO_GET_MEM					0x85
#define USB_AUDIO_GET_STAT					0xFF

// USB Audio class terminal control selectors
#define USB_AUDIO_TE_CONTROL_UNDEFINED		0x00
#define USB_AUDIO_COPY_PROTECT_CONTROL		0x01

// USB Audio class feature unit control selectors
#define USB_AUDIO_FU_CONTROL_UNDEFINED		0x00
#define USB_AUDIO_MUTE_CONTROL				0x01
#define USB_AUDIO_VOLUME_CONTROL			0x02
#define USB_AUDIO_BASS_CONTROL				0x03
#define USB_AUDIO_MID_CONTROL				0x04
#define USB_AUDIO_TREBLE_CONTROL			0x05
#define USB_AUDIO_GRAPHIC_EQUALIZER_CONTROL	0x06
#define USB_AUDIO_AUTOMATIC_GAIN_CONTROL	0x07
#define USB_AUDIO_DELAY_CONTROL				0x08
#define USB_AUDIO_BASS_BOOST_CONTROL		0x09
#define USB_AUDIO_LOUDNESS_CONTROL			0x0A

// USB Audio class feature unit control selector bit fields
#define USB_AUDIO_FU_MUTE_CONTROL			0x0001
#define USB_AUDIO_FU_VOLUME_CONTROL			0x0002
#define USB_AUDIO_FU_BASS_CONTROL			0x0004
#define USB_AUDIO_FU_MID_CONTROL			0x0008
#define USB_AUDIO_FU_TREBLE_CONTROL			0x0010
#define USB_AUDIO_FU_GRAPHEQ_CONTROL		0x0020
#define USB_AUDIO_FU_AUTOGAIN_CONTROL		0x0040
#define USB_AUDIO_FU_DELAY_CONTROL			0x0080
#define USB_AUDIO_FU_BASSBOOST_CONTROL		0x0100
#define USB_AUDIO_FU_LOUDNESS_CONTROL		0x0200

// Up/down-mix processing unit control selectors
#define USB_AUDIO_UD_CONTROL_UNDEFINED		0x00
#define USB_AUDIO_UD_ENABLE_CONTROL			0x01
#define USB_AUDIO_UD_MODE_SELECT_CONTROL	0x02

// Dolby ProLogic processing unit control selectors
#define USB_AUDIO_DP_CONTROL_UNDEFINED		0x00
#define USB_AUDIO_DP_ENABLE_CONTROL			0x01
#define USB_AUDIO_DP_MODE_SELECT_CONTROL	0x02

// 3D stereo extender processing unit control selectors
#define USB_AUDIO_3D_CONTROL_UNDEFINED		0x00
#define USB_AUDIO_3D_ENABLE_CONTROL			0x01
#define USB_AUDIO_SPACIOUSNESS_CONTROL		0x02

// Reverberation processing unit control selectors
#define USB_AUDIO_RV_CONTROL_UNDEFINED		0x00
#define USB_AUDIO_RV_ENABLE_CONTROL			0x01
#define USB_AUDIO_REVERB_LEVEL_CONTROL		0x02
#define USB_AUDIO_REVERB_TIME_CONTROL		0x03
#define USB_AUDIO_REVERB_FEEDBACK_CONTROL	0x04

// Chorus processing unit control selectors
#define USB_AUDIO_CH_CONTROL_UNDEFINED		0x00
#define USB_AUDIO_CH_ENABLE_CONTROL			0x01
#define USB_AUDIO_CHORUS_LEVEL_CONTROL		0x02
#define USB_AUDIO_CHORUS_RATE_CONTROL		0x03
#define USB_AUDIO_CHORUS_DEPTH_CONTROL		0x04

// Dynamic range compressor processing unit control selectors
#define USB_AUDIO_DR_CONTROL_UNDEFINED		0x00
#define USB_AUDIO_DR_ENABLE_CONTROL			0x01
#define USB_AUDIO_COMPRESSION_RATE_CONTROL	0x02
#define USB_AUDIO_MAXAMPL_CONTROL			0x03
#define USB_AUDIO_THRESHOLD_CONTROL			0x04
#define USB_AUDIO_ATTACK_TIME				0x05
#define USB_AUDIO_RELEASE_TIME				0x06

// Extension unit control selectors
#define USB_AUDIO_XU_CONTROL_UNDEFINED		0x00
#define USB_AUDIO_XU_ENABLE_CONTROL			0x01

// Endpoint control selectors
#define USB_AUDIO_EP_CONTROL_UNDEFINED		0x00
#define USB_AUDIO_SAMPLING_FREQ_CONTROL		0x01
#define USB_AUDIO_PITCH_CONTROL				0x02

// Request types
#define SET_CUR								0x01
#define SET_MIN								0x02
#define SET_MAX								0x03
#define SET_RES								0x04
#define SET_MEM								0x05
#define GET_CUR								0x81
#define GET_MIN								0x82
#define GET_MAX								0x83
#define GET_RES								0x84
#define GET_MEM								0x85

#define USB_AUDIO_TERMINAL_TYPE_STREAMING	0x0101
#define USB_AUDIO_TERMINAL_TYPE_SPEAKER		0x0301
#define USB_AUDIO_TERMINAL_TYPE_HEADPHONE	0x0302

// Audio data formats
#define USB_AUDIO_DATA_FORMAT_PCM			0x0001
#define USB_AUDIO_FORMAT_TYPE_I				0x01

struct usb_audio_header_desc {
	uint8_t		bLength;
	uint8_t		bDescriptorType;
	uint8_t		bDescriptorSubtype;
	uint16_t	bcdADC;
	uint16_t	wTotalLength;
	uint8_t		bInCollection;
	uint8_t		baInterFaceNr[1];
} __attribute__ ((packed));

struct usb_audio_input_terminal_desc {
	uint8_t		bLength;
	uint8_t		bDescriptorType;
	uint8_t		bDescriptorSubtype;
	uint8_t		bTerminalID;
	uint16_t	wTerminalType;
	uint8_t		bAssocTerminal;
	uint8_t		bNrChannels;
	uint16_t	wChannelConfig;
	uint8_t		iChannelNames;
	uint8_t		iTerminal;
} __attribute__ ((packed));

struct usb_audio_output_terminal_desc {
	uint8_t		bLength;
	uint8_t		bDescriptorType;
	uint8_t		bDescriptorSubtype;
	uint8_t		bTerminalID;
	uint16_t	wTerminalType;
	uint8_t		bAssocTerminal;
	uint8_t		bSourceID;
	uint8_t		iTerminal;
	
} __attribute__ ((packed));

// Missing descriptors:
// Mixer Unit
// Processing Unit
// Selector Unit
// Up/down-mix Processing Unit
// Dolby ProLogic Processing Unit
// 3D Stereo Extender Processing Unit
// Reverberation Processing Unit
// Chorus Processing Unit
// Dynamic Range Compressor Processing Unit

// The unit descriptors contain variable size elements in some cases
// This makes it tricky to implement them in structs
// In this particular descriptor, the second last item is of variable length
// Had it always been the last item, it would have been OK. But in this case
// we need to include the last item into the variable size item
struct usb_audio_feature_unit_desc {
	uint8_t		bLength;
	uint8_t		bDescriptorType;
	uint8_t		bDescriptorSubtype;
	uint8_t		bUnitID;
	uint8_t		bSourceID;
	uint8_t		bControlSize;
	uint16_t	bmaControls[3];
	uint8_t		iFeature;
} __attribute__ ((packed));

// Standard AC (Audio Control) Interrupt Endpoint Descriptor
struct usb_audio_ac_int_endp_desc {
	uint8_t		bLength;
	uint8_t		bDescriptorType;
	uint8_t		bEndpointAddress;
	uint8_t		bmAttributes;
	uint16_t	wMaxpacketSize;
	uint8_t		bInterval;
	uint8_t		bRefresh;
	uint8_t		bSynchAddress;
} __attribute__ ((packed));

// Standard AS (AudioStreaming) Interface Descriptor
struct usb_audio_as_std_int_desc {
	uint8_t		bLength;
	uint8_t		bDescriptorType;
	uint8_t		bInterfaceNumber;
	uint8_t		bAlternateSetting;
	uint8_t		bNumEndpoints;
	uint8_t		bInterfaceClass;
	uint8_t		bInterfaceSubClass;
	uint8_t		bInterfaceProtocol;
	uint8_t		iInterface;
} __attribute__ ((packed));

// Class-Specific AS General Interface Descriptor
struct usb_audio_as_spec_int_desc {
	uint8_t		bLength;
	uint8_t		bDescriptorType;
	uint8_t		bDescriptorSubtype;
	uint8_t		bTerminalLink;
	uint8_t		bDelay;
	uint16_t	wFormatTag;
} __attribute__ ((packed));

// Type I Format Interface Descriptor
struct byte3 {
	uint8_t		b1;
	uint8_t		b2;
	uint8_t		b3;
} __attribute__ ((packed));

struct usb_audio_as_formatI_int_desc {
	uint8_t			bLength;
	uint8_t			bDescriptorType;
	uint8_t			bDescriptorSubtype;
	uint8_t			bFormatType;
	uint8_t			bNrChannels;
	uint8_t			bSubFrameSize;
	uint8_t			bBitResolution;
	uint8_t			bSamFreqType;
	struct byte3	tSamFreq[4];
	
}  __attribute__ ((packed));

// Standard AS Isochronous Audio Data Endpoint Descriptor
struct usb_audio_as_iso_std_endp_desc {
	uint8_t		bLength;
	uint8_t		bDescriptorType;
	uint8_t		bEndpointAddress;
	uint8_t		bmAttributes;
	uint16_t	wMaxPacketSize;
	uint8_t		bInterval;
	uint8_t		bRefresh;
	uint8_t		bSynchAddress;
} __attribute__ ((packed));

// Class-Specific AS Isochronous Audio Data Endpoint Descriptor
struct usb_audio_as_iso_spec_endp_desc {
	uint8_t		bLength;
	uint8_t		bDescriptorType;
	uint8_t		bDescriptorSubtype;
	uint8_t		bmAttributes;
	uint8_t		bLockDelayUnits;
	uint16_t	wLockDelay;
} __attribute__ ((packed));

// Standard AS Isochronous Synch Endpoint Descriptor
struct usb_audio_as_iso_synch_endp_desc {
	uint8_t		bLength;
	uint8_t		bDescriptorType;
	uint8_t		bEndpointAddress;
	uint8_t		bmAttributes;
	uint16_t	wMaxPacketSize;
	uint8_t		bInterval;
	uint8_t		bRefresh;
	uint8_t		bSynchAddress;
} __attribute__ ((packed));

#ifdef __cplusplus
    }
#endif

#endif /* _USB_AUDIO_H_ */