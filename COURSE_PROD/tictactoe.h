#ifndef TICTACTOE_H
#define TICTACTOE_H

#define BOARD_SIZE 15
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <raylib.h>

typedef enum CellState { EMPTY, CROSS, ZERO } CellState;

typedef enum { AUTH_ID, AUTH_LOGIN } AuthType;

typedef union {
    int id;
    char login[15];
} PlayerData;

typedef struct {
    AuthType type;
    PlayerData data;
} CurrentPlayer;

extern CurrentPlayer g_current_user;


typedef struct MoveNode {
    int row;
    int col;
    CellState player;
    struct MoveNode* next;
} MoveNode;

extern CellState board[BOARD_SIZE][BOARD_SIZE];
extern MoveNode* history_head;

void init_board();
int check_win(int r, int c, CellState player);
void add_move_to_history(int r, int c, CellState player);
void free_history();
void save_history_to_file();
void bot_make_move(enum CellState bot_choice);

#endif