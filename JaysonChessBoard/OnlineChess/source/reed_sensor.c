/*
 * reed_sensor.c
 *
 *  Created on: 21 nov. 2023
 *      Author: jayso
 */


#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include "reed_sensor.h"

/*******************************************************************************
* Function Name: keypad_setup
********************************************************************************
* Summary:
* Initialize the keypad matrix.
*
* Parameters:
* int rows: number of rows in the matrix.
* int cols: number of columns in the matrix.
* int *row_pins: array of pins assigned to rows.
* int *col_pins: array of pins assigned to columns.
*
* Return:
* void
*******************************************************************************/
void matrix_setup(int rows, int cols, int *row_pins, int *col_pins)
{
	cy_rslt_t result;

    // Initialize rows as output and set them high
    for (int i = 0; i < rows; i++) {
        result = cyhal_gpio_init(row_pins[i], CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);

        if (result != CY_RSLT_SUCCESS)
        {
        	printf("pin %d | Output werkt niet\r\n",i);
            CY_ASSERT(0);
        }
    }

    // Initialize columns as input with pull-up enabled
    for (int j = 0; j < cols; j++) {
        result = cyhal_gpio_init(col_pins[j], CYHAL_GPIO_DIR_INPUT,   CYHAL_GPIO_DRIVE_PULLDOWN, 0);

        if (result != CY_RSLT_SUCCESS)
        {
        	printf("pin %d | Input werkt niet\r\n",j);
            CY_ASSERT(0);
        }
    }

    printf("All pins initialized\r\n");
    printf("All pins initialized\r\n");
    printf("All pins initialized\r\n");
    printf("All pins initialized\r\n");
    printf("All pins initialized\r\n");
    printf("All pins initialized\r\n");
    printf("All pins initialized\r\n");
    printf("All pins initialized\r\n");
    printf("All pins initialized\r\n");
    printf("All pins initialized\r\n");
    printf("All pins initialized\r\n");
    printf("All pins initialized\r\n");
    printf("All pins initialized\r\n");
}

/*******************************************************************************
* Function Name: keypad_read
********************************************************************************
* Summary:
* Read a pressed key from the keypad matrix.
*******************************************************************************/
void matrix_read(bool status[NUM_ROWS][NUM_COLS],int *rowpins, int *colpins)
{
    for (int row = 0; row < NUM_ROWS; row++) {
        // Activate the current row
        cyhal_gpio_write(rowpins[row], 1);
        // Delay for row activation
        cyhal_system_delay_ms(10);

        // Check each column
        for (int col = 0; col < NUM_COLS; col++) {
            // Read the state of each column pin and store it in the status array
            status[row][col] = cyhal_gpio_read(colpins[col]);
        }

        // Deactivate the current row before moving to the next
        cyhal_gpio_write(rowpins[row], 0);
    }
}

