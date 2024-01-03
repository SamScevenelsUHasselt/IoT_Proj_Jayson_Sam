/*
 * LED_task.h
 *
 *  Created on: 11 Dec 2023
 *      Author: SamSc
 */

#ifndef SOURCE_LED_TASK_H_
#define SOURCE_LED_TASK_H_

#include "FreeRTOS.h"
#include "queue.h"

struct LED_Data{
	unsigned char red;
	unsigned char green;
	unsigned char blue;
	bool blink;
	int blinkTimeMS;
};


extern QueueHandle_t led_data_q;
void LED_Task(void *pvParameters);

#endif /* SOURCE_LED_TASK_H_ */
