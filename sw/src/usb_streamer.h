#ifndef USB_STREAMER_H
#define	USB_STREAMER_H

typedef struct {
	int			sampling_frequency;
	int			bit_depth;
	int			playing;
	int			active;
	uint8_t		mute;
} AudioSettings;

AudioSettings			audioSettings;

void USBDeviceInit(void);
void USBDeviceEnable(int enable);

#endif