#include "app_ipc.h"
#include "app_cfg.h"

QueueHandle_t     xQueueSensor  = NULL;
QueueHandle_t     xQueueDisplay = NULL;
SemaphoreHandle_t xMutexI2C     = NULL;
SemaphoreHandle_t xMutexSPI     = NULL;

volatile UBaseType_t g_stackHwm[3] = {0, 0, 0};

BaseType_t app_ipc_init(void)
{
    xQueueSensor  = xQueueCreate(APP_Q_SENSOR_LEN,  sizeof(ScaleData_t));
    xQueueDisplay = xQueueCreate(APP_Q_DISPLAY_LEN, sizeof(ScaleDisplay_t));
    xMutexI2C     = xSemaphoreCreateMutex();
    xMutexSPI     = xSemaphoreCreateMutex();

    if (xQueueSensor == NULL || xQueueDisplay == NULL ||
        xMutexI2C == NULL || xMutexSPI == NULL) {
        return pdFAIL;
    }
    return pdPASS;
}
