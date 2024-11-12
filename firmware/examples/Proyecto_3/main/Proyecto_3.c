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
 *  * |   EDU-ESP    | Periferico|
 * |:----------:|:-----------|
 * | CH0    | Sensor de humedad	| 
 * | +3.3V    | 	+3.3V   |
 * | GND    | 		GND   |
 * | GPIO_1   | Rele	|
 * | GND    | 		GND   |
 * | +5V    | 		+5V   |
 * | GND    | 		GND   |
 * | GPIO_3    | 	ECHO	|
 * | GPIO_2    | 	TRIGGER |
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
#include "timer_mcu.h"
#include "gpio_mcu.h"
#include "ble_mcu.h"
#include "led.h"
#include "hc_sr04.h"
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
bool tanquevacio=0;
uint16_t distancia;
uint16_t volumen_restante;
uint16_t auxiliar;
/*==================[internal functions declaration]=========================*/
/**
 * @brief Notifica a la tarea asociada para la detección
 * @param param puntero a un parámetro que no se utiliza
 */
void FuncTimerDeteccion(void *param)
{
    vTaskNotifyGiveFromISR(deteccionHumedad_handle, pdFALSE); /* Envía una notificación a la tarea asociada */
}
/**
 * @brief Notifica a la tarea asociada para que envíe los datos por Bluetooth
 * @param param puntero a un parámetro que no se utiliza
 */
void FuncTimerBT(void *param)
{
    vTaskNotifyGiveFromISR(notifyBlueTooth_handle, pdFALSE); /* Envía una notificación a la tarea asociada */
}
/**
 * @brief  Lee el valor del sensor de humedad cada vez que recibe una notificación y
 *         lo envía por UART. Si el valor es mayor al umbral, apaga el rele, de lo
 *         contrario lo enciende.
 * @param pvParameter puntero a un parámetro que no se utiliza
 */
static void deteccionHumedad(void *pvParameter)
{

    while (true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        AnalogInputReadSingle(CH0, &valorLectura);
        UartSendString(UART_PC, (char *)UartItoa(valorLectura, 10));
        UartSendString(UART_PC, "\r\n");
    if (!tanquevacio)
    {
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
      //  vTaskDelay(CONFIG_PERIOD_US / portTICK_PERIOD_MS);
        distancia = HcSr04ReadDistanceInCentimeters();
        auxiliar = 17.0 - distancia;
        volumen_restante = (11.0*11.0* (float)auxiliar);
        UartSendString(UART_PC, (char *)UartItoa(distancia, 10));
        UartSendString(UART_PC, "\r\n");
        UartSendString(UART_PC, "El volumen restante es: ");
        UartSendString(UART_PC, (char *)UartItoa(volumen_restante, 10));
        UartSendString(UART_PC, "\r\n");
        if (volumen_restante < 1)
        tanquevacio = 1;
        else
        tanquevacio = 0;
    }
}
/**
 * @brief Envía por Bluetooth el porcentaje de humedad y el estado del riego
 *        cada CONFIG_BLINK_PERIOD milisegundos.
 *
 * @param pvParameter puntero a un parámetro que no se utiliza
 */
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
        sprintf(msg, "*A%d \n", (int)volumen_restante);
        BleSendString(msg);


         if (gpio_rele == 1)
            {
                BleSendString("*CLa planta se rego \n");
            }
            else
            BleSendString("*C\n");
    }
}

/*==================[external functions definition]==========================*/
void app_main(void)
{
    LedsInit();

    HcSr04Init(GPIO_3, GPIO_2);

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
