
#define _CRT_SECURE_NO_WARNINGS
#include "tictactoe.h"
#include <stdlib.h>
#include <limits.h>

#define AI_DEPTH   4
#define MAX_CANDS  20
#define NEAR_DIST  2

#define W5        100000
#define W4_OPEN    10000
#define W4_CLOSED   1000
#define W3_OPEN      500
#define W3_CLOSED     50
#define W2_OPEN       10

typedef struct { int r, c, score; } Cand;
#define CANDS_BUF (BOARD_SIZE * BOARD_SIZE)
static Cand g_cands[AI_DEPTH][CANDS_BUF];
static int eval_line(int r, int c, int dr, int dc, CellState p)
{
    int cnt = 1, oe = 0, nr, nc;


    nr = r + dr; nc = c + dc;
    while (nr >= 0 && nr < BOARD_SIZE && nc >= 0 && nc < BOARD_SIZE
        && board[nr][nc] == p) {
        cnt++; nr += dr; nc += dc;
    }
    if (nr >= 0 && nr < BOARD_SIZE && nc >= 0 && nc < BOARD_SIZE
        && board[nr][nc] == EMPTY) oe++;


    nr = r - dr; nc = c - dc;
    while (nr >= 0 && nr < BOARD_SIZE && nc >= 0 && nc < BOARD_SIZE
        && board[nr][nc] == p) {
        cnt++; nr -= dr; nc -= dc;
    }
    if (nr >= 0 && nr < BOARD_SIZE && nc >= 0 && nc < BOARD_SIZE
        && board[nr][nc] == EMPTY) oe++;

    if (cnt >= 5) return W5;
    if (cnt == 4) return oe == 2 ? W4_OPEN : W4_CLOSED;
    if (cnt == 3) return oe == 2 ? W3_OPEN : W3_CLOSED;
    if (cnt == 2) return oe == 2 ? W2_OPEN : 0;
    return 0;
}

static int cscore(int r, int c, CellState p)
{
    static const int DR[4] = { 0, 1, 1,  1 };
    static const int DC[4] = { 1, 0, 1, -1 };
    CellState op = (p == CROSS) ? ZERO : CROSS;
    int s = 0, i;
    for (i = 0; i < 4; i++) {
        s += eval_line(r, c, DR[i], DC[i], p);
        s += eval_line(r, c, DR[i], DC[i], op) * 11 / 10;
    }
    return s;
}

static int beval(CellState ai)
{
    static const int DR[4] = { 0, 1, 1,  1 };
    static const int DC[4] = { 1, 0, 1, -1 };
    CellState op = (ai == CROSS) ? ZERO : CROSS;
    int s = 0, r, c, i;

    // поиск границ
    int min_r = BOARD_SIZE, max_r = -1;
    int min_c = BOARD_SIZE, max_c = -1;

    for (r = 0; r < BOARD_SIZE; r++) {
        for (c = 0; c < BOARD_SIZE; c++) {
            if (board[r][c] != EMPTY) {
                if (r < min_r) min_r = r;
                if (r > max_r) max_r = r;
                if (c < min_c) min_c = c;
                if (c > max_c) max_c = c;
            }
        }
    }

    if (min_r > max_r) return 0;

  
    for (r = min_r; r <= max_r; r++) {
        for (c = min_c; c <= max_c; c++) {
            if (board[r][c] == EMPTY) continue;
            for (i = 0; i < 4; i++) {
                if (board[r][c] == ai)
                    s += eval_line(r, c, DR[i], DC[i], ai);
                else
                    s -= eval_line(r, c, DR[i], DC[i], op) * 11 / 10;
            }
        }
    }
    return s;
}

static int is_cand(int r, int c)
{
    int dr, dc;
    if (board[r][c] != EMPTY) return 0;
    for (dr = -NEAR_DIST; dr <= NEAR_DIST; dr++)
        for (dc = -NEAR_DIST; dc <= NEAR_DIST; dc++) {
            int nr = r + dr, nc = c + dc;
            if (nr >= 0 && nr < BOARD_SIZE && nc >= 0 && nc < BOARD_SIZE
                && board[nr][nc] != EMPTY) return 1;
        }
    return 0;
}

static int cmp_cand(const void* a, const void* b)
{
    return ((Cand*)b)->score - ((Cand*)a)->score;
}

static int get_cands(Cand* ca, CellState cur)
{
    int counter = 0, r, c;
    int min_r = BOARD_SIZE, max_r = -1;
    int min_c = BOARD_SIZE, max_c = -1;

    // границы
    for (r = 0; r < BOARD_SIZE; r++) {
        for (c = 0; c < BOARD_SIZE; c++) {
            if (board[r][c] != EMPTY) {
                if (r < min_r) min_r = r;
                if (r > max_r) max_r = r;
                if (c < min_c) min_c = c;
                if (c > max_c) max_c = c;
            }
        }
    }

    // увеличиваем рамку на NEAR_DIST
    if (min_r <= max_r) {
        min_r = (min_r - NEAR_DIST < 0) ? 0 : min_r - NEAR_DIST;
        max_r = (max_r + NEAR_DIST >= BOARD_SIZE) ? BOARD_SIZE - 1 : max_r + NEAR_DIST;
        min_c = (min_c - NEAR_DIST < 0) ? 0 : min_c - NEAR_DIST;
        max_c = (max_c + NEAR_DIST >= BOARD_SIZE) ? BOARD_SIZE - 1 : max_c + NEAR_DIST;
    }
    else {
        
        min_r = max_r = BOARD_SIZE / 2;
        min_c = max_c = BOARD_SIZE / 2;
    }

    
    for (r = min_r; r <= max_r; r++) {
        for (c = min_c; c <= max_c; c++) {
            if (is_cand(r, c)) {
                ca[counter].r = r; ca[counter].c = c;
                ca[counter].score = cscore(r, c, cur);
                counter++;
            }
        }
    }

    qsort(ca, counter, sizeof(Cand), cmp_cand);
    if (counter > MAX_CANDS) counter = MAX_CANDS;
    return counter;
}

static int minimax(int depth, int alpha, int beta,
    int maxing, CellState ai, CellState hu)
{
    Cand* ca = g_cands[depth];
    CellState cur = maxing ? ai : hu;
    int nc = get_cands(ca, cur);
    int best = maxing ? INT_MIN : INT_MAX;
    int i;

    if (!nc) return 0;

    for (i = 0; i < nc; i++) {
        int r = ca[i].r, c = ca[i].c, val;

        board[r][c] = cur;

        if (check_win(r, c, cur)) {

            val = maxing
                ? (W5 - (AI_DEPTH - depth))
                : (-W5 + (AI_DEPTH - depth));
            board[r][c] = EMPTY;
            return val;
        }

        if (depth <= 1) {
            val = beval(ai);
            board[r][c] = EMPTY;
        }
        else {
            val = minimax(depth - 1, alpha, beta, !maxing, ai, hu);
            board[r][c] = EMPTY;
        }


        if (maxing) {
            if (val > best)  best = val;
            if (best > alpha) alpha = best;
        }
        else {
            if (val < best)  best = val;
            if (best < beta)  beta = best;
        }
        if (beta <= alpha) break;
    }
    return best;
}

void bot_make_move(CellState bot_choice)
{
    CellState hu = (bot_choice == CROSS) ? ZERO : CROSS;
    Cand* ca = g_cands[0];
    int nc, i, br, bc, bs;


    if (board[BOARD_SIZE / 2][BOARD_SIZE / 2] == EMPTY) {
        board[BOARD_SIZE / 2][BOARD_SIZE / 2] = bot_choice;
        add_move_to_history(BOARD_SIZE / 2, BOARD_SIZE / 2, bot_choice);
        return;
    }

    nc = get_cands(ca, bot_choice);
    if (!nc) return;


    for (i = 0; i < nc; i++) {
        board[ca[i].r][ca[i].c] = bot_choice;
        if (check_win(ca[i].r, ca[i].c, bot_choice)) {
            add_move_to_history(ca[i].r, ca[i].c, bot_choice);
            return;
        }
        board[ca[i].r][ca[i].c] = EMPTY;
    }


    for (i = 0; i < nc; i++) {
        board[ca[i].r][ca[i].c] = hu;
        if (check_win(ca[i].r, ca[i].c, hu)) {
            board[ca[i].r][ca[i].c] = bot_choice;
            add_move_to_history(ca[i].r, ca[i].c, bot_choice);
            return;
        }
        board[ca[i].r][ca[i].c] = EMPTY;
    }


    bs = INT_MIN; br = ca[0].r; bc = ca[0].c;
    for (i = 0; i < nc; i++) {
        int r = ca[i].r, c = ca[i].c, s;
        board[r][c] = bot_choice;
        s = minimax(AI_DEPTH - 1, INT_MIN, INT_MAX, 0, bot_choice, hu);
        board[r][c] = EMPTY;
        if (s > bs) { bs = s; br = r; bc = c; }
    }
    board[br][bc] = bot_choice;
    add_move_to_history(br, bc, bot_choice);
}