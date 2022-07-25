#ifndef AUDIO_H_
#define	AUDIO_H_

// Number of samples per channel and millisecond for different sampling frequencies
#define SAMPLES44100	44
#define SAMPLES48000	48
#define SAMPLES88200	88
#define SAMPLES96000	96

// Ratio to increase write buffer beyond what is absolutely needed
#define BUF_MARGIN		8

// Size of write buffer as number of 16-bit integers
#define BUF_SIZE		(4 * SAMPLES96000 * BUF_MARGIN)

struct audio_stat {
	int		writePtr;
	int		readPtr;
	int		diff;
};

volatile uint16_t 			audio_buffer[BUF_SIZE]; // Allocate memory for write buffer
volatile struct audio_stat	audio_status;

void AudioInit(void);
int AudioReconfigure(int fs);
void EnableAudio(void);
void DisableAudio(void);

#endif