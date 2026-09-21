#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

#define IR_PIN GPIO_NUM_4
#define LED_PIN GPIO_NUM_2

#define DEBOUNCE_DELAY_MS 100

QueueHandle_t interruptQueue;

int objectCount = 0;

// ISR
void IRAM_ATTR irISR(void *arg)
{
    int msg = 1;

    xQueueSendFromISR(
        interruptQueue,
        &msg,
        NULL);
}

// Task
void irTask(void *param)
{
    int received;

    TickType_t debounceTicks =
        pdMS_TO_TICKS(DEBOUNCE_DELAY_MS);

    while (1)
    {
        if (xQueueReceive(
                interruptQueue,
                &received,
                portMAX_DELAY))
        {
            // Disable interrupt temporarily
            gpio_intr_disable(IR_PIN);

            // Small debounce delay
            vTaskDelay(debounceTicks);

            // Check if object still present
            if (gpio_get_level(IR_PIN) == 0)
            {
                // Object detected

                objectCount++;

                gpio_set_level(LED_PIN, 1);

                printf(
                    "Object Detected! Count = %d\n",
                    objectCount);

                // Wait until object removed
                while (gpio_get_level(IR_PIN) == 0)
                {
                    vTaskDelay(pdMS_TO_TICKS(50));
                }

                gpio_set_level(LED_PIN, 0);
            }

            // Re-enable interrupt
            gpio_intr_enable(IR_PIN);
        }
    }
}

void app_main()
{
    interruptQueue =
        xQueueCreate(10, sizeof(int));

    gpio_set_direction(
        IR_PIN,
        GPIO_MODE_INPUT);

    gpio_set_intr_type(
        IR_PIN,
        GPIO_INTR_NEGEDGE);

    gpio_set_direction(
        LED_PIN,
        GPIO_MODE_OUTPUT);

    gpio_install_isr_service(0);

    gpio_isr_handler_add(
        IR_PIN,
        irISR,
        NULL);

    xTaskCreate(
        irTask,
        "IR Task",
        2048,
        NULL,
        1,
        NULL);
}
