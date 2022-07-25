#include "stm32f4xx.h"
#include "debounce.h"

#define MAXCOUNT2	5

#define NBUTTONS	4

volatile int8_t		buttonRising[NBUTTONS], buttonFalling[NBUTTONS], 
					buttonSteady[NBUTTONS], button[NBUTTONS], old[NBUTTONS], 
					count[NBUTTONS];

void debounceInit(void) {

	int8_t	i;

	// Clocks for GPIOA and GPIOB activated in main.c
	
	for(i = 0; i < NBUTTONS; ++i)
		button[i] = buttonRising[i] = buttonFalling[i] = buttonSteady[i] = old[i] = count[i] = 0;
		
	//ticks2 = 0;
	
	// Set up a timer compare interrupt that triggers once every 1 ms
	__disable_irq();
	RCC->APB1ENR |= RCC_APB1ENR_TIM5EN;
	TIM5->PSC = 4800 - 1;
	TIM5->ARR = 10 - 1;
	TIM5->CR1 |= TIM_CR1_CEN;
	TIM5->DIER |= TIM_DIER_UIE;
	NVIC_SetPriority(TIM5_IRQn, 15);
	NVIC_EnableIRQ(TIM5_IRQn);
	__enable_irq();
	
}

int buttonPressed(void) {
	
	if(buttonRising[0]) {
		buttonRising[0] = 0;
		buttonFalling[0] = 0;
		buttonSteady[0] = 0;
		return 1;
	}
	else if(buttonRising[1]) {
		buttonRising[1] = 0;
		buttonFalling[1] = 0;
		buttonSteady[1] = 0;
		return 2;
	}
	else if(buttonRising[2]) {
		buttonRising[2] = 0;
		buttonFalling[2] = 0;
		buttonSteady[2] = 0;
		return 3;
	}
	else if(buttonRising[3]) {
		buttonRising[3] = 0;
		buttonFalling[3] = 0;
		buttonSteady[3] = 0;
		return 4;
	}
	else
		return 0;
}

int buttonReleased(void) {
	
	if(buttonFalling[0]) {
		buttonFalling[0] = 0;
		buttonRising[0] = 0;
		buttonSteady[0] = 0;
		return 1;
	}
	else if(buttonFalling[1]) {
		buttonFalling[1] = 0;
		buttonRising[1] = 0;
		buttonSteady[1] = 0;
		return 2;
	}
	else if(buttonFalling[2]) {
		buttonFalling[2] = 0;
		buttonRising[2] = 0;
		buttonSteady[2] = 0;
		return 3;
	}
	else if(buttonFalling[3]) {
		buttonFalling[3] = 0;
		buttonRising[3] = 0;
		buttonSteady[3] = 0;
		return 4;
	}
	else
		return 0;
}

int buttonNPressed(int n) {
	
	if(n <= NBUTTONS && buttonFalling[n-1]) {
		buttonFalling[n-1] = 0;
		buttonRising[n-1] = 0;
		buttonSteady[n-1] = 0;
		return 1;
	}
	return 0;
}

int buttonNDown(int n) {

	switch(n) {
		case 1:
			return !(GPIOB->IDR & GPIO_IDR_ID1);
			break;
		case 2:
			return (GPIOB->IDR & GPIO_IDR_ID2);
			break;
		case 3:
			return !(GPIOA->IDR & GPIO_IDR_ID4);
			break;
		case 4:
			return !(GPIOB->IDR & GPIO_IDR_ID0);
			break;
		default:
			return 0;
	}
}

int buttonDown(void) {
	
	if((GPIOB->IDR & GPIO_IDR_ID1) == 0)
		return 1;
	else if((GPIOB->IDR & GPIO_IDR_ID2) == 1)
		return 2;
	else if((GPIOA->IDR & GPIO_IDR_ID4) == 0)
		return 3;
	else if((GPIOB->IDR & GPIO_IDR_ID0) == 0)
		return 4;
	else
		return 0;
}

void TIM5_IRQHandler(void) {
	
	// Check buttons
	count[0] += 1;
	if(!(GPIOB->IDR & GPIO_IDR_ID1)) {
		if(!buttonRising[0] && !buttonSteady[0]) {
			count[0] = 0;
			buttonRising[0] = 1;
			buttonFalling[0] = 0;
		}
		else if(count[0] >= MAXCOUNT2)
			// Steady state reached
			buttonSteady[0] = 1;
	}
	else if(buttonSteady[0]) {
		if(!buttonFalling[0]) {
			count[0] = 0;
			buttonFalling[0] = 1;
		}
		else if(count[0] >= MAXCOUNT2)
			buttonSteady[0] = 0;
	}
	
	count[1] += 1;
	if(GPIOB->IDR & GPIO_IDR_ID2) {
		if(!buttonRising[1] && !buttonSteady[1]) {
			count[1] = 0;
			buttonRising[1] = 1;
			buttonFalling[1] = 0;
		}
		else if(count[1] >= MAXCOUNT2)
			// Steady state reached
			buttonSteady[1] = 1;
	}
	else if(buttonSteady[1]) {
		if(!buttonFalling[1]) {
			count[1] = 0;
			buttonFalling[1] = 1;
		}
		else if(count[1] >= MAXCOUNT2)
			buttonSteady[1] = 0;
	}

	count[2] += 1;
	if(!(GPIOA->IDR & GPIO_IDR_ID4)) {
		if(!buttonRising[2] && !buttonSteady[2]) {
			count[2] = 0;
			buttonRising[2] = 1;
			buttonFalling[2] = 0;
		}
		else if(count[2] >= MAXCOUNT2)
			// Steady state reached
			buttonSteady[2] = 1;
	}
	else if(buttonSteady[2]) {
		if(!buttonFalling[2]) {
			count[2] = 0;
			buttonFalling[2] = 1;
		}
		else if(count[2] >= MAXCOUNT2)
			buttonSteady[2] = 0;
	}

	count[3] += 1;
	if(!(GPIOB->IDR & GPIO_IDR_ID0)) {
		if(!buttonRising[3] && !buttonSteady[3]) {
			count[3] = 0;
			buttonRising[3] = 1;
			buttonFalling[3] = 0;
		}
		else if(count[3] >= MAXCOUNT2)
			// Steady state reached
			buttonSteady[3] = 1;
	}
	else if(buttonSteady[3]) {
		if(!buttonFalling[3]) {
			count[3] = 0;
			buttonFalling[3] = 1;
		}
		else if(count[3] >= MAXCOUNT2)
			buttonSteady[3] = 0;
	}

	TIM5->SR = 0;
}