#pragma bank 255
/* Archer's Tabletop: GB Studio scene type whose init never returns. It owns the
   screen, input, audio and the save, and dispatches to the game modules. */
#include <gb/gb.h>
#include <gb/cgb.h>
#include "interrupts.h"
#include "states/tt_gfx.h"

extern void tt_vblank(void) NONBANKED;
extern void tt_lcd(void) NONBANKED;
extern void tt_menu_tick(void) BANKED;

uint8_t tt_state, tt_old_state, tt_pressed, tt_held;
uint16_t tt_clock;
uint8_t tt_scratch[512];
uint8_t tt_game_cache[512];
uint8_t tt_cpu_fast_flag, tt_model;
static uint8_t previous;

static void tick(void) {
    switch (tt_state) {
    case TT_PUZZLE:
        puzzle_tick();
        break;
    case TT_GOMOKU:
    case TT_RESULT:
        gomoku_tick();
        break;
    default:
        tt_menu_tick();
        break;
    }
}

void tabletop_update(void) BANKED {}

void tabletop_init(void) BANKED {
    uint8_t j;
    disable_interrupts();
    remove_VBL(VBL_isr);
    remove_LCD_ISRs();
    DISPLAY_OFF;
    LCDC_REG = LCDCF_BG8000 | LCDCF_BG9800 | LCDCF_WIN9C00 | LCDCF_OBJON | LCDCF_BGON;
    SCX_REG = SCY_REG = 0;
    HIDE_WIN;
    SPRITES_8x8;
    _shadow_OAM_base = (uint8_t)((uint16_t)&shadow_OAM >> 8);
    tt_save_init();
    tt_gfx_init();
    tt_audio_init();
    STAT_REG = STATF_LYC;
    LYC_REG = 23;
    add_VBL(tt_vblank);
    add_LCD(tt_lcd);
    set_interrupts(VBL_IFLAG | LCD_IFLAG);
    enable_interrupts();
    if (_cpu == CGB_TYPE) cpu_fast();
    tt_cpu_fast_flag = KEY1_REG & 0x80;
    tt_model = _cpu;
    DISPLAY_ON;
    SHOW_SPRITES;
    tt_title();
    while (1) {
        tt_frame();
        j = joypad();
        tt_pressed = j & ~previous;
        previous = j;
        tt_held = j;
        if (tt_pressed) tt_entropy(tt_pressed, DIV_REG, sys_time);
        tick();
    }
}
