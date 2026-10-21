/*
 * Name: Luke Gray
 * Date: 7/29/26
 * Assignment: Lab 6
 * Youtube: https://youtu.be/nwuEiS112sY
 * 
 * This code uses an ADC to read a light sensor to determine if an LED should
 * be turned on or off
 */
#include "stm32f0xx.h"

static inline void init(void){
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;//Enable GPIO A, as we will be using PA0 which connects to channel 0

	GPIOA->MODER |= (3u);//Set PA0 to analog mode
	GPIOA->MODER &= ~(3u << (5*2));
	GPIOA->MODER |= (1u << (5*2));//Set PA5 (LED) to output mode

	RCC->APB2ENR |= RCC_APB2ENR_ADCEN;//Enable ADC clock
	if (ADC1->CR & ADC_CR_ADEN){//Check if ADC is enabled
		ADC1->CR |= ADC_CR_ADDIS;//Disable ADC for setup
		while(ADC1->CR & ADC_CR_ADEN){}//Wait for ADC to be disabled
	}
	ADC1->CHSELR = ADC_CHSELR_CHSEL0;//Select channel 0, as PA0 is being used

	ADC1->SMPR |= ADC_SMPR_SMP_0;// Set 8 clock cycle sample time
	ADC1->CFGR1 &= ~(3u << 3);//Set 12 bit resolution by writing 00 to RES
	ADC->CCR |= ADC_CCR_VREFEN;//Set reference voltage
	ADC1->CR|= ADC_CR_ADCAL;//Begin ADC calibration
	while (ADC1->CR & ADC_CR_ADCAL){}//Wait for calibration to finish. ADCAL is cleared automatically once finished
	ADC1->CFGR2 &= ~(3u << 30);//Set clock to ADCCLK

	ADC1->ISR |= ADC_ISR_ADRDY;//Set ready flag
	ADC1->CR |= ADC_CR_ADEN;//Enable ADC
	while (!(ADC1->ISR & ADC_ISR_ADRDY));//Wait for ADC to be ready

}

static inline void SetLED(void){
	GPIOA->ODR |= (1u << 5);
}
static inline void ResetLED(void){
	GPIOA->ODR &= ~(1u << 5);
}

static inline float ReadSensor(){
	ADC1->CR |= ADC_CR_ADSTART;//Begin sample
	while (!(ADC1->ISR & ADC_ISR_EOC)){}//Wait for sampling to finish

	return (((uint16_t)ADC1->DR)*(3.3/4096));//Return data
}

int main(){
	init();
	float data;
	while (1){
		data = ReadSensor();
		if (data < 0.3){//We found that sensor outputs ~0.28 when covered with a finger
			SetLED();
		}
		else{
			ResetLED();
		}
	}

	return 0;
}
