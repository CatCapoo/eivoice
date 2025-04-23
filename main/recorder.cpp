#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/i2s.h"
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include "recorder.h"
#include "es8388.h"
#include "esp_err.h"
#include  "spilcd.h"

#include "esp_timer.h"


static const char *TAG = "KWS";


QueueHandle_t sample_queue;
uint8_t buffer_pool[SAMPLE_QUEUE_LEN][BUFFER_SIZE];
int buffer_index = 0;
QueueHandle_t display_queue;



static int16_t features[FEATURE_LEN] = {0};


void recorder_init()
{
    myi2s_init();                         // 初始化 I2S 硬件
    es8388_adda_cfg(0, 1);               // 启用 ADC
    es8388_input_cfg(0);                 // 设置麦克风输入
    es8388_mic_gain(8);                  // MIC 增益
    es8388_alc_ctrl(3, 4, 4);            // ALC 配置
    es8388_output_cfg(0, 0);             // 关闭耳机输出
    es8388_spkvol_set(0);
    es8388_i2s_cfg(0, 3);                // 16bit, I2S标准
    i2s_set_samplerate_bits_sample(I2S_SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT);
    i2s_trx_start();
}


void display_task(void *param)
{
    char displayMessage[64];
    while(1){
        if(xQueueReceive(display_queue, &displayMessage, portMAX_DELAY) == pdPASS){
            spilcd_show_string(10, 10, 300, 100, 32,displayMessage, BLUE);
            vTaskDelay(pdMS_TO_TICKS(1000));
            spilcd_clear(WHITE);
        }
    }
}


void audio_capture_task(void *param)
{

    size_t bytes_read;

    while (1)
    {
        uint8_t *buf = buffer_pool[buffer_index];
        ESP_LOGI(TAG, "Before read: %llu", esp_timer_get_time());
        esp_err_t ret = (i2s_channel_read(rx_handle, buf, BUFFER_SIZE, &bytes_read, 4000));
        ESP_LOGI(TAG, "After read: %llu, bytes_read: %d", esp_timer_get_time(), bytes_read);



        if (bytes_read == BUFFER_SIZE)
        {
            if (xQueueSend(sample_queue, &buf, portMAX_DELAY) != pdTRUE)
                ESP_LOGW(TAG, "Queue full, drop frame");

            buffer_index = (buffer_index + 1) % SAMPLE_QUEUE_LEN;
        }
    }
}



void audio_inference_task(void *param)
{
    ei_impulse_result_t result;
    EI_IMPULSE_ERROR res;

    signal_t signal;
    signal.total_length = FEATURE_LEN;
    signal.get_data = [](size_t offset, size_t length, float *out_ptr) -> int {
        for (size_t i = 0; i < length; i++)
            out_ptr[i] = (float)features[offset + i];
        return EIDSP_OK;
    };
    char displayMessage[64] = "Hello, world!";
    while (1)
    {
        uint8_t *recv_buf;
        if (xQueueReceive(sample_queue, &recv_buf, portMAX_DELAY) == pdTRUE)
        {
            static int count = 0;
            memmove(features, features + (BUFFER_SIZE / 2), sizeof(int16_t) * (FEATURE_LEN - BUFFER_SIZE / 2));

            for (int i = 0; i < BUFFER_SIZE / 2; i++)
            {
                features[FEATURE_LEN - BUFFER_SIZE / 2 + i] =
                    (int16_t)(recv_buf[2 * i] | (recv_buf[2 * i + 1] << 8));
            }

       
            res = run_classifier_continuous(&signal, &result, false, true);
            if (res == EI_IMPULSE_OK)
            {
                ei_printf("Predictions:\n");
                for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++)
                {
                    ei_printf("  %s: %.5f\n", ei_classifier_inferencing_categories[i],
                              result.classification[i].value);
                    
                }
                ei_printf("Timing: DSP %d ms, inference %d ms, anomaly %d ms\r\n",
                    result.timing.dsp,
                    result.timing.classification,
                    result.timing.anomaly);
                if(result.classification[0].value >= 0.4) 
                    xQueueSend(display_queue, &displayMessage, portMAX_DELAY);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));  
    }
}




