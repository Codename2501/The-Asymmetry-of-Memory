// Matsui V7.6 - The Mirror Universe (Relative Coordinate Hashing & Auto-Scan)
// Build: clang -framework SDL2 -O3 -o matsui_stream matsui_stream_v7_6.c
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <dispatch/dispatch.h>
#include <time.h>

#define N 129 // Odd number to prevent ghost-swapping
#define TOTAL_POINTS (N * N * N)
#define SCREEN_SIZE 800
#define CELL_SIZE (SCREEN_SIZE / N)

#define SPAWN_RATE 40
#define LIFE_SPAN 120

typedef enum { TYPE_VOID, TYPE_PLUS, TYPE_MINUS, TYPE_FLASH } CellType;

typedef struct {
    CellType type;
    int32_t life;
    uint32_t id;
} Node;

static Node *src_universe = NULL;
static Node *dst_universe = NULL;

// Statistics for monitoring
static volatile int32_t current_pop_red = 0;
static volatile int32_t current_pop_blue = 0;
static int32_t total_spawn_red = 0;
static int32_t total_spawn_blue = 0;

static inline int idx(int x, int y, int z) { return x + y * N + z * N * N; }

// Hash Function (Keys removed)
// Uses 'relative_y' to ensure mathematical symmetry between Red and Blue.
static inline uint32_t hash_rand(int x, int relative_y, int z, uint32_t t) {
    uint32_t h = (uint32_t)x * 374761393U + (uint32_t)relative_y * 668265263U + (uint32_t)z * 352462463U + t;
    h = (h ^ (h >> 13)) * 1274126177U;
    return h ^ (h >> 16);
}

static inline int wrap_x(int x) {
    if (x < 0) return x + N;
    if (x >= N) return x - N;
    return x;
}

static inline int get_drift(uint32_t id, uint32_t t) {
    uint32_t h = hash_rand(id, 0, 0, t); // ID-based jitter
    int r = h % 100;
    if (r < 40) return 0;
    if (r < 70) return 1;
    return -1;
}

void init_mirror() {
    if (src_universe) free(src_universe);
    if (dst_universe) free(dst_universe);
    src_universe = (Node *)calloc(TOTAL_POINTS, sizeof(Node));
    dst_universe = (Node *)calloc(TOTAL_POINTS, sizeof(Node));
    if (!src_universe || !dst_universe) exit(1);
    for(int i=0; i<TOTAL_POINTS; i++) src_universe[i].type = TYPE_VOID;
}

void step_mirror(uint32_t frame_count) {
    current_pop_red = 0;
    current_pop_blue = 0;
    
    __block int32_t frame_spawn_red = 0;
    __block int32_t frame_spawn_blue = 0;

    dispatch_apply(N, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^(size_t z) {
        int32_t local_pop_red = 0;
        int32_t local_pop_blue = 0;
        int32_t local_spawn_red = 0;
        int32_t local_spawn_blue = 0;

        for (int y = 0; y < N; y++) {
            for (int x = 0; x < N; x++) {
                int i = idx(x, y, (int)z);
                
                Node out;
                out.type = TYPE_VOID;
                out.life = 0;
                out.id = 0;
                
                if (src_universe[i].type == TYPE_FLASH) {
                    out.type = TYPE_FLASH;
                    out.life = src_universe[i].life - 10;
                    if (out.life <= 0) out.type = TYPE_VOID;
                    dst_universe[i] = out;
                    continue;
                }

                // Detection Logic
                Node in_plus; in_plus.type = TYPE_VOID;
                int up_y = y - 1;
                if (up_y >= 0) {
                    int offsets[] = {0, -1, 1};
                    for (int k=0; k<3; k++) {
                        int tx = wrap_x(x + offsets[k]);
                        int ti = idx(tx, up_y, (int)z);
                        Node n = src_universe[ti];
                        if (n.type == TYPE_PLUS) {
                            int drift = (k==0) ? 0 : (k==1 ? 1 : -1);
                            if (get_drift(n.id, frame_count) == drift) { in_plus = n; break; }
                        }
                    }
                }

                Node in_minus; in_minus.type = TYPE_VOID;
                int down_y = y + 1;
                if (down_y < N) {
                    int offsets[] = {0, -1, 1};
                    for (int k=0; k<3; k++) {
                        int tx = wrap_x(x + offsets[k]);
                        int ti = idx(tx, down_y, (int)z);
                        Node n = src_universe[ti];
                        if (n.type == TYPE_MINUS) {
                            int drift = (k==0) ? 0 : (k==1 ? 1 : -1);
                            if (get_drift(n.id, frame_count) == drift) { in_minus = n; break; }
                        }
                    }
                }

                // Collision and Movement
                if (in_plus.type == TYPE_PLUS && in_minus.type == TYPE_MINUS) {
                    out.type = TYPE_FLASH; out.life = 255;
                }
                else if (in_plus.type == TYPE_PLUS) {
                    out = in_plus; out.life--;
                    if (out.life <= 0) out.type = TYPE_VOID;
                    else local_pop_red++;
                }
                else if (in_minus.type == TYPE_MINUS) {
                    out = in_minus; out.life--;
                    if (out.life <= 0) out.type = TYPE_VOID;
                    else local_pop_blue++;
                }

                // --- Mirror Spawn Logic ---
                
                if (y == 0) { // Ceiling (Red Spawn)
                    // Relative Y: 0
                    uint32_t r = hash_rand(x, 0, (int)z, frame_count) % 1000;
                    if (r < SPAWN_RATE) {
                        out.type = TYPE_PLUS; out.life = LIFE_SPAN; out.id = r;
                        local_spawn_red++; local_pop_red++;
                    }
                }
                else if (y == N - 1) { // Floor (Blue Spawn)
                    // Relative Y: (N-1) - (N-1) = 0
                    // Using '0' for Blue ensures it uses the EXACT SAME hash logic as Red.
                    // If x, z, t are the same, Red and Blue share the exact same destiny.
                    uint32_t r = hash_rand(x, 0, (int)z, frame_count) % 1000;
                    if (r < SPAWN_RATE) {
                        out.type = TYPE_MINUS; out.life = LIFE_SPAN; out.id = r;
                        local_spawn_blue++; local_pop_blue++;
                    }
                }

                dst_universe[i] = out;
            }
        }
        
        __sync_fetch_and_add(&current_pop_red, local_pop_red);
        __sync_fetch_and_add(&current_pop_blue, local_pop_blue);
        __sync_fetch_and_add(&frame_spawn_red, local_spawn_red);
        __sync_fetch_and_add(&frame_spawn_blue, local_spawn_blue);
    });

    Node *tmp = src_universe;
    src_universe = dst_universe;
    dst_universe = tmp;
    
    total_spawn_red += frame_spawn_red;
    total_spawn_blue += frame_spawn_blue;
}

void render_mirror(SDL_Renderer *ren, SDL_Texture *tex, int z_slice) {
    static uint32_t *pixels = NULL;
    if (!pixels) pixels = (uint32_t *)malloc(sizeof(uint32_t) * SCREEN_SIZE * SCREEN_SIZE);

    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            int i = idx(x, y, z_slice);
            Node n = src_universe[i];
            uint8_t r = 0, g = 0, b = 0;
            
            if (n.type == TYPE_PLUS) {
                int val = n.life * 2; if(val>255) val=255;
                r=val; g=val*0.8; b=0; 
            }
            else if (n.type == TYPE_MINUS) {
                int val = n.life * 2; if(val>255) val=255;
                r=0; g=val*0.8; b=val; 
            }
            else if (n.type == TYPE_FLASH) {
                int val = n.life; if(val>255) val=255;
                r=val; g=val; b=val;
            }
            
            for (int py = 0; py < CELL_SIZE; py++) {
                int yy = y * CELL_SIZE + py;
                for (int px = 0; px < CELL_SIZE; px++) {
                    int xx = x * CELL_SIZE + px;
                    pixels[yy * SCREEN_SIZE + xx] = (255u<<24) | (b<<16) | (g<<8) | r;
                }
            }
        }
    }
    SDL_UpdateTexture(tex, NULL, pixels, SCREEN_SIZE * sizeof(uint32_t));
    SDL_RenderCopy(ren, tex, NULL, NULL);
    SDL_RenderPresent(ren);
    
    printf("Z:%3d | POP: R %d vs B %d | SPAWN: R %d vs B %d\n", 
           z_slice, current_pop_red, current_pop_blue, total_spawn_red, total_spawn_blue);
}

int main(int argc, char **argv) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return 1;
    SDL_Window *win = SDL_CreateWindow("Matsui V7.6: The Mirror Universe", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_SIZE, SCREEN_SIZE, 0);
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture *tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_SIZE, SCREEN_SIZE);
    
    init_mirror();
    
    int running = 1;
    int z_slice = N/2;
    uint32_t frame_count = 0;
    
    printf("V7.6 Mirror Universe. Auto-scanning Z-slices.\n");

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = 0;
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) running = 0;
                if (e.key.keysym.sym == SDLK_r) {
                    init_mirror();
                    total_spawn_red = 0; total_spawn_blue = 0;
                }
                // Manual Z-slice control
                if (e.key.keysym.sym == SDLK_LEFT) z_slice = (z_slice - 1 + N) % N;
                if (e.key.keysym.sym == SDLK_RIGHT) z_slice = (z_slice + 1) % N;
            }
        }
        
        step_mirror(frame_count++);
        
        // Auto-scan function: Moves the cross-section every 5 frames to scan the entire universe
        if (frame_count % 5 == 0) {
            z_slice = (z_slice + 1) % N;
        }
        
        render_mirror(ren, tex, z_slice);
    }
    
    free(src_universe); free(dst_universe);
    SDL_DestroyTexture(tex); SDL_DestroyRenderer(ren); SDL_DestroyWindow(win); SDL_Quit();
    return 0;
}
