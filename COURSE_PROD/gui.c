#define _CRT_SECURE_NO_WARNINGS
#pragma execution_character_set("utf-8")
#include "tictactoe.h"
#include <stdio.h>
#include <string.h>


#define WIN_W      820
#define WIN_H      680
#define CELL       40
#define BOARD_PX   (CELL * BOARD_SIZE)
#define BX         20
#define BY         ((WIN_H - BOARD_PX) / 2)
#define PIECE_R    14


#define COL_BG       RAYWHITE
#define COL_GRID     BLACK
#define COL_CROSS    (Color){210, 30,  30,  255}
#define COL_ZERO     (Color){30,  90,  210, 255}
#define COL_LASTMARK (Color){255, 200, 0,   255}
#define COL_BTN      (Color){40,  40,  40,  255}
#define COL_BTN_H    (Color){80,  80,  80,  255}
#define COL_BTN_TXT  WHITE
#define COL_GAMEOVER (Color){200, 20,  20,  255}
#define COL_WIN      (Color){212, 175, 55,  255}
#define COL_SHADOW   (Color){0,   0,   0,   60}


typedef enum {
    SCR_AUTH,
    SCR_MENU,
    SCR_CHOOSE,
    SCR_GAME,
    SCR_RESULT_WIN,
    SCR_RESULT_LOSE,
    SCR_HELP,
    SCR_ABOUT
} Screen;

static Screen g_screen = SCR_AUTH;
static CellState g_human = CROSS;
static CellState g_bot = ZERO;
static int            g_over = 0;
static int            g_moves = 0;
static Font           g_font;
static int g_should_close = 0;
CurrentPlayer g_current_user;

static int g_auth_method = 0; // 0 - по ID, 1 - по Логину
static char g_input_buffer[16] = "\0";
static int g_letter_count = 0;

static double g_bot_total_time = 0.0;
static int    g_bot_moves_count = 0;

static void txt(const char* text, int x, int y, int size, Color col)
{
    DrawTextEx(g_font, text,
        (Vector2) {
        (float)x, (float)y
    },
        (float)size, 1.0f, col);
}


static int txt_w(const char* text, int size)
{
    Vector2 v = MeasureTextEx(g_font, text, (float)size, 1.0f);
    return (int)v.x;
}



static int mouse_in(Rectangle r)
{
    return CheckCollisionPointRec(GetMousePosition(), r);
}

static int draw_button(Rectangle r, const char* text, int font_size)
{
    int hovered = mouse_in(r);
    int clicked = hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

    DrawRectangleRec((Rectangle) { r.x + 3, r.y + 3, r.width, r.height }, COL_SHADOW);
    DrawRectangleRec(r, hovered ? COL_BTN_H : COL_BTN);
    DrawRectangleLinesEx(r, 1, (Color) { 120, 120, 120, 255 });

    int tw = txt_w(text, font_size);
    int th = font_size;
    txt(text,
        (int)(r.x + (r.width - tw) / 2),
        (int)(r.y + (r.height - th) / 2),
        font_size, COL_BTN_TXT);
    return clicked;
}

static Vector2 cell_pos(int row, int col)
{
    return (Vector2) {
        (float)(BX + col * CELL + CELL / 2),
            (float)(BY + row * CELL + CELL / 2)
    };
}

static void draw_board(void)
{
    int r, c;


    for (r = 0; r <= BOARD_SIZE; r++)
        DrawLine(BX, BY + r * CELL, BX + BOARD_SIZE * CELL, BY + r * CELL, COL_GRID);
    for (c = 0; c <= BOARD_SIZE; c++)
        DrawLine(BX + c * CELL, BY, BX + c * CELL, BY + BOARD_SIZE * CELL, COL_GRID);

    for (r = 0; r < BOARD_SIZE; r++) {
        for (c = 0; c < BOARD_SIZE; c++) {
            if (board[r][c] == EMPTY) continue;

            Vector2 p = cell_pos(r, c);
            int     px = (int)p.x, py = (int)p.y;


            if (history_head &&
                history_head->row == r && history_head->col == c) {
                DrawRectangle(px - PIECE_R - 2, py - PIECE_R - 2,
                    (PIECE_R + 2) * 2, (PIECE_R + 2) * 2, COL_LASTMARK);
            }

            if (board[r][c] == CROSS) {
                int d = PIECE_R - 2;
                DrawLineEx((Vector2) { (float)(px - d), (float)(py - d) },
                    (Vector2) {
                    (float)(px + d), (float)(py + d)
                }, 4.0f, COL_CROSS);
                DrawLineEx((Vector2) { (float)(px + d), (float)(py - d) },
                    (Vector2) {
                    (float)(px - d), (float)(py + d)
                }, 4.0f, COL_CROSS);
            }
            else {
                DrawCircle(px, py, PIECE_R, COL_ZERO);
                DrawCircle(px, py, PIECE_R - 5, COL_BG);
            }
        }
    }
}


static void update_draw_menu(void)
{
    const char* title = "КРЕСТИКИ НОЛИКИ";
    //const char* sub = "15 х 15  *  пять в ряд";

    txt(title, (WIN_W - txt_w(title, 42)) / 2, 90, 42, BLACK);
    //txt(sub, (WIN_W - txt_w(sub, 18)) / 2, 142, 18, DARKGRAY);

    float bw = 300, bh = 52, bx = (WIN_W - bw) / 2;

    if (draw_button((Rectangle) { bx, 220, bw, bh }, "ИГРАТЬ С БОТОМ", 22))
        g_screen = SCR_CHOOSE;
    if (draw_button((Rectangle) { bx, 290, bw, bh }, "СПРАВКА", 22))
        g_screen = SCR_HELP;
    if (draw_button((Rectangle) { bx, 360, bw, bh }, "О ПРОГРАММЕ", 22))
        g_screen = SCR_ABOUT;
    if (draw_button((Rectangle) { bx, 430, bw, bh }, "ВЫХОД", 22))
        g_should_close = 1;

}

static void make_bot_move_measured(void)
{
    double t_start = GetTime();
    bot_make_move(g_bot);
    double t_end = GetTime();

    double ms = (t_end - t_start) * 1000.0;
    g_bot_total_time += ms;
    g_bot_moves_count++;

    printf("Bot's turn time for %d turn: %.2f ms | average time: %.2f ms\n",
        g_bot_moves_count, ms, g_bot_total_time / g_bot_moves_count);
}


static void start_game(void)
{
    init_board();
    g_bot_total_time = 0.0;
    g_bot_moves_count = 0;
    g_over = 0;
    g_moves = 0;
    g_screen = SCR_GAME;
    if (g_bot == CROSS) {
        make_bot_move_measured();
        g_moves++;
    }
}

static void update_draw_choose(void)
{
    const char* prompt = "Выберите вашу сторону";
    txt(prompt, (WIN_W - txt_w(prompt, 28)) / 2, 220, 28, BLACK);

    float bw = 160, bh = 80, gap = 60;
    float bx = (WIN_W - (bw * 2 + gap)) / 2;


    Rectangle rx = { (float)bx, 300.0f, (float)bw, (float)bh };
    DrawRectangleRec(rx, mouse_in(rx)
        ? (Color) { 240, 200, 200, 255 } : (Color) { 255, 220, 220, 255 });
    DrawRectangleLinesEx(rx, 2, COL_CROSS);
    txt("Х",
        (int)(rx.x + (bw - txt_w("Х", 48)) / 2),
        (int)(rx.y + (bh - 48) / 2), 48, COL_CROSS);
    txt("Крестики",
        (int)(rx.x + (bw - txt_w("Крестики", 14)) / 2),
        (int)(rx.y + bh - 22), 14, DARKGRAY);
    if (mouse_in(rx) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        g_human = CROSS; g_bot = ZERO;
        start_game();
    }


    Rectangle ro = { (float)(bx + bw + gap), 300.0f, (float)bw, (float)bh };
    DrawRectangleRec(ro, mouse_in(ro)
        ? (Color) { 200, 210, 255, 255 } : (Color) { 220, 230, 255, 255 });
    DrawRectangleLinesEx(ro, 2, COL_ZERO);
    txt("О",
        (int)(ro.x + (bw - txt_w("О", 48)) / 2),
        (int)(ro.y + (bh - 48) / 2), 48, COL_ZERO);
    txt("Нолики",
        (int)(ro.x + (bw - txt_w("Нолики", 14)) / 2),
        (int)(ro.y + bh - 22), 14, DARKGRAY);
    if (mouse_in(ro) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        g_human = ZERO; g_bot = CROSS;
        start_game();
    }

    if (draw_button((Rectangle) { 20, WIN_H - 60, 120, 40 }, "Назад", 18))
        g_screen = SCR_MENU;
}


static void update_draw_game(void)
{

    int panel_x = BX + BOARD_SIZE * CELL + 20;

    txt("ХОД:", panel_x, BY + 10, 16, DARKGRAY);

    if (!g_over) {
        const char* whose;
        Color wc;
        if (g_moves % 2 == 0) {
            whose = (g_human == CROSS) ? "Х (Вы)" : "Х (Бот)";
            wc = COL_CROSS;
        }
        else {
            whose = (g_human == ZERO) ? "О (Вы)" : "О (Бот)";
            wc = COL_ZERO;
        }
        txt(whose, panel_x, BY + 32, 22, wc);
    }

    txt(TextFormat("Ходов: %d", g_moves), panel_x, BY + 70, 16, DARKGRAY);
    txt("Легенда:", panel_x, BY + 110, 15, DARKGRAY);
    txt("Х", panel_x, BY + 130, 24, COL_CROSS);
    txt(g_human == CROSS ? "- Вы" : "- Бот", panel_x + 22, BY + 135, 14, DARKGRAY);
    txt("О", panel_x, BY + 158, 24, COL_ZERO);
    txt(g_human == ZERO ? "- ВЫ" : "- Бот", panel_x + 22, BY + 163, 14, DARKGRAY);
    txt("[Esc] - выйти", panel_x, BY + BOARD_PX - 30, 14, LIGHTGRAY);

    draw_board();

    if (g_over) return;


    CellState cur_turn = (g_moves % 2 == 0) ? CROSS : ZERO;
    if (cur_turn == g_human && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 m = GetMousePosition();
        int col = (int)((m.x - BX) / CELL);
        int row = (int)((m.y - BY) / CELL);

        if (row >= 0 && row < BOARD_SIZE &&
            col >= 0 && col < BOARD_SIZE &&
            board[row][col] == EMPTY) {

            board[row][col] = g_human;
            add_move_to_history(row, col, g_human);
            g_moves++;

            if (check_win(row, col, g_human)) {
                g_over = 1;
                save_history_to_file();
                g_screen = SCR_RESULT_WIN;
                return;
            }
            make_bot_move_measured();
            g_moves++;

            if (history_head &&
                check_win(history_head->row, history_head->col, g_bot)) {
                g_over = 1;
                save_history_to_file();
                g_screen = SCR_RESULT_LOSE;
                return;
            }
        }
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        save_history_to_file();
        g_screen = SCR_MENU;
    }
}


static void draw_result(int won)
{
    DrawRectangle(0, 0, WIN_W, WIN_H, (Color) { 0, 0, 0, 140 });

    const char* msg = won ? "YOU WIN!" : "GAME OVER";
    Color        col = won ? COL_WIN : COL_GAMEOVER;
    int          fs = 72;
    txt(msg, (WIN_W - txt_w(msg, fs)) / 2, WIN_H / 2 - 80, fs, col);

    const char* sub = won ? "Вы обыграли бота!" : "Бот оказался сильнее...";
    txt(sub, (WIN_W - txt_w(sub, 22)) / 2, WIN_H / 2 + 10, 22, WHITE);

    float bw = 220, bh = 50;
    if (draw_button((Rectangle) { WIN_W / 2 - bw - 20, WIN_H / 2 + 70, bw, bh },
        "Играть снова", 20))
        g_screen = SCR_CHOOSE;
    if (draw_button((Rectangle) { WIN_W / 2 + 20, WIN_H / 2 + 70, bw, bh },
        "Главное меню", 20))
        g_screen = SCR_MENU;
}


static void update_draw_help(void)
{
    const char* title = "Справка";
    txt(title, (WIN_W - txt_w(title, 34)) / 2, 60, 34, BLACK);

    const char* lines[] = {
        "Цель игры - выстроить 5 своих фишек в ряд:",
        "по горизонтали, вертикали или диагонали.",
        "",
        "Управление:",
        "  Левая кнопка мыши - поставить фишку",
        "  Esc - выйти из игры",
        "",
        "Обозначения:",
        "  X (красный) - крестики",
        "  O (синий)   - нолики",
        "",
        "Желтый квадрат - последний сделанный ход.",
        "",
        "История партии сохраняется в файл:",
        "  game_history.txt  ",
    };
    int n = sizeof(lines) / sizeof(lines[0]);
    int y = 120;
    for (int i = 0; i < n; i++) {
        txt(lines[i], 80, y, 18, DARKGRAY);
        y += 28;
    }

    if (draw_button((Rectangle) { (WIN_W - 160) / 2, WIN_H - 80, 160, 44 }, "Назад", 20))
        g_screen = SCR_MENU;
}

static void update_draw_about(void)
{
    const char* title = "О ПРОГРАММЕ";
    txt(title, (WIN_W - txt_w(title, 34)) / 2, 60, 34, BLACK);

    const char* lines[] = {
        "Игра: Крестики-нолики, 15x15, 5 в ряд",
        "",
        "Авторы:",
        "  Ксения Гаськова",
        "  Мария Аветисян",
        "",
        "Группа:    5131001/50602",
        "Год:       2026",
        "ВУЗ:       СПбПУ Петра Великого",
        "",
        "Алгоритм бота: минимакс с отсечением Альфа-Бета",
        "Глубина поиска: 4 уровня",
        "Графика: Raylib",
    };
    int n = sizeof(lines) / sizeof(lines[0]);
    int y = 120;
    for (int i = 0; i < n; i++) {
        txt(lines[i], 80, y, 18, DARKGRAY);
        y += 28;
    }

    if (draw_button((Rectangle) { (WIN_W - 160) / 2, WIN_H - 80, 160, 44 }, "Назад", 20))
        g_screen = SCR_MENU;
}


static void update_draw_auth(void)
{
    txt("АВТОРИЗАЦИЯ ИГРОКА", (WIN_W - txt_w("АВТОРИЗАЦИЯ ИГРОКА", 32)) / 2, 100, 32, BLACK);

    float bw = 140, bh = 40;
    if (draw_button((Rectangle) { WIN_W / 2 - 150, 180, bw, bh }, "Вход по ID", 16)) g_auth_method = 0;
    if (draw_button((Rectangle) { WIN_W / 2 + 10, 180, bw, bh }, "Вход по Логину", 16)) g_auth_method = 1;

    if (g_auth_method == 0) DrawRectangleLines(WIN_W / 2 - 150, 180, bw, bh, RED);
    else DrawRectangleLines(WIN_W / 2 + 10, 180, bw, bh, RED);

    txt(g_auth_method == 0 ? "Введите ваш цифровой ID:" : "Введите ваш Логин:",
        80, 260, 20, DARKGRAY);

    int key = GetCharPressed();
    while (key > 0) {
        int is_valid = (g_auth_method == 0) ? (key >= '0' && key <= '9') : (key >= 32 && key <= 125);

        if (is_valid && (g_letter_count < 14)) {
            g_input_buffer[g_letter_count] = (char)key;
            g_letter_count++;
            g_input_buffer[g_letter_count] = '\0';
        }
        key = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE)) {
        g_letter_count--;
        if (g_letter_count < 0) g_letter_count = 0;
        g_input_buffer[g_letter_count] = '\0';
    }

    DrawRectangleLinesEx((Rectangle) { 80, 300, WIN_W - 160, 50 }, 2, BLACK);
    txt(g_input_buffer, 95, 312, 24, BLUE);


    if (g_letter_count > 0) {
        if (draw_button((Rectangle) { (WIN_W - 200) / 2, 400, 200, 50 }, "ВОЙТИ", 22) || IsKeyPressed(KEY_ENTER)) {
            if (g_auth_method == 0) {
                g_current_user.type = AUTH_ID;
                g_current_user.data.id = atoi(g_input_buffer);
            }
            else {
                g_current_user.type = AUTH_LOGIN;
                strcpy(g_current_user.data.login, g_input_buffer);
            }
            g_screen = SCR_MENU;
        }
    }
}


int main(void)
{
    SetTraceLogLevel(LOG_NONE);
    SetConfigFlags(FLAG_WINDOW_HIGHDPI);
    InitWindow(WIN_W, WIN_H, "Крестики  нолики 15 х 15");
    SetTargetFPS(60);


    int count = 0;
    const char* fontChars = "абвгдеёжзийклмнопрстуфхцчшщъыьэюяАБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ"
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
        " 0123456789!?:;.,-_+=()[]/*";
    int* codepoints = LoadCodepoints(fontChars, &count);


    g_font = LoadFontEx("arial.ttf", 48, codepoints, count);

    UnloadCodepoints(codepoints);
    SetTextureFilter(g_font.texture, TEXTURE_FILTER_BILINEAR);

    while (!WindowShouldClose() && !g_should_close) {
        BeginDrawing();
        ClearBackground(COL_BG);

        switch (g_screen) {
        case SCR_MENU:        update_draw_menu();    break;
        case SCR_CHOOSE:      update_draw_choose();  break;
        case SCR_GAME:        update_draw_game();    break;
        case SCR_RESULT_WIN:  draw_board(); draw_result(1); break;
        case SCR_RESULT_LOSE: draw_board(); draw_result(0); break;
        case SCR_HELP:        update_draw_help();    break;
        case SCR_ABOUT:       update_draw_about();   break;
        case SCR_AUTH: update_draw_auth(); break;
        }

        EndDrawing();
    }

    UnloadFont(g_font);
    free_history();
    CloseWindow();
    return 0;
}
