#include <SDL2/SDL.h>
#include "sql/sqlite3.h"
#include "src/chip8.h"
#undef main

#define SAMPLE_RATE 44100
#define TONE_FREQUENCY 440.0
#define AMPLITUDE 3000
#define SCALE 10

static SDL_AudioDeviceID audio_dev;
static double audio_phase = 0.0;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

 void audio_callback(void *userdata, Uint8 *stream, int len) {
    CHIP8 *chip = (CHIP8*)userdata;
    Sint16 *buffer = (Sint16*)stream;
    int samples = len / sizeof(Sint16);

    for (int i = 0; i < samples; i++) {
        // Читаем звуковой таймер 
        if (chip->ST > 0) {
            // Прямоугольный сигнал
            buffer[i] = (audio_phase < 0.5) ? AMPLITUDE : -AMPLITUDE;
            audio_phase += TONE_FREQUENCY / SAMPLE_RATE;
            if (audio_phase >= 1.0)
                audio_phase -= 1.0;
        } else {
            buffer[i] = 0;
            audio_phase = 0.0; 
        }
    }
}

 void init_audio(CHIP8 *chip) {
    SDL_AudioSpec desired, obtained;
    SDL_zero(desired);
    desired.freq = SAMPLE_RATE;
    desired.format = AUDIO_S16SYS;  // 16-битные сэмплы, системный порядок байт
    desired.channels = 1;            // Моно
    desired.samples = 2048;          // Размер буфера 
    desired.callback = audio_callback;
    desired.userdata = chip;

    audio_dev = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, 0);
    if (audio_dev == 0) {
        printf("Warning: cannot open audio device: %s\n", SDL_GetError());
    } else {
        SDL_PauseAudioDevice(audio_dev, 0);  // Снимаем с паузы
    }
}

 void handle_input(CHIP8 *chip) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) exit(0);

        if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
            int pressed = (e.type == SDL_KEYDOWN);
            switch (e.key.keysym.sym) {
                case SDLK_1: chip->keys[0x1] = pressed; break;
                case SDLK_2: chip->keys[0x2] = pressed; break;
                case SDLK_3: chip->keys[0x3] = pressed; break;
                case SDLK_4: chip->keys[0xC] = pressed; break;
                case SDLK_q: chip->keys[0x4] = pressed; break;
                case SDLK_w: chip->keys[0x5] = pressed; break;
                case SDLK_e: chip->keys[0x6] = pressed; break;
                case SDLK_r: chip->keys[0xD] = pressed; break;
                case SDLK_a: chip->keys[0x7] = pressed; break;
                case SDLK_s: chip->keys[0x8] = pressed; break;
                case SDLK_d: chip->keys[0x9] = pressed; break;
                case SDLK_f: chip->keys[0xE] = pressed; break;
                case SDLK_z: chip->keys[0xA] = pressed; break;
                case SDLK_x: chip->keys[0x0] = pressed; break;
                case SDLK_c: chip->keys[0xB] = pressed; break;
                case SDLK_v: chip->keys[0xF] = pressed; break;
            }
        }
    }
}

bool init_SDL() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        printf("SDL init error: %s\n", SDL_GetError());
        return false;
    }
    
    window = SDL_CreateWindow("CHIP-8 Emulator",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              SCREEN_WIDTH * SCALE,
                              SCREEN_HEIGHT * SCALE,
                              SDL_WINDOW_SHOWN);
    if (!window) {
        printf("SDL window error: %s\n", SDL_GetError());
        return false;
    }
    
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("SDL renderer error: %s\n", SDL_GetError());
        return false;
    }
    
    return true;
}

void draw_screen(CHIP8 *chip) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        uint64_t row = chip->display[y]; 
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            if (row & (1ULL << x)) {
                SDL_Rect rect = {
                    x * SCALE,
                    y * SCALE,
                    SCALE,
                    SCALE
                };
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }
    
    SDL_RenderPresent(renderer);
    chip->draw_flag = 0;
}

int main(int argc, char *argv[]) {
    if (!init_SDL()) return 1;
    
    CHIP8 chip;
    initialize(&chip);
     if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {  // AUDIO
        printf("SDL init error: %s\n", SDL_GetError());
        return 1;
    }
    if (!load_rom(&chip,argv[1])) {
        printf("Trying to load test rom...\n");
        if (!load_rom(&chip, "test.ch8")) {
            SDL_Quit();
            return 1;
        }
    }
    
    srand(time(NULL));
    init_audio(&chip);
    
    // Основной цикл эмуляции
    int running = 1;
    while (running) {
        handle_input(&chip);
        
        // Выполняем несколько инструкций за кадр для скорости
        for (int i = 0; i < 10; i++) {
            uint16_t opcode = fetch_opcode(&chip);
            execute(&chip, opcode);
        }
        
        update_timers(&chip);
        
        if (chip.draw_flag) {
            draw_screen(&chip);
        }   
        
        SDL_Delay(32); // ~60 FPS
    }
    if (audio_dev != 0)
        SDL_CloseAudioDevice(audio_dev);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}