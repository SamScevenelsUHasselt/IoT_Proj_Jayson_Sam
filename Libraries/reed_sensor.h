/*
 * reed_sensor.h
 *
 *  Created on: 21 nov. 2023
 *      Author: jayso
 */

#ifndef REED_SENSOR_H_
#define REED_SENSOR_H_

#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"

#define NUM_ROWS 8
#define NUM_COLS 8

void matrix_setup(int rows, int cols, int *row_pins, int *col_pins);
void matrix_read(bool status[NUM_ROWS][NUM_COLS],int *rowpins, int *colpins);

#endif /* REED_SENSOR_H_ */
