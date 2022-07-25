#include "stm32f4xx.h"
#include "utils.h"

volatile uint32_t msTicks, ticks;

void SysTick_Handler(void)  {
	
	if(msTicks != 0)
		msTicks--;
		
	if(ticks != 0)
		ticks--;
}

void delay_ms(uint32_t ms) {
	
	msTicks = ms;
	while(msTicks);
}

uint32_t getSysTicks(void) {
	
	return ticks;
}

void setSysTicks(uint32_t newTicks) {
	
	ticks = newTicks;
}