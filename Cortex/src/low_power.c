#include "low_power.h"
#include "delay.h"

void stop_cortex(void){
    if(!(RCC->APB1ENR1 & RCC_APB1ENR1_PWREN)){
        RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
    }
        /* флаг PDDS определяет выбор между Stop и Standby, его надо сбросить */
    // EXTI->PR1 |= 0x0DFFFFF; //clear all interrupt flags
    // EXTI->PR2 |= 0x00000F0;
    /* флаг Wakeup должн быть очищен, иначе есть шанс проснуться немедленно */
    // PWR->CR3 &= ~PWR_CR3_EIWUL;  // enable or disable Internal wakeup line.
    PWR->SCR |= 0x9F; //clear all wakeup flags
    PWR->PDCRB |= PWR_PDCRB_PB8;  // MOSI and SCL pull down
    // PWR->PDCRC |= PWR_PUCRC_PC6 | PWR_PDCRC_PC13; // NSS pull up
    // PWR->PDCRC |= PWR_PDCRC_PC7;  // soil sensor power pull down
    // PWR->PDCRA |= PWR_PUCRA_PA15;  // LoRa Reset pull up
    PWR->PDCRA |= PWR_PDCRA_PA2;  // temp sensor power pull down
    /* стабилизатор питания в low-power режим, у нас в Stop потребления-то почти не будет */
    // PWR->CR1 |= PWR_CR1_LPSDSR;

    /* источник опорного напряжения Vref выключить автоматически */
    // PWR->CR2 &= ~PWR_CR2_PVDE;
    PWR->CR3 |= PWR_CR3_APC | PWR_CR3_EIWUL;
    /* с точки зрения ядра Cortex-M, что Stop, что Standby - это режим Deep Sleep */
    /* поэтому надо в ядре включить Deep Sleep */
    PWR->CR1 |= PWR_CR1_LPMS_SHUTDOWN;
    (void)PWR->CR1; // Ensure that the previous PWR register operations have been completed
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
// #ifdef NDEBUG
    // DBGMCU->CR = 0; // Disable debug, trace and IWDG in low-power modes
// #endif
    /* выключили прерывания; пробуждению по ним это не помешает */
    __disable_irq();
    while(1){
    /* завершили незавершённые операция сохранения данных */
        __DSB();
        __WFI();
        Delay(10);
    }
}