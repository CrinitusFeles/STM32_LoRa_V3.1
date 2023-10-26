#include "main.h"
#include "low_power.h"
#include "string.h"
#include "System.h"


void LED_BLINK(){
	gpio_toggle(LED);
	// Delay(1000);
}

int main(){
    System_Init();

    while(1){
        battary_voltage = !battary_voltage;
        Delay(1000);
        gpio_toggle(LED);
        // stop_cortex();
    }
}


