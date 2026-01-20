// Matsui V9.0 - The Final Verdict (Safe Render + Sequential Logic)
// Build: clang -framework SDL2 -O3 -o matsui_final matsui_final.c
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>

// --- 設定 ---
#define N 128
#define TOTAL_POINTS (N * N * N)
#define SCREEN_SIZE 800
#define CELL_SIZE (SCREEN_SIZE / N)
#define INIT_DENSITY 10 // 10%くらいでスタート

typedef enum
{
    TYPE_VOID,
    TYPE_RED,
    TYPE_BLUE
} CellType;

typedef struct
{
    CellType type;
    uint32_t id;
} Node;

static Node *src_universe = NULL;
static Node *dst_universe = NULL;

static int count_red = 0;
static int count_blue = 0;

static inline int idx(int x, int y, int z) { return x + y * N + z * N * N; }
static inline int wrap(int v)
{
    if (v < 0)
        return v + N;
    if (v >= N)
        return v - N;
    return v;
}

// 疑似乱数
static uint32_t xor_rnd(uint32_t y)
{
    y ^= (y << 13);
    y ^= (y >> 17);
    return y ^ (y << 5);
}

void init_final()
{
    src_universe = (Node *)calloc(TOTAL_POINTS, sizeof(Node));
    dst_universe = (Node *)calloc(TOTAL_POINTS, sizeof(Node));

    for (int i = 0; i < TOTAL_POINTS; i++)
    {
        int r = rand() % 100;
        if (r < INIT_DENSITY / 2)
        {
            src_universe[i].type = TYPE_RED;
            src_universe[i].id = rand();
        }
        else if (r < INIT_DENSITY)
        {
            src_universe[i].type = TYPE_BLUE;
            src_universe[i].id = rand();
        }
        else
        {
            src_universe[i].type = TYPE_VOID;
        }
    }
}

// ★ここが核心：不公平なステップ処理★
void step_unfair()
{
    count_red = 0;
    count_blue = 0;

    // 1. 未来をクリア
    for (int i = 0; i < TOTAL_POINTS; i++)
        dst_universe[i].type = TYPE_VOID;

    // 2. 0番地から順に処理（i++）
    // これにより、indexが小さい粒子（画面左上、またはメモリの前方）が
    // 移動先の空席を「先に」確保する権利を持つ。
    for (int i = 0; i < TOTAL_POINTS; i++)
    {
        Node self = src_universe[i];
        if (self.type == TYPE_VOID)
            continue;

        // 座標計算
        int z = i / (N * N);
        int rem = i % (N * N);
        int y = rem / N;
        int x = rem % N;

        // 移動方向決定
        int dx = 0, dy = 0, dz = 0;
        uint32_t r = xor_rnd(self.id + i);

        // 赤は右、青は左へ行きたがる
        if (self.type == TYPE_RED)
            dx = 1;
        else
            dx = -1;

        // ランダム拡散 (30%)
        if ((r % 100) < 30)
        {
            dy = (r % 3) - 1;
            dz = ((r >> 2) % 3) - 1;
        }

        int tx = wrap(x + dx);
        int ty = wrap(y + dy);
        int tz = wrap(z + dz);
        int target_i = idx(tx, ty, tz);

        // ★椅子取りゲーム（早い者勝ち）★
        // dst_universe[target_i] が空なら入れる。
        // ここで「赤」と「青」が同じ場所を狙った場合、
        // ループの前の方にいる粒子（確率的に赤が多い配置なら赤、あるいは左側の粒子）が勝つ。

        if (dst_universe[target_i].type == TYPE_VOID)
        {
            dst_universe[target_i] = self;
            if (self.type == TYPE_RED)
                count_red++;
            else
                count_blue++;
        }
        else
        {
            // 移動失敗（衝突死）
            // 席が埋まっていたら消滅するルールにすることで、勝敗を明確にする
        }
    }

    // 入れ替え
    Node *tmp = src_universe;
    src_universe = dst_universe;
    dst_universe = tmp;
}

// 安全な描画（四角形）
void render_safe(SDL_Renderer *ren, int z_slice)
{
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255); // 背景黒
    SDL_RenderClear(ren);

    for (int y = 0; y < N; y++)
    {
        for (int x = 0; x < N; x++)
        {
            int i = idx(x, y, z_slice);
            Node n = src_universe[i];

            if (n.type != TYPE_VOID)
            {
                SDL_Rect rect = {x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE};
                if (n.type == TYPE_RED)
                {
                    SDL_SetRenderDrawColor(ren, 255, 50, 50, 255); // 赤
                }
                else
                {
                    SDL_SetRenderDrawColor(ren, 50, 50, 255, 255); // 青
                }
                SDL_RenderFillRect(ren, &rect);
            }
        }
    }
    SDL_RenderPresent(ren);
}

int main(int argc, char **argv)
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *win = SDL_CreateWindow("Matsui V9.0: The Final Verdict", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_SIZE, SCREEN_SIZE, 0);
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

    srand(time(NULL));
    init_final();

    int running = 1;
    int z = N / 2;
    int speed = 1; // 高速モードで開始

    printf("=== Matsui V9.0 Final ===\n");
    printf("Visuals: Safe Mode (No Red Bug)\n");
    printf("Logic  : Sequential Bias (i++ wins)\n");
    printf("---------------------------\n");
    printf("[SPACE]: Pause\n");
    printf("[R]    : Reset\n");

    while (running)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
                running = 0;
            if (e.type == SDL_KEYDOWN)
            {
                if (e.key.keysym.sym == SDLK_ESCAPE)
                    running = 0;
                if (e.key.keysym.sym == SDLK_r)
                    init_final();
                if (e.key.keysym.sym == SDLK_SPACE)
                {
                    // ポーズ機能（簡易）
                    printf("Paused. Press SPACE to resume.\n");
                    while (1)
                    {
                        SDL_WaitEvent(&e);
                        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_SPACE)
                            break;
                        if (e.type == SDL_QUIT)
                        {
                            running = 0;
                            break;
                        }
                    }
                }
            }
        }

        // 不公平な処理を実行
        step_unfair();
        render_safe(ren, z);

        // ログ出力：今度こそ「赤が増える」はず
        printf("\rRed: %6d vs Blue: %6d | Ratio: %.1f%%  ",
               count_red, count_blue,
               (double)count_red / (count_red + count_blue + 1) * 100.0);
        fflush(stdout);
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
