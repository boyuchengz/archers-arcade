#include <gb/gb.h>
/* Interrupt handlers (non-banked): screen shake and the header/body scroll split. */
extern uint8_t tt_scx, tt_scy, tt_split_scx, tt_split_on, tt_shake;

void tt_vblank(void) NONBANKED {
    uint8_t s = tt_shake;
    if (s) {
        s = (s >> 2) + 1;
        SCX_REG = (sys_time & 2) ? tt_scx + s : tt_scx - s;
        SCY_REG = (sys_time & 4) ? tt_scy + 1 : tt_scy;
    } else {
        SCX_REG = tt_scx;
        SCY_REG = tt_scy;
    }
}

void tt_lcd(void) NONBANKED {
    if (tt_split_on) {
        while (STAT_REG & STATF_BUSY);
        SCX_REG = tt_split_scx;
    }
}
