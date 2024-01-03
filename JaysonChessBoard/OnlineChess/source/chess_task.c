/******************************************************************************
* File Name:   main.c
*
* Description: Source code for a chess game implementation using HAL APIs. It includes
*              LED control for a chessboard display and manages the game state.
*
* Related Document: See README.md
*
*******************************************************************************/

/* Include necessary headers */
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include "ws2812.h"
#include "chess_engine.h"
#include "reed_sensor.h"
#include <stdio.h>
#include <ctype.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "publisher_task.h"
#include "subscriber_task.h"
#include "cy_rtc.h"


/* Define constants and global variables */
int rowPins[NUM_ROWS] = {P6_0,P6_1,P5_2,P5_3,P5_4,P5_5,P5_6,P5_7};
int colPins[NUM_COLS] = {P11_2,P11_7,P11_5,P11_6,P12_5,P12_0,P12_1,P12_4};
#define STRING_BUFFER_SIZE (80)

typedef struct {
    int ledStart;
    int ledsToLight;
} LedTileInfo;

LedTileInfo ledTileInfo[NUM_ROWS][NUM_COLS];
cyhal_rtc_t rtc_obj;

void initializeLedTileInfo() {
    for (int row = 0; row < NUM_ROWS; row++) {
        for (int col = 0; col < NUM_COLS; col++) {
            int rowStart = row * 25;
            int tileInRow = (row % 2 == 1) ? (NUM_COLS - 1 - col) : col;
            ledTileInfo[row][col].ledStart = rowStart + tileInRow * 3;
            ledTileInfo[row][col].ledsToLight = (tileInRow == 7) ? 4 : 3;
        }
    }
}

/* Function to process the FEN (Forsyth-Edwards Notation) string */
void processFEN(const char *FEN, bool needsPiece[NUM_ROWS][NUM_COLS]) {
    int row = 0, col = 0;

    for (int i = 0; FEN[i] != '\0' && FEN[i] != ' '; i++) {
        if (isalpha((unsigned char)FEN[i])) {
            needsPiece[row][col] = true;
            col++;
        } else if (isdigit((unsigned char)FEN[i])) {
            col += FEN[i] - '0';
        } else if (FEN[i] == '/') {
            row++;
            col = 0;
        }
    }
}

int gamePrerequisites(bool status[NUM_ROWS][NUM_COLS],
                      bool prev_status[NUM_ROWS][NUM_COLS],
                      bool needsPiece[NUM_ROWS][NUM_COLS],
                      ws2812_obj_t *ws2812_channel1, ws2812_obj_t *ws2812_channel2) {
    bool allPiecesCorrect = true; // Assume initially that all pieces are correct
    bool update_needed = false;

    for(int row = 0; row < NUM_ROWS; row++) {
        for(int col = 0; col < NUM_COLS; col++) {
            // Check if the status of the cell has changed
            bool hasChanged = status[row][col] != prev_status[row][col];
            if(hasChanged) {
                update_needed = true; // Mark that an update is needed
                prev_status[row][col] = status[row][col]; // Update the previous status
            }

            // Calculate LED indices for updating
            int ledStart = ledTileInfo[row][col].ledStart;
            int ledsToLight = ledTileInfo[row][col].ledsToLight;

            // Update the LED colors and check piece correctness
            for(int j = 0; j < ledsToLight; j++) {
                int ledIndex = ledStart + j;
                if (status[row][col] != needsPiece[row][col]) {
                    allPiecesCorrect = false; // Piece placement is incorrect
                    int red = status[row][col] ? 255 : 0; // Red if piece is present but not needed, or needed but not present
                    int green = 0, blue = 0;
                    if(ledStart<100)
                    	ws2812_setRGB(ws2812_channel1, ledIndex, red, green, blue);
                    else
                    	ws2812_setRGB(ws2812_channel2, ledIndex-100, red, green, blue);
                } else {
                    int green = status[row][col] ? 255 : 0; // Green if piece is correctly placed
                    int blue = status[row][col] ? 0 : 255; // Blue if no piece is required or present
                    if(ledStart<100)
                    	ws2812_setRGB(ws2812_channel1, ledIndex, 0, green, blue);
                    else
                    	ws2812_setRGB(ws2812_channel2, ledIndex-100, 0, green, blue);
                }
            }
        }
    }

    if(update_needed) {
        ws2812_update(ws2812_channel1);
        ws2812_update(ws2812_channel2);
    }

    return allPiecesCorrect ? 1 : 0;
}



bool lightUpPlayerTiles(enum color playerColor, struct piece board[8][8], ws2812_obj_t *ws2812_channel1, ws2812_obj_t *ws2812_channel2) {
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {

        	// Calculate LED indices based on the current position
            int ledStart = ledTileInfo[row][col].ledStart;
            int ledsToLight = ledTileInfo[row][col].ledsToLight;


        	for(int j = 0; j < ledsToLight; j++) {
        		int ledIndex = ledStart + j;

        		if (board[row][col].color == playerColor) {
        			if(ledStart<100)
        				ws2812_setRGB(ws2812_channel1, ledIndex, 255, 255, 255); // white
        			else
        				ws2812_setRGB(ws2812_channel2, ledIndex-100, 255, 255, 255); // white
            	}
        		else {
        			if(ledStart<100)
        				ws2812_setRGB(ws2812_channel1, ledIndex, 0, 0, 0); // Off
        			else
        				ws2812_setRGB(ws2812_channel2, ledIndex-100, 0, 0, 0); // Off
            	}
        	}
        }
    }

    ws2812_update(ws2812_channel1);
    ws2812_update(ws2812_channel2);
    return 1;

}

void lightUpMoveableTiles(int row, int col, bool capt, ws2812_obj_t *ws2812_channel1, ws2812_obj_t *ws2812_channel2) {

	// Calculate LED indices based on the current position
	int ledStart = ledTileInfo[row][col].ledStart;
	int ledsToLight = ledTileInfo[row][col].ledsToLight;


	for(int j = 0; j < ledsToLight; j++) {
		int ledIndex = ledStart + j;

		// set leds to green where a piece can be moved to, if an enemy piece can be attack turn that tile red
		if (capt) {
			if(ledStart<100)
				ws2812_setRGB(ws2812_channel1, ledIndex, 255, 0, 0); // red
			else
				ws2812_setRGB(ws2812_channel2, ledIndex-100, 255, 0, 0); // red
		}
		else {
			if(ledStart<100)
				ws2812_setRGB(ws2812_channel1, ledIndex, 0, 255, 0); // green
			else
				ws2812_setRGB(ws2812_channel2, ledIndex-100, 0, 255, 0); // green
		}
	}

    ws2812_update(ws2812_channel1);
    ws2812_update(ws2812_channel2);
    vTaskDelay(pdMS_TO_TICKS(200));

}

void makeMove(int srcRow, int srcCol, int destRow, int destCol, struct piece board[8][8], bool online) {
	publisher_data_t publish_data;
	publish_data.cmd = PUBLISH_MQTT_MSG;
	static char msg[] = "Xx-x to x-x";
	static char fenCode[100];

	if(board[srcRow][srcCol].color == White){
		msg[0] = 'W';
	}

	else if (board[srcRow][srcCol].color == Black){
		msg[0] = 'B';
	}

    // Move the piece to the new location
    board[destRow][destCol].piece_type = board[srcRow][srcCol].piece_type;
    board[destRow][destCol].color = board[srcRow][srcCol].color;

    // Set the original location as empty
    board[srcRow][srcCol].piece_type = Empty;
    board[srcRow][srcCol].color = None;

	if(online){
	    // send coordinates to mqqt
		msg[1] = '0'+srcRow;
		msg[3] = '0'+srcCol;
		msg[8] = '0'+destRow;
		msg[10] = '0'+destCol;
	    publish_data.data = msg;
	    xQueueSend(publisher_task_q,&publish_data,portMAX_DELAY);
		makeFENCode(fenCode,board);
		publish_data.data = fenCode;
		xQueueSend(publisher_task_q,&publish_data,portMAX_DELAY);
	    vTaskDelay(pdMS_TO_TICKS(100));
	}

}

void processPlayerMove(int row, int col, bool *turnDone, bool *playerTilesLit, struct piece board[8][8], bool status[NUM_ROWS][NUM_COLS], bool prev_status[NUM_ROWS][NUM_COLS], struct castles castles, ws2812_obj_t *ws2812_channel1, ws2812_obj_t *ws2812_channel2, bool online) {
    struct moves moves;
    prev_status[row][col] = status[row][col];
    printf("A piece was picked up\r\n");

    // Find possible moves for the picked-up piece
    FindMoves(&moves, row, col, board, castles);

    // Light up the original tile and possible move tiles
    lightUpMoveableTiles(row, col, 0, ws2812_channel1, ws2812_channel2);
    for(int i = 0; i < moves.length; i++){
        lightUpMoveableTiles(moves.movelist[i].dest_x, moves.movelist[i].dest_y, moves.movelist[i].capt, ws2812_channel1, ws2812_channel2 );
    }

    // Loop to wait for player to complete the move
    while(!*turnDone) {
        matrix_read(status, rowPins, colPins);

        // Check if the piece is placed back on its original tile
        if(!prev_status[row][col] && status[row][col]){
            prev_status[row][col] = status[row][col];
            printf("A piece was placed down on its original tile\r\n");
            *playerTilesLit = lightUpPlayerTiles(board[row][col].color, board, ws2812_channel1, ws2812_channel2);
            break; // Break the loop to allow choosing another piece
        }

        // Iterate through the list of possible move
        for(int i = 0; i < moves.length; i++){
            int destRow = moves.movelist[i].dest_x;
            int destCol = moves.movelist[i].dest_y;
            bool capture = moves.movelist[i].capt;

            // Check if a move has been made
            if(!prev_status[destRow][destCol] && status[destRow][destCol] && !capture){
                makeMove(row, col, destRow, destCol, board, online);
                prev_status[destRow][destCol] = status[destRow][destCol];
                *turnDone = true;
                printf("A piece was placed down on a new tile\r\n");
            } else if(prev_status[destRow][destCol] && !status[destRow][destCol] && capture){
                makeMove(row, col, destRow, destCol, board, online);
                *turnDone = true;
                printf("A piece was captured\r\n");
            }

            if(*turnDone){
                printf("Turn ended!\r\n");
                break; // Break the loop as the move is completed
            }
        }
    }
}

void handlePlayerTurn(enum color playerColor, bool *turnDone, bool *playerTilesLit, struct piece board[8][8], bool status[NUM_ROWS][NUM_COLS], bool prev_status[NUM_ROWS][NUM_COLS], struct castles castles, ws2812_obj_t *ws2812_channel1, ws2812_obj_t *ws2812_channel2, bool online) {
    if(!*playerTilesLit){
        *playerTilesLit = lightUpPlayerTiles(playerColor, board, ws2812_channel1, ws2812_channel2);
        printf("Player tiles are lit\r\n");
    }

    for(int row = 0; row < NUM_ROWS && !*turnDone; row++) {
        for(int col = 0; col < NUM_COLS && !*turnDone; col++) {
            if(board[row][col].color == playerColor && prev_status[row][col] && !status[row][col]) {
                processPlayerMove(row, col, turnDone, playerTilesLit, board, status, prev_status, castles, ws2812_channel1, ws2812_channel2, online);
            }
        }
    }
}

bool hasLegalMoves(enum color playerColor, struct piece board[8][8], struct castles castles) {
    struct moves moves;
    for (int row = 0; row < NUM_ROWS; row++) {
        for (int col = 0; col < NUM_COLS; col++) {
            if (board[row][col].color == playerColor) {
                FindMoves(&moves, row, col, board, castles);
                if (moves.length > 0) {
                    return true; // Found a legal move
                }
            }
        }
    }
    return false; // No legal moves found
}

void setLedColor(ws2812_obj_t *ws2812_channel1, ws2812_obj_t *ws2812_channel2, int ledIndex, int red, int green, int blue) {
    if (ledIndex < 100)
        ws2812_setRGB(ws2812_channel1, ledIndex, red, green, blue);
    else
        ws2812_setRGB(ws2812_channel2, ledIndex - 100, red, green, blue);
}

bool verifyPiecePlacement(bool status[NUM_ROWS][NUM_COLS], struct piece board[8][8], ws2812_obj_t *ws2812_channel1, ws2812_obj_t *ws2812_channel2) {
    bool allPiecesCorrect = true; // Assume initially that all pieces are correct

    for (int row = 0; row < NUM_ROWS; row++) {
        for (int col = 0; col < NUM_COLS; col++) {
            int ledStart = ledTileInfo[row][col].ledStart;
            int ledsToLight = ledTileInfo[row][col].ledsToLight;
            bool pieceExpected = board[row][col].piece_type != Empty;
            bool piecePresent = status[row][col];

            for (int j = 0; j < ledsToLight; j++) {
                int ledIndex = ledStart + j;
                if (pieceExpected != piecePresent) {
                    // Piece placement is incorrect
                    allPiecesCorrect = false;
                    int red = 255, green = 0, blue = 0; // Red for incorrect placement
                    setLedColor(ws2812_channel1, ws2812_channel2, ledIndex, red, green, blue);
                } else {
                    // Piece placement is correct
                    int green = piecePresent ? 255 : 0; // Green if piece is correctly placed
                    int blue = piecePresent ? 0 : 255; // Blue if no piece is required or present
                    setLedColor(ws2812_channel1, ws2812_channel2, ledIndex, 0, green, blue);
                }
            }
        }
    }

    ws2812_update(ws2812_channel1);
    ws2812_update(ws2812_channel2);
    return allPiecesCorrect;
}


void startGame(bool status[NUM_ROWS][NUM_COLS], ws2812_obj_t *ws2812_channel1, struct piece board[8][8], struct castles castles, bool Online, ws2812_obj_t *ws2812_channel2) {
    bool prev_status[NUM_ROWS][NUM_COLS] = {{0}};
    bool turnWhite = true; // White starts first
    bool playerTilesLit = false;
    bool turnDone = false;
    chess_message_t subscriber_q_data;
    bool ready = 0;
    bool coloring = 0;
	publisher_data_t publish_data;
	publish_data.cmd = PUBLISH_MQTT_MSG;
	bool piecesCorrectlyPlaced = false;

    //wait till sam is ready
    if(Online){
    	while(!ready){
    		xQueueReceive(subscriber_msg_q, &subscriber_q_data, portMAX_DELAY);
    		if(subscriber_q_data.cmd == ready ){
    			ready = 1;
    			coloring = 0;
    			publish_data.data = "json you black";
    			xQueueSend(publisher_task_q,&publish_data,portMAX_DELAY);
    		}
    		else if(subscriber_q_data.cmd == you_are_white){
    			coloring = 0;
    			ready = 1;
    		}
    		else if(subscriber_q_data.cmd == you_are_black){
    			coloring = 1;
    			turnWhite = false;
    			ready = 1;
    		}
    	}

    	printf("I am color %d \r\n", coloring);
    }

    memcpy(prev_status, status, sizeof(prev_status));

    //if you play online and play as black then wait till white does his first move
    if(Online){
    	ready = 0;
    	if(coloring == 1){
    		while(!ready){
    			xQueueReceive(subscriber_msg_q, &subscriber_q_data, portMAX_DELAY);
    			if(subscriber_q_data.cmd == move){
    				  board[subscriber_q_data.to_x][subscriber_q_data.to_y] = board[subscriber_q_data.from_x][subscriber_q_data.from_y];
    				  board[subscriber_q_data.from_x][subscriber_q_data.from_y] = (struct piece){Empty, None};
    				  printf("%d %d %d %d",subscriber_q_data.to_x,subscriber_q_data.to_y,subscriber_q_data.from_x,subscriber_q_data.from_y);
    				  ready = 1;
    			}
    		}
    	}

    }

    while(true) {
        matrix_read(status, rowPins, colPins);

        //check if all pieces are on the correct tiles
        while (!piecesCorrectlyPlaced) {
            matrix_read(status, rowPins, colPins);
            piecesCorrectlyPlaced = verifyPiecePlacement(status, board, ws2812_channel1, ws2812_channel2);
            vTaskDelay(pdMS_TO_TICKS(100)); // Delay to debounce and allow for manual adjustments
        }

        if (!hasLegalMoves(Black, board, castles)) {
            printf("Game over! Black has no legal moves.\n");
            // Game ended
            break;
        }
        if (!hasLegalMoves(White, board, castles)) {
            printf("Game over! White has no legal moves.\n");
            // Game ended
            break;
        }

        if(turnWhite) {
            handlePlayerTurn(White, &turnDone, &playerTilesLit, board, status, prev_status, castles, ws2812_channel1, ws2812_channel2, Online);
        } else {
            handlePlayerTurn(Black, &turnDone, &playerTilesLit, board, status, prev_status, castles, ws2812_channel1, ws2812_channel2, Online);
        }

        if(turnDone) {
        	piecesCorrectlyPlaced = false;

            if(!Online){
            turnWhite = !turnWhite;
            }
            turnDone = false;
            playerTilesLit = false;


            if(Online){
            	ready = 0;
        		while(!ready){
        			xQueueReceive(subscriber_msg_q, &subscriber_q_data, portMAX_DELAY);
        			if(subscriber_q_data.cmd == move && ((board[subscriber_q_data.from_x][subscriber_q_data.from_y].color == White && coloring == 1) || (board[subscriber_q_data.from_x][subscriber_q_data.from_y].color == Black && coloring == 0))   ){
        				if(board[subscriber_q_data.to_x][subscriber_q_data.to_y].color == board[subscriber_q_data.from_x][subscriber_q_data.from_y].color){ //check if he did castle his king by checking if both tiles are the same color
        					int temp = board[subscriber_q_data.to_x][subscriber_q_data.to_y].piece_type;
        					board[subscriber_q_data.to_x][subscriber_q_data.to_y].piece_type = board[subscriber_q_data.from_x][subscriber_q_data.from_y].piece_type;
        					board[subscriber_q_data.from_x][subscriber_q_data.from_y].piece_type = temp;
        					ready = 1;
        				}
        				else if(subscriber_q_data.cmd == move && ((board[subscriber_q_data.from_x][subscriber_q_data.from_y].color == White && coloring == 1) || (board[subscriber_q_data.from_x][subscriber_q_data.from_y].color == Black && coloring == 0))){
        					board[subscriber_q_data.to_x][subscriber_q_data.to_y] = board[subscriber_q_data.from_x][subscriber_q_data.from_y];
        					board[subscriber_q_data.from_x][subscriber_q_data.from_y] = (struct piece){Empty, None};
        					ready = 1;
        				}
        			}
        		}

            	//if you play online you stay the same color!
            	if(coloring == 0){
            		turnWhite = 1;
            	}
            	else{
            		turnWhite = 0;
            	}
            }

            if (!hasLegalMoves(Black, board, castles)) {
                printf("Game over! Black has no legal moves.\n");
                // Game ended
                break;
            }
            if (!hasLegalMoves(White, board, castles)) {
                printf("Game over! White has no legal moves.\n");
                // Game ended
                break;
            }

        }

    }

}

/* Main function */
void chess_task(void* arg)
{
	/* Declare a WS2812 LED object */
	ws2812_obj_t ws2812_channel1;
	ws2812_obj_t ws2812_channel2;

    /* Initialize variables */
    bool status[NUM_ROWS][NUM_COLS];
    bool prev_status[NUM_ROWS][NUM_COLS] = {{0}};
    bool needsPiece[NUM_ROWS][NUM_COLS] = {{0}};
    bool canGameStart = 0;
    bool Online = 0;

	publisher_data_t publish_data;
	publish_data.cmd = PUBLISH_MQTT_MSG;

    // Define the initial board state in FEN
    char FEN[] = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    struct piece board[8][8];
    struct castles castles = makeBoard(board, FEN);

    // Initialize the keypad matrix and NeoPixel LEDs
    initializeLedTileInfo();
    matrix_setup(NUM_ROWS, NUM_COLS, rowPins, colPins);
    ws2812_init(&ws2812_channel1, 100, P10_0, NC);
    ws2812_init(&ws2812_channel2, 100, P9_0, NC);

    cyhal_gpio_init(CYBSP_USER_LED, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, CYBSP_LED_STATE_OFF);
    cyhal_gpio_init(CYBSP_USER_BTN, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_PULLUP, CYBSP_BTN_OFF);

    // Process the initial FEN to set up the game
    processFEN(FEN, needsPiece);

    // Main loop
    for (;;)
    {
    	if(!cyhal_gpio_read(CYBSP_USER_BTN) && !Online){
    		cyhal_gpio_write(CYBSP_USER_LED, CYBSP_LED_STATE_ON);
    		Online = 1;
    		printf("Playing online\n\r");
    	}

    	else if(!cyhal_gpio_read(CYBSP_USER_BTN) && Online){
    	    cyhal_gpio_write(CYBSP_USER_LED, CYBSP_LED_STATE_OFF);
    	    Online = 0;
    	    printf("Playing offline\n\r");
    	}

        // checks if the game can start
        if(canGameStart == 1){
        	// game starts...
        	if(Online){
                publish_data.data = "JaysonReady";
                xQueueSend(publisher_task_q,&publish_data,portMAX_DELAY);
        	}
        	startGame(status, &ws2812_channel1, board, castles, Online, &ws2812_channel2);
        	// game is stuck in for loop until it breaks ( which means the game has ended)
        	canGameStart = 0;
        }
        else{
        	// Continuesly read the matrix and it's changes
        	matrix_read(status, rowPins, colPins);
        	// check if the game can start; Are all pieces on their correct tile?
        	canGameStart = gamePrerequisites(status, prev_status, needsPiece, &ws2812_channel1, &ws2812_channel2);
        }

        // Delay to prevent rapid changes
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
