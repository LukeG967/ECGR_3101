/*
 *	Name: Luke Gray
 *	Date: 7/15/26
 *	Assignment: Lab 4
 *	Youtube: https://youtu.be/n-k89OHXrwI
 *
 *	This program uses the UART peripheral to play songs on a
 *	DFPlayer mini wired to a speaker
 */
#include "stm32f0xx.h"
#include <stdbool.h>

//PC13 is the button
//PA9 is the USART1_TX (AF1)
static inline void init(void){
	RCC->AHBENR |= RCC_AHBENR_GPIOCEN;//Enable GPIO C clock
	GPIOC->MODER &= ~(3u << (13*2));//Set PC13 as input
	GPIOC->PUPDR &= ~(3u << (13 * 2));//No pull-up/pull-down resistor

	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;//Enable GPIO A clock
	GPIOA->MODER &= ~(3u << (9*2));
	GPIOA->MODER |= (2u << (9*2));//Set PA9 as alternate function

	RCC->APB2ENR |= RCC_APB2ENR_USART1EN;//Enable USART1 clock
	GPIOA->AFR[1] &= ~(15u << 4);
	GPIOA->AFR[1] |= (1u << 4);//Set PA9 to AF1 (USART1_TX)
	USART1->BRR = 833u;//Set baud rate to roughly 9600
	USART1->CR2 &= ~(3u << (12));//Sets one stop bit
	USART1->CR1 |= USART_CR1_TE;//Enable USART1 transmission.
	USART1->CR1 |= USART_CR1_UE;//Enable UART1 peripheral
}

static inline uint8_t readButton(void){
	return ((GPIOC->IDR & (1u << 13)) == 0u) ? 1u: 0u;//Active low
}

static inline void delay(int ms){
	int j = 2000*ms;
	for (int i=0; i<j; i++);
}

static inline void UART_SendByte(uint8_t data){
	while((USART1->ISR & USART_ISR_TXE) == 0u){
		//Do nothing, transmit register isn't clear yet
	}
	USART1->TDR = data;
}

static inline void DFPlayer_SendCMD(uint8_t cmd, uint8_t dh, uint8_t dl){
	uint16_t checksum = 0 - (0xFF + 0x06 + cmd + 0x00 + dh + dl);
	uint8_t checksumhigh = (uint8_t)(checksum >> 8);//8 most significant bits of checksum
	uint8_t checksumlow = (uint8_t)(checksum & 0xFF);//8 least significant bits of checksum

	UART_SendByte(0x7E);		//1. Start byte
	UART_SendByte(0xFF);		//2. Version
	UART_SendByte(0x06);		//3. Length (6)
	UART_SendByte(cmd);			//4. Command
	UART_SendByte(0x00);		//5. Feedback (none)
	UART_SendByte(dh);			//6. Parameter high byte
	UART_SendByte(dl);			//7. Parameter low byte
	UART_SendByte(checksumhigh);//8. Checksum high byte
	UART_SendByte(checksumlow);	//9. Checksum low byte
	UART_SendByte(0xEF);		//10. End byte
}

static inline void DFPlayer_Pause(void){
	DFPlayer_SendCMD(0x0E, 0x00, 0x00);
}

static inline void DFPlayer_Resume(void){
	DFPlayer_SendCMD(0x0D, 0x00, 0x00);
}

static inline void DFPlayer_Set_Volume(void){
	DFPlayer_SendCMD(0x06, 0x00, 0x14);
}

static inline void DFPlayer_Play_Next(void){
	DFPlayer_SendCMD(0x01, 0x00, 0x00);
}

static inline void DFPlayer_Play_First_Track(void){
	DFPlayer_SendCMD(0x03, 0x00, 0x03);
}
static inline void DFPlayer_Play_Second_Track(void){
	DFPlayer_SendCMD(0x03, 0x00, 0x01);
}
static inline void DFPlayer_Play_Third_Track(void){
	DFPlayer_SendCMD(0x03, 0x00, 0x02);
}
int main(void){
	init();//Initialize peripherals

	delay(2000);// wait for DFPLayer to boot

	DFPlayer_Set_Volume();

	bool paused = true;
	uint8_t track = 1;

	DFPlayer_Play_First_Track();
	DFPlayer_Pause();

	while(1){

		if(readButton() == 1u){
			delay(25);//de-bounce button
			uint32_t time_held = 0;
			while(readButton()){
				delay(1);
				time_held++;
			}
			if (time_held > 225){
				switch (track){
				case 1:
					DFPlayer_Play_Second_Track();
					track = 2;
					break;

				case 2:
					DFPlayer_Play_Third_Track();
					track = 3;
					break;

				case 3:
					DFPlayer_Play_First_Track();
					track = 1;
					break;
				}
			}
			else{
				if (paused){
					DFPlayer_Resume();
					paused = false;
				}
				else{
					DFPlayer_Pause();
					paused = true;
				}
			}
		}
	}
	return 0;
}