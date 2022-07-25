#include <stdint.h>
#include <math.h>
#include "arm_math.h"
#include "stm32f4xx.h"
#include "audio.h"

void AudioInit(void) {

	int			tmpReg, i;
	
	audio_status.writePtr = 3; // Not working...
	audio_status.readPtr = 0;
	audio_status.diff = 0;
	
	for(i = 0; i < BUF_SIZE; ++i)
		audio_buffer[i] = 0;
	
	// I2S2/SPI2
	RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
	
	// Make sure I2S is disabled
	SPI2->I2SCFGR &= ~SPI_I2SCFGR_I2SE;
	
	tmpReg = SPI2->I2SCFGR;
	// Set I2S mode
	tmpReg |= SPI_I2SCFGR_I2SMOD;
	
	// Master - transmit mode
	tmpReg &= ~SPI_I2SCFGR_I2SCFG;
	tmpReg |= 2 << SPI_I2SCFGR_I2SCFG_Pos;
	
	// I2S Philips standard
	tmpReg &= ~SPI_I2SCFGR_I2SSTD;
	
	// I2S clock steady state is low level
	tmpReg &= ~SPI_I2SCFGR_CKPOL;
	
	// Set to 24 bit data length
	tmpReg &= ~SPI_I2SCFGR_DATLEN;
	tmpReg |= 1 << SPI_I2SCFGR_DATLEN_Pos;
	
	tmpReg |= SPI_I2SCFGR_CHLEN;
	
	SPI2->I2SCFGR = tmpReg;
	
	// I2S master clock output enable
	tmpReg = SPI2->I2SPR;
	tmpReg |= SPI_I2SPR_MCKOE;
	
	tmpReg |= SPI_I2SPR_ODD;
	
	// Linear prescaler value set to 3
	tmpReg &= ~SPI_I2SPR_I2SDIV;
	tmpReg |= 3 << SPI_I2SPR_I2SDIV_Pos;
	SPI2->I2SPR = tmpReg;
	
	SPI2->CR2 |= SPI_CR2_TXDMAEN; 
	
	// Initialize DMA1 stream 4
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
	
	DMA1_Stream4->CR &= ~DMA_SxCR_EN;
	while(DMA1_Stream4->CR & DMA_SxCR_EN);
	
	// SPI2->DR port register address
	DMA1_Stream4->PAR = (uint32_t)&(SPI2->DR);
	
	// Memory address
	DMA1_Stream4->M0AR = (uint32_t)audio_buffer;
	
	// Total number of data items to be transferred
	DMA1_Stream4->NDTR = BUF_SIZE;
	
	tmpReg = DMA1_Stream4->CR;
	
	// Channel 0 (SPI2_TX)
	tmpReg &= ~DMA_SxCR_CHSEL;
	
	tmpReg &= ~DMA_SxCR_DIR;
	tmpReg |= 1 << DMA_SxCR_DIR_Pos;
	
	tmpReg &= ~DMA_SxCR_PFCTRL; // DMA is flow controller
	
	tmpReg &= ~DMA_SxCR_PL;
	tmpReg |= 3 << DMA_SxCR_PL_Pos; // Highest priority
	
	tmpReg &= ~DMA_SxCR_MBURST; // Single memory transfer
	tmpReg &= ~DMA_SxCR_PBURST; // Single peripheral transfer
	tmpReg &= ~DMA_SxCR_DBM; // No double buffer
	tmpReg &= ~DMA_SxCR_PINCOS;
	
	tmpReg &= ~DMA_SxCR_MSIZE;
	tmpReg |= 1 << DMA_SxCR_MSIZE_Pos;
	
	tmpReg &= ~DMA_SxCR_PSIZE;
	tmpReg |= 1 << DMA_SxCR_PSIZE_Pos;
	
	tmpReg |= DMA_SxCR_MINC;
	tmpReg &= ~DMA_SxCR_PINC;
	tmpReg |= DMA_SxCR_CIRC;
	
	DMA1_Stream4->CR = tmpReg;
	
	// Enable DMA and then disable it in order to flush all FIFOs etc
	DMA1->HIFCR |= (DMA_HIFCR_CTCIF4 | DMA_HIFCR_CHTIF4 | DMA_HIFCR_CTEIF4 | DMA_HIFCR_CDMEIF4 | DMA_HIFCR_CFEIF4);
	DMA1_Stream4->CR |= DMA_SxCR_EN;
	
	// Disable DMA
	DMA1_Stream4->CR &= ~DMA_SxCR_EN;
	while(DMA1_Stream4->CR & DMA_SxCR_EN);
	
}

int AudioReconfigure(int fs) {
	
	int		tmpReg;
	
	// Disable DMA
	DMA1_Stream4->CR &= ~DMA_SxCR_EN;
	while(DMA1_Stream4->CR & DMA_SxCR_EN);
	
	// Make sure I2S is disabled
	SPI2->I2SCFGR &= ~SPI_I2SCFGR_I2SE;
	while(!(SPI2->SR & SPI_SR_TXE) && (SPI2->SR & SPI_SR_BSY));
	
	// Disable I2S PLL
	RCC->CR &= ~RCC_CR_PLLI2SON;
	while(RCC->CR & RCC_CR_PLLI2SRDY);
	
	// Reconfigure PLL for chosen sampling frequency
	switch(fs) {
		case 44100:
		case 88200:
			// Set I2S PLL dividers to:
			// M = 25, N = 271, R = 2 for 44.1 kHz and 88.2 kHz
			// This gives an I2S clock of  25 MHz / 25 * 271 / 2 = 135.5 MHz
			// and a sampling frequency of fs = 44.108 kHz when I2SDIV = 3 and I2SODD = 0
			// and fs = 88.216 kHz when I2SDIV = 3 and I2SODD = 0
			// In this case, MCLK = I2S clock / (2 * I2SDIV + I2SODD)
			// MCLK is 256xfs
			tmpReg = RCC->PLLI2SCFGR;
			tmpReg &= ~RCC_PLLI2SCFGR_PLLI2SR;
			tmpReg |= 2 << RCC_PLLI2SCFGR_PLLI2SR_Pos;
	
			tmpReg &= ~RCC_PLLI2SCFGR_PLLI2SN;
			tmpReg |= 271 << RCC_PLLI2SCFGR_PLLI2SN_Pos;
		
			tmpReg &= ~RCC_PLLI2SCFGR_PLLI2SM;
			tmpReg |= 25 << RCC_PLLI2SCFGR_PLLI2SM_Pos;
			RCC->PLLI2SCFGR = tmpReg;
			
			tmpReg = SPI2->I2SPR;
			tmpReg &= ~SPI_I2SPR_ODD;
	
			tmpReg &= ~SPI_I2SPR_I2SDIV;
			if(fs == 44100)
				tmpReg |= 6 << SPI_I2SPR_I2SDIV_Pos;
			else
				tmpReg |= 3 << SPI_I2SPR_I2SDIV_Pos;
			SPI2->I2SPR = tmpReg;
			break;
		case 48000:
		case 96000:
			// Set I2S PLL dividers to:
			// M = 25, N = 344, R = 2 for 96 kHz and
			// M = 25, N = 172, R = 2 for 48 kHz
			// This gives an I2S clock of  25 MHz / 25 * 344 / 2 = 172 MHz for 96 kHz
			// and 25 MHz / 25 * 172 / 2 = 86 MHz for 48 kHz
			// and a sampling frequency of fs = 95.982 (47.991) kHz when I2SDIV = 3 and I2SODD = 1
			// In this case, MCLK = I2S clock / (2 * I2SDIV + I2SODD)
			// MCLK is 256xfs
			tmpReg = RCC->PLLI2SCFGR;
			tmpReg &= ~RCC_PLLI2SCFGR_PLLI2SR;
			tmpReg |= 2 << RCC_PLLI2SCFGR_PLLI2SR_Pos;
	
			tmpReg &= ~RCC_PLLI2SCFGR_PLLI2SN;
			if(fs == 48000)
				tmpReg |= 172 << RCC_PLLI2SCFGR_PLLI2SN_Pos;
			else
				tmpReg |= 344 << RCC_PLLI2SCFGR_PLLI2SN_Pos;
	
			tmpReg &= ~RCC_PLLI2SCFGR_PLLI2SM;
			tmpReg |= 25 << RCC_PLLI2SCFGR_PLLI2SM_Pos;
			RCC->PLLI2SCFGR = tmpReg;
			
			tmpReg = SPI2->I2SPR;
			tmpReg |= SPI_I2SPR_ODD;
	
			tmpReg &= ~SPI_I2SPR_I2SDIV;
			tmpReg |= 3 << SPI_I2SPR_I2SDIV_Pos;
			SPI2->I2SPR = tmpReg;
			break;
		default:
			;
	}
	
	DMA1->HIFCR |= (DMA_HIFCR_CTCIF4 | DMA_HIFCR_CHTIF4 | DMA_HIFCR_CTEIF4 | DMA_HIFCR_CDMEIF4 | DMA_HIFCR_CFEIF4);
	DMA1_Stream4->CR |= DMA_SxCR_EN;
	
	// Enable I2S PLL
	RCC->CR |= RCC_CR_PLLI2SON;
	while(!(RCC->CR & RCC_CR_PLLI2SRDY));
	
	return 1;
}

void EnableAudio(void) {
	
	audio_status.writePtr = 3;
	audio_status.readPtr = 0;
	audio_status.diff = 0;
	
	// Reset and enable DMA memory
	DMA1_Stream4->CR &= ~DMA_SxCR_EN;
	while(DMA1_Stream4->CR & DMA_SxCR_EN);
	
	DMA1->HIFCR |= (DMA_HIFCR_CTCIF4 | DMA_HIFCR_CHTIF4 | DMA_HIFCR_CTEIF4 | DMA_HIFCR_CDMEIF4 | DMA_HIFCR_CFEIF4);
	DMA1_Stream4->CR |= DMA_SxCR_EN;
	while(!(DMA1_Stream4->CR & DMA_SxCR_EN));

}

void DisableAudio(void) {
	
	int	i;
	
	// Disable I2S
	while(!(SPI2->SR & SPI_SR_TXE) && (SPI2->SR & SPI_SR_BSY));
	SPI2->I2SCFGR &= ~SPI_I2SCFGR_I2SE;
	
	// Disable DMA
	DMA1_Stream4->CR &= ~DMA_SxCR_EN;
	while(DMA1_Stream4->CR & DMA_SxCR_EN);
	
	for(i = 0; i < BUF_SIZE; ++i)
		audio_buffer[i] = 0;
}