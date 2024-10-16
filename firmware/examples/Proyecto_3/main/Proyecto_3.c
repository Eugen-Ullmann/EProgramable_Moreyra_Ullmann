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
#define CONFIG_BLINK_PERIOD 150
#define GPIO_RELE GPIO_1
/*==================[internal data definition]===============================*/
uint32_t threshold = 2400;

/*==================[internal functions declaration]=========================*/

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
    uint16_t valorLectura = 0;

    serial_config_t my_uart = {
        .port = UART_PC,
        .baud_rate = 115200,
        .func_p = NULL,
        .param_p = NULL};

    UartInit(&my_uart);

    GPIOInit(GPIO_RELE, GPIO_OUTPUT);
    GPIOOff(GPIO_RELE);
    GPIOOn(GPIO_RELE);

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
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    
}


/*==================[end of file]============================================*/