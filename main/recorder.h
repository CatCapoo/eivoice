#ifndef __RECORDER_H
#define __RECORDER_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "ff.h"
#include "es8388.h"
#include "driver/i2s.h"
#include "myi2s.h"
#include "esp_log.h"
#ifdef __cplusplus
extern "C" {
#endif
#define SRate 16000
#define BUFFER_SIZE         22400
#define SAMPLE_QUEUE_LEN    5
#define SAMPLE_QUEUE_LEN    5
#define FEATURE_LEN         16000   // 1秒窗口，16K点

void recorder_init();
void audio_capture_task(void *param);
void audio_inference_task(void *param);
void display_task(void *param);

extern QueueHandle_t sample_queue;
extern uint8_t buffer_pool[SAMPLE_QUEUE_LEN][BUFFER_SIZE];
extern int buffer_index;
extern QueueHandle_t display_queue;

#ifdef __cplusplus
}
#endif

#endif
