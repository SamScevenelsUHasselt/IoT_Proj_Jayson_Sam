/*******************************************************************************
* File Name:   main.c
*
* Description: This is the source code for the Empty Application Example
*              for ModusToolbox.
*
* Related Document: See README.md
*
*
********************************************************************************
* Copyright 2021-2023, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

/*******************************************************************************
* Header Files
*******************************************************************************/
#include "cyhal.h"
#include "cybsp.h"
#include "ws2812.h"
#include "cy_retarget_io.h"
/*******************************************************************************
* Macros
*******************************************************************************/


/*******************************************************************************
* Global Variables
*******************************************************************************/
#define NUM_ROWS 4
#define NUM_COLS 4
#define DEBOUNCE_DELAY 50  // Debounce delay in milliseconds
#define TOTAL_LEDS 60
#define FLICKER_DELAY 250  // Flicker every 250ms for example

// Define your row and column pins here
int rowPins[NUM_ROWS] = {P11_2,P11_7,P11_5,P11_6};
int colPins[NUM_COLS] = {P12_5,P12_0,P12_1,P12_4};

int values[16] = {  1,   2,    3,  'A',       // Values for each key in the 4x4
                    4,   5,    6,  'B',
                    7,   8,    9,  'C',
                  '*',   0,  '#',  'D' };
int current_led = 0;  // Default LED index
bool led_status[TOTAL_LEDS] = {false};  // Track status of each LED (on/off)
bool flicker_status = false;  // Track LED flicker state


// Initialize a 4x4 array to represent the button states
int buttonStates[NUM_ROWS][NUM_COLS] = {0};

ws2812_obj_t ws2812_channel1;

/*******************************************************************************
* Function Prototypes
*******************************************************************************/


/*******************************************************************************
* Function Definitions
*******************************************************************************/
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
* int *key_values: array of key values.
*
* Return:
* void
*******************************************************************************/
void keypad_setup(int rows, int cols, int *row_pins, int *col_pins, int *key_values)
{
    // Initialize rows as output and set them high
    for (int i = 0; i < rows; i++) {
        cyhal_gpio_init(row_pins[i], CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 1);
    }

    // Initialize columns as input with pull-up enabled
    for (int j = 0; j < cols; j++) {
        cyhal_gpio_init(col_pins[j], CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_PULLUP, 1);
    }
}


/*******************************************************************************
* Function Name: keypad_read
********************************************************************************
* Summary:
* Read a pressed key from the keypad matrix.
*
* Return:
* int: value of the key pressed. Returns -1 if no key is pressed.
*******************************************************************************/
int keypad_read()
{
    for (int row = 0; row < NUM_ROWS; row++) {
        // Activate the current row
        cyhal_gpio_write(rowPins[row], 0);

        // Check each column
        for (int col = 0; col < NUM_COLS; col++) {
            if (!cyhal_gpio_read(colPins[col])) { // If button pressed (reading LOW)
                cyhal_gpio_write(rowPins[row], 1); // Deactivate the current row
                cyhal_system_delay_ms(DEBOUNCE_DELAY); // Debounce delay

                // Return the corresponding value from the values array
                return values[(row * NUM_COLS) + col];
            }
        }

        // Deactivate the current row before moving to the next
        cyhal_gpio_write(rowPins[row], 1);
    }

    return -1; // No key pressed
}
/*******************************************************************************
* Function Name: main
********************************************************************************
* Summary:
* This is the main function for CPU. It...
*    1.
*    2.
*
* Parameters:
*  void
*
* Return:
*  int
*
*******************************************************************************/
cyhal_uart_t uart_obj;
const cyhal_uart_cfg_t uart_config =
{
	.data_bits = 8,
	.stop_bits = 1,
	.parity = CYHAL_UART_PARITY_NONE,
	.rx_buffer = NULL,
	.rx_buffer_size = 0
};

int main(void)
{
    cy_rslt_t result;

#if defined (CY_DEVICE_SECURE)
    cyhal_wdt_t wdt_obj;

    /* Clear watchdog timer so that it doesn't trigger a reset */
    result = cyhal_wdt_init(&wdt_obj, cyhal_wdt_get_max_timeout_ms());
    CY_ASSERT(CY_RSLT_SUCCESS == result);
    cyhal_wdt_free(&wdt_obj);
#endif

    /* Initialize the device and board peripherals */
    result = cybsp_init();

    /* Board init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Enable global interrupts */
    __enable_irq();

    // LED init
    ws2812_init(&ws2812_channel1, 60, P10_0, NC);

    // Initialize UART for debug printing
    cy_retarget_io_init_fc(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX,
                                            CYBSP_DEBUG_UART_CTS,CYBSP_DEBUG_UART_RTS,
                                            CY_RETARGET_IO_BAUDRATE);

    // Set up the keypad matrix
    keypad_setup(NUM_ROWS, NUM_COLS, rowPins, colPins, values);

    int key;
        for (;;)
        {
            key = keypad_read();  // Get pressed key (or -1 for none)

            if(key != -1)  // Check if a valid key was pressed
            {
                if(key <= 9)
                    printf("key = %d\r\n", key);  // If <= 9, display as decimal
                else
                    printf("key = %c\r\n", key);  // Display as character

                switch(key)
                {
                    case 1:
                        ws2812_setRGB(&ws2812_channel1, current_led, 255, 0, 0);  // Red
                        break;
                    case 4:
                        ws2812_setRGB(&ws2812_channel1, current_led, 0, 0, 255);  // Blue
                        break;
                    case 7:
                        ws2812_setRGB(&ws2812_channel1, current_led, 0, 255, 0);  // Green
                        break;
                    case 3:
                        if (current_led > 0) current_led--;  // Go left
                        break;
                    case 'A':
                        if (current_led < TOTAL_LEDS - 1) current_led++;  // Go right
                        break;
                    case 6:
                        led_status[current_led] = true;  // Enable current LED
                        break;
                    case 'B':
                        led_status[current_led] = false;  // Disable current LED
                        ws2812_setRGB(&ws2812_channel1, current_led, 0, 0, 0);  // Turn off
                        break;
                    default:
                        break;
                }

                if (led_status[current_led])
                    ws2812_update(&ws2812_channel1);  // If LED is enabled, update
            }

            cyhal_system_delay_ms(DEBOUNCE_DELAY);    // Optional: delay to prevent rapid firing
        }
    }
/* [] END OF FILE */
