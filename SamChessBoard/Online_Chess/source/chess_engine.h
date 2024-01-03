/*
 * chess_engine.h
 *
 *  Created on: 20 nov. 2023
 *      Author: J&S
 */

#ifndef CHESS_ENGINE_H
#define CHESS_ENGINE_H

#include <stdio.h>
#include <stdlib.h>

// Enumeration for piece types
enum piece_type {
    Empty,
    Pawn,
    Bishop,
    Knight,
    Rook,
    Queen,
    King,
    EnPassentGhost
};

// Enumeration for colors
enum color {
    White,
    Black,
    None // for empty squares
};

// Structure for a piece
struct piece {
    enum piece_type piece_type;
    enum color color;
};

// Structure for moves
struct moves {
    unsigned char length;
    struct move* movelist;
};

// Structure for a single move
struct move {
    unsigned char piece_x;
    unsigned char piece_y;
    unsigned char dest_x;
    unsigned char dest_y;
    unsigned char capt;
};

// Structure for castles
struct castles {
    unsigned char Black_OO;
    unsigned char White_OO;
    unsigned char Black_OOO;
    unsigned char White_OOO;
};

// Structure for previous state
struct prevState {
    struct piece piece;
    unsigned char piece_x;
    unsigned char piece_y;
};

// Function prototypes
unsigned char kingInCheck(enum color color, struct piece board[8][8], struct move move);
unsigned char piece_attacked(unsigned char x, unsigned char y, struct piece board[8][8], struct move* move, enum color color);
enum color invertColor(enum color color);
unsigned char canMove(struct moves* moves, unsigned char x, unsigned char y, unsigned char target_x, unsigned char target_y, struct piece board[8][8], unsigned char checkForCheck);
void PawnMoves(struct moves* moves, unsigned char x, unsigned char y, struct piece board[8][8], unsigned char checkForCheck);
void BishopMoves(struct moves* moves, unsigned char x, unsigned char y, struct piece board[8][8], unsigned char checkForCheck);
void KnightMoves(struct moves* moves, unsigned char x, unsigned char y, struct piece board[8][8], unsigned char checkForCheck);
void RookMoves(struct moves* moves, unsigned char x, unsigned char y, struct piece board[8][8], unsigned char checkForCheck);
void QueenMoves(struct moves* moves, unsigned char x, unsigned char y, struct piece board[8][8], unsigned char checkForCheck);
void KingMoves(struct moves* moves, unsigned char x, unsigned char y, struct piece board[8][8], struct castles castles, unsigned char checkForCheck);
void FindMoves(struct moves* moves, unsigned char x, unsigned char y, struct piece board[8][8], struct castles castles);
struct castles makeBoard(struct piece board[8][8], const char FEN[]);
void makeFENCode(char* code, struct piece board[8][8]);

#endif // CHESS_ENGINE_H
