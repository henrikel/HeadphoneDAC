#ifndef __USART_H__
#define __USART_H__

void USARTInit(int baud);
void printMsg(char *str, ...);
void printMem(char *mem, int len);

#endif