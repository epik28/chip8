#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <SDL2/SDL.h>
#include "src/chip8.h"
#include "src/sdl.h"
#undef main

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