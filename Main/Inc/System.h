#ifndef SYSTEM_H
#define SYSTEM_H



// #define F_CPU  80000000

#define F_CPU  4000000
//#define QUARTZ_8MHz
// #define QUARTZ_16MHz

void System_Init();
void SetupRunTimeStatsTimer();
uint32_t GetRunTimeCounterValue();



#endif //SYSTEM_H
