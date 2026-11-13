/* Non-banked trampolines: they capture the caller's ROM bank so string literals
   stored next to the calling code can be read from the graphics bank. */
#include <gb/gb.h>
#include "states/tt_gfx.h"

void tt_text(uint8_t x, uint8_t y, const char *s, uint8_t attr) NONBANKED {
    tt_text_far(x, y, s, attr, CURRENT_BANK);
}

void tt_ctext(uint8_t y, const char *s, uint8_t attr) NONBANKED {
    uint8_t bank = CURRENT_BANK;
    uint8_t n = tt_strlen_far(s, bank);
    tt_text_far((20 - n) >> 1, y, s, attr, bank);
}
