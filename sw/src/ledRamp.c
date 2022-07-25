#include "stm32f4xx.h"
#include "adc.h"
#include "ledRamp.h"

#define MAXVAL		4000
#define NLEDS		10
#define STEP		2

#define	TIMEOUT		100

// -40, -20, -10, -7, -5, -3, -1, 0, +3, +6 dB
int				thresholds[NLEDS] = {20, 200, 632, 893, 1125, 1416, 1783, 2000, 2825, 4000};
volatile int	channel, peakVal[2], timeout[2];

// LEDs: A2, A3, A7, A8, A10, B7, B8, B9, B10, C14
// Row selector: C15

void LEDRampInit(void) {
	
	channel = 0;
	peakVal[0] = peakVal[1] = -1;
	timeout[0] = timeout[1] = 0;
	
	// Set up timer 4 to update the LED ramp
	RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;
	
	// Run timer at 200 Hz
	TIM4->PSC = 4800 - 1;
	TIM4->ARR = 50 - 1;
	TIM4->CNT = 0;
		
	// Timer 4 update interrupt enable
	TIM4->DIER |= TIM_DIER_UIE;
	
	NVIC_EnableIRQ(TIM4_IRQn);
	
	// Enable Timer 4
	TIM4->CR1 |= TIM_CR1_CEN;
	
}

void TIM4_IRQHandler(void) {  

	int	val, level;
		
	channel = (channel + 1) % 2;
		
	val = ADCRead(channel);
		
	level = setLEDRampVal(val);
		
	timeout[channel] += 1;
		
	if(level < peakVal[channel]) {
		if(timeout[channel] < TIMEOUT)
			setLEDPeakVal(peakVal[channel]);
		else {
			timeout[channel] = 0;
			peakVal[channel] = -1;
		}
	}
	else {
		peakVal[channel] = level;
		timeout[channel] = 0;
	}
		
	if(channel)
		GPIOC->BSRR |= GPIO_BSRR_BR15;
	else
		GPIOC->BSRR |= GPIO_BSRR_BS15;

	if(TIM4->SR & TIM_SR_UIF)
		TIM4->SR &= ~TIM_SR_UIF;

}
	
int setLEDRampVal(int val) {
	
	int		level;
	
	if(val > thresholds[9]) {
		GPIOA->BSRR |= GPIO_BSRR_BS2;
		GPIOA->BSRR |= GPIO_BSRR_BS3;
		GPIOA->BSRR |= GPIO_BSRR_BS7;
		GPIOA->BSRR |= GPIO_BSRR_BS8;
		GPIOA->BSRR |= GPIO_BSRR_BS10;
		GPIOB->BSRR |= GPIO_BSRR_BS7;
		GPIOB->BSRR |= GPIO_BSRR_BS8;
		GPIOB->BSRR |= GPIO_BSRR_BS9;
		GPIOB->BSRR |= GPIO_BSRR_BS10;
		GPIOC->BSRR |= GPIO_BSRR_BS14;
		level = 9;
	}
	else if(val > thresholds[8]) {
		GPIOA->BSRR |= GPIO_BSRR_BS2;
		GPIOA->BSRR |= GPIO_BSRR_BS3;
		GPIOA->BSRR |= GPIO_BSRR_BS7;
		GPIOA->BSRR |= GPIO_BSRR_BS8;
		GPIOA->BSRR |= GPIO_BSRR_BS10;
		GPIOB->BSRR |= GPIO_BSRR_BS7;
		GPIOB->BSRR |= GPIO_BSRR_BS8;
		GPIOB->BSRR |= GPIO_BSRR_BS9;
		GPIOB->BSRR |= GPIO_BSRR_BS10;
		GPIOC->BSRR |= GPIO_BSRR_BR14;
		level = 8;
	}
	else if(val > thresholds[7]) {
		GPIOA->BSRR |= GPIO_BSRR_BS2;
		GPIOA->BSRR |= GPIO_BSRR_BS3;
		GPIOA->BSRR |= GPIO_BSRR_BS7;
		GPIOA->BSRR |= GPIO_BSRR_BS8;
		GPIOA->BSRR |= GPIO_BSRR_BS10;
		GPIOB->BSRR |= GPIO_BSRR_BS7;
		GPIOB->BSRR |= GPIO_BSRR_BS8;
		GPIOB->BSRR |= GPIO_BSRR_BS9;
		GPIOB->BSRR |= GPIO_BSRR_BR10;
		GPIOC->BSRR |= GPIO_BSRR_BR14;
		level = 7;
	}
	else if(val > thresholds[6]) {
		GPIOA->BSRR |= GPIO_BSRR_BS2;
		GPIOA->BSRR |= GPIO_BSRR_BS3;
		GPIOA->BSRR |= GPIO_BSRR_BS7;
		GPIOA->BSRR |= GPIO_BSRR_BS8;
		GPIOA->BSRR |= GPIO_BSRR_BS10;
		GPIOB->BSRR |= GPIO_BSRR_BS7;
		GPIOB->BSRR |= GPIO_BSRR_BS8;
		GPIOB->BSRR |= GPIO_BSRR_BR9;
		GPIOB->BSRR |= GPIO_BSRR_BR10;
		GPIOC->BSRR |= GPIO_BSRR_BR14;
		level = 6;
	}
	else if(val > thresholds[5]) {
		GPIOA->BSRR |= GPIO_BSRR_BS2;
		GPIOA->BSRR |= GPIO_BSRR_BS3;
		GPIOA->BSRR |= GPIO_BSRR_BS7;
		GPIOA->BSRR |= GPIO_BSRR_BS8;
		GPIOA->BSRR |= GPIO_BSRR_BS10;
		GPIOB->BSRR |= GPIO_BSRR_BS7;
		GPIOB->BSRR |= GPIO_BSRR_BR8;
		GPIOB->BSRR |= GPIO_BSRR_BR9;
		GPIOB->BSRR |= GPIO_BSRR_BR10;
		GPIOC->BSRR |= GPIO_BSRR_BR14;
		level = 5;
	}
	else if(val > thresholds[4]) {
		GPIOA->BSRR |= GPIO_BSRR_BS2;
		GPIOA->BSRR |= GPIO_BSRR_BS3;
		GPIOA->BSRR |= GPIO_BSRR_BS7;
		GPIOA->BSRR |= GPIO_BSRR_BS8;
		GPIOA->BSRR |= GPIO_BSRR_BS10;
		GPIOB->BSRR |= GPIO_BSRR_BR7;
		GPIOB->BSRR |= GPIO_BSRR_BR8;
		GPIOB->BSRR |= GPIO_BSRR_BR9;
		GPIOB->BSRR |= GPIO_BSRR_BR10;
		GPIOC->BSRR |= GPIO_BSRR_BR14;
		level = 4;
	}
	else if(val > thresholds[3]) {
		GPIOA->BSRR |= GPIO_BSRR_BS2;
		GPIOA->BSRR |= GPIO_BSRR_BS3;
		GPIOA->BSRR |= GPIO_BSRR_BS7;
		GPIOA->BSRR |= GPIO_BSRR_BS8;
		GPIOA->BSRR |= GPIO_BSRR_BR10;
		GPIOB->BSRR |= GPIO_BSRR_BR7;
		GPIOB->BSRR |= GPIO_BSRR_BR8;
		GPIOB->BSRR |= GPIO_BSRR_BR9;
		GPIOB->BSRR |= GPIO_BSRR_BR10;
		GPIOC->BSRR |= GPIO_BSRR_BR14;
		level = 3;
	}
	else if(val > thresholds[2]) {
		GPIOA->BSRR |= GPIO_BSRR_BS2;
		GPIOA->BSRR |= GPIO_BSRR_BS3;
		GPIOA->BSRR |= GPIO_BSRR_BS7;
		GPIOA->BSRR |= GPIO_BSRR_BR8;
		GPIOA->BSRR |= GPIO_BSRR_BR10;
		GPIOB->BSRR |= GPIO_BSRR_BR7;
		GPIOB->BSRR |= GPIO_BSRR_BR8;
		GPIOB->BSRR |= GPIO_BSRR_BR9;
		GPIOB->BSRR |= GPIO_BSRR_BR10;
		GPIOC->BSRR |= GPIO_BSRR_BR14;
		level = 2;
	}
	else if(val > thresholds[1]) {
		GPIOA->BSRR |= GPIO_BSRR_BS2;
		GPIOA->BSRR |= GPIO_BSRR_BS3;
		GPIOA->BSRR |= GPIO_BSRR_BR7;
		GPIOA->BSRR |= GPIO_BSRR_BR8;
		GPIOA->BSRR |= GPIO_BSRR_BR10;
		GPIOB->BSRR |= GPIO_BSRR_BR7;
		GPIOB->BSRR |= GPIO_BSRR_BR8;
		GPIOB->BSRR |= GPIO_BSRR_BR9;
		GPIOB->BSRR |= GPIO_BSRR_BR10;
		GPIOC->BSRR |= GPIO_BSRR_BR14;
		level = 1;
	}
	else if(val > thresholds[0]) {
		GPIOA->BSRR |= GPIO_BSRR_BS2;
		GPIOA->BSRR |= GPIO_BSRR_BR3;
		GPIOA->BSRR |= GPIO_BSRR_BR7;
		GPIOA->BSRR |= GPIO_BSRR_BR8;
		GPIOA->BSRR |= GPIO_BSRR_BR10;
		GPIOB->BSRR |= GPIO_BSRR_BR7;
		GPIOB->BSRR |= GPIO_BSRR_BR8;
		GPIOB->BSRR |= GPIO_BSRR_BR9;
		GPIOB->BSRR |= GPIO_BSRR_BR10;
		GPIOC->BSRR |= GPIO_BSRR_BR14;
		level = 0;
	}
	else {
		GPIOA->BSRR |= GPIO_BSRR_BR2;
		GPIOA->BSRR |= GPIO_BSRR_BR3;
		GPIOA->BSRR |= GPIO_BSRR_BR7;
		GPIOA->BSRR |= GPIO_BSRR_BR8;
		GPIOA->BSRR |= GPIO_BSRR_BR10;
		GPIOB->BSRR |= GPIO_BSRR_BR7;
		GPIOB->BSRR |= GPIO_BSRR_BR8;
		GPIOB->BSRR |= GPIO_BSRR_BR9;
		GPIOB->BSRR |= GPIO_BSRR_BR10;
		GPIOC->BSRR |= GPIO_BSRR_BR14;
		level = -1;
	}
	
	return level;
}

void setLEDPeakVal(int val) {
	
	switch(val) {
		case 0:
			GPIOA->BSRR |= GPIO_BSRR_BS2;
			break;
		case 1:
			GPIOA->BSRR |= GPIO_BSRR_BS3;
			break;
		case 2:
			GPIOA->BSRR |= GPIO_BSRR_BS7;
			break;
		case 3:
			GPIOA->BSRR |= GPIO_BSRR_BS8;
			break;
		case 4:
			GPIOA->BSRR |= GPIO_BSRR_BS10;
			break;
		case 5:
			GPIOB->BSRR |= GPIO_BSRR_BS7;
			break;
		case 6:
			GPIOB->BSRR |= GPIO_BSRR_BS8;
			break;
		case 7:
			GPIOB->BSRR |= GPIO_BSRR_BS9;
			break;
		case 8:
			GPIOB->BSRR |= GPIO_BSRR_BS10;
			break;
		case 9:
			GPIOC->BSRR |= GPIO_BSRR_BS14;
			break;
	}
}