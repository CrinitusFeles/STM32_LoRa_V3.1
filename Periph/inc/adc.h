#ifndef INC_ADC_H_
#define INC_ADC_H_

#include "stm32l4xx.h"
#include "gpio.h"
#include "stdbool.h"

#define VREFINT  0x1FFF75AA
#define TS_CAL1  0x1FFF75A8
#define TS_CAL2  0x1FFF75CA

typedef enum ADC_Resolution{
    ADC_12bit,
    ADC_10bit,
    ADC_8bit,
    ADC_6bit
} ADC_Resolution;

typedef enum ADC_ChannelNum{
    adc_ch_uninitialized,
    VREF,
    CH1,
    CH2,
    CH3,
    CH4,
    CH5,
    CH6,
    CH7,
    CH8,
    CH9,
    CH10,
    CH11,
    CH12,
    CH13,
    CH14,
    CH15,
    CH16,
    TEMP,
    VBAT
} ADC_ChannelNum;

typedef enum ADC_ClockDevider{
    ADC_ClockDevider_1,
    ADC_ClockDevider_2,
    ADC_ClockDevider_4,
    ADC_ClockDevider_6,
    ADC_ClockDevider_8,
    ADC_ClockDevider_10,
    ADC_ClockDevider_12,
    ADC_ClockDevider_16,
    ADC_ClockDevider_32,
    ADC_ClockDevider_64,
    ADC_ClockDevider_128,
    ADC_ClockDevider_256
} ADC_ClockDevider;

typedef enum ADC_OversamplingRatio{
    OVRSMPL_Disable,
    OVRSMPL_2x,
    OVRSMPL_4x,
    OVRSMPL_8x,
    OVRSMPL_16x,
    OVRSMPL_32x,
    OVRSMPL_64x,
    OVRSMPL_128x,
    OVRSMPL_256x
} ADC_OversamplingRatio;

typedef enum ADC_CH_SMP_Time{
    SMP_2,
    SMP_6,
    SMP_12,
    SMP_24,
    SMP_47,
    SMP_92,
    SMP_247,
    SMP640
} ADC_CH_SMP_Time;

typedef enum ADC_CH_Group{
    ADC_CH_Injected,
    ADC_CH_Regular
} ADC_CH_Group;

typedef struct ADC_channel{
    ADC_ChannelNum ch_num;
    GPIO_Pin pin;
    ADC_CH_Group group;
    ADC_CH_SMP_Time smp_time;
    const char *label;
    uint16_t result;
    uint16_t result_mv;
} ADC_channel;

typedef enum ADC_Mode{
    ADC_SINGLE_MODE,
    ADC_CONT_MODE
} ADC_Mode;

typedef struct ADC_Internal_channels{
    bool vref;
    bool temp;
    bool vbat;
} ADC_Internal_channels;

typedef enum ADC_EXTI_Polarity{
    ADC_Software_trigger,
    ADC_EXTI_Rising,
    ADC_EXTI_Falling,
    ADC_EXTI_Rising_Falling
} ADC_EXTI_Polarity;


/// @brief
/*
External triggers for regular channels
______________________________
Name  Source       EXTSEL[3:0]
______________________________
EXT0  TIM1_CH1     0000
EXT1  TIM1_CH2     0001
EXT2  TIM1_CH3     0010
EXT3  TIM2_CH2     0011
EXT4  TIM3_TRGO(1) 0100
EXT5  -            0101
EXT6  EXTI line 11 0110
EXT9  TIM1_TRGO    1001
EXT10 TIM1_TRGO2   1010
EXT11 TIM2_TRGO    1011
EXT13 TIM6_TRGO    1101
EXT14 TIM15_TRGO   1110
======================================
External trigger for injected channels
______________________________________
Name   Source           JEXTSEL[3..0]
______________________________________
JEXT0  TIM1_TRGO        0000
JEXT1  TIM1_CH4         0001
JEXT2  TIM2_TRGO        0010
JEXT3  TIM2_CH1         0011
JEXT4  TIM3_CH4(1)      0100
JEXT5  -                0101
JEXT6  EXTI line 15     0110
JEXT8  TIM1_TRGO2       1000
JEXT14 TIM6_TRGO        1110
JEXT15 TIM15_TRGO       1111
*/
typedef struct ADC_Trigger{
    uint8_t exti_channel;  // (J)EXT0-(J)EXT14  устанавливает источник внешнего триггера
    ADC_EXTI_Polarity polarity;  // по какому фронту будет срабатывать триггер
} ADC_Trigger;

typedef struct ADC{
    ADC_TypeDef *ADCx;
    ADC_channel reg_channel_queue[16];
    ADC_Internal_channels internal_channels;
    ADC_Mode mode;
    ADC_Resolution resolution;
    ADC_Trigger trigger;
    ADC_OversamplingRatio ovrsmpl_ratio;
    ADC_ClockDevider clk_devider;
    uint8_t reg_ch_amount;  // количество опрашиваемых регулярных каналов
    uint8_t inj_ch_amount;  // количество опрашиваемых инжектированных каналов
    uint32_t vrefinternal_cal;  // = *(uint16_t*)0x1FFF75AA
    uint8_t reg_ch_queue_pointer;  // текущий номер регулярного канала, данные которого лежат в регистре (нужен для обработчика прерываний)
    // uint8_t __ch_counter;
    uint16_t vdda_mvolt;
    uint8_t measure_process;
    void (*delay_ms)(uint32_t);
} ADC;

// void ADC_InitGPIO(*ADC);
void ADC_Init(ADC *ADC_struct);
void ADC_Enable(ADC *ADC_struct);
void ADC_Start(ADC *ADC_struct);
// void ADC_Disable(ADC *ADC_struct);
uint8_t ADC_WaitMeasures(ADC *adc, uint32_t timeout);
void ADC_InitRegChannel(ADC *ADC_struct, ADC_ChannelNum ch_num, GPIO_Pin gpio, ADC_CH_SMP_Time smp_time);
// void adc_single_conversion( ADC_TypeDef* ADCx, uint16_t* adc_data_arr );
float ADC_internal_temp(uint16_t adc_data);
void ADC_Handler();
uint16_t ADC_array_to_str(ADC *adc, uint32_t length, char *buf, uint32_t buf_size, uint16_t offset);

#endif /* INC_ADC_H_ */
