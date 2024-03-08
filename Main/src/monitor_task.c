#include "FreeRTOS.h"
#include "task.h"
#include "monitor_task.h"
#include "stdio.h"

void _task_state_to_char(eTaskState state, char *buffer)
{
	switch (state)
	{
		case eReady:		snprintf(buffer, 10, "Ready"); break;
		case eBlocked:		snprintf(buffer, 10, "Blocked"); break;
		case eSuspended:	snprintf(buffer, 10, "Suspended"); break;
		case eDeleted:		snprintf(buffer, 10, "Deleted"); break;
		default:			snprintf(buffer, 10, "Running"); break;
	}
}

void show_monitor(){
    UBaseType_t task_count = uxTaskGetNumberOfTasks();
    unsigned long _total_runtime;
    TaskStatus_t _buffer[MAX_TASKS_MONITOR];
    char status_buffer[10];
    task_count = uxTaskGetSystemState(_buffer, task_count, &_total_runtime);
    _total_runtime /= 100;
    printf("NAME\tSTATUS\tPRIORITY\tSTACK REMAINING\tRUNNING TIME, ms\tLOAD\n\r");
    for (uint8_t task = 0; task < task_count; task++)
    {
        _task_state_to_char(_buffer[task].eCurrentState, status_buffer);
        printf("%s\t%s\t%u\t%u\t%u\t%u%%\n\r",
            _buffer[task].pcTaskName,
            status_buffer,
            _buffer[task].uxCurrentPriority,
            _buffer[task].usStackHighWaterMark,
            _buffer[task].ulRunTimeCounter / 1000,
            _buffer[task].ulRunTimeCounter / _total_runtime
        );
    }
        printf("Current Heap Free Size: %u\n\r", xPortGetFreeHeapSize());
        printf("Minimal Heap Free Size: %u\n\r", xPortGetMinimumEverFreeHeapSize());
        printf("Total RunTime:  %u ms\n\r", _total_runtime / 10);
        printf("System Uptime:  %u ms\n\r",  xTaskGetTickCount() * portTICK_PERIOD_MS);
}

void MonitorTask(){

    while(1){
        printf("\033[2J\033[H\n\r");
        show_monitor();
        vTaskDelay(3000);
    }
    vTaskDelete(NULL);
}