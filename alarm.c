/*
 * 	Name: Luke Gray
 * 	Date: 7/22/26
 * 	Assignment: Lab 5
 * 	Youtube: https://youtu.be/2iRQCRTiBOQ
 *
 * 	This code uses a timer to measure the distance that an ultrasonic wave travels. Depending on this distance,
 * 	another timer is used to buzz an alarm.
 */
#include "stm32f0xx.h"

static inline void init(void){
	RCC->AHBENR |= RCC_AHBENR_GPIOCEN;//Enable GPIOC clock

	GPIOC->MODER &= ~(3u << (13*2));//Set PC13 as input
	GPIOC->PUPDR &= ~(3u << (13*2));//No pull-up/pull-down resistor. Active low

	GPIOC->MODER &= ~(3u << (8*2));
	GPIOC->MODER |= (1u << (8*2));//Set PC8 as output

	GPIOC->MODER &= ~(3u << (6*2));
	GPIOC->MODER |= (1u << (6*2));//Set PC6 to output

	GPIOC->MODER &= ~(3u << (5*2));//Set PC5 to input

	/////////////////////////////////////////////////////////////////////////

	RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;//Enable TIM3 for speaker
	TIM3->CR1 &= ~TIM_CR1_CEN;//Disable TIM3 for setup
	TIM3->PSC = 0;//No pre-scaler, 8 MHz clock
	TIM3->ARR = 15999;// 16000 counts -> 250 Hz speaker frequency
	TIM3->CNT = 0;
	TIM3->EGR |= TIM_EGR_UG;//Update TIM3
	TIM3->SR &= ~TIM_SR_UIF;//Clear update flag
	TIM3->DIER |= TIM_DIER_UIE;//Enable interrupts for TIM3
	NVIC_EnableIRQ(TIM3_IRQn);//Enable TIM3 interrupts in NVIC

	/////////////////////////////////////////////////////////////////////////

	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;//Enable TIM2 for sensor
	TIM2->CR1 &= ~TIM_CR1_CEN;//Disable TIM2 for setup
	TIM2->PSC = 0;// No pre-scaler, 8 MHz clock
	TIM2->ARR = 0xFFFFFFFF;//Set range to maximum
	TIM2->CNT = 0;
	TIM2->EGR |= TIM_EGR_UG;//Update TIM2
	TIM2->SR &= ~TIM_SR_UIF;//Clear flag
}

static inline void delay(int ms){
	int j = 2000*ms;
	for (int i=0; i<j; i++);
}

static inline void ToggleSpeaker(void){
	GPIOC->ODR ^= (1u << 8);
}
static inline void TIM3_Start(void){
	TIM3->CR1 |= TIM_CR1_CEN;
}
static inline void TIM3_Stop(void){
	TIM3->CR1 &= ~TIM_CR1_CEN;
}
static inline void TIM2_Start(void){
	TIM2->CR1 |= TIM_CR1_CEN;
}
static inline void TIM2_Stop(void){
	TIM2->CR1 &= ~TIM_CR1_CEN;
}

void TIM3_IRQHandler(void){
	if (TIM3->SR & TIM_SR_UIF){
		TIM3->SR &= ~TIM_SR_UIF;//Clear interrupt flag
		ToggleSpeaker();
	}
}

static inline float GetDistance(void){//Use PC6 as trigger and PC5 as echo
	GPIOC->ODR |= (1u << 6);//Drive PC6 high
	delay(0.01);// 1 uS delay
	GPIOC->ODR &= ~(1u << 6);//Drive PC6 low

	while(!(GPIOC->IDR & (1u << 5))){//Wait for pin to go high
	}
	TIM2_Start();
	while(GPIOC->IDR & (1u << 5)){///Wait for pin to go low
	}
	uint32_t count = TIM2->CNT;
	TIM2_Stop();
	TIM2->CNT = 0;//Reset count

	return ((count*1125.0)/(2.0*8000000));
}
typedef enum status{
	Idle_State, Armed_State, Alarm_State
} tripwire_state_t;
int main(void){
	init();
	tripwire_state_t status = Idle_State;
	float distance;
	while(1){
		switch (status){

		case Idle_State:
			while(GPIOC->IDR & (1u << 13)){}//Button not pressed
			delay(10);

			status = Armed_State;
			break;

		case Armed_State:
			distance = GetDistance();
			delay(10);
			while(1){
				float temp_distance = GetDistance();
				if ((temp_distance < (distance*0.9)) || (temp_distance > (distance*1.1))){
					status = Alarm_State;
					break;
				}
				delay(10);
			}
			break;

		case Alarm_State:
			TIM3_Start();
			while(GPIOC->IDR & (1u << 13)){}//Button not pressed
			TIM3_Stop();
			delay(50);
			status = Idle_State;
			break;
		}
	}

	return 0;
}