#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define TILEMAP_WIDTH 320   // Original tilemap dimensions
#define TILEMAP_HEIGHT 288
#define TILE_SIZE 16        // Original tile size
#define DISPLAY_SCALE 4     // Scale factor for display (increased to 4x)
#define TILES_ACROSS 20     // 320 / 16 = 20 tiles
#define TILES_DOWN 18       // 288 / 16 = 18 tiles
#define WINDOW_WIDTH (TILEMAP_WIDTH * DISPLAY_SCALE + 300)  // Extra space for UI
#define WINDOW_HEIGHT (TILEMAP_HEIGHT * DISPLAY_SCALE + 100)

typedef enum {
    COLLISION_SAFE = 0,              // Green
    COLLISION_CRASH = 1,             // Red - Hit barrier/grass
    COLLISION_WATER = 2,             // Blue - Fall in water
    COLLISION_ROAD_LEFT_HALF = 3,    // Yellow - Road on left
    COLLISION_ROAD_RIGHT_HALF = 4,   // Orange - Road on right
    COLLISION_SLOW = 5,              // Brown - Mud/rough terrain
    COLLISION_BOOST = 6,             // Cyan - Speed boost
    COLLISION_WHEELIE = 7,           // Magenta - Wheelie ramp
    COLLISION_JUMP = 8,              // White - Jump ramp
    COLLISION_TYPE_COUNT
} CollisionType;

// Color definitions
SDL_Color collision_colors[COLLISION_TYPE_COUNT] = {
    {0, 255, 0, 255},      // Green - Safe
    {255, 0, 0, 255},      // Red - Crash
    {0, 100, 255, 255},    // Blue - Water
    {255, 255, 0, 255},    // Yellow - Left half
    {255, 165, 0, 255},    // Orange - Right half
    {139, 69, 19, 255},    // Brown - Slow
    {0, 255, 255, 255},    // Cyan - Boost
    {255, 0, 255, 255},    // Magenta - Wheelie
    {255, 255, 255, 255}   // White - Jump
};

const char* collision_names[COLLISION_TYPE_COUNT] = {
    "Safe Road",
    "Crash/Barrier",
    "Water",
    "Road Left Half",
    "Road Right Half", 
    "Slow Terrain",
    "Speed Boost",
    "Wheelie Ramp",
    "Jump Ramp"
};

char collision_table[360];  // 20x18 = 360 tiles
int current_collision_type = 0;
int show_collision_overlay = 1;
int overlay_alpha = 128;  // Semi-transparent
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Texture* tilemap_texture = NULL;

void UpdateWindowTitle(); // Function declaration


void UpdateWindowTitle() {
    char title[256];
    snprintf(title, sizeof(title), "Collision Editor - Current: %s [%d] - Click tiles to set collision type", 
             collision_names[current_collision_type], current_collision_type);
    SDL_SetWindowTitle(window, title);
}

int InitSDL() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 0;
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
        SDL_Quit();
        return 0;
    }

    window = SDL_CreateWindow("Collision Editor - Click tiles to set collision type",
                             SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                             WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        return 0;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 0;
    }

    return 1;
}

void CleanupSDL() {
    if (tilemap_texture) SDL_DestroyTexture(tilemap_texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
}

SDL_Texture* LoadTilemap(const char* filename) {
    SDL_Surface* surface = IMG_Load(filename);
    if (!surface) {
        printf("Unable to load tilemap %s! SDL_image Error: %s\n", filename, IMG_GetError());
        return NULL;
    }

    // Verify dimensions
    if (surface->w != TILEMAP_WIDTH || surface->h != TILEMAP_HEIGHT) {
        printf("Warning: Expected %dx%d tilemap, got %dx%d\n", 
               TILEMAP_WIDTH, TILEMAP_HEIGHT, surface->w, surface->h);
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    if (!texture) {
        printf("Unable to create texture from %s! SDL Error: %s\n", filename, SDL_GetError());
        return NULL;
    }

    printf("Loaded tilemap: %s\n", filename);
    return texture;
}

void InitCollisionTable() {
    // Initialize all tiles as crash by default
    for (int i = 0; i < 360; i++) {  // 20x18 = 360 tiles
        collision_table[i] = COLLISION_CRASH;
    }
}

void DrawTileCollision(int tile_x, int tile_y, int tile_id) {
    SDL_Rect tile_rect = {
        tile_x * TILE_SIZE * DISPLAY_SCALE, 
        tile_y * TILE_SIZE * DISPLAY_SCALE, 
        TILE_SIZE * DISPLAY_SCALE, 
        TILE_SIZE * DISPLAY_SCALE
    };
    
    // Debug: Check for potential array bounds issues
    if (tile_id >= 360) {
        printf("ERROR: tile_id %d is out of bounds! (tile_x=%d, tile_y=%d)\n", tile_id, tile_x, tile_y);
        return;
    }
    
    if (show_collision_overlay) {
        // Show ALL collision types, including safe tiles
        SDL_Color color = collision_colors[collision_table[tile_id]];
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, overlay_alpha);
        
        // Enable blend mode for transparency
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_RenderFillRect(renderer, &tile_rect);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }
    
    // Draw tile border for grid visibility
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 32);  // Very light border
    SDL_RenderDrawRect(renderer, &tile_rect);
}

void DrawUI() {
    int ui_x = TILEMAP_WIDTH * DISPLAY_SCALE + 30;
    
    // Draw current collision type info (larger)
    SDL_Rect info_rect = {ui_x, 30, 200, 120};
    SDL_Color current_color = collision_colors[current_collision_type];
    SDL_SetRenderDrawColor(renderer, current_color.r, current_color.g, current_color.b, current_color.a);
    SDL_RenderFillRect(renderer, &info_rect);
    
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(renderer, &info_rect);
    
    // Toggle overlay button (larger)
    SDL_Rect toggle_rect = {ui_x, 170, 200, 40};
    SDL_SetRenderDrawColor(renderer, show_collision_overlay ? 0 : 64, 128, 0, 255);
    SDL_RenderFillRect(renderer, &toggle_rect);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(renderer, &toggle_rect);
    
    // Instructions
    printf("\rCurrent: %s [%d] | T: Toggle overlay | Alpha: %d | Tiles: 20x18 (360 total)", 
           collision_names[current_collision_type], current_collision_type, overlay_alpha);
    fflush(stdout);
}

void Render() {
    // Clear screen
    SDL_SetRenderDrawColor(renderer, 32, 32, 32, 255);
    SDL_RenderClear(renderer);
    
    // Draw tilemap background if loaded
    if (tilemap_texture) {
        SDL_Rect src_rect = {0, 0, TILEMAP_WIDTH, TILEMAP_HEIGHT};
        SDL_Rect dst_rect = {0, 0, TILEMAP_WIDTH * DISPLAY_SCALE, TILEMAP_HEIGHT * DISPLAY_SCALE};
        SDL_RenderCopy(renderer, tilemap_texture, &src_rect, &dst_rect);
    }
    
    // Debug: Count how many safe tiles we're trying to render
    static int debug_counter = 0;
    int safe_tiles_found = 0;
    int safe_tiles_rendered = 0;
    
    // Draw collision overlay for each tile (20x18 = 360 tiles)
    for (int tile_y = 0; tile_y < TILES_DOWN; tile_y++) {
        for (int tile_x = 0; tile_x < TILES_ACROSS; tile_x++) {
            int tile_id = tile_y * TILES_ACROSS + tile_x;
            
            if (collision_table[tile_id] == COLLISION_SAFE) {
                safe_tiles_found++;
                if (safe_tiles_found <= 24) safe_tiles_rendered++;
            }
            
            DrawTileCollision(tile_x, tile_y, tile_id);
        }
    }
    
    // Debug output every 60 frames
    if (debug_counter++ % 60 == 0) {
        printf("\rSafe tiles found: %d, Safe tiles rendered: %d    ", 
               safe_tiles_found, safe_tiles_rendered);
        fflush(stdout);
    }
    
    DrawUI();
    SDL_RenderPresent(renderer);
}

void HandleMouseClick(int mouse_x, int mouse_y) {
    // Check if click is within tilemap area
    if (mouse_x < TILEMAP_WIDTH * DISPLAY_SCALE && mouse_y < TILEMAP_HEIGHT * DISPLAY_SCALE) {
        int tile_x = mouse_x / (TILE_SIZE * DISPLAY_SCALE);
        int tile_y = mouse_y / (TILE_SIZE * DISPLAY_SCALE);
        
        // Ensure we're within bounds
        if (tile_x < TILES_ACROSS && tile_y < TILES_DOWN) {
            int tile_id = tile_y * TILES_ACROSS + tile_x;
            collision_table[tile_id] = current_collision_type;
            printf("\nSet tile %d (pos %d,%d) to %s [index check: %d*%d+%d=%d]\n", 
                   tile_id, tile_x, tile_y, collision_names[current_collision_type],
                   tile_y, TILES_ACROSS, tile_x, tile_id);
        }
    }
    
    // Check UI clicks
    int ui_x = TILEMAP_WIDTH * DISPLAY_SCALE + 20;
    if (mouse_x >= ui_x && mouse_x <= ui_x + 200 && mouse_y >= 140 && mouse_y <= 170) {
        show_collision_overlay = !show_collision_overlay;
        printf("\nOverlay %s\n", show_collision_overlay ? "ON" : "OFF");
    }
}

void SaveCollisionData(const char* filename) {
    FILE* file = fopen(filename, "wb");
    if (file) {
        fwrite(collision_table, 1, 360, file);  // Save 360 tiles (20x18)
        fclose(file);
        printf("\nSaved collision data to %s (360 tiles)\n", filename);
        
        // Verify by showing some key positions
        printf("Sample collision values:\n");
        printf("Top-left (0,0) = tile %d, value %d\n", 0, collision_table[0]);
        printf("Top-right (19,0) = tile %d, value %d\n", 19, collision_table[19]);
        printf("Bottom-left (0,17) = tile %d, value %d\n", 17*20, collision_table[17*20]);
        printf("Bottom-right (19,17) = tile %d, value %d\n", 17*20+19, collision_table[17*20+19]);
    } else {
        printf("\nError: Could not save to %s\n", filename);
    }
}

void LoadCollisionData(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (file) {
        // Get file size first
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        fseek(file, 0, SEEK_SET);
        
        // Clear the collision table first
        for (int i = 0; i < 360; i++) {
            collision_table[i] = COLLISION_CRASH;
        }
        
        size_t bytes_to_read = (file_size < 360) ? file_size : 360;
        size_t bytes_read = fread(collision_table, 1, bytes_to_read, file);
        fclose(file);
        
        printf("\nLoaded collision data from %s\n", filename);
        printf("File size: %ld bytes, Read: %zu bytes\n", file_size, bytes_read);
        
        // Debug: Show first few collision values
        printf("First 10 collision values: ");
        for (int i = 0; i < 10 && i < bytes_read; i++) {
            printf("%d ", collision_table[i]);
        }
        printf("\n");
        
    } else {
        printf("\nError: Could not load from %s\n", filename);
    }
}

int main(int argc, char* argv[]) {
    if (!InitSDL()) {
        return 1;
    }
    
    InitCollisionTable();
    
    // Try to load tilemap if provided as argument
    if (argc > 1) {
        tilemap_texture = LoadTilemap(argv[1]);
    } else {
        printf("Usage: %s <tilemap.png>\n", argv[0]);
        printf("You can still use the editor without a tilemap for reference.\n");
    }
    
    // Set initial window title
    UpdateWindowTitle();
    
    printf("Collision Editor Controls:\n");
    printf("1-9: Select collision type\n");
    printf("Click: Set tile collision type\n");
    printf("T: Toggle collision overlay\n");
    printf("+/-: Adjust overlay transparency\n");
    printf("S: Save collision.dat\n");
    printf("L: Load collision.dat\n");
    printf("ESC: Quit\n\n");
    
    int running = 1;
    SDL_Event e;
    
    while (running) {
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_QUIT:
                    running = 0;
                    break;
                    
                case SDL_MOUSEBUTTONDOWN:
                    if (e.button.button == SDL_BUTTON_LEFT) {
                        HandleMouseClick(e.button.x, e.button.y);
                    }
                    break;
                    
                case SDL_KEYDOWN:
                    switch (e.key.keysym.sym) {
                        case SDLK_ESCAPE:
                            running = 0;
                            break;
                        case SDLK_1:
                            current_collision_type = 0;
                            UpdateWindowTitle();
                            break;
                        case SDLK_2:
                            current_collision_type = 1;
                            UpdateWindowTitle();
                            break;
                        case SDLK_3:
                            current_collision_type = 2;
                            UpdateWindowTitle();
                            break;
                        case SDLK_4:
                            current_collision_type = 3;
                            UpdateWindowTitle();
                            break;
                        case SDLK_5:
                            current_collision_type = 4;
                            UpdateWindowTitle();
                            break;
                        case SDLK_6:
                            current_collision_type = 5;
                            UpdateWindowTitle();
                            break;
                        case SDLK_7:
                            current_collision_type = 6;
                            UpdateWindowTitle();
                            break;
                        case SDLK_8:
                            current_collision_type = 7;
                            UpdateWindowTitle();
                            break;
                        case SDLK_9:
                            current_collision_type = 8;
                            UpdateWindowTitle();
                            break;
                        case SDLK_t:
                            show_collision_overlay = !show_collision_overlay;
                            break;
                        case SDLK_PLUS:
                        case SDLK_EQUALS:
                            overlay_alpha = (overlay_alpha < 255) ? overlay_alpha + 16 : 255;
                            break;
                        case SDLK_MINUS:
                            overlay_alpha = (overlay_alpha > 16) ? overlay_alpha - 16 : 16;
                            break;
                        case SDLK_s:
                            SaveCollisionData("collision.dat");
                            break;
                        case SDLK_l:
                            LoadCollisionData("collision.dat");
                            break;
                    }
                    break;
            }
        }
        
        Render();
        SDL_Delay(16);  // ~60fps
    }
    
    CleanupSDL();
    printf("\nCollision editor closed.\n");
    return 0;
}

// Compile with: gcc collision_editor.c -lSDL2 -lSDL2_image -o collision_editor
