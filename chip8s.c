#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <SDL2/SDL.h>
#undef main

#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#define SCALE 10
#define MEMORY_SIZE 4096
#define STACK_SIZE 16
#define REGISTERS_COUNT 16
#define KEY_COUNT 16

typedef struct CHIP8 {
    uint8_t memory[MEMORY_SIZE];   // Память
    uint8_t V[REGISTERS_COUNT];    // Регистры V0-VF
    uint16_t I;                    // Регистр адреса
    uint8_t SP;                    // Указатель стека
    uint16_t PC;                   // Счётчик команд
    uint8_t DT;                    // Таймер задержки
    uint8_t ST;                    // Звуковой таймер
    uint16_t stack[STACK_SIZE];    // Стек
    uint8_t keys[KEY_COUNT];       // Состояние клавиш
    uint8_t display[SCREEN_HEIGHT][SCREEN_WIDTH]; // Экран (32x64)
    uint8_t draw_flag;             // Флаг обновления экрана
} CHIP8;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

// Шрифты CHIP-8 (0x0-0xF)
const uint8_t fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

void initialize(CHIP8 *chip) {
    memset(chip, 0, sizeof(CHIP8));
    
    // Загрузка шрифтов в память
    memcpy(&chip->memory[0x50], fontset, sizeof(fontset));
    
    chip->PC = 0x200; // Стартовый адрес программ
    chip->draw_flag = 1;
}

int load_rom(CHIP8 *chip, const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("Error: Cannot open file %s\n", filename);
        return 0;
    }
    
    // Получаем размер файла
    fseek(file, 0, SEEK_END); // ишет конец фала
    long size = ftell(file);
    rewind(file);   // возврашает в начало
    
    // Проверяем, помещается ли ROM в память
    if (size > MEMORY_SIZE - 0x200) {
        printf("Error: ROM too large\n");
        fclose(file);
        return 0;
    }
    
    // Читаем ROM в память начиная с 0x200
    if (fread(&chip->memory[0x200], 1, size, file) != size) {
        printf("Error: Reading ROM failed\n");
        fclose(file);
        return 0;
    }
    
    fclose(file);
    printf("Loaded ROM: %s (%ld bytes)\n", filename, size);
    return 1;
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

void clear_screen(CHIP8 *chip) {
    memset(chip->display, 0, sizeof(chip->display));
    chip->draw_flag = 1;
}

uint16_t fetch_opcode(CHIP8 *chip) {
    return (chip->memory[chip->PC] << 8) | chip->memory[chip->PC + 1];
}

void execute(CHIP8 *chip, uint16_t opcode) {
    uint8_t x = (opcode >> 8) & 0x0F;
    uint8_t y = (opcode >> 4) & 0x0F;
    uint8_t n = opcode & 0x0F;
    uint8_t kk = opcode & 0xFF;
    uint16_t nnn = opcode & 0x0FFF;

    switch (opcode & 0xF000) {
        case 0x0000:
            switch (opcode) {
                case 0x00E0: // Очистка экрана
                    clear_screen(chip);
                    chip->PC += 2;
                    break;
                case 0x00EE: // Возврат из подпрограммы
                    chip->SP--;
                    chip->PC = chip->stack[chip->SP];
                    chip->PC += 2;
                    break;
                default:
                    chip->PC += 2;
                    break;
            }
            break;

        case 0x1000: // Прыжок на адрес
            chip->PC = nnn;
            break;

        case 0x2000: // Вызов подпрограммы
            chip->stack[chip->SP] = chip->PC;
            chip->SP++;
            chip->PC = nnn;
            break;

        case 0x3000: // Пропуск если Vx == kk
            chip->PC += (chip->V[x] == kk) ? 4 : 2;
            break;

        case 0x4000: // Пропуск если Vx != kk
            chip->PC += (chip->V[x] != kk) ? 4 : 2;
            break;

        case 0x5000: // Пропуск если Vx == Vy
            chip->PC += (chip->V[x] == chip->V[y]) ? 4 : 2;
            break;

        case 0x6000: // Установка Vx = kk
            chip->V[x] = kk;
            chip->PC += 2;
            break;

        case 0x7000: // Добавление к Vx значения kk
            chip->V[x] += kk;
            chip->PC += 2;
            break;

        case 0x8000:
            switch (n) {
                case 0x0: // Vx = Vy
                    chip->V[x] = chip->V[y];
                    chip->PC += 2;
                    break;
                case 0x1: // Vx = Vx OR Vy
                    chip->V[x] |= chip->V[y];
                    chip->PC += 2;
                    break;
                case 0x2: // Vx = Vx AND Vy
                    chip->V[x] &= chip->V[y];
                    chip->PC += 2;
                    break;
                case 0x3: // Vx = Vx XOR Vy
                    chip->V[x] ^= chip->V[y];
                    chip->PC += 2;
                    break;
                case 0x4: // Vx = Vx + Vy, установка VF
                    {
                        uint16_t sum = chip->V[x] + chip->V[y];
                        chip->V[0xF] = (sum > 0xFF) ? 1 : 0;
                        chip->V[x] = sum & 0xFF;
                        chip->PC += 2;
                    }
                    break;
                case 0x5: // Vx = Vx - Vy, установка VF
                    chip->V[0xF] = (chip->V[x] > chip->V[y]) ? 1 : 0;
                    chip->V[x] -= chip->V[y];
                    chip->PC += 2;
                    break;
                case 0x6: // Vx = Vx SHR 1
                    chip->V[0xF] = chip->V[x] & 0x1;
                    chip->V[x] >>= 1;
                    chip->PC += 2;
                    break;
                case 0x7: // Vx = Vy - Vx
                    chip->V[0xF] = (chip->V[y] > chip->V[x]) ? 1 : 0;
                    chip->V[x] = chip->V[y] - chip->V[x];
                    chip->PC += 2;
                    break;
                case 0xE: // Vx = Vx SHL 1
                    chip->V[0xF] = (chip->V[x] & 0x80) >> 7;
                    chip->V[x] <<= 1;
                    chip->PC += 2;
                    break;
                default:
                    chip->PC += 2;
                    break;
            }
            break;

        case 0x9000: // Пропуск если Vx != Vy
            chip->PC += (chip->V[x] != chip->V[y]) ? 4 : 2;
            break;

        case 0xA000: // Установка I = nnn
            chip->I = nnn;
            chip->PC += 2;
            break;

        case 0xB000: // Прыжок на nnn + V0
            chip->PC = nnn + chip->V[0];
            break;

        case 0xC000: // Vx = случайное число AND kk
            chip->V[x] = (rand() % 256) & kk;
            chip->PC += 2;
            break;

        case 0xD000: // Отображение спрайта
            {
                uint8_t vx = chip->V[x];
                uint8_t vy = chip->V[y];
                uint8_t height = n;
                
                chip->V[0xF] = 0;
                
                for (int row = 0; row < height; row++) {
                    uint8_t sprite = chip->memory[chip->I + row];
                    
                    for (int col = 0; col < 8; col++) {
                        if (sprite & (0x80 >> col)) {
                            int pixel_x = (vx + col) % SCREEN_WIDTH;
                            int pixel_y = (vy + row) % SCREEN_HEIGHT;
                            
                            if (chip->display[pixel_y][pixel_x] == 1) {
                                chip->V[0xF] = 1;
                            }
                            
                            chip->display[pixel_y][pixel_x] ^= 1;
                        }
                    }
                }
                
                chip->draw_flag = 1;
                chip->PC += 2;
            }
            break;

        case 0xE000:
            switch (kk) {
                case 0x9E: // Пропуск если клавиша нажата
                    chip->PC += (chip->keys[chip->V[x]]) ? 4 : 2;
                    break;
                case 0xA1: // Пропуск если клавиша не нажата
                    chip->PC += (!chip->keys[chip->V[x]]) ? 4 : 2;
                    break;
                default:
                    chip->PC += 2;
                    break;
            }
            break;

        case 0xF000:
            switch (kk) {
                case 0x07: // Vx = DT
                    chip->V[x] = chip->DT;
                    chip->PC += 2;
                    break;
                case 0x0A: // Ожидание нажатия клавиши
                    {
                        int key_pressed = -1;
                        for (int i = 0; i < KEY_COUNT; i++) {
                            if (chip->keys[i]) {
                                key_pressed = i;
                                break;
                            }
                        }
                        
                        if (key_pressed != -1) {
                            chip->V[x] = key_pressed;
                            chip->PC += 2;
                        }
                        // Если клавиша не нажата, не увеличиваем PC
                    }
                    break;
                case 0x15: // DT = Vx
                    chip->DT = chip->V[x];
                    chip->PC += 2;
                    break;
                case 0x18: // ST = Vx
                    chip->ST = chip->V[x];
                    chip->PC += 2;
                    break;
                case 0x1E: // I += Vx
                    chip->I += chip->V[x];
                    chip->PC += 2;
                    break;
                case 0x29: // I = адрес спрайта для символа Vx
                    chip->I = 0x50 + (chip->V[x] * 5);
                    chip->PC += 2;
                    break;
                case 0x33: // Сохранение BCD представления Vx в памяти
                    chip->memory[chip->I] = chip->V[x] / 100;
                    chip->memory[chip->I + 1] = (chip->V[x] / 10) % 10;
                    chip->memory[chip->I + 2] = chip->V[x] % 10;
                    chip->PC += 2;
                    break;
                case 0x55: // Сохранение регистров V0-Vx в памяти
                    for (int i = 0; i <= x; i++) {
                        chip->memory[chip->I + i] = chip->V[i];
                    }
                    chip->PC += 2;
                    break;
                case 0x65: // Загрузка регистров V0-Vx из памяти
                    for (int i = 0; i <= x; i++) {
                        chip->V[i] = chip->memory[chip->I + i];
                    }
                    chip->PC += 2;
                    break;
                default:
                    chip->PC += 2;
                    break;
            }
            break;

        default:
            printf("Unknown opcode: 0x%04X\n", opcode);
            chip->PC += 2;
            break;
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
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            if (chip->display[y][x]) {
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

void update_timers(CHIP8 *chip) {
    if (chip->DT > 0) chip->DT--;
    if (chip->ST > 0) {
        chip->ST--;
        // Здесь можно добавить звук
    }
}

int main(int argc, char *argv[]) {
    if (!init_SDL()) return 1;
    
    CHIP8 chip;
    initialize(&chip);
    
    if (!load_rom(&chip,argv[1])) {
        printf("Trying to load test rom...\n");
        if (!load_rom(&chip, "test.ch8")) {
            SDL_Quit();
            return 1;
        }
    }
    
    srand(time(NULL));
    
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
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}