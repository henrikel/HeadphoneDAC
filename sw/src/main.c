#include "stm32f4xx.h"
#include "utils.h"
#include "audio.h"
#include "adc.h"
#include "ledRamp.h"
#include "shutdownCtl.h"
#ifdef DEBUG
#include "usart.h"
#endif
#include "debounce.h"
#include "usb_streamer.h"

void ClockInit(void) {

	//unsigned int	ra;
	int			tmpReg;

	// Clock initialization
	RCC->CIR = 0x00bf0000;
	
	// Turn on HSE clock
	RCC->CR |= RCC_CR_HSEON;
	while(!(RCC->CR & RCC_CR_HSERDY));
	
	/* set voltage scale to 1 for max frequency */
    /* first enable power interface clock (APB1ENR:bit 28) */
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;

    /* then set voltage scale to 1 for max frequency (PWR_CR:bit 14 and 15)
     * (0) scale 2 for fCLK <= 144 Mhz
     * (1) scale 1 for 144 Mhz < fCLK <= 168 Mhz
     */
    PWR->CR |= 3 << PWR_CR_VOS_Pos; //(3 << 14);
	
	// Configure PLL
	// M = 4, N = 192, P = 2, Q = 4
	// Gives a system clock of 96 MHz
	tmpReg = RCC->PLLCFGR;
	tmpReg &= ~RCC_PLLCFGR_PLLM;
	tmpReg |= 25 << RCC_PLLCFGR_PLLM_Pos;
	
	tmpReg &= ~RCC_PLLCFGR_PLLN;
	tmpReg |= 192 << RCC_PLLCFGR_PLLN_Pos;
	
	tmpReg &= ~RCC_PLLCFGR_PLLP;
	tmpReg |= ((2 >> 1) - 1) << RCC_PLLCFGR_PLLP_Pos;
	
	tmpReg &= ~RCC_PLLCFGR_PLLQ;
	tmpReg |= 4 << RCC_PLLCFGR_PLLQ_Pos;
	
	// Cortex system timer??? 1 or 8?
	
	// Set HSE as PLL source
	tmpReg |= RCC_PLLCFGR_PLLSRC_HSE;
	RCC->PLLCFGR = tmpReg;
	
	// PLL On
	RCC->CR |= RCC_CR_PLLON;
	while(!(RCC->CR & RCC_CR_PLLRDY));
	
	// AHB prescaler = 1, APB1 prescaler = 2, APB2 prescaler = 1
	// AHB prescaler
	tmpReg = RCC->CFGR;
	tmpReg &= ~(RCC_CFGR_HPRE); //remove old prescaler 
	tmpReg |= RCC_CFGR_HPRE_DIV1; //set AHB prescaler = 1. 
	
	//set APB1 prescaler
	tmpReg &= ~(RCC_CFGR_PPRE1);
	tmpReg |= RCC_CFGR_PPRE1_DIV2;
	
	//set APB2 prescaler
	tmpReg &= ~(RCC_CFGR_PPRE2);
	tmpReg |= RCC_CFGR_PPRE2_DIV1;
	RCC->CFGR = tmpReg;
	
	//set flash wait states to 2
	tmpReg = FLASH->ACR;
	tmpReg &= ~(FLASH_ACR_LATENCY); 
	tmpReg |= FLASH_ACR_LATENCY_2WS;
	FLASH->ACR = tmpReg;
	
	// Set system to PLL clock
	RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_SW)) | RCC_CFGR_SW_PLL;
	while((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
	
	// Set I2S PLL dividers to:
	// M = 25, N = 344, R = 2
	// This gives an I2S clock of  25 MHz / 25 * 344 / 2 = 172 MHz
	// and a sampling frequency of fs = 95.982 kHz when I2SDIV = 3 and I2SODD = 1
	// In this case, MCLK = I2S clock / (2 * I2SDIV + I2SODD)
	// MCLK is 256xfs
	//
	tmpReg = RCC->PLLI2SCFGR;
	tmpReg &= ~RCC_PLLI2SCFGR_PLLI2SR;
	tmpReg |= 2 << RCC_PLLI2SCFGR_PLLI2SR_Pos;
	
	tmpReg &= ~RCC_PLLI2SCFGR_PLLI2SN;
	tmpReg |= 344 << RCC_PLLI2SCFGR_PLLI2SN_Pos;
	
	tmpReg &= ~RCC_PLLI2SCFGR_PLLI2SM;
	tmpReg |= 25 << RCC_PLLI2SCFGR_PLLI2SM_Pos;
	RCC->PLLI2SCFGR = tmpReg;
	
	// Enable the I2S PLL
	RCC->CR |= RCC_CR_PLLI2SON;
	while(!(RCC->CR & RCC_CR_PLLI2SRDY));
	
	// Clear All Interrupts
	RCC->CIR = 0x00df0000;
	
}

void GPIOInit() {
	
	// Enable all relevant GPIO ports
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
	
	// ADC pin A0 and A1 enable
	GPIOA->OTYPER &= ~GPIO_OTYPER_OT0;
	GPIOA->PUPDR &= ~GPIO_PUPDR_PUPDR0;
	GPIOA->OSPEEDR &= ~GPIO_OSPEEDR_OSPEED0;
	GPIOA->MODER &= ~GPIO_MODER_MODER0;
	GPIOA->MODER |= (3 << GPIO_MODER_MODER0_Pos);
	
	GPIOA->OTYPER &= ~GPIO_OTYPER_OT1;
	GPIOA->PUPDR &= ~GPIO_PUPDR_PUPDR1;
	GPIOA->OSPEEDR &= ~GPIO_OSPEEDR_OSPEED1;
	GPIOA->MODER &= ~GPIO_MODER_MODER1;
	GPIOA->MODER |= (3 << GPIO_MODER_MODER1_Pos);
	
	// Set up I2S
	// PA6: MCK (SCK)
	// PB12: WS (LRCK)
	// PB13: CK (BCK)
	// PB15: SD (DATA)
	GPIOA->MODER &= ~GPIO_MODER_MODER6;
	GPIOA->MODER |= GPIO_MODER_MODER6_1;
	
	GPIOB->MODER &= ~GPIO_MODER_MODER12;
	GPIOB->MODER |= GPIO_MODER_MODER12_1;
	
	GPIOB->MODER &= ~GPIO_MODER_MODER13;
	GPIOB->MODER |= GPIO_MODER_MODER13_1;
	
	GPIOB->MODER &= ~GPIO_MODER_MODER15;
	GPIOB->MODER |= GPIO_MODER_MODER15_1;
	
	GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL6;
	GPIOA->AFR[0] |= 6 << GPIO_AFRL_AFSEL6_Pos;
	
	GPIOB->AFR[1] &= ~GPIO_AFRH_AFSEL12;
	GPIOB->AFR[1] |= 5 << GPIO_AFRH_AFSEL12_Pos;
	
	GPIOB->AFR[1] &= ~GPIO_AFRH_AFSEL13;
	GPIOB->AFR[1] |= 5 << GPIO_AFRH_AFSEL13_Pos;
	
	GPIOB->AFR[1] &= ~GPIO_AFRH_AFSEL15;
	GPIOB->AFR[1] |= 5 << GPIO_AFRH_AFSEL15_Pos;
	
	// Set up interface for indicator LEDs
	// PB6 and PB5
	GPIOB->MODER &= ~GPIO_MODER_MODER6;
    GPIOB->MODER |= 1 << GPIO_MODER_MODER6_Pos;
    GPIOB->MODER &= ~GPIO_MODER_MODER5;
    GPIOB->MODER |= 1 << GPIO_MODER_MODER5_Pos;
    
	// Feedback status LED
	GPIOC->MODER &= ~GPIO_MODER_MODER13;
	GPIOC->MODER |= 1 << GPIO_MODER_MODER13_Pos;
    
    // Set alternate functions for USB_OTG pins
	// PA11: DM (AF10)
	// PA12: DP (AF10)
	GPIOA->MODER &= ~GPIO_MODER_MODER11;
	GPIOA->MODER |= GPIO_MODER_MODER11_1;
	
	GPIOA->MODER &= ~GPIO_MODER_MODER12;
	GPIOA->MODER |= GPIO_MODER_MODER12_1;
		
	GPIOA->AFR[1] &= ~GPIO_AFRH_AFSEL11;
	GPIOA->AFR[1] |= GPIO_AFRH_AFSEL11_1 | GPIO_AFRH_AFSEL11_3;
	
	GPIOA->AFR[1] &= ~GPIO_AFRH_AFSEL12;
	GPIOA->AFR[1] |= GPIO_AFRH_AFSEL12_1 | GPIO_AFRH_AFSEL12_3;
	
	// Alternate function for external trigger for TIM2
	GPIOA->MODER &= ~GPIO_MODER_MODER5;
	GPIOA->MODER |= 2 << GPIO_MODER_MODER5_Pos;
	GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL5;
	GPIOA->AFR[0] |= 1 << GPIO_AFRL_AFSEL5_Pos;
	
	// Buttons
	// Button 1 (scan backward): PB1
	// Button 2 (Play/pause):    PB2
	// Button 3 (Scan forward):  PA4
	// Button 4 (Mute):          PB0
	GPIOA->MODER &= ~GPIO_MODER_MODER4;
	GPIOB->MODER &= ~GPIO_MODER_MODER0;
	GPIOB->MODER &= ~GPIO_MODER_MODER1;
	GPIOB->MODER &= ~GPIO_MODER_MODER2;
	
	// Activate pull-ups on pushbuttons
	GPIOA->PUPDR &= ~GPIO_PUPDR_PUPDR4;
	GPIOA->PUPDR |= GPIO_PUPDR_PUPDR4_0;
	GPIOB->PUPDR &= ~GPIO_PUPDR_PUPDR0;
	GPIOB->PUPDR |= GPIO_PUPDR_PUPDR0_0;
	GPIOB->PUPDR &= ~GPIO_PUPDR_PUPDR1;
	GPIOB->PUPDR |= GPIO_PUPDR_PUPDR1_0;
	
	// LEDs
	// PB4: Play
	// PB3: Mute
	GPIOB->MODER &= ~GPIO_MODER_MODER3;
	GPIOB->MODER |= 1 << GPIO_MODER_MODER3_Pos;
	GPIOB->MODER &= ~GPIO_MODER_MODER4;
	GPIOB->MODER |= 1 << GPIO_MODER_MODER4_Pos;
	
	// VU LEDs
	// VU1: PA2
	// VU2: PA3
	// VU3: PA7
	// VU4: PA8
	// VU5: PA10
	// VU6: PB7
	// VU7: PB8
	// VU8: PB9
	// VU9: PB10
	// VU10: PC14
	// Channel select: PC15
	GPIOA->MODER &= ~GPIO_MODER_MODER2;
	GPIOA->MODER |= 1 << GPIO_MODER_MODER2_Pos;
	GPIOA->MODER &= ~GPIO_MODER_MODER3;
	GPIOA->MODER |= 1 << GPIO_MODER_MODER3_Pos;
	GPIOA->MODER &= ~GPIO_MODER_MODER7;
	GPIOA->MODER |= 1 << GPIO_MODER_MODER7_Pos;
	GPIOA->MODER &= ~GPIO_MODER_MODER8;
	GPIOA->MODER |= 1 << GPIO_MODER_MODER8_Pos;
	GPIOA->MODER &= ~GPIO_MODER_MODER10;
	GPIOA->MODER |= 1 << GPIO_MODER_MODER10_Pos;
	GPIOB->MODER &= ~GPIO_MODER_MODER7;
	GPIOB->MODER |= 1 << GPIO_MODER_MODER7_Pos;
	GPIOB->MODER &= ~GPIO_MODER_MODER8;
	GPIOB->MODER |= 1 << GPIO_MODER_MODER8_Pos;
	GPIOB->MODER &= ~GPIO_MODER_MODER9;
	GPIOB->MODER |= 1 << GPIO_MODER_MODER9_Pos;
	GPIOB->MODER &= ~GPIO_MODER_MODER10;
	GPIOB->MODER |= 1 << GPIO_MODER_MODER10_Pos;
	GPIOC->MODER &= ~GPIO_MODER_MODER14;
	GPIOC->MODER |= 1 << GPIO_MODER_MODER14_Pos;
	GPIOC->MODER &= ~GPIO_MODER_MODER15;
	GPIOC->MODER |= 1 << GPIO_MODER_MODER15_Pos;
	
	// RELAY pin
	// PB14
	GPIOB->MODER &= ~GPIO_MODER_MODER14;
	GPIOB->MODER |= 1 << GPIO_MODER_MODER14_Pos;
	GPIOB->PUPDR &= ~GPIO_PUPDR_PUPDR14;
	GPIOB->PUPDR |= GPIO_PUPDR_PUPDR14_0;
	
}

int main (void) {

	SystemInit();
	ClockInit();
	SystemCoreClockUpdate();
	(void) SysTick_Config(SystemCoreClock / 1000);
	GPIOInit();
	ShutdownCtlInit();
	
	// Make sure output relay is switched off
	GPIOB->BSRR |= GPIO_BSRR_BR14;
	
	// Disable PLAY and MUTE LEDs
	GPIOB->BSRR |= GPIO_BSRR_BR4;
	GPIOB->BSRR |= GPIO_BSRR_BR3;
	
	// Set sampling rate indicator to 44.1 kHz
	GPIOB->BSRR |= GPIO_BSRR_BR5;
	GPIOB->BSRR |= GPIO_BSRR_BR6;
	
	// Reset all VU LEDs
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
	
	debounceInit();
	
#ifdef DEBUG
	USARTInit(115200);
	printMsg("\r\nStart of initialization\r\n");
#endif

	AudioInit();
	USBDeviceInit();
	
	// Run a LED ramp sequence at startup to show all LEDs are operational
	GPIOC->BSRR |= GPIO_BSRR_BR13;
	delay_ms(100);
	GPIOC->BSRR |= GPIO_BSRR_BS13;
	
	GPIOC->BSRR |= GPIO_BSRR_BR15;
	delay_ms(1);
	
	GPIOA->BSRR |= GPIO_BSRR_BS2;
	delay_ms(50);
	GPIOA->BSRR |= GPIO_BSRR_BS3;
	delay_ms(50);
	GPIOA->BSRR |= GPIO_BSRR_BS7;
	delay_ms(50);
	GPIOA->BSRR |= GPIO_BSRR_BS8;
	delay_ms(50);
	GPIOA->BSRR |= GPIO_BSRR_BS10;
	delay_ms(50);
	GPIOB->BSRR |= GPIO_BSRR_BS7;
	delay_ms(50);
	GPIOB->BSRR |= GPIO_BSRR_BS8;
	delay_ms(50);
	GPIOB->BSRR |= GPIO_BSRR_BS9;
	delay_ms(50);
	GPIOB->BSRR |= GPIO_BSRR_BS10;
	delay_ms(50);
	GPIOC->BSRR |= GPIO_BSRR_BS14;
	delay_ms(50);
	GPIOC->BSRR |= GPIO_BSRR_BS15;
	delay_ms(50);
	GPIOC->BSRR |= GPIO_BSRR_BR14;
	delay_ms(50);
	GPIOB->BSRR |= GPIO_BSRR_BR10;
	delay_ms(50);
	GPIOB->BSRR |= GPIO_BSRR_BR9;
	delay_ms(50);
	GPIOB->BSRR |= GPIO_BSRR_BR8;
	delay_ms(50);
	GPIOB->BSRR |= GPIO_BSRR_BR7;
	delay_ms(50);
	GPIOA->BSRR |= GPIO_BSRR_BR10;
	delay_ms(50);
	GPIOA->BSRR |= GPIO_BSRR_BR8;
	delay_ms(50);
	GPIOA->BSRR |= GPIO_BSRR_BR7;
	delay_ms(50);
	GPIOA->BSRR |= GPIO_BSRR_BR3;
	delay_ms(50);
	GPIOA->BSRR |= GPIO_BSRR_BR2;
	
	ADCInit();
	LEDRampInit();
	
	delay_ms(1000);
	
	// Switch on output relay
	GPIOB->BSRR |= GPIO_BSRR_BS14;
	
    while (1) {
        //__WFI(); // Wait for interrupt
    }

    // Return 0 to satisfy compiler
    return 0;
}
