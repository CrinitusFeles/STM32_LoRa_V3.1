#include "global_variables.h"


void init_global_variables(){
	// --------- System condition -----------//
	SYSTEM_init_status = SUCCESS;
	SYSTEM_I2C_error_flag = SUCCESS;
	SYSTEM_I2C_error_counter = 0;

	// ===================================== //

	// --------- ADC -----------//
	battary_voltage = 0;
	//===========================//

	// RTC_struct rtc = {20, 2, 1, 3, 18, 23, 0};


}
