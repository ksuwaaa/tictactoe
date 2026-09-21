#include "tictactoe.h"

CellState board[BOARD_SIZE][BOARD_SIZE];
MoveNode* history_head = NULL;
extern CurrentPlayer g_current_user;

void init_board(void)
{
    int i, j;
    for (i = 0; i < BOARD_SIZE; i++)
        for (j = 0; j < BOARD_SIZE; j++)
            board[i][j] = EMPTY;
    free_history();
}

void add_move_to_history(int r, int c, CellState player)
{
    MoveNode* node = (MoveNode*)malloc(sizeof(MoveNode));
    if (node == NULL) return;
    node->row = r;
    node->col = c;
    node->player = player;
    node->next = history_head;
    history_head = node;
}

void free_history(void)
{
    MoveNode* cur = history_head;
    while (cur) {
        MoveNode* nxt = cur->next;
        free(cur);
        cur = nxt;
    }
    history_head = NULL;
}


void save_history_to_file(void)
{
    FILE* f = fopen("game_history.txt", "a");
    if (!f) return;

    time_t now = time(NULL);
    char* ts = ctime(&now);
    if (ts) {
        size_t len = strlen(ts);
        if (len > 0 && ts[len - 1] == '\n') ts[len - 1] = '\0';
    }

    fprintf(f, "\n========================================\n");
    if (g_current_user.type == AUTH_ID) {
        fprintf(f, "ПАРТИЯ ИГРОКА (ID: %d) от %s\n", g_current_user.data.id, ts ? ts : "—");
    }
    else {
        fprintf(f, "ПАРТИЯ ИГРОКА (Login: %s) от %s\n", g_current_user.data.login, ts ? ts : "—");
    }
    fprintf(f, "========================================\n");

    MoveNode* cur = history_head;
    if (!cur) {
        fprintf(f, "Партия завершена без ходов.\n");
    }
    else {
        fprintf(f, "Ходы (от последнего к первому):\n");
        while (cur) {
            if (cur->player == CROSS) {
                fprintf(f, "  Крестик -> [%d, %d]\n", cur->row, cur->col);
            }
            else {
                fprintf(f, "  Нолик   -> [%d, %d]\n", cur->row, cur->col);
            }
            cur = cur->next;
        }
    }
    fprintf(f, "--- КОНЕЦ ПАРТИИ ---\n");
    fclose(f);
}

static int count_dir(int r, int c, int dr, int dc, CellState player)
{
    int count = 0, i;
    for (i = 1; i < 5; i++) {
        int nr = r + dr * i, nc = c + dc * i;
        if (nr < 0 || nr >= BOARD_SIZE || nc < 0 || nc >= BOARD_SIZE) break;
        if (board[nr][nc] != player) break;
        count++;
    }
    return count;
}

int check_win(int r, int c, CellState player)
{
    int dirs[4][2] = { {0,1}, {1,0}, {1,1}, {1,-1} };
    int d;
    for (d = 0; d < 4; d++) {
        int cnt = 1;
        cnt += count_dir(r, c, dirs[d][0], dirs[d][1], player);
        cnt += count_dir(r, c, -dirs[d][0], -dirs[d][1], player);
        if (cnt >= 5) return 1;
    }
    return 0;
}
