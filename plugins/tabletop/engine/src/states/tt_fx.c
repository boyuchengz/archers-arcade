#pragma bank 255
/* Tiny sprite particle system: sparkles, confetti, floating text. */
#include <gb/gb.h>
#include "states/tt_gfx.h"

typedef struct {
    int16_t x, y;      /* 12.4 fixed point pixels */
    int8_t vx, vy;     /* 1/16 px per frame */
    uint8_t life, tile, prop, kind;
} Particle;

static Particle fx[SPR_FX_COUNT];
uint8_t fx_n = SPR_FX_COUNT;   /* the tower lends the top slots to its enemies */

void fx_clear(void) BANKED {
    uint8_t i;
    for (i = 0; i < SPR_FX_COUNT; i++) {
        fx[i].life = 0;
        hide_sprite(SPR_FX + i);
    }
}

void fx_spawn(uint8_t kind, uint8_t x, uint8_t y, int8_t vx, int8_t vy, uint8_t tile, uint8_t prop, uint8_t life) BANKED {
    uint8_t i, best = 0, best_life = 255;
    for (i = 0; i < fx_n; i++) {
        if (!fx[i].life) { best = i; break; }
        if (fx[i].life < best_life) { best_life = fx[i].life; best = i; }
    }
    fx[best].x = (int16_t)x << 4;
    fx[best].y = (int16_t)y << 4;
    fx[best].vx = vx;
    fx[best].vy = vy;
    fx[best].tile = tile;
    fx[best].prop = prop;
    fx[best].kind = kind;
    fx[best].life = life;
}

static const int8_t burst_vx[8] = {0, 22, 32, 22, 0, -22, -32, -22};
static const int8_t burst_vy[8] = {-32, -22, 0, 22, 32, 22, 0, -22};

void fx_burst(uint8_t x, uint8_t y, uint8_t count) BANKED {
    uint8_t i, k = (uint8_t)tt_random_below(8);
    for (i = 0; i < count; i++, k += (count > 4 ? 1 : 2)) {
        fx_spawn(FX_SPARK, x, y, burst_vx[k & 7], burst_vy[k & 7], TT_S_SPARK0, OP_SPARK, 18 + (i & 3));
    }
}

void fx_confetti(uint8_t count) BANKED {
    while (count--) {
        uint8_t x = (uint8_t)tt_random_below(152);
        fx_spawn(FX_CONFETTI, x, (uint8_t)tt_random_below(24),
                 (int8_t)((int8_t)tt_random_below(17) - 8), (int8_t)(4 + tt_random_below(14)),
                 TT_S_CONFETTI0 + (uint8_t)tt_random_below(4),
                 ((count & 1) ? OP_CONF_A : OP_CONF_B) | (tt_random_below(2) ? S_FLIPX : 0),
                 90 + (uint8_t)tt_random_below(60));
    }
}

/* Floating text using font tiles as sprites (rises and vanishes). */
void fx_text(uint8_t x, uint8_t y, const char *s) BANKED {
    while (*s) {
        if (*s != ' ') fx_spawn(FX_FLOAT, x, y, 0, -10, (uint8_t)*s, OP_TEXT, 40);
        x += 7;
        s++;
    }
}

void fx_tick(void) BANKED {
    uint8_t i, sx, sy, t;
    Particle *p = fx;
    for (i = 0; i < fx_n; i++, p++) {
        if (!p->life) continue;
        if (!--p->life) {
            hide_sprite(SPR_FX + i);
            continue;
        }
        p->x += p->vx;
        p->y += p->vy;
        t = p->tile;
        switch (p->kind) {
        case FX_SPARK:
            /* slow down, cycle through the 4 sparkle frames */
            p->vx -= p->vx >> 3;
            p->vy -= p->vy >> 3;
            t = TT_S_SPARK0 + (p->life > 14 ? 2 : p->life > 9 ? 3 : p->life > 4 ? 1 : 0);
            break;
        case FX_CONFETTI:
            if (p->vy < 20) p->vy++;
            if ((p->life & 7) == 0) p->vx = -p->vx;
            if (p->y > (152 << 4)) p->life = 1;
            break;
        case FX_FLOAT:
            if (p->life < 12 && (p->life & 1)) {
                hide_sprite(SPR_FX + i);
                continue;
            }
            break;
        default:
            break;
        }
        sx = (uint8_t)(p->x >> 4);
        sy = (uint8_t)(p->y >> 4);
        set_sprite_tile(SPR_FX + i, t);
        set_sprite_prop(SPR_FX + i, p->prop);
        move_sprite(SPR_FX + i, sx + 8, sy + 16);
    }
}
