/*
 * game_controller_task.c
 *
 *  Created on: 08 Dec 2023
 *      Author: SamSc
 */
#include "game_controller_task.h"
#include "sensor_board_task.h"
#include "mqtt_task.h"
#include "chess_engine.h"
#include "LED_task.h"
#include "publisher_task.h"
#include "subscriber_task.h"

#include "cyhal.h"
#include "cybsp.h"

#include "FreeRTOS.h"
#include "task.h"

#define USER_BTN_INTR_PRIORITY	(configMAX_PRIORITIES-1)

#define ONLINE_MODE 1
#define LOCAL_MODE_WITH_ONLINE_SPECTATOR 0
#define OFFLINE_MODE 0

static void isr_button_press(void *callback_arg, cyhal_gpio_event_t event);

//Board starting position
bool startingPosition[8][8] = {
		{1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1},
		{0,0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0,0},
		{1,1,1,1,1,1,1,1},
		{1,1,1,1,1,1,1,1}
};

/* Structure that stores the callback data for the GPIO interrupt event. */
cyhal_gpio_callback_data_t cb_data =
{
    .callback = isr_button_press,
    .callback_arg = NULL
};

typedef struct{
	bool MyTurn;
	enum color myColor;
}ConnectReturn;

enum EndCondition{
	Checkmate,
	Draw,
	Continue
};

void mqtt_setup(){
	printf("GAME CONTROLLER: STARTING MQTT\n\r");
	xTaskCreate(mqtt_client_task, "MQTT Client task", MQTT_CLIENT_TASK_STACK_SIZE,
	                NULL, MQTT_CLIENT_TASK_PRIORITY, NULL);
	//Wait for MQTT to be done
	while(mqtt_setup_done == 0){vTaskDelay(pdMS_TO_TICKS(100));}
	printf("GAME CONTROLLER: MQTT IS DONE!!!!!\n\r");
}

void wait_for_full_board(){
	struct LED_Data LED_Data = (struct LED_Data){255,255,0,1,250};
	xQueueSend(led_data_q,&LED_Data,portMAX_DELAY);
	printf("Waiting for a ready board\n\r");

	sensor_board_task_cmd_q = xQueueCreate(1,sizeof(enum sensorCommand));
	sensor_board_task_difference_q = xQueueCreate(1,sizeof(struct readOutData));
	sensor_board_task_board_q = xQueueCreate(1,sizeof(bool[8][8]));
	xTaskCreate(sensor_board_task, "Sensor board task", configMINIMAL_STACK_SIZE*2,
		                NULL, configMAX_PRIORITIES-4, NULL);
	vTaskDelay(pdMS_TO_TICKS(1000u));

	enum sensorCommand sensorCommand = Get_Board;
	bool boardSetUp = 0;
	bool difference = 0;
	bool sensorBoard[8][8];
	while (boardSetUp == 0){
		difference = 0;
		xQueueSend(sensor_board_task_cmd_q,&sensorCommand,portMAX_DELAY);
		if (pdTRUE == xQueueReceive(sensor_board_task_board_q, &sensorBoard, portMAX_DELAY)){
			for(int x = 0;x<8;x++){
				for(int y = 0;y<8;y++){
					if(sensorBoard[x][y]!=startingPosition[x][y]){difference = 1;}
				}
			}
			if(difference == 0){boardSetUp = 1;}
		}
	}
	printf("Board Ready!!!!\n\r");
}

ConnectReturn connect_to_opponent(){
	ConnectReturn returnObject;
	publisher_data_t publish_data;
	publish_data.cmd = PUBLISH_MQTT_MSG;

	struct LED_Data LED_Data = (struct LED_Data){0,0,255,1,250};
	xQueueSend(led_data_q,&LED_Data,portMAX_DELAY);
	//Send ready command to opponent and wait for ready or ack back
	bool connected = 0;
	printf("Waiting for opponent!!!\n\r");
	publish_data.data = "SamReady";
	xQueueSend(publisher_task_q,&publish_data,portMAX_DELAY);
	chess_message_t chess_msg;
	while(!connected){
		if (pdTRUE == xQueueReceive(subscriber_msg_q, &chess_msg, portMAX_DELAY))
		{
			switch(chess_msg.cmd){
			case ready: //i was first. i can be white
				publish_data.data = "sam you black";
				xQueueSend(publisher_task_q,&publish_data,portMAX_DELAY);
				returnObject.myColor = White;
				returnObject.MyTurn = 1;
				connected = 1;
				break;
			case you_are_white:
				returnObject.myColor = White;
				returnObject.MyTurn = 1;
				connected = 1;
				break;
			case you_are_black:
				returnObject.myColor = Black;
				returnObject.MyTurn = 0;
				connected = 1;
				break;
			default:
				break;
			}
		}
	}
	LED_Data = (struct LED_Data){0,255,0,0,0};
	xQueueSend(led_data_q,&LED_Data,portMAX_DELAY);
	vTaskDelay(pdMS_TO_TICKS(1000u));
	return returnObject;
}

enum EndCondition no_moves(struct piece board[8][8], enum color myColor, struct castles castles){
	bool kingInCheck = 0;
	struct moves moves;
	for(int x = 0; x < 8; x++){
		for(int y = 0; y < 8; y++){
			if(board[x][y].color == myColor){
				if(board[x][y].piece_type == King){//check if the king is in check
					if(piece_attacked(x,y,board,NULL,myColor)){
						kingInCheck = 1;
					}
				}

				FindMoves(&moves,x,y,board,castles);
				if(moves.length != 0){return Continue;}
			}
		}
	}
	if(kingInCheck == 1){
		return Checkmate;
	}else{
		return Draw;
	}
}

bool handle_my_turn(struct piece board[8][8], char fenCode[100], enum color *myColor, bool *checkMate, bool *draw, struct castles *castles){
	struct LED_Data LED_Data = (struct LED_Data){255,0,0,0,0};
	xQueueSend(led_data_q,&LED_Data,portMAX_DELAY);

	publisher_data_t publish_data;
	publish_data.cmd = PUBLISH_MQTT_MSG;
	bool moveCompleted = 0;
	bool pieceChosen = 0;
	bool validMoveReady = 0;
	enum sensorCommand sensorCommand;
	unsigned char movelistIndex = 0;
	struct readOutData readOutData;
	struct cords original_cords;
	struct moves moves;
	char msg[] = "Xx-x to x-x";

	//wait for piece lifted
	while(!moveCompleted){
		sensorCommand = Get_Difference_Cords;
		xQueueSend(sensor_board_task_cmd_q,&sensorCommand,0);
		if (pdTRUE == xQueueReceive(sensor_board_task_difference_q, &readOutData, portMAX_DELAY)){
			//Button was pressed and a valid move was made
			if(readOutData.readOutCmd == Button && validMoveReady){
				moveCompleted = 1;
				sensorCommand = Stop;
				xQueueOverwrite(sensor_board_task_cmd_q,&sensorCommand);
				vTaskDelay(pdMS_TO_TICKS(100u));
			}else if(readOutData.readOutCmd == Sensorboard){
				// a new piece has been picked up which the user wants to move
				if(pieceChosen == 0 && readOutData.cords.sensor_value == 0 && board[readOutData.cords.x][readOutData.cords.y].color == *myColor){
					original_cords = readOutData.cords;//save the cords of this piece to enable placing it back and selecting another piece
					FindMoves(&moves,readOutData.cords.x,readOutData.cords.y,board,*castles);//get all legal moves with this piece
					pieceChosen = 1;
					if(moves.length == 0){
						LED_Data = (struct LED_Data){255,0,0,1,250};
						xQueueSend(led_data_q,&LED_Data,portMAX_DELAY);
					}else{
						LED_Data = (struct LED_Data){255,255,0,0,0};
						xQueueSend(led_data_q,&LED_Data,portMAX_DELAY);
					}

				}
				//wait for piece to be put down or enemy piece picked up to be captured
				else if(pieceChosen == 1){
					if(validMoveReady == 1 && readOutData.cords.x == moves.movelist[movelistIndex].dest_x && readOutData.cords.y == moves.movelist[movelistIndex].dest_y && readOutData.cords.sensor_value == 0){
						validMoveReady = 0;
						LED_Data = (struct LED_Data){255,255,0,0,0};
						xQueueSend(led_data_q,&LED_Data,portMAX_DELAY);
					}
					if(validMoveReady == 0){
						if(readOutData.cords.x == original_cords.x && readOutData.cords.y == original_cords.y && readOutData.cords.sensor_value == 1){
							pieceChosen = 0;
							LED_Data = (struct LED_Data){255,0,0,0,0};
							xQueueSend(led_data_q,&LED_Data,portMAX_DELAY);
						}//piece was put back on its original position
						else{
							for(int i = 0; i < moves.length;i++){
								if(moves.movelist[i].dest_x == readOutData.cords.x && moves.movelist[i].dest_y == readOutData.cords.y){
									if(moves.movelist[i].capt ^ readOutData.cords.sensor_value){
										validMoveReady = 1;
										movelistIndex = i;
										//moveCompleted = 1;//REMOVE AFTER TESTING!!!!!!!!!!!!!!
										LED_Data = (struct LED_Data){0,255,0,0,0};
										xQueueSend(led_data_q,&LED_Data,portMAX_DELAY);
										printf("Move made:%d-%d to %d-%d\n\r",original_cords.x,original_cords.y,readOutData.cords.x,readOutData.cords.y);
										break;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	//update board with move
	//CASTLE AND EN PASSENT LOGIC !!!!!!!!
	board[moves.movelist[movelistIndex].dest_x][moves.movelist[movelistIndex].dest_y] = board[original_cords.x][original_cords.y];
	board[original_cords.x][original_cords.y] = (struct piece) {Empty,None};

	//check if opponent has any moves left
	enum EndCondition EndCondition = no_moves(board, invertColor(*myColor), *castles);
	switch(EndCondition){
		case(Checkmate):
			*checkMate = 1;
			break;
		case(Draw):
			*draw = 1;
			break;
		default:
			break;
	}

	if(ONLINE_MODE){
		//send move and end turn
		if(*myColor == White){msg[0] = 'W';}
		else{msg[0] = 'B';}
		msg[1] = '0' + original_cords.x;
		msg[3] = '0' + original_cords.y;
		msg[8] = '0' + moves.movelist[movelistIndex].dest_x;
		msg[10] = '0' + moves.movelist[movelistIndex].dest_y;

		publish_data.data = msg;
		xQueueSend(publisher_task_q,&publish_data,portMAX_DELAY);
		printf("Move Registered: %s\n\r",publish_data.data);
	}else{
		if(*myColor == White){*myColor = Black;}
		else{*myColor = White;}
	}
	if(LOCAL_MODE_WITH_ONLINE_SPECTATOR || ONLINE_MODE){
		makeFENCode(fenCode,board);
		publish_data.data = fenCode;
		xQueueSend(publisher_task_q,&publish_data,portMAX_DELAY);
		printf("FEN Code Sent\n\r");
	}
	if(ONLINE_MODE){
		return 0;
	}else{
		return 1;
	}
}

bool handle_opponent_turn(struct piece board[8][8], enum color myColor, bool *checkMate, bool *draw, struct castles *castles){
	//wait for opponent move
	struct LED_Data LED_Data = (struct LED_Data){0,0,255,0,0};
	xQueueSend(led_data_q,&LED_Data,portMAX_DELAY);
	bool opponentMoved = 0;
	struct readOutData readOutData;
	enum sensorCommand sensorCommand;
	chess_message_t chess_msg;

	while(!opponentMoved){
		if (pdTRUE == xQueueReceive(subscriber_msg_q, &chess_msg, portMAX_DELAY))
		{
			if(chess_msg.cmd == move){
				if((chess_msg.color != myColor)){
					opponentMoved = 1;
				}
			}
		}
	}
	//wait for opponent piece picked up and placed in correct position and end turn
	printf("Make move: %c%d to %c%d\n\r",'a'+chess_msg.from_y,1+chess_msg.from_x,'a'+chess_msg.to_y,1+chess_msg.to_x);
	LED_Data = (struct LED_Data){255,255,0,1,500};
	xQueueSend(led_data_q,&LED_Data,portMAX_DELAY);
	bool moveCompleted = 0;
	bool correctPiece = 0;
	while(!moveCompleted){
		sensorCommand = Get_Difference_Cords;
		xQueueSend(sensor_board_task_cmd_q,&sensorCommand,0);
		if (pdTRUE == xQueueReceive(sensor_board_task_difference_q, &readOutData, pdMS_TO_TICKS(20u))){
			if(readOutData.readOutCmd == Sensorboard && readOutData.cords.sensor_value == 0){
				if(readOutData.cords.x == chess_msg.from_x && readOutData.cords.y == chess_msg.from_y){
					LED_Data = (struct LED_Data){0,255,0,1,250};
					xQueueSend(led_data_q,&LED_Data,portMAX_DELAY);
					correctPiece = 1;
				}
			}else if(readOutData.readOutCmd == Sensorboard && readOutData.cords.sensor_value == 1){
				if(readOutData.cords.x == chess_msg.to_x && readOutData.cords.y == chess_msg.to_y && correctPiece){
					LED_Data = (struct LED_Data){0,255,0,0,0};
					xQueueSend(led_data_q,&LED_Data,portMAX_DELAY);
					moveCompleted = 1;
				}
			}
		}
	}
	//CASTLE AND EN PASSENT LOGIC !!!!!!!!
	board[chess_msg.to_x][chess_msg.to_y] = board[chess_msg.from_x][chess_msg.from_y];
	board[chess_msg.from_x][chess_msg.from_y] = (struct piece) {Empty,None};

	enum EndCondition EndCondition = no_moves(board, myColor, *castles);
	switch(EndCondition){
		case(Checkmate):
			*checkMate = 1;
			break;
		case(Draw):
			*draw = 1;
			break;
		default:
			break;
	}

	vTaskDelay(pdMS_TO_TICKS(1000u));
	return 1;
}

void game_controller_task(void *pvParameters){
	//Button int settup
	cyhal_gpio_init(P5_5, CYHAL_GPIO_DIR_INPUT,
	                    CYHAL_GPIO_DRIVE_PULLUP, 1u);
	cyhal_gpio_register_callback(P5_5, &cb_data);
	cyhal_gpio_enable_event(P5_5, CYHAL_GPIO_IRQ_FALL,
							USER_BTN_INTR_PRIORITY, true);

	//Led Data object
	struct LED_Data LED_Data = {0,0,255,1,500};
	xQueueSend(led_data_q,&LED_Data,portMAX_DELAY);

	//Setup chess board object
	char FEN[] = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
	char fenCode[100];
	struct piece board[8][8];
	struct castles castles = makeBoard(board,FEN);

	//Start MQTT
	if(ONLINE_MODE || LOCAL_MODE_WITH_ONLINE_SPECTATOR){
		mqtt_setup();
	}

	//Wait for a full board
	wait_for_full_board();

	bool MyTurn;
	enum color myColor = None;
	if(ONLINE_MODE){
		ConnectReturn returnObject = connect_to_opponent();
		MyTurn = returnObject.MyTurn;
		myColor = returnObject.myColor;
	}else{myColor = White; MyTurn=1;}

	bool checkMate = 0;
	bool draw = 0;
	while(!checkMate && !draw){
		//if your turn
		if(MyTurn){
			MyTurn = handle_my_turn(board, fenCode, &myColor, &checkMate, &draw, &castles);
		}
		//if opponent turn
		else{
			MyTurn = handle_opponent_turn(board, myColor, &checkMate, &draw, &castles);
		}
	}
	//When checkmate or draw end connection.
	printf("Game Is Done\n\r");
	if(checkMate){
		struct LED_Data LED_Data = {0,255,0,1,100};
		xQueueSend(led_data_q,&LED_Data,portMAX_DELAY);
	}else if(draw){
		struct LED_Data LED_Data = {255,255,255,1,250};
		xQueueSend(led_data_q,&LED_Data,portMAX_DELAY);
	}
	for(;;){}
}

static void isr_button_press(void *callback_arg, cyhal_gpio_event_t event)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* To avoid compiler warnings */
    (void) callback_arg;
    (void) event;

    /* Assign the publish command to be sent to the publisher task. */
    struct readOutData readOutData;
    readOutData.readOutCmd = Button;

    /* Send the command and data to publisher task over the queue */
    xQueueSendFromISR(sensor_board_task_difference_q, &readOutData, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
