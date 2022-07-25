#include "stm32f4xx.h"
#include "shutdownCtl.h"

void ShutdownCtlInit(void) {
	
	// Enable PVD and set threshold at 2.9 V
	PWR->CR |= 0xf0;
	
	// Enable PVD interrupt, rising edge.
	EXTI->IMR |= EXTI_IMR_MR16;
	EXTI->RTSR |= EXTI_RTSR_TR16;
	
	NVIC_SetPriority(PVD_IRQn, 0);
	NVIC_EnableIRQ(PVD_IRQn);
}

void PVD_IRQHandler(void) {

	// Switch off output relay
	GPIOB->BSRR |= GPIO_BSRR_BR14;
	GPIOC->BSRR |= GPIO_BSRR_BS13;
}