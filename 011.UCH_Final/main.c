
#include "device_driver.h"
#include <stdlib.h>

#define MAP_WIDTH  16
#define MAP_HEIGHT 12
#define TILE_SIZE  20

#define EMPTY  0
#define WALL   1
#define DOT    2
#define PACMAN 3
#define GHOST  4
#define CAR    5

#define COLOR_EMPTY  BLACK
#define COLOR_WALL   BLUE
#define COLOR_DOT    YELLOW
#define COLOR_PACMAN GREEN
#define COLOR_GHOST  RED
#define COLOR_CAR    WHITE

#define MAX_GHOSTS 5
#define MAX_CARS 2
#define MAX_STAGE 5

/////////////////////////////////////////////////////////////////////////////////////////////////////////
#define BASE 380  // 기본 박자 (ms)
#define REST -1   // 쉼표(무음) 정의
#define MAPLE_LEN (sizeof(maple_theme)/sizeof(maple_theme[0]))

enum key {
    C1, C1_, D1, D1_, E1, E1_, F1, F1_, G1, G1_, A1, A1_, B1,
    C2, C2_, D2, D2_, E2, E2_, F2, F2_, G2, G2_, A2, A2_, B2,
    C3, C3_, D3, D3_, E3, E3_, F3, F3_, G3
};
enum note { N16 = BASE/4, N8 = BASE/2, N4 = BASE, N2 = BASE*2, N1 = BASE*4 };

const unsigned short tone_freq[] = {
    // C1  ~ B1
    261, 277, 293, 311, 329, 349, 369, 391, 415, 440, 466, 493,
    // C2  ~ B2
    523, 554, 587, 622, 659, 698, 739, 783, 830, 880, 932, 987,
    // C3 ~ G3
    1046, 1108, 1174, 1244, 1318, 1396, 1480, 1568, 1661
};

const int maple_theme[][2] = {
    // {E2_, N2}, {E2_, N8}, {F2_, N8}, {G2_, N8}, {E2_, N4},
    // {E2_, N2}, {E2_, N8}, {F2_, N8}, {G2_, N8}, {E2_, N4},
    // {E2_, N2}, {E2_, N8}, {F2_, N8}, {G2_, N8}, {E2_, N4},
    // {E2_, N2}, {F2, N8}, {G2_, N8}, {C3_, N4}, {A2_, N8}
    {E2, N8}, {D2_, N8}, {E2, N8}, {D2_, N8},
    {E2, N8}, {B1, N8}, {D2, N8}, {C2, N8},
    {A1, N4}, {REST, N8}, {E2, N8}, {G2, N8},
    {A2, N8}, {B2, N4}
};

int note_index = 0;

/////////////////////////////////////////////////////////////////////////////////////////////////////////


typedef struct { int x, y; } ENTITY;

unsigned char map[MAP_HEIGHT][MAP_WIDTH];
unsigned short color[] = {COLOR_EMPTY, COLOR_WALL, COLOR_DOT, COLOR_PACMAN, COLOR_GHOST, COLOR_CAR};



ENTITY pacman;
ENTITY ghosts[MAX_GHOSTS];
ENTITY cars[MAX_CARS];
int car_dir_x[MAX_CARS];
int car_dir_y[MAX_CARS];
unsigned char ghost_under[MAX_GHOSTS];

int score = 0;
int total_dots = 0;
int game_over = 0;
int game_clear = 0;
int stage = 1;

extern volatile int Jog_key_in;
extern volatile int Jog_key;
extern volatile int TIM4_expired;

const int stage_ghost_count[MAX_STAGE] = {1, 2, 2, 3, 3};  // Stage별 고스트 수

const unsigned char maps[MAX_STAGE][MAP_HEIGHT][MAP_WIDTH] = {
    {
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,2,2,2,1,2,2,2,2,2,1,2,2,2,2,1},
        {1,2,1,1,1,2,1,2,1,2,1,1,1,2,2,1},
        {1,2,1,2,2,2,2,2,2,2,2,2,2,2,2,1},
        {1,2,1,2,1,1,1,2,2,1,1,1,1,2,2,1},
        {1,2,2,2,2,1,2,2,2,2,2,1,2,2,2,1},
        {1,2,1,1,2,1,1,1,1,1,2,1,1,1,2,1},
        {1,2,2,1,2,2,2,2,2,2,2,2,2,1,2,1},
        {1,2,1,1,1,1,1,2,1,1,1,1,2,1,2,1},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
        {1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
    },
    {
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
        {1,2,1,1,1,2,2,2,1,1,2,1,1,1,2,1},
        {1,2,2,2,1,2,2,2,1,2,2,2,2,1,2,1},
        {1,2,1,2,1,1,1,2,1,1,1,2,2,1,2,1},
        {1,2,2,2,2,2,1,2,2,2,2,2,2,2,2,1},
        {1,2,1,1,1,2,1,1,1,1,2,1,2,1,2,1},
        {1,2,2,2,1,2,2,2,2,2,2,2,2,2,2,1},
        {1,2,1,2,1,1,1,2,1,1,1,1,2,1,2,1},
        {1,2,1,2,2,2,2,2,2,2,2,2,2,1,2,1},
        {1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
    },
    {
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,2,2,2,2,1,2,2,2,1,2,2,2,2,2,1},
        {1,2,1,1,2,1,1,2,1,1,2,1,1,1,2,1},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
        {1,2,1,1,2,1,1,2,1,1,1,2,2,1,2,1},
        {1,2,2,2,2,2,1,2,2,2,2,2,2,1,2,1},
        {1,2,1,1,1,2,1,2,1,1,1,1,1,1,2,1},
        {1,2,2,2,1,2,2,2,2,2,2,2,2,1,2,1},
        {1,2,1,2,1,1,1,2,1,1,1,1,2,1,2,1},
        {1,2,1,2,2,2,2,2,2,2,2,2,2,1,2,1},
        {1,2,2,2,1,1,1,1,1,1,1,1,2,2,2,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
    },
    {
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,2,2,1,2,2,2,2,2,2,2,1,2,2,2,1},
        {1,2,1,1,1,2,1,2,1,2,1,1,1,2,1,1},
        {1,2,1,2,2,2,2,2,2,2,2,2,2,2,2,1},
        {1,2,1,2,1,1,1,2,1,1,1,2,2,2,2,1},
        {1,2,2,2,1,2,2,2,2,2,1,2,2,2,2,1},
        {1,2,1,1,1,2,1,2,1,2,1,1,1,1,2,1},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
        {1,2,1,1,1,1,1,2,1,1,1,1,1,1,2,1},
        {1,2,2,2,1,2,2,2,2,2,2,2,2,2,2,1},
        {1,1,1,2,2,2,1,2,1,1,1,1,2,2,2,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
    },
    {
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,2,2,2,1,2,2,2,1,2,2,2,1,2,2,1},
        {1,2,1,2,1,2,1,2,1,2,2,2,1,2,1,1},
        {1,2,1,2,2,2,1,2,2,2,1,2,2,2,2,1},
        {1,2,1,1,1,2,1,1,1,2,1,1,2,2,1,1},
        {1,2,2,2,1,2,2,2,1,2,2,2,2,2,2,1},
        {1,1,1,2,1,1,1,2,1,1,1,2,1,1,2,1},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
        {1,2,1,1,1,1,1,1,1,1,1,1,1,1,2,1},
        {1,2,1,2,2,2,1,2,2,2,1,2,2,2,2,1},
        {1,2,2,2,1,2,2,2,1,2,2,2,1,2,2,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
    }
};

void Draw_Tile(int x, int y, int type) { Lcd_Draw_Box(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, color[type]); }

int Is_Ghost_Position(int x, int y) {
    int i;
    for (i = 0; i < stage_ghost_count[stage-1]; i++) {
        if (ghosts[i].x == x && ghosts[i].y == y)
            return 1;
    }
    return 0;
}

void Draw_Map(void) {
    int x, y;
    total_dots = 0;
    for (y = 0; y < MAP_HEIGHT; y++) {
        for (x = 0; x < MAP_WIDTH; x++) {
            map[y][x] = maps[stage-1][y][x];
            Draw_Tile(x, y, map[y][x]);
            if (maps[stage-1][y][x] == DOT)
                total_dots++;
        }
    }

    int i;
    for (i = 0; i < stage_ghost_count[stage-1]; i++) {
        int gx = ghosts[i].x;
        int gy = ghosts[i].y;
        ghost_under[i] = (maps[stage-1][gy][gx] == DOT) ? DOT : EMPTY;
    }
}

void Draw_Entities(void) {
    int i;
    Draw_Tile(pacman.x, pacman.y, PACMAN);
    for (i = 0; i < stage_ghost_count[stage-1]; i++)
        Draw_Tile(ghosts[i].x, ghosts[i].y, GHOST);
    for (i = 0; i < MAX_CARS; i++)
        Draw_Tile(cars[i].x, cars[i].y, CAR);
}

void Move_Pacman(int dx, int dy) {
    int nx = pacman.x + dx;
    int ny = pacman.y + dy;

    if (map[ny][nx] == WALL || game_over || game_clear)
        return;

    Draw_Tile(pacman.x, pacman.y, map[pacman.y][pacman.x]);

    if (map[ny][nx] == DOT) {
        score++;
        map[ny][nx] = EMPTY;
        if (score == total_dots)
            game_clear = 1;
    }

    pacman.x = nx;
    pacman.y = ny;
    Draw_Tile(pacman.x, pacman.y, PACMAN);
    Lcd_Printf(0, 0, WHITE, BLACK, 1, 1, "SCORE: %d", score);
}

void Move_Ghosts(void) {
    int i;
    for (i = 0; i < stage_ghost_count[stage-1]; i++) {
        int dx = 0, dy = 0;
        if (ghosts[i].x < pacman.x) dx = 1;
        else if (ghosts[i].x > pacman.x) dx = -1;
        else if (ghosts[i].y < pacman.y) dy = 1;
        else if (ghosts[i].y > pacman.y) dy = -1;

        int nx = ghosts[i].x + dx;
        int ny = ghosts[i].y + dy;

        if (map[ny][nx] != WALL && !Is_Ghost_Position(nx, ny)) {
            if (ghost_under[i] == DOT)
                Draw_Tile(ghosts[i].x, ghosts[i].y, DOT);
            else
                Draw_Tile(ghosts[i].x, ghosts[i].y, EMPTY);

            ghost_under[i] = map[ny][nx];
            ghosts[i].x = nx;
            ghosts[i].y = ny;
            Draw_Tile(ghosts[i].x, ghosts[i].y, GHOST);
        }

        if (ghosts[i].x == pacman.x && ghosts[i].y == pacman.y)
            game_over = 1;
    }
}

void Move_Cars(void) {
    int i;
    for (i = 0; i < MAX_CARS; i++) {
        if (!Is_Ghost_Position(cars[i].x, cars[i].y)) {
            if (map[cars[i].y][cars[i].x] == DOT)
                Draw_Tile(cars[i].x, cars[i].y, DOT);
            else
                Draw_Tile(cars[i].x, cars[i].y, EMPTY);
        }

        int nx = cars[i].x + car_dir_x[i];
        int ny = cars[i].y + car_dir_y[i];

        if (map[ny][nx] == WALL) {
            car_dir_x[i] = -car_dir_x[i];
            car_dir_y[i] = -car_dir_y[i];
            nx = cars[i].x + car_dir_x[i];
            ny = cars[i].y + car_dir_y[i];
        }

        if (map[ny][nx] == DOT || map[ny][nx] == EMPTY) {
            cars[i].x = nx;
            cars[i].y = ny;
        }

        if (!Is_Ghost_Position(cars[i].x, cars[i].y)) {
            Draw_Tile(cars[i].x, cars[i].y, CAR);
        }

        if (cars[i].x == pacman.x && cars[i].y == pacman.y)
            game_over = 1;
    }
}

void Init_Stage(void) {
    int i;
    Lcd_Clr_Screen();
    total_dots = 0;
    score = 0;
    game_clear = 0;
    game_over = 0;

    pacman.x = 1; pacman.y = 1;

    for (i = 0; i < MAX_GHOSTS; i++) {
        ghosts[i].x = 10 + i;
        ghosts[i].y = 3 + i;
        ghost_under[i] = EMPTY;
    }

    cars[0] = (ENTITY){1, 5};
    cars[1] = (ENTITY){7, 1};
    car_dir_x[0] = 1; car_dir_y[0] = 0;
    car_dir_x[1] = 0; car_dir_y[1] = 1;

    int ghost_speed = 800;
    if (stage == 3 || stage == 5) {
        ghost_speed = 533;
    }
    TIM4_Repeat_Interrupt_Enable(1, ghost_speed);

    Draw_Map();
    Draw_Entities();
    Lcd_Printf(0, 0, WHITE, BLACK, 1, 1, "STAGE %d", stage);
}

#define DISPLAY_MODE 3

void System_Init(void) {
    Clock_Init();
    LED_Init();
    Key_Poll_Init();
    Uart1_Init(115200);
    SCB->VTOR = 0x08003000;
    SCB->SHCSR = 7 << 16;
}

void Main(void) {
    System_Init();
    Lcd_Init(DISPLAY_MODE);
    Jog_Poll_Init();
    Jog_ISR_Enable(1);
    Uart1_RX_Interrupt_Enable(1);
    Init_Stage();

    TIM3_Out_Init();
    TIM2_Repeat_Interrupt_Enable(1, BASE); 

    for (;;) {
        if (Jog_key_in && !game_over && !game_clear) {
            switch (Jog_key) {
                case 0: Move_Pacman(0, -1); break;
                case 1: Move_Pacman(0, 1); break;
                case 2: Move_Pacman(-1, 0); break;
                case 3: Move_Pacman(1, 0); break;
            }
            Jog_key_in = 0;
        }

        if (TIM4_expired) {
            
            if (!game_over && !game_clear) {
                Move_Ghosts();
                Move_Cars();
            }
            if (game_clear) {
                Lcd_Printf(10, 100, GREEN, BLACK, 2, 2, "STAGE CLEAR!");
                Lcd_Printf(10, 140, WHITE, BLACK, 1, 1, "PRESS SW1 TO CONTINUE");
                int key = Jog_Get_Pressed();
                if (key == 32) {
                    game_clear = 0;
                    stage++;
                    if (stage > MAX_STAGE) {
                        Lcd_Printf(10, 100, YELLOW, BLACK, 2, 2, "ALL CLEAR!");
                        Lcd_Printf(10, 140, WHITE, BLACK, 1, 1, "THANK YOU FOR PLAYING!");
                        while(1);
                    }
                    Init_Stage();
                }
            }
            if (game_over)
                Lcd_Printf(30, 100, RED, BLACK, 2, 2, "GAME OVER");

            TIM4_expired = 0;
        }
    }
}

int main(void) {
    Main();
    return 0;
}
