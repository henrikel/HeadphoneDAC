#include "stm32f4xx.h"
#ifdef DEBUG
#include "usart.h"
#endif
#include "adc.h"

#define	NSAMP	512
volatile int	ch, samples[2][NSAMP], sPtr[2], sum[2];
int				adcData[2];

void ADCInit(void) {
	
	// Enable ADC
	RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
	
	// Enable Timer 3
	RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
	
	ADC1->CR2 |= ADC_CR2_ADON;
	
	// Scan mode enable
	ADC1->CR1 |= ADC_CR1_SCAN;
	
	// Continuous mode disable
	ADC1->CR2 &= ~ADC_CR2_CONT;
	
	ADC1->CR2 |= ADC_CR2_EOCS;
	
	// Sample time 480 cycles for channel 0 and 1
	ADC1->SMPR2 |= 7;
	ADC1->SMPR2 |= (7 << 3);
	
	// 2 conversions
	ADC1->SQR1 |= 1 << ADC_SQR1_L_Pos;
	
	// 1st conversion, channel 0
	ADC1->SQR3 &= 0x1f;
	
	// 2nd conversion, channel 1
	ADC1->SQR3 |= 1 << ADC_SQR3_SQ2_Pos;
	
	// Trigger on rising edge
	ADC1->CR2 &= ~ADC_CR2_EXTEN;
	ADC1->CR2 |= (1 << ADC_CR2_EXTEN_Pos);
	
	// External event select: Timer 3 TRGO
	ADC1->CR2 &= ~ADC_CR2_EXTSEL;
	ADC1->CR2 |= (8 << ADC_CR2_EXTSEL_Pos);
	
	// Enable ADC interrupt
	ADC1->CR1 |= ADC_CR1_EOCIE;
	
	NVIC_SetPriority(ADC_IRQn, 0);
	NVIC_EnableIRQ(ADC_IRQn);
	
	ch = 0;
	adcData[0] = 0;
	adcData[1] = 0;
	
	ADC1->CR2 |= ADC_CR2_SWSTART;
	
	// Timer 3 configuration
	// APB1 prescaler is 2, system clock is 96 MHz.
	// So 2399 gives a timer frequency (i.e., sample rate) of 20 kHz
	TIM3->PSC = 2400 - 1;
	TIM3->ARR = 1;
	
	// Select TRGO source
	TIM3->CR2 &= ~TIM_CR2_MMS;
	TIM3->CR2 |= TIM_CR2_MMS_1;
	
	// Enable Timer 3
	TIM3->CR1 |= TIM_CR1_CEN;
}

void ADC_IRQHandler(void) {

	int		newVal = ADC1->DR;
	
	sum[ch] = sum[ch] - samples[ch][sPtr[ch]] + newVal;
	samples[ch][sPtr[ch]] = newVal;
	
	sPtr[ch] = (sPtr[ch] + 1) % NSAMP;
	
	adcData[ch] = sum[ch] >> 9;

	ch = (ch + 1) % 2;
	
#ifdef DEBUG
//printMsg("ADC: %d\r\n", adcData[ch]);
#endif
}

int ADCRead(int ch) {
	
	return adcData[ch];
}