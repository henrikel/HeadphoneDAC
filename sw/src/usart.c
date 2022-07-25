#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include "stm32f4xx.h"
#include "usart.h"

void USARTInit(int baud) {
	
	uint16_t	uartDiv;
	
	// USART1 TX
	GPIOA->MODER &= ~GPIO_MODER_MODER15;
	GPIOA->MODER |= GPIO_MODER_MODER15_1;
	
	//USART1 RX
	//GPIOA->MODER &= ~GPIO_MODER_MODER10;
	 
	GPIOA->AFR[1] &= ~GPIO_AFRH_AFSEL15;
	GPIOA->AFR[1] |= 7 << GPIO_AFRH_AFSEL15_Pos;
	
	//GPIOA->AFR[1] &= ~GPIO_AFRH_AFSEL10;
	//GPIOA->AFR[1] |= 7 << GPIO_AFRH_AFSEL10_Pos;
	
	RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
	
	// Set speed
	uartDiv = SystemCoreClock / baud;
	USART1->BRR = ((uartDiv / 16) << USART_BRR_DIV_Mantissa_Pos) | 
	              ((uartDiv % 16) << USART_BRR_DIV_Fraction_Pos);
	              
	// 8 bits per byte, no parity, 1 stop bit
	USART1->CR1 |= USART_CR1_TE | USART_CR1_UE;
}

void write(char *data, int size) {
	
	int	count = size;
	
	while(count--) {
		while(!(USART1->SR & USART_SR_TXE));
		USART1->DR = *data++;
	}
}

void printMsg(char *str, ...) {

	char sz[100];
	va_list args;
	
	va_start(args, str);
	vsprintf(sz, str, args);
	write(sz, strlen(sz));
	va_end(args);

}

void printMem(char *mem, int len) {
	
	int		i;
	char	buf[3];
	
	for(i = 0; i < len; ++i) {
		sprintf(buf, "%02x", mem[i]);
		printMsg(buf);
	}
	printMsg("\r\n");
}