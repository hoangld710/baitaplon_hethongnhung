/* Hook của FreeRTOS: cần configCHECK_FOR_STACK_OVERFLOW = 2 và configUSE_MALLOC_FAILED_HOOK = 1 */
#include "FreeRTOS.h"
#include "task.h"
#include "bsp_led.h"

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;          /* đặt breakpoint ở đây để biết task nào tràn stack */
    taskDISABLE_INTERRUPTS();
    bsp_led_fatal(6);
}

void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();  /* heap không đủ: tăng configTOTAL_HEAP_SIZE */
    bsp_led_fatal(7);
}
