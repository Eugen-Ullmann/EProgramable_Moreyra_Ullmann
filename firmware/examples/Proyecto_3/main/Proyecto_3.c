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
#include "ble_mcu.h"
#include "led.h"
/*==================[macros and definitions]=================================*/
#define GPIO_RELE GPIO_1
#define CONFIG_PERIOD_US 150 * 1000
#define CONFIG_PERIOD_US2 500 * 1000
#define CONFIG_BLINK_PERIOD 500
/*==================[internal data definition]===============================*/
uint32_t threshold = 2400;
TaskHandle_t deteccionHumedad_handle = NULL;
TaskHandle_t notifyBlueTooth_handle = NULL;
uint16_t valorLectura = 0;
bool gpio_rele = 0;
/*==================[internal functions declaration]=========================*/
void FuncTimerDeteccion(void *param)
{
    vTaskNotifyGiveFromISR(deteccionHumedad_handle, pdFALSE); /* Envía una notificación a la tarea asociada */
}
void FuncTimerBT(void *param)
{
    vTaskNotifyGiveFromISR(notifyBlueTooth_handle, pdFALSE); /* Envía una notificación a la tarea asociada */
}
static void deteccionHumedad(void *pvParameter)
{

    while (true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        AnalogInputReadSingle(CH0, &valorLectura);
        UartSendString(UART_PC, (char *)UartItoa(valorLectura, 10));
        UartSendString(UART_PC, "\r\n");

        if (valorLectura > threshold)
        {
            GPIOOff(GPIO_RELE);
            gpio_rele = 0;
        }
        else
        {
            GPIOOn(GPIO_RELE);
            gpio_rele = 1;
        }
    }
}
static void notifyBT(void *pvParameter)
{
    uint16_t humedadMinima = 3300;
    uint16_t humedadMaxima = 500;

    char msg[48];
    while (true)
    {
        vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
        float porcentajeHumedad = 100.0 * (humedadMinima - valorLectura) / (humedadMinima - humedadMaxima);
        sprintf(msg, "*H%d \n", (int)porcentajeHumedad);
        BleSendString(msg);
        printf(msg);

         if (gpio_rele == 1)
            {
                BleSendString("*CLa planta se esta regando \n");
            }
            else
            BleSendString("*C\n");
    }
}
/*==================[external functions definition]==========================*/
void app_main(void)
{
    LedsInit();

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

    /* Bluetooth configuration */
    ble_config_t ble_configuration = {
        "Regador Automatizado",

    };
    BleInit(&ble_configuration);

    /* Timer configuration */
    timer_config_t timer_deteccion = {
        .timer = TIMER_A,
        .period = CONFIG_PERIOD_US,
        .func_p = FuncTimerDeteccion,
        .param_p = NULL};
    TimerInit(&timer_deteccion);
    TimerStart(timer_deteccion.timer);

    timer_config_t timer_BT = {
        .timer = TIMER_B,
        .period = CONFIG_PERIOD_US2,
        .func_p = FuncTimerBT,
        .param_p = NULL};
    TimerInit(&timer_BT);
    TimerStart(timer_BT.timer);

    xTaskCreate(&deteccionHumedad, "Sensado", 512, NULL, 5, &deteccionHumedad_handle);
    xTaskCreate(&notifyBT, "Bluetooth", 2048, NULL, 5, &notifyBlueTooth_handle);

    while (1)
    {
        vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
        switch (BleStatus())
        {
        case BLE_OFF:
            
            LedsOffAll();LedOff(LED_2);
            break;
        case BLE_DISCONNECTED:
            LedToggle(LED_3);
            LedOff(LED_1);
            LedOff(LED_2);
            break;
        case BLE_CONNECTED:
            
            LedsOffAll();
            LedOn(LED_1);
            break;
        }
    }
}

/*==================[end of file]============================================*/
