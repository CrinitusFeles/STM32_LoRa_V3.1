// #include "main.h"
#include "global_variables.h"
#include "adc.h"
#include "delay.h"
#include "stdio.h"

void ADC_reset_registers(ADC_TypeDef* ADCx){
    ADCx->CR = 0x20000000;
    ADCx->CFGR = 0x80000000;  // reset value
    ADCx->IER = 0;
    ADCx->ISR = 0;
    ADCx->CFGR2 = 0;
    ADCx->SMPR1 = 0;
    ADCx->SMPR2 = 0;
    ADCx->TR1 = 0xFF0000;
    ADCx->TR2 = 0xFF0000;
    ADCx->TR3 = 0xFF0000;
    ADCx->SQR1 = 0;
    ADCx->SQR2 = 0;
    ADCx->SQR3 = 0;
    ADCx->SQR4 = 0;
    ADCx->JSQR = 0;
    ADCx->OFR1 = 0;
    ADCx->OFR2 = 0;
    ADCx->OFR3 = 0;
    ADCx->OFR4 = 0;
}

void ADC_Init(ADC *ADC_struct){
    ADC_reset_registers(ADC_struct->ADCx);
	RCC->AHB2ENR |= RCC_AHB2ENR_ADCEN;  // или придется настраивать делитель PLLADC1CLK
	RCC->CCIPR &= ~RCC_CCIPR_ADCSEL;
	RCC->CCIPR |= 3 << RCC_CCIPR_ADCSEL_Pos;  // set clock source from system clock

	ADC_struct->ADCx->CR &= ~ADC_CR_DEEPPWD;  // Bring the ADC out of 'deep power-down' mode.
	ADC_struct->ADCx->CR |= ADC_CR_ADVREGEN;  // Enable the ADC voltage regulator.
	Delay(30);  // Delay for a handful of microseconds.
	ADC_struct->ADCx->CR |= ADC_CR_ADCAL;  // Calibrate the ADC.
	while (ADC_struct->ADCx->CR & ADC_CR_ADCAL);

    ADC_struct->ADCx->CFGR |= ADC_struct->mode << ADC_CFGR_CONT_Pos;  // continuous or single mode
    ADC_struct->ADCx->CFGR |= ADC_CFGR_OVRMOD;  // data will be always actual
    ADC_struct->ADCx->CFGR |= ADC_CFGR_JQDIS;  // Injected Queue disabled
    ADC_struct->ADCx->CFGR |= ADC_CFGR_AUTDLY;
    ADC_struct->ADCx->CFGR |= ADC_struct->trigger.polarity << ADC_CFGR_EXTEN_Pos;
    ADC_struct->ADCx->CFGR |= ADC_struct->trigger.exti_channel << ADC_CFGR_EXTSEL_Pos;
    ADC_struct->ADCx->CFGR |= ADC_struct->resolution << ADC_CFGR_RES_Pos;

    // enable internal channels
    if(ADC_struct->internal_channels.vref) ADC1_COMMON->CCR |= ADC_CCR_VREFEN;  // CH0
    if(ADC_struct->internal_channels.vbat) ADC1_COMMON->CCR |= ADC_CCR_VBATEN;  // CH18
    if(ADC_struct->internal_channels.temp) ADC1_COMMON->CCR |= ADC_CCR_TSEN;    // CH17

    // enable oversampling
    if(ADC_struct->ovrsmpl_ratio != OVRSMPL_Disable){
        // ADC_struct->ADCx->CFGR2 |= ADC_CFGR2_JOVSE;
        ADC_struct->ADCx->CFGR2 |= ADC_CFGR2_ROVSE;  // Regular channels Oversampling Enable
        ADC_struct->ADCx->CFGR2 |= (ADC_struct->ovrsmpl_ratio - 1) << ADC_CFGR2_OVSR_Pos;
        ADC_struct->ADCx->CFGR2 |= (ADC_struct->ovrsmpl_ratio) << ADC_CFGR2_OVSS_Pos;
    }

    ADC_struct->vrefinternal_cal = 3000 * (*(uint16_t*)VREFINT);
    ADC_struct->reg_ch_queue_pointer = 0;
    ADC_struct->reg_ch_amount = 0;

    // enable interrupts
    ADC_struct->ADCx->IER |= ADC_IER_EOCIE | ADC_IER_EOSIE;
    ADC_struct->ADCx->IER |= ADC_IER_JEOSIE; // | ADC_IER_JEOCIE;
    NVIC_EnableIRQ(ADC1_2_IRQn);

}

float ADC_internal_temp(uint16_t adc_data){
    return (float)(100.0 / ((*(uint16_t*)TS_CAL2) - (*(uint16_t*)TS_CAL1))) * (adc_data - (*(uint16_t*)TS_CAL1)) + 30;
}

void ADC_InitRegChannel(ADC *ADC_struct, ADC_ChannelNum ch_num, GPIO_Pin gpio, ADC_CH_SMP_Time smp_time){
	if(gpio != uninitialized)
        gpio_init(gpio, Analog_mode, Open_drain, no_pull, Input);
    ADC_struct->reg_channel_queue[ADC_struct->reg_ch_amount].ch_num = ch_num;
    ADC_struct->reg_channel_queue[ADC_struct->reg_ch_amount].pin = gpio;
    ADC_struct->reg_channel_queue[ADC_struct->reg_ch_amount].group = ADC_CH_Regular;
    ADC_struct->reg_channel_queue[ADC_struct->reg_ch_amount].smp_time = smp_time;
    ch_num--;
    if (ch_num <= 9) {
		ADC_struct->ADCx->SMPR1 |= (smp_time << (ch_num * 3));
	} else {
		ADC_struct->ADCx->SMPR2 |= (smp_time << ((ch_num * 3) - 30));
	}
    ADC_struct->reg_ch_amount++;
    ADC_struct->ADCx->SQR1 &= ~0x0F;
    ADC_struct->ADCx->SQR1 |= (ADC_struct->reg_ch_amount) << ADC_SQR1_L_Pos;
    if(ADC_struct->reg_ch_amount <= 4)
        ADC_struct->ADCx->SQR1 |= ch_num << (ADC_struct->reg_ch_amount * 6);
    else if(ADC_struct->reg_ch_amount <= 9)
        ADC_struct->ADCx->SQR2 |= ch_num << ((ADC_struct->reg_ch_amount * 6) - 30);
    else if(ADC_struct->reg_ch_amount <= 14)
        ADC_struct->ADCx->SQR3 |= ch_num << ((ADC_struct->reg_ch_amount * 6) - 60);
    else if(ADC_struct->reg_ch_amount <= 16)
        ADC_struct->ADCx->SQR4 |= ch_num << ((ADC_struct->reg_ch_amount * 6) - 90);
}

void ADC_Start(ADC *ADC_struct){
    ADC_struct->ADCx->CR |= ADC_CR_ADSTART;
    ADC_struct->measure_process = 1;
}

void ADC_Disable(ADC *ADC_struct){

}
// void ADC_StartConversion(ADC *ADC_struct){
//     ADC_struct->ADCx->CR |= ADC_CR_ADSTART;
// }

void ADC_Handler(){
    if(adc.ADCx->ISR & ADC_ISR_ADRDY){
        adc.ADCx->ISR |= ADC_ISR_ADRDY;
    }
    if(adc.ADCx->ISR & ADC_ISR_EOC){  // Inside the regular sequence, after each conversion is complete
        uint16_t result = adc.ADCx->DR;
        adc.reg_channel_queue[adc.reg_ch_queue_pointer].result = result;
        adc.reg_ch_queue_pointer++;
    }
    if(adc.ADCx->ISR & ADC_ISR_EOS){  // After the regular sequence is complete
        adc.reg_ch_queue_pointer = 0;
        adc.vdda_mvolt = adc.vrefinternal_cal / (float)(adc.reg_channel_queue[11].result);
        for(uint8_t i = 0; i < 11; i++){
            adc.reg_channel_queue[i].result_mv = adc.reg_channel_queue[i].result * adc.vdda_mvolt / 4095;
        }
        adc.ADCx->ISR |= ADC_ISR_EOS;
        adc.measure_process = 0;
    }
    if(adc.ADCx->ISR & ADC_ISR_JEOC){  // Inside the injected sequence, after each conversion is complete:
        adc.ADCx->ISR |= ADC_ISR_JEOC;
    }
    if(adc.ADCx->ISR & ADC_ISR_JEOS){  // After the injected sequence is complete
        adc.ADCx->ISR |= ADC_ISR_JEOS;
    }
}

uint16_t ADC_array_to_str(ADC *adc, size_t length, char *buf, size_t buf_size, uint16_t offset){
    uint16_t b_end = offset;
    for(uint16_t i = 0; i < buf_size; i++){
        if(buf[i + offset] == 0){
            b_end += i;
            break;
        }
    }
    uint16_t wrote_count = 0;
    uint8_t w_size = 0;
    for(uint8_t i = 0; i < length; i++){
        w_size = snprintf(buf + b_end + wrote_count, 10, "%d\t", adc->reg_channel_queue[i].result);
        if(w_size > 0)
            wrote_count += w_size;
    }
    w_size = snprintf(buf + b_end + wrote_count, 10, "%d\r\n", adc->vdda_mvolt);
    if(w_size > 0)
        wrote_count += w_size;
    return wrote_count;
}

void ADC_Enable(ADC *adc){
    adc->ADCx->CR |= ADC_CR_ADEN;
    while(!(ADC1->ISR & ADC_ISR_ADRDY));
    adc->ADCx->ISR |= ADC_ISR_ADRDY;
}

uint8_t ADC_WaitMeasures(ADC *adc, uint32_t timeout){
    while(adc->measure_process && timeout--);
    if(timeout == 0) return 1;
    return 0;
}