/*
 * LED_task.c
 *
 *  Created on: 11 Dec 2023
 *      Author: SamSc
 */
#include "LED_task.h"

#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "cycfg.h"

#include "FreeRTOS.h"
#include "task.h"

QueueHandle_t led_data_q;

void LED_Task(void *pvParameters){
	//Timer for blinking
    cyhal_timer_t timer_obj;
    const cyhal_timer_cfg_t timer_cfg =
    {
        .compare_value = 0,                 /* Timer compare value, not used */
        .period = 20000,                    /* Timer period set to a large enough value
                                             * compared to event being measured */
        .direction = CYHAL_TIMER_DIR_UP,    /* Timer counts up */
        .is_compare = false,                /* Don't use compare mode */
        .is_continuous = true,             /* Do not run timer indefinitely */
        .value = 0                          /* Initial value of counter */
    };
    cyhal_timer_init(&timer_obj, NC, NULL);
    cyhal_timer_configure(&timer_obj, &timer_cfg);
    cyhal_timer_set_frequency(&timer_obj, 10000);

	struct LED_Data LED_Data;
	float duty_r = 0;
	float duty_g = 0;
	float duty_b = 0;
	bool blink = 1;
	int blinkTime = 5000;

	cyhal_pwm_t pwm_r;
	cyhal_pwm_t pwm_g;
	cyhal_pwm_t pwm_b;

	cyhal_pwm_init(&pwm_r, P5_2, NULL);
	cyhal_pwm_init(&pwm_g, P5_3, NULL);
	cyhal_pwm_init(&pwm_b, P5_4, NULL);

	cyhal_pwm_set_duty_cycle(&pwm_r, 100, 100000);
	cyhal_pwm_set_duty_cycle(&pwm_g, 100, 100000);
	cyhal_pwm_set_duty_cycle(&pwm_b, 100, 100000);

	cyhal_pwm_start(&pwm_r);
	cyhal_pwm_start(&pwm_g);
	cyhal_pwm_start(&pwm_b);

	cyhal_timer_start(&timer_obj);
	for(;;){
		if(blink && cyhal_timer_read(&timer_obj)>blinkTime){
			cyhal_pwm_set_duty_cycle(&pwm_r, 100, 100000);
			cyhal_pwm_set_duty_cycle(&pwm_g, 100, 100000);
			cyhal_pwm_set_duty_cycle(&pwm_b, 100, 100000);

			if (pdTRUE == xQueueReceive(led_data_q,&LED_Data,pdMS_TO_TICKS(15u))){
				duty_r = ((255-LED_Data.red)/255)*100;
				duty_g = ((255-LED_Data.green)/255)*100;
				duty_b = ((255-LED_Data.blue)/255)*100;
				blink = LED_Data.blink;
				blinkTime = LED_Data.blinkTimeMS*10;
				cyhal_timer_reset(&timer_obj);
			}

			if(cyhal_timer_read(&timer_obj)>2*blinkTime){cyhal_timer_reset(&timer_obj);}
		}
		else{
			cyhal_pwm_set_duty_cycle(&pwm_r, duty_r, 100000);
			cyhal_pwm_set_duty_cycle(&pwm_g, 100, 100000);
			cyhal_pwm_set_duty_cycle(&pwm_b, 100, 100000);
			vTaskDelay(pdMS_TO_TICKS(5u));
			cyhal_pwm_set_duty_cycle(&pwm_r, 100, 100000);
			cyhal_pwm_set_duty_cycle(&pwm_g, duty_g, 100000);
			cyhal_pwm_set_duty_cycle(&pwm_b, 100, 100000);
			vTaskDelay(pdMS_TO_TICKS(5u));
			cyhal_pwm_set_duty_cycle(&pwm_r, 100, 100000);
			cyhal_pwm_set_duty_cycle(&pwm_g, 100, 100000);
			cyhal_pwm_set_duty_cycle(&pwm_b, duty_b, 100000);
			if (pdTRUE == xQueueReceive(led_data_q,&LED_Data,pdMS_TO_TICKS(5u))){
				duty_r = ((255-LED_Data.red)/255)*100;
				duty_g = ((255-LED_Data.green)/255)*100;
				duty_b = ((255-LED_Data.blue)/255)*100;
				blink = LED_Data.blink;
				blinkTime = LED_Data.blinkTimeMS*10;
				cyhal_timer_reset(&timer_obj);
			}
		}

	}
}
