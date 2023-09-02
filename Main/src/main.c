#include "main.h"
#include "low_power.h"
#include "string.h"
#include "System.h"


volatile time_t current_time;
void LED_BLINK(){
	gpio_toggle(LED);
	// Delay(1000);
}

int main(){
    System_Init();

    while(1){
        battary_voltage = !battary_voltage;
        Delay(3000);
        gpio_state(LED, HIGH);
        Delay(3000);
        gpio_state(LED, LOW);
        // stop_cortex();
    }
}


