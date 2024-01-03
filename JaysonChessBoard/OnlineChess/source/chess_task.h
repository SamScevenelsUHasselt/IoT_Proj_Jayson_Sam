/*
 * chess_task.h
 *
 *  Created on: 10 dec. 2023
 *      Author: jayso
 */

#ifndef CHESS_TASK_H
#define CHESS_TASK_H

#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include "ws2812.h"
#include "chess_engine.h"
#include "reed_sensor.h"
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "cy_rtc.h"

/* Define constants */
#define NUM_ROWS 8
#define NUM_COLS 8
#define TOTAL_LEDS 200

/* Type definitions */
QueueHandle_t chess_command_q;
TaskHandle_t Chess_Task;

/* Function Declarations */
void initializeLedTileInfo();
void processFEN(const char *FEN, bool needsPiece[NUM_ROWS][NUM_COLS]);
int gamePrerequisites(bool status[NUM_ROWS][NUM_COLS], bool prev_status[NUM_ROWS][NUM_COLS], bool needsPiece[NUM_ROWS][NUM_COLS], ws2812_obj_t *ws2812_channel);
bool lightUpPlayerTiles(enum color playerColor, struct piece board[8][8], ws2812_obj_t *ws2812_channel);
void lightUpMoveableTiles(int row, int col, bool capt, ws2812_obj_t *ws2812_channel);
void makeMove(int srcRow, int srcCol, int destRow, int destCol, struct piece board[8][8]);
void processPlayerMove(int row, int col, bool *turnDone, bool *playerTilesLit, struct piece board[8][8], bool status[NUM_ROWS][NUM_COLS], bool prev_status[NUM_ROWS][NUM_COLS], struct castles castles, ws2812_obj_t *ws2812_channel);
void handlePlayerTurn(enum color playerColor, bool *turnDone, bool *playerTilesLit, struct piece board[8][8], bool status[NUM_ROWS][NUM_COLS], bool prev_status[NUM_ROWS][NUM_COLS], struct castles castles, ws2812_obj_t *ws2812_channel);
bool hasLegalMoves(enum color playerColor, struct piece board[8][8], struct castles castles);
bool verifyPiecePlacement(bool status[NUM_ROWS][NUM_COLS], bool needsPiece[NUM_ROWS][NUM_COLS], ws2812_obj_t *ws2812_channel);
void startGame(bool status[NUM_ROWS][NUM_COLS], ws2812_obj_t *ws2812_channel, struct piece board[8][8], struct castles castles);
void chess_task(void* arg);

#endif /* CHESS_TASK_H */
