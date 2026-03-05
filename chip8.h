#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>
#include <string.h>
#include <time.h>

#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#define SCREEN_WORDS (SCREEN_WIDTH * SCREEN_HEIGHT / 64) // 32
#define MEMORY_SIZE 4096
#define STACK_SIZE 16
#define REGISTERS_COUNT 16
#define KEY_COUNT 16

typedef struct CHIP8 {
    uint64_t display[SCREEN_WORDS]; // Экран (32x64) в битах
    uint16_t stack[STACK_SIZE];     // Стек
    uint16_t PC;                     // Счётчик команд
    uint16_t I;                       // Регистр адреса
    uint8_t memory[MEMORY_SIZE];      // Память
    uint8_t V[REGISTERS_COUNT];       // Регистры V0-VF
    uint8_t SP;                        // Указатель стека
    uint8_t DT;                         // Таймер задержки
    volatile uint8_t ST;                 // Звуковой таймер
    uint8_t keys[KEY_COUNT];             // Состояние клавиш
    uint8_t draw_flag;                    // Флаг обновления экрана
} CHIP8;

void initialize(CHIP8 *chip);
int load_rom(CHIP8 *chip, const char *filename);
uint16_t fetch_opcode(CHIP8 *chip);
void clear_screen(CHIP8 *chip);
void execute(CHIP8 *chip, uint16_t opcode);
void update_timers(CHIP8 *chip);

#endif