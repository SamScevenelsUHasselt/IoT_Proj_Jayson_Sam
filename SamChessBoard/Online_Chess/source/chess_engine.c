/*
 * chess_engine.c
 *
 *  Created on: 20 nov. 2023
 *      Author: J&S
 */

#include <stdio.h>
#include <stdlib.h>
#include "chess_engine.h"

unsigned char canMove(struct moves* moves,unsigned char x, unsigned char y,unsigned char target_x, unsigned char target_y, struct piece board[8][8], unsigned char checkForCheck){//Move logic for Rook, Knight, Bishop and Queen
    if(board[target_x][target_y].piece_type == Empty || board[target_x][target_y].piece_type == EnPassentGhost){
        if(checkForCheck){
            if(!kingInCheck(board[x][y].color,board,(struct move){x,y,target_x,target_y,0})){
                moves->movelist[moves->length] = (struct move){x,y,target_x,target_y,0};
                moves->length++;
            }
        }else{
            moves->movelist[moves->length] = (struct move){x,y,target_x,target_y,0};
            moves->length++;
        }
        return 0;
    }else{
        if(board[target_x][target_y].color == invertColor(board[x][y].color)){
            if(checkForCheck){
                if(!kingInCheck(board[x][y].color,board,(struct move){x,y,target_x,target_y,1})){
                    moves->movelist[moves->length] = (struct move){x,y,target_x,target_y,1};
                    moves->length++;
                }
            }else{
                moves->movelist[moves->length] = (struct move){x,y,target_x,target_y,1};
                moves->length++;
            }
        }
        return 1;
    }
}

void PawnMoves(struct moves* moves, unsigned char x, unsigned char y, struct piece board[8][8],unsigned char checkForCheck){
    moves->movelist = malloc(4*sizeof(struct move));
    moves->length = 0;
    if(board[x][y].color == White){
        if((x == 1 && board[x+1][y].piece_type == Empty && board[x+2][y].piece_type == Empty)){
            if(checkForCheck){
                if(!kingInCheck(White,board,(struct move){x,y,x+2,y,0})){
                    moves->movelist[moves->length] = (struct move){x,y,x+2,y,0};
                    moves->length++;
                }
            }else{
                moves->movelist[moves->length] = (struct move){x,y,x+2,y,0};
                moves->length++;
            }
        }
        if(board[x+1][y].piece_type == Empty){
            if(checkForCheck){
                if(!kingInCheck(White,board,(struct move){x,y,x+1,y,0})){
                    moves->movelist[moves->length] = (struct move){x,y,x+1,y,0};
                    moves->length++;
                }
            }else{
                moves->movelist[moves->length] = (struct move){x,y,x+1,y,0};
                moves->length++;
            }
        }
        if(y<7){
            if(board[x+1][y+1].color == Black){
                if(checkForCheck){
                    if(!kingInCheck(White,board,(struct move){x,y,x+1,y+1,1})){
                        moves->movelist[moves->length] = (struct move){x,y,x+1,y+1,1};
                        moves->length++;
                    }
                }else{
                    moves->movelist[moves->length] = (struct move){x,y,x+1,y+1,1};
                    moves->length++;
                }
            }
        }
        if(y!=0){
            if(board[x+1][y-1].color == Black){
                if(checkForCheck){
                    if(!kingInCheck(White,board,(struct move){x,y,x+1,y-1,1})){
                        moves->movelist[moves->length] = (struct move){x,y,x+1,y-1,1};
                        moves->length++;
                    }
                }else{
                    moves->movelist[moves->length] = (struct move){x,y,x+1,y-1,1};
                    moves->length++;
                }
            }
        }
    }
    else if(board[x][y].color == Black){
        if(x == 6 && board[x-1][y].piece_type == Empty && board[x-2][y].piece_type == Empty){
            if(checkForCheck){
                if(!kingInCheck(Black,board,(struct move){x,y,x-2,y,0})){
                    moves->movelist[moves->length] = (struct move){x,y,x-2,y,0};
                    moves->length++;
                }
            }else{
                moves->movelist[moves->length] = (struct move){x,y,x-2,y,0};
                moves->length++;
            }
        }
        if(board[x-1][y].piece_type == Empty){
            if(checkForCheck){
                if(!kingInCheck(Black,board,(struct move){x,y,x-1,y,0})){
                    moves->movelist[moves->length] = (struct move){x,y,x-1,y,0};
                    moves->length++;
                }
            }else{
                moves->movelist[moves->length] = (struct move){x,y,x-1,y,0};
                moves->length++;
            }
        }
        if(y<7){
            if(board[x-1][y+1].color == White){
                if(checkForCheck){
                    if(!kingInCheck(Black,board,(struct move){x,y,x-1,y+1,0})){
                        moves->movelist[moves->length] = (struct move){x,y,x-1,y+1,0};
                        moves->length++;
                    }
                }else{
                    moves->movelist[moves->length] = (struct move){x,y,x-1,y+1,0};
                    moves->length++;
                }
            }
        }
        if(y!=0){
            if(board[x-1][y-1].color == White){
                if(checkForCheck){
                    if(!kingInCheck(Black,board,(struct move){x,y,x-1,y-1,0})){
                        moves->movelist[moves->length] = (struct move){x,y,x-1,y-1,0};
                        moves->length++;
                    }
                }else{
                    moves->movelist[moves->length] = (struct move){x,y,x-1,y-1,0};
                    moves->length++;
                }
            }
        }
    }
}

void BishopMoves(struct moves* moves, unsigned char x, unsigned char y, struct piece board[8][8], unsigned char checkForCheck){
    moves->movelist = malloc(13*sizeof(struct move));
    moves->length = 0;
    for (int i = 1; i < 8-x && i < 8-y; ++i) { // to top right
        if(canMove(moves,x,y,x+i,y+i,board,checkForCheck)){ break;}
    }
    for (int i = 1; i < 8-x && i < y+1; ++i) { // to top left
        if(canMove(moves,x,y,x+i,y-i,board,checkForCheck)){ break;}
    }
    for (int i = 1; i < x+1 && i < y+1; ++i) { // to bottom left
        if(canMove(moves,x,y,x-i,y-i,board,checkForCheck)){ break;}
    }
    for (int i = 1; i < x+1 && i < 8-y; ++i) { // to bottom right
        if(canMove(moves,x,y,x-i,y+i,board,checkForCheck)){ break;}
    }
}

void KnightMoves(struct moves* moves, unsigned char x, unsigned char y, struct piece board[8][8], unsigned char checkForCheck){
    moves->movelist = malloc(8*sizeof(struct move));
    moves->length = 0;
    if(x<6&&y<7){ //x+2 y+1
        canMove(moves,x,y,x+2,y+1,board,checkForCheck);
    }
    if(x<7&&y<6){ //x+1 y+2
        canMove(moves,x,y,x+1,y+2,board,checkForCheck);
    }
    if(x>0&&y<6){ //x-1 y+2
        canMove(moves,x,y,x-1,y+2,board,checkForCheck);
    }
    if(x>1&&y<7){ //x-2,y+1
        canMove(moves,x,y,x-2,y+1,board,checkForCheck);
    }
    if(x>1&&y>0){ //x-2,y-1
        canMove(moves,x,y,x-2,y-1,board,checkForCheck);
    }
    if(x>0&&y>1){ //x-1,y-2
        canMove(moves,x,y,x-1,y-2,board,checkForCheck);
    }
    if(x<7&&y>1){ //x+1,y-2
        canMove(moves,x,y,x+1,y-2,board,checkForCheck);
    }
    if(x<6&&y>0){ //x+2,y-1
        canMove(moves,x,y,x+2,y-1,board,checkForCheck);
    }
}

void RookMoves(struct moves* moves, unsigned char x, unsigned char y, struct piece board[8][8], unsigned char checkForCheck){
    moves->movelist = malloc(14*sizeof(struct move));
    moves->length = 0;
    for (int i = 1; i < 8-x; ++i) { //up
        if(canMove(moves,x,y,x+i,y,board,checkForCheck)){ break;}
    }
    for (int i = 1; i < x+1; ++i) { //down
        if(canMove(moves,x,y,x-i,y,board,checkForCheck)){ break;}
    }
    for (int i = 1; i < y+1; ++i) { //left
        if(canMove(moves,x,y,x,y-i,board,checkForCheck)){ break;}
    }
    for (int i = 1; i < 8-y; ++i) { //right
        if(canMove(moves,x,y,x,y+i,board,checkForCheck)){ break;}
    }
}

void QueenMoves(struct moves* moves, unsigned char x, unsigned char y, struct piece board[8][8], unsigned char checkForCheck){
    moves->movelist = malloc(27*sizeof(struct move));
    moves->length = 0;
    //bishop moves
    for (int i = 1; i < 8-x && i < 8-y; ++i) { // to top right
        if(canMove(moves,x,y,x+i,y+i,board,checkForCheck)){ break;}
    }
    for (int i = 1; i < 8-x && i < y+1; ++i) { // to top left
        if(canMove(moves,x,y,x+i,y-i,board,checkForCheck)){ break;}
    }
    for (int i = 1; i < x+1 && i < y+1; ++i) { // to bottom left
        if(canMove(moves,x,y,x-i,y-i,board,checkForCheck)){ break;}
    }
    for (int i = 1; i < x+1 && i < 8-y; ++i) { // to bottom right
        if(canMove(moves,x,y,x-i,y+i,board,checkForCheck)){ break;}
    }
    //rook moves
    for (int i = 1; i < 8-x; ++i) { //up
        if(canMove(moves,x,y,x+i,y,board,checkForCheck)){ break;}
    }
    for (int i = 1; i < x+1; ++i) { //down
        if(canMove(moves,x,y,x-i,y,board,checkForCheck)){ break;}
    }
    for (int i = 1; i < y+1; ++i) { //left
        if(canMove(moves,x,y,x,y-i,board,checkForCheck)){ break;}
    }
    for (int i = 1; i < 8-y; ++i) { //right
        if(canMove(moves,x,y,x,y+i,board,checkForCheck)){ break;}
    }
}

void KingMoves(struct moves* moves, unsigned char x, unsigned char y, struct piece board[8][8],struct castles castles, unsigned char checkForCheck){
    moves->movelist = malloc(10*sizeof(struct move));
    moves->length = 0;
    if(x<7){//top
        canMove(moves,x,y,x+1,y,board,checkForCheck);
    }
    if(x<7&&y<7){//top right
        canMove(moves,x,y,x+1,y+1,board,checkForCheck);
    }
    if(y<7){//right
        canMove(moves,x,y,x,y+1,board,checkForCheck);
    }
    if(x>0&&y<7){//bottom right
        canMove(moves,x,y,x-1,y+1,board,checkForCheck);
    }
    if(x>0){//bottom
        canMove(moves,x,y,x-1,y,board,checkForCheck);
    }
    if(x>0&&y>0){//bottom left
        canMove(moves,x,y,x-1,y-1,board,checkForCheck);
    }
    if(y>0){//left
        canMove(moves,x,y,x,y-1,board,checkForCheck);
    }
    if(x<7&&y>0){//top left
        canMove(moves,x,y,x+1,y-1,board,checkForCheck);
    }
    //castles
    if(board[x][y].color==Black) {
        if (castles.Black_OO) {// king and rooks still in starting position
            if (board[7][5].piece_type == Empty && board[7][6].piece_type == Empty && !piece_attacked(7,4,board,NULL,Black) && !piece_attacked(7,5,board,NULL,Black) && !piece_attacked(7,6,board,NULL,Black)) {
                moves->movelist[moves->length] = (struct move){x,y,7,6,0};
                moves->length++;
            }
        }
        if (castles.Black_OOO) {
            if (board[7][3].piece_type == Empty && board[7][2].piece_type == Empty && board[7][1].piece_type == Empty && !piece_attacked(7,4,board,NULL,Black) && !piece_attacked(7,3,board,NULL,Black) && !piece_attacked(7,2,board,NULL,Black)) {
                moves->movelist[moves->length] = (struct move){x,y,7,2,0};
                moves->length++;
            }
        }
    }
    else if(board[x][y].color==White){
        if(castles.White_OO){
            if(board[0][5].piece_type == Empty && board[0][6].piece_type == Empty && !piece_attacked(0,4,board,NULL,White) && !piece_attacked(0,5,board,NULL,White) && !piece_attacked(0,6,board,NULL,White)){
                moves->movelist[moves->length] = (struct move){x,y,0,6,0};
                moves->length++;
            }
        }
        if(castles.White_OOO){
            if (board[0][3].piece_type == Empty && board[0][2].piece_type == Empty && board[0][1].piece_type == Empty && !piece_attacked(0,4,board,NULL,White) && !piece_attacked(0,3,board,NULL,White) && !piece_attacked(0,2,board,NULL,White)) {
                moves->movelist[moves->length] = (struct move){x,y,0,2,0};
                moves->length++;
            }
        }
    }
}

unsigned char piece_attacked(unsigned char x, unsigned char y,struct piece board[8][8],struct move* move,enum color color){
    struct moves moves={0,NULL};
    unsigned char attacked = 0;
    struct prevState prevStates[4];
    unsigned char prevState_Length = 0;

    if(move != NULL) {
        prevStates[0].piece = board[move->piece_x][move->piece_y];
        prevStates[0].piece_x = move->piece_x;
        prevStates[0].piece_y = move->piece_y;
        prevStates[1].piece = board[move->dest_x][move->dest_y];
        prevStates[1].piece_x = move->dest_x;
        prevStates[1].piece_y = move->dest_y;
        prevState_Length = 2;
        board[move->dest_x][move->dest_y] = board[move->piece_x][move->piece_y];
        board[move->piece_x][move->piece_y] = (struct piece) {Empty, None};
    }
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            if(board[i][j].color == invertColor(color)){// only check pieces of other color
                switch (board[i][j].piece_type) {
                    case Pawn:
                        PawnMoves(&moves,i,j,board,0);
                        break;
                    case Bishop:
                        BishopMoves(&moves,i,j,board,0);
                        break;
                    case Knight:
                        KnightMoves(&moves,i,j,board,0);
                        break;
                    case Rook:
                        RookMoves(&moves,i,j,board,0);
                        break;
                    case Queen:
                        QueenMoves(&moves,i,j,board,0);
                        break;
                    case King:
                        KingMoves(&moves,i,j,board,(struct castles){0,0,0,0},0);
                        break;
                    default:
                        break;
                }
                for (int k = 0; k < moves.length; ++k) {
                    if(moves.movelist[k].dest_x == x && moves.movelist[k].dest_y == y){
                        attacked = 1;
                    }
                }
                free(moves.movelist);
            }
        }
    }
    //set back moves
    for (int i = 0; i < prevState_Length; ++i) {
        board[prevStates[i].piece_x][prevStates[i].piece_y] = prevStates[i].piece;
    }
    return attacked;
}

unsigned char kingInCheck(enum color color,struct piece board[8][8], struct move move){
    if(board[move.piece_x][move.piece_y].piece_type==King){
        return piece_attacked(move.dest_x,move.dest_y,board,&move,color);
    }
    else{
        for (int i = 0; i < 8; ++i) {
            for (int j = 0; j < 8; ++j) {
                if(board[i][j].piece_type == King && board[i][j].color == color){
                    return piece_attacked(i,j,board,&move,color);
                }
            }
        }
        return 0;
    }
}

void FindMoves(struct moves* moves, unsigned char x, unsigned char y, struct piece board[8][8],struct castles castles){
    switch (board[x][y].piece_type) {
        case Empty:
        case EnPassentGhost:
            moves->length = 0; moves->movelist = NULL;
            break;
        case Pawn:
            PawnMoves(moves,x,y,board,1);
            break;
        case Bishop:
            BishopMoves(moves,x,y,board,1);
            break;
        case Knight:
            KnightMoves(moves,x,y,board,1);
            break;
        case Rook:
            RookMoves(moves,x,y,board,1);
            break;
        case Queen:
            QueenMoves(moves,x,y,board,1);
            break;
        case King:
            KingMoves(moves,x,y,board,castles,1);
            break;
    }
}

struct castles makeBoard(struct piece board[8][8],const char FEN[]){
    struct castles castles = {0,0,0,0};
    unsigned char x=7;
    unsigned char y=0;
    unsigned char done = 0;
    unsigned char cnt = 0;
    for(int i = 0; FEN[i]!='\0'; ++i) {
        if(!done) {
            switch (FEN[i]) {
                case ' ':
                    done = 1;
                    break;
                case '/':
                    x--;
                    y = 0;
                    break;
                case 'r':
                    board[x][y] = (struct piece) {Rook, Black};
                    y++;
                    break;
                case 'n':
                    board[x][y] = (struct piece) {Knight, Black};
                    y++;
                    break;
                case 'b':
                    board[x][y] = (struct piece) {Bishop, Black};
                    y++;
                    break;
                case 'q':
                    board[x][y] = (struct piece) {Queen, Black};
                    y++;
                    break;
                case 'k':
                    board[x][y] = (struct piece) {King, Black};
                    y++;
                    break;
                case 'p':
                    board[x][y] = (struct piece) {Pawn, Black};
                    y++;
                    break;
                case 'R':
                    board[x][y] = (struct piece) {Rook, White};
                    y++;
                    break;
                case 'N':
                    board[x][y] = (struct piece) {Knight, White};
                    y++;
                    break;
                case 'B':
                    board[x][y] = (struct piece) {Bishop, White};
                    y++;
                    break;
                case 'Q':
                    board[x][y] = (struct piece) {Queen, White};
                    y++;
                    break;
                case 'K':
                    board[x][y] = (struct piece) {King, White};
                    y++;
                    break;
                case 'P':
                    board[x][y] = (struct piece) {Pawn, White};
                    y++;
                    break;
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                    for (int j = 0; j < FEN[i] - '0'; ++j) {
                        board[x][y] = (struct piece) {Empty, None};
                        y++;
                    }
                    break;
                default:
                    break;
            }
        }else{
            if(FEN[i]==' '){cnt++;}
            if(cnt==1){
                switch (FEN[i]) {
                    case 'K':
                        castles.White_OO = 1;
                        break;
                    case 'Q':
                        castles.White_OOO = 1;
                        break;
                    case 'k':
                        castles.Black_OO = 1;
                        break;
                    case 'q':
                        castles.Black_OOO = 1;
                        break;
                }
            }
        }
    }
    return castles;
}

void makeFENCode(char* code, struct piece board[8][8]){
	int codeIndex = 0;
	int counter = 0;
	for(int x = 7; x >= 0; x--){
		for(int y = 0; y < 8; y++){
			switch(board[x][y].color){
				case None:
					counter++;
					break;
				case White:
					if(counter!=0){code[codeIndex] = '0'+counter; codeIndex++; counter = 0;}
					switch(board[x][y].piece_type){
						case Rook:
							code[codeIndex] = 'R';
							codeIndex++;
							break;
						case Knight:
							code[codeIndex] = 'N';
							codeIndex++;
							break;
						case Bishop:
							code[codeIndex] = 'B';
							codeIndex++;
							break;
						case Queen:
							code[codeIndex] = 'Q';
							codeIndex++;
							break;
						case King:
							code[codeIndex] = 'K';
							codeIndex++;
							break;
						case Pawn:
							code[codeIndex] = 'P';
							codeIndex++;
							break;
						default:
							break;
					}
					break;
				case Black:
					if(counter!=0){code[codeIndex] = '0'+counter; codeIndex++; counter = 0;}
					switch(board[x][y].piece_type){
						case Rook:
							code[codeIndex] = 'r';
							codeIndex++;
							break;
						case Knight:
							code[codeIndex] = 'n';
							codeIndex++;
							break;
						case Bishop:
							code[codeIndex] = 'b';
							codeIndex++;
							break;
						case Queen:
							code[codeIndex] = 'q';
							codeIndex++;
							break;
						case King:
							code[codeIndex] = 'k';
							codeIndex++;
							break;
						case Pawn:
							code[codeIndex] = 'p';
							codeIndex++;
							break;
						default:
							break;
					}
					break;
			}
		}
		if(counter!=0){code[codeIndex] = '0'+counter; codeIndex++; counter = 0;}
		if(x!=0){code[codeIndex] = '/'; codeIndex++;}
	}
	code[codeIndex] = '\0';
}

enum color invertColor(enum color color){
    switch (color) {
        case White:
            return Black;
        case Black:
            return White;
        case None:
            return None;
    }
    return None;
}

