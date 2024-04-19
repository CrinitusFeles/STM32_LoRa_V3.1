#include "FreeRTOS.h"
#include "task.h"
#include "monitor_task.h"
#include "xprintf.h"

void _task_state_to_char(eTaskState state, char *buffer)
{
	switch (state)
	{
		case eReady:		xsprintf(buffer, "Ready"); break;
		case eBlocked:		xsprintf(buffer, "Blocked"); break;
		case eSuspended:	xsprintf(buffer, "Suspended"); break;
		case eDeleted:		xsprintf(buffer, "Deleted"); break;
		default:			xsprintf(buffer, "Running"); break;
	}
}

void show_monitor(){
    UBaseType_t task_count = uxTaskGetNumberOfTasks();
    unsigned long _total_runtime;
    TaskStatus_t _buffer[MAX_TASKS_MONITOR];
    char status_buffer[10];
    task_count = uxTaskGetSystemState(_buffer, task_count, &_total_runtime);
    _total_runtime /= 100;
    xprintf("|    NAME     |   STATUS  | PRIORITY | STACK REMAINING | RUNNING TIME, ms |  LOAD  |\n");
    for (uint8_t task = 0; task < task_count; task++)
    {
        _task_state_to_char(_buffer[task].eCurrentState, status_buffer);
        xprintf("|%12s |%10s |%9u |%16u |%17u |%6u%% |\n",
            _buffer[task].pcTaskName,
            status_buffer,
            _buffer[task].uxCurrentPriority,
            _buffer[task].usStackHighWaterMark,
            _buffer[task].ulRunTimeCounter / 1000,
            _buffer[task].ulRunTimeCounter / _total_runtime
        );
    }
        xprintf("Current Heap Free Size: %u\n", xPortGetFreeHeapSize());
        xprintf("Minimal Heap Free Size: %u\n", xPortGetMinimumEverFreeHeapSize());
        xprintf("Total RunTime:  %u ms\n", _total_runtime / 10);
        xprintf("System Uptime:  %u ms\n",  xTaskGetTickCount() * portTICK_PERIOD_MS);
}
