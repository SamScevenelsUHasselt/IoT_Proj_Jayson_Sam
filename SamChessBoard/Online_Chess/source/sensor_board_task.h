/*
 * sensor_board_task.h
 *
 *  Created on: 08 Dec 2023
 *      Author: SamSc
 */

#ifndef SOURCE_SENSOR_BOARD_TASK_H_
#define SOURCE_SENSOR_BOARD_TASK_H_

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

enum sensorCommand{
	Do_Nothing,
	Get_Difference_Cords,
	Get_Board,
	Stop
};

enum readOutCmd{
	Sensorboard,
	Button
};

struct cords{
	unsigned char x;
	unsigned char y;
	unsigned char sensor_value;
};

struct readOutData{
	enum readOutCmd readOutCmd;
	struct cords cords;
};

extern QueueHandle_t sensor_board_task_cmd_q;
extern QueueHandle_t sensor_board_task_difference_q;
extern QueueHandle_t sensor_board_task_board_q;
extern TaskHandle_t sensortask;

void sensor_board_task(void *arg);

#endif /* SOURCE_SENSOR_BOARD_TASK_H_ */
