#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "my_spi.h"
#include "myiic.h"
#include "xl9555.h"
#include "spilcd.h"
#include "spi_sd.h"
#include "sdmmc_cmd.h"
#include "es8388.h"
#include "myi2s.h"
#include "recorder.h"
#include <stdio.h>

TaskHandle_t Task1Task_Handler;  
TaskHandle_t Task2Task_Handler;  
TaskHandle_t Task3Task_Handler;


extern "C"
{
void app_main(void)
{
    esp_err_t ret;

    ret = nvs_flash_init();    
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }


    my_spi_init();              
    myiic_init();               
    xl9555_init();              
    spilcd_init();             

    while (es8388_init())      
    {
        spilcd_show_string(30, 110, 200, 16, 16, "ES8388 Error", RED);
        vTaskDelay(pdMS_TO_TICKS(200));
        spilcd_fill(30, 110, 239, 126, WHITE);
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    xl9555_pin_write(SPK_EN_IO, 0);    
    spilcd_clear(WHITE);   

    vTaskDelay(pdMS_TO_TICKS(1000));

    recorder_init();
    i2s_set_samplerate_bits_sample(SRate,I2S_BITS_PER_SAMPLE_16BIT); 
    i2s_trx_start();       

    sample_queue = xQueueCreate(SAMPLE_QUEUE_LEN, sizeof(uint8_t *));
    assert(sample_queue != NULL);
    display_queue = xQueueCreate(10, sizeof(char[64]));

    xTaskCreatePinnedToCore((TaskFunction_t   )audio_capture_task,         /* 任务函数 */
                                (const char*      )"audio_capture_task",       /* 任务名称 */
                                (uint32_t         )1024*8,/* 任务堆栈大小 */
                                (void*            )NULL,          /* 传递给任务函数的参数 */
                                (UBaseType_t      )3,    /* 任务优先级 */
                                (TaskHandle_t*    )NULL,
                                (BaseType_t       ) 0);           /* 该任务哪个内核运行 */

    xTaskCreatePinnedToCore(
        audio_inference_task,         
        "audio_inference_task",       
        1024*16,
        NULL,         
        2,    
        NULL,
        1);         
    xTaskCreatePinnedToCore(
        display_task,
        "display_task",
        1024*4,
        NULL,
        1,
        NULL,
        0
    );



}
}
