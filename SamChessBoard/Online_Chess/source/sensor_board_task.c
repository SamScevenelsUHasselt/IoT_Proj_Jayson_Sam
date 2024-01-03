/*
 * SensorReadout.c
 *
 *  Created on: 06 Dec 2023
 *      Author: SamSc
 */

#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "cycfg.h"
#include "cy_retarget_io.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "sensor_board_task.h"


QueueHandle_t sensor_board_task_cmd_q;
QueueHandle_t sensor_board_task_difference_q;
QueueHandle_t sensor_board_task_board_q;

TaskHandle_t sensortask;

struct cords boardtranslator(uint32_t index){
	struct cords cords;
	if(index<8){
		cords.x = 0;
	}else if(index<16){
		cords.x = 1;
	}else if(index<24){
		cords.x = 2;
	}else if(index<32){
		cords.x = 3;
	}else if(index<40){
		cords.x = 4;
	}else if(index<48){
		cords.x = 5;
	}else if(index<56){
		cords.x = 6;
	}else if(index<64){
		cords.x = 7;
	}
	uint32_t index_mod16 = index%16;
	if(index_mod16==0||index_mod16==15){
		cords.y = 7;
	}else if(index_mod16==1||index_mod16==14){
		cords.y = 6;
	}else if(index_mod16==2||index_mod16==13){
		cords.y = 5;
	}else if(index_mod16==3||index_mod16==12){
		cords.y = 4;
	}else if(index_mod16==4||index_mod16==11){
		cords.y = 3;
	}else if(index_mod16==5||index_mod16==10){
		cords.y = 2;
	}else if(index_mod16==6||index_mod16==9){
		cords.y = 1;
	}else if(index_mod16==7||index_mod16==8){
		cords.y = 0;
	}

	return cords;
}

void sensor_board_task(void *arg){
	struct cords cords;
    bool sensorBoard[8][8];
    bool sensorBoard_Prev[8][8];
    bool firstPass = 1;
    bool differenceFlag = 0;
    struct readOutData readOutData;
    readOutData.readOutCmd = Sensorboard;
    struct cords differenceCords;
    enum sensorCommand sensorCommand;
    for (;;)
    {
    	if (pdTRUE == xQueueReceive(sensor_board_task_cmd_q, &sensorCommand, portMAX_DELAY)){
    		if(sensorCommand == Get_Difference_Cords){
    			differenceFlag=0;
    			while(differenceFlag==0){
					for(int selectVal = 0;selectVal<16;selectVal++){
						Cy_GPIO_Write(Select_1_PORT,Select_1_NUM,(selectVal&1));
						Cy_GPIO_Write(Select_2_PORT,Select_2_NUM,(selectVal>>1)&1);
						Cy_GPIO_Write(Select_3_PORT,Select_3_NUM,(selectVal>>2)&1);
						Cy_GPIO_Write(Select_4_PORT,Select_4_NUM,(selectVal>>3)&1);

						CyDelayUs(1);//max rise time of the multiplexer is 1us
						cords = boardtranslator(selectVal);
						sensorBoard[cords.x][cords.y] = !Cy_GPIO_Read(Input_1_PORT,Input_1_NUM);

						cords = boardtranslator(selectVal+16);
						sensorBoard[cords.x][cords.y] = !Cy_GPIO_Read(Input_2_PORT,Input_2_NUM);

						cords = boardtranslator(selectVal+32);
						sensorBoard[cords.x][cords.y] = !Cy_GPIO_Read(Input_3_PORT,Input_3_NUM);

						cords = boardtranslator(selectVal+48);
						sensorBoard[cords.x][cords.y] = !Cy_GPIO_Read(Input_4_PORT,Input_4_NUM);
					}
					if(firstPass){
						for(int x = 0;x<8;x++){
							for(int y = 0;y<8;y++){
								sensorBoard_Prev[x][y] = sensorBoard[x][y];
								firstPass = 0;
							}
						}
					}
					differenceFlag = 0;
					for(int x = 0;x<8;x++){
						for(int y = 0;y<8;y++){
							if(sensorBoard[x][y] != sensorBoard_Prev[x][y]){
								readOutData.cords = (struct cords){x,y,sensorBoard[x][y]};
								differenceFlag = 1;
							}
						}
					}
					for(int x = 0;x<8;x++){
						for(int y = 0;y<8;y++){
							sensorBoard_Prev[x][y] = sensorBoard[x][y];
							firstPass = 0;
						}
					}
					if(differenceFlag == 1){
						xQueueSendToBack(sensor_board_task_difference_q,&readOutData,portMAX_DELAY);
					}
					if (pdTRUE == xQueuePeek(sensor_board_task_cmd_q, &sensorCommand, 0)){//See if stop command was sent
						if(sensorCommand == Stop)
						{
							xQueueReceive(sensor_board_task_cmd_q, &sensorCommand, portMAX_DELAY);
							differenceFlag = 1;
						}
					}
    			}
    		}else if(sensorCommand == Get_Board){
    			for(int selectVal = 0;selectVal<16;selectVal++){
					Cy_GPIO_Write(Select_1_PORT,Select_1_NUM,(selectVal&1));
					Cy_GPIO_Write(Select_2_PORT,Select_2_NUM,(selectVal>>1)&1);
					Cy_GPIO_Write(Select_3_PORT,Select_3_NUM,(selectVal>>2)&1);
					Cy_GPIO_Write(Select_4_PORT,Select_4_NUM,(selectVal>>3)&1);

					CyDelayUs(1);//max rise time of the multiplexer is 1us
					cords = boardtranslator(selectVal);
					sensorBoard[cords.x][cords.y] = !Cy_GPIO_Read(Input_1_PORT,Input_1_NUM);

					cords = boardtranslator(selectVal+16);
					sensorBoard[cords.x][cords.y] = !Cy_GPIO_Read(Input_2_PORT,Input_2_NUM);

					cords = boardtranslator(selectVal+32);
					sensorBoard[cords.x][cords.y] = !Cy_GPIO_Read(Input_3_PORT,Input_3_NUM);

					cords = boardtranslator(selectVal+48);
					sensorBoard[cords.x][cords.y] = !Cy_GPIO_Read(Input_4_PORT,Input_4_NUM);
				}
				xQueueSendToBack(sensor_board_task_board_q,&sensorBoard,portMAX_DELAY);
    		}
    	}
    }
}
