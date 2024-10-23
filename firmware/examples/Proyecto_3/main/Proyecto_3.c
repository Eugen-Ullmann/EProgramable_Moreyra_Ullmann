/*! @mainpage Project 3
 *
 * \section genDesc General Description
 *
 *.
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 12/09/2023 | Document creation		                         |
 *
 * @author Eugen Ullmann (eu.ullmann@gmail.com) & Jesus Moreyra (jesus.moreyra@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "analog_io_mcu.h"
#include "uart_mcu.h"
#include "freertos/task.h"
#include "timer_mcu.h"
#include "gpio_mcu.h"
/*==================[macros and definitions]=================================*/
#define GPIO_RELE GPIO_1
#define CONFIG_PERIOD_US 150 * 1000
/*==================[internal data definition]===============================*/
uint32_t threshold = 2400;
TaskHandle_t deteccionHumedad_handle = NULL;

/*==================[internal functions declaration]=========================*/
void FuncTimerDeteccion(void *param)
{
    vTaskNotifyGiveFromISR(deteccionHumedad_handle, pdFALSE); /* Envía una notificación a la tarea asociada */
}
static void deteccionHumedad(void *pvParameter)
{
    uint16_t valorLectura = 0;
    while (true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        AnalogInputReadSingle(CH0, &valorLectura);
        UartSendString(UART_PC, (char *)UartItoa(valorLectura, 10));
        UartSendString(UART_PC, "\r\n");

        if (valorLectura > threshold)
        {
            GPIOOff(GPIO_RELE);
        }
        else
        {
            GPIOOn(GPIO_RELE);
        }
    }
}
/*==================[external functions definition]==========================*/
void app_main(void)
{
    analog_input_config_t config;

    config.input = CH0;
    config.mode = ADC_SINGLE;
    config.func_p = NULL;
    config.param_p = NULL;
    config.sample_frec = NULL;

    AnalogInputInit(&config);

    serial_config_t my_uart = {
        .port = UART_PC,
        .baud_rate = 115200,
        .func_p = NULL,
        .param_p = NULL};

    UartInit(&my_uart);

    GPIOInit(GPIO_RELE, GPIO_OUTPUT);
    GPIOOff(GPIO_RELE);
    GPIOOn(GPIO_RELE);

    /* Timer configuration */
    timer_config_t timer_deteccion = {
        .timer = TIMER_A,
        .period = CONFIG_PERIOD_US,
        .func_p = FuncTimerDeteccion,
        .param_p = NULL};
    TimerInit(&timer_deteccion);
    TimerStart(timer_deteccion.timer);
 

    xTaskCreate(&deteccionHumedad, "Sensado", 512, NULL, 5, &deteccionHumedad_handle);
}

/*==================[end of file]============================================*/   



/*
        while (true)
        {

            AnalogInputReadSingle(CH0, &valorLectura);
            UartSendString(UART_PC, (char *)UartItoa(valorLectura, 10));
            UartSendString(UART_PC, "\r\n");

            if (valorLectura > threshold)
            {
                GPIOOff(GPIO_RELE);
            }
            else
            {
                GPIOOn(GPIO_RELE);
            }
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }   */