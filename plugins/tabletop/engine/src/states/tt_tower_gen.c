#pragma bank 255
/* ARROW TOWER room builder.
   Every floor is either a hand-made template (tools/tower_rooms.py, mirrored at
   random, with difficulty-dependent hazards, enemies and coins) or a brand-new
   layout built here step by step from the entry to the exit, so the climb is
   endless and always climbable. The room is written into the 15 ring rows
   above the current room. The work is spread over frames so play never
   stutters: build (one attempt per frame), resolve in two halves, decorate,
   then one row of tiles per frame. Most of it runs during the camera pan. */
#include <string.h>
#ifndef TT_HOST
#include <gb/gb.h>
#include <gb/cgb.h>
#endif
#include "states/tt_tower.h"
#include "states/tt_tower_rooms.h"

#define TB 8          /* attribute: VRAM bank 1 */
#define FX 32
#define FY 64
#define P_STONE 1
#define P_WALL 2
#define P_WOOD 3
#define P_METAL 4
#define P_COIN 5
#define P_DECOR 6
#define P_MISC 7
#define PROT 0x80

/* template rows through a lookup table (row * 18 would cost a multiply per access) */
static const uint8_t trow_off[15] = {0, 18, 36, 54, 72, 90, 108, 126, 144, 162, 180, 198, 216, 234, 252};
#define TR(y) ((uint8_t *)TW->tmpl + trow_off[y])

static uint8_t ring0, gen_zone, gen_dl, gen_hint;
uint8_t tw_gen_proc;   /* last room was built procedurally (read by the desktop test) */
static const uint8_t lane_x[3] = {1, 7, 13};

static uint8_t gen_phase, gen_entry, gen_tries, gen_proc_try;
static uint16_t gen_floor, gs = 0xACE1;

/* fast 16-bit xorshift for level building, seeded from the save RNG per room */
static uint8_t rnd(uint8_t n) {
    gs ^= gs << 7;
    gs ^= gs >> 9;
    gs ^= gs << 8;
    return (uint8_t)(((uint16_t)(uint8_t)(gs >> 8) * n) >> 8);
}

uint8_t tw_zone_of(uint16_t floor) BANKED {
    return (uint8_t)(((floor - 1) / 10) % 6);
}

uint8_t tw_difficulty(uint16_t floor) BANKED {
    uint16_t d = (floor - 1) / 5;
    return d > 15 ? 15 : (uint8_t)d;
}

/* ------------------------------------------------------------------ templates */
static void decode(uint8_t id, uint8_t mirror) {
    const uint8_t *p = tw_room_data + tw_room_offsets[id];
    uint8_t *o = &TR(0)[0];
    uint16_t n = 0;
    uint8_t v, k;
    while (n < 270) {
        v = *p++;
        if (v & 0x80) {
            for (k = v & 0x7F; k; k--) o[n++] = 0;
        } else {
            o[n++] = v;
        }
    }
    if (mirror) {
        uint8_t y, x, t;
        for (y = 0; y < 15; y++) {
            for (x = 0; x < 9; x++) {
                t = TR(y)[x];
                TR(y)[x] = TR(y)[17 - x];
                TR(y)[17 - x] = t;
            }
        }
    }
}

static uint8_t pick_template(uint16_t floor, uint8_t entry, uint8_t *mirror) {
    uint8_t maxt, mint, i, n = 0, j, ok, m;
    uint8_t cand[TW_ROOM_COUNT];
    uint16_t z = (floor - 1) / 10;
    maxt = z >= 4 ? 5 : (uint8_t)(1 + z);
    mint = maxt > 2 ? maxt - 2 : 1;
    if (floor > 60) mint = 1;
    for (i = 2; i < TW_ROOM_COUNT; i++) {
        const uint8_t *inf = tw_room_info + i * 4;
        if (inf[0] < mint || inf[0] > maxt) continue;
        if (!(inf[1] & (1 << entry)) && !(inf[1] & (1 << (2 - entry)))) continue;
        ok = 1;
        for (j = 0; j < 4; j++) if (tw_recent[j] == i) ok = 0;
        if (ok) cand[n++] = i;
    }
    if (!n) {
        for (i = 2; i < TW_ROOM_COUNT; i++) {
            const uint8_t *inf = tw_room_info + i * 4;
            if (inf[0] <= maxt && ((inf[1] & (1 << entry)) || (inf[1] & (1 << (2 - entry))))) cand[n++] = i;
        }
    }
    i = cand[rnd(n)];
    m = tw_room_info[i * 4 + 1];
    if ((m & (1 << entry)) && (m & (1 << (2 - entry)))) *mirror = rnd(2);
    else *mirror = (m & (1 << entry)) ? 0 : 1;
    tw_recent[3] = tw_recent[2];
    tw_recent[2] = tw_recent[1];
    tw_recent[1] = tw_recent[0];
    tw_recent[0] = i;
    return i;
}

/* ------------------------------------------------------------------ procedural rooms */
static void protect(uint8_t x0, uint8_t x1, uint8_t y0, uint8_t y1) {
    uint8_t x, y;
    if (x0 > x1) { x = x0; x0 = x1; x1 = x; }
    for (y = y0; y <= y1 && y < 15; y++)
        for (x = x0; x <= x1 && x < 18; x++) TR(y)[x] |= PROT;
}

static uint8_t is_free(uint8_t x0, uint8_t x1, uint8_t y0, uint8_t y1) {
    uint8_t x, y;
    for (y = y0; y <= y1; y++)
        for (x = x0; x <= x1; x++)
            if (TR(y)[x]) return 0;
    return 1;
}

/* a column of [cl,cr] outside [nl,nr] and within jumping reach of it; returns 255 if none */
static uint8_t jump_col(uint8_t cl, uint8_t cr, uint8_t nl, uint8_t nr) {
    if (cl < nl) {
        uint8_t c = cr < nl ? cr : nl - 1;
        if (nl - c <= 3) return c;
    }
    if (cr > nr) {
        uint8_t c = cl > nr ? cl : nr + 1;
        if (c - nr <= 3) return c;
    }
    return 255;
}

static uint8_t path_material(void) {
    uint8_t r = rnd(10);
    if (gen_zone == 3 && r < 5) return RC_ICE;
    if (gen_dl >= 3 && r == 0) return RC_CRUMBLE;
    if (r < 3) return RC_PLANK;
    if (r == 3) return RC_WOOD;
    return RC_STONE;
}

static uint8_t span_ok(uint8_t row, uint8_t l, uint8_t r) {
    for (; l <= r; l++)
        if (TR(row)[l]) return 0;
    return 1;
}

/* Build a fresh layout from the floor to the ceiling. Each ledge is placed
   within jumping reach of the one below, and every jump arc and standing spot
   is marked protected so nothing placed later can block it. Returns 0 if a
   layout could not be finished (the caller then uses a template). */
static uint8_t proc_build(uint8_t entry) {
    uint8_t cur = 15, cl = 0, cr = 17, s, nrow, len, nl = 0, nr = 0, c, tries, lane, x, y, k, ok, d, first;
    uint8_t pl[6], pr[6], prow[6], np = 0, dir = rnd(2);
    memset(TW->tmpl, 0, 270);
    memset(TR(0), RC_STONE, 18);
    /* the entry lane stays clear */
    protect(lane_x[entry] - 1, lane_x[entry] + 4, 10, 14);
    while (1) {
        /* climb 2 or 3 rows at a time and never leave a 1-row step under the exit;
           the first step off the floor is always 3 rows */
        k = cur - 3;
        if (k <= 3) s = k;
        else {
            s = (cur == 15 || rnd(3)) ? 3 : 2;
            if (k - s == 1) s = s == 3 ? 2 : 3;
        }
        nrow = cur - s;
        ok = 0;
        if (nrow == 3) {
            /* final ledge under an exit lane, stretched toward the ledge below */
            first = rnd(3);
            for (k = 0; k < 3 && !ok; k++) {
                lane = (first + k) % 3;
                nl = lane_x[lane] - 1;
                nr = lane_x[lane] + 4 > 17 ? 17 : lane_x[lane] + 4;
                for (tries = 0; tries < 10; tries++) {
                    if (span_ok(nrow, nl, nr) && jump_col(cl, cr, nl, nr) != 255 && (s == 3 || cr < nl || cl > nr)) {
                        ok = 1;
                        tw_next_exit = lane;
                        break;
                    }
                    if (cr < nl && nl > 0) nl--;
                    else if (cl > nr && nr < 17) nr++;
                    else break;
                }
            }
        } else {
            for (tries = 0; tries < 24 && !ok; tries++) {
                len = 3 + rnd(4);
                d = tries < 12 ? dir : !dir;
                if (s == 2) {
                    if (d) { nl = cr + 1 + rnd(3); nr = nl + len - 1; }
                    else {
                        if (cl < 2) continue;
                        nr = cl - 1 - (cl >= 4 ? rnd(3) : 0);
                        nl = nr + 1 >= len ? nr + 1 - len : 0;
                    }
                } else {
                    c = cl + rnd(cr - cl + 1);
                    if (d) { nl = c + 1 + rnd(3); nr = nl + len - 1; }
                    else {
                        if (c < 1) continue;
                        nr = c - 1 - (c >= 3 ? rnd(3) : 0);
                        nl = nr + 1 >= len ? nr + 1 - len : 0;
                    }
                }
                if (nr > 17 || nl > nr) continue;
                if (!span_ok(nrow, nl, nr)) continue;
                if (jump_col(cl, cr, nl, nr) == 255) continue;
                if (s == 2 && !(cr < nl || cl > nr)) continue;
                ok = 1;
            }
            if (ok) dir = nl > cl ? 0 : 1;
        }
        if (!ok) return 0;
        c = jump_col(cl, cr, nl, nr);
        /* the jump arc and the standing room above the new ledge stay clear */
        protect(c, nl > c ? nl : nr, nrow - 2, cur - 1);
        protect(nl, nr, nrow - 2, nrow - 1);
        if (cur == 15) protect(lane_x[entry] + 1, c, 13, 14);
        k = nrow == 3 ? RC_STONE : path_material();
        for (x = nl; x <= nr; x++) TR(nrow)[x] = k | PROT;
        pl[np] = nl; pr[np] = nr; prow[np] = nrow; np++;
        if (nrow == 3) break;
        cl = nl; cr = nr; cur = nrow;
    }
#ifdef TT_HOST
    {
        extern int tw_debug;
        if (tw_debug) {
            for (k = 0; k < np; k++) fprintf(stderr, "[row %u %u-%u] ", prow[k], pl[k], pr[k]);
            fprintf(stderr, "\n");
        }
    }
#endif
    /* ceiling exit */
    for (x = 0; x < 4; x++) TR(0)[lane_x[tw_next_exit] + x] = RC_PLANK;
    /* coins along the climb */
    for (k = 0; k < np; k++) {
        if (rnd(2)) {
            x = pl[k] + rnd(pr[k] - pl[k] + 1);
            y = prow[k] - 1;
            if ((TR(y)[x] & 0x7F) == 0) TR(y)[x] = RC_COIN | PROT;
        }
    }
    /* side ledges with treasure or spikes */
    for (k = 0; k < 6; k++) {
        uint8_t w = 2 + rnd(3), rr = 4 + rnd(9);
        x = rnd(18 - w);
        if (!is_free(x, x + w - 1, rr - 2, rr)) continue;
        if (rr + 1 < 15 && !is_free(x, x + w - 1, rr + 1, rr + 1)) continue;
        for (y = 0; y < w; y++) TR(rr)[x + y] = gen_zone == 3 && rnd(2) ? RC_ICE : RC_STONE;
        if (rnd(3) == 0 && gen_dl >= 2) {
            for (y = 0; y < w; y++) TR(rr - 1)[x + y] = RC_SPIKE;
        } else {
            TR(rr - 1)[x + rnd(w)] = rnd(5) == 0 ? RC_GEM : RC_COIN;
            if (w > 2) TR(rr - 1)[x + rnd(w)] = RC_COIN;
        }
    }
    /* floor spikes away from the path */
    if (gen_dl >= 1) {
        for (k = 0; k < 2; k++) {
            uint8_t w = 2 + rnd(3);
            x = rnd(18 - w);
            if (is_free(x, x + w - 1, 13, 14)) for (y = 0; y < w; y++) TR(14)[x + y] = RC_SPIKE;
        }
    }
    /* enemies */
    for (k = 1; k < np; k++) {
        if (prow[k] == 3) continue;
        if (pr[k] - pl[k] >= 3 && gen_dl >= 1 && rnd(3) == 0 && TR(prow[k])[pl[k]] != (RC_PLANK | PROT)) {
            TR(prow[k] - 1)[pl[k] + 1] = RC_SLIME;
        } else if (pr[k] - pl[k] >= 4 && gen_dl >= 6 && rnd(3) == 0) {
            for (x = pl[k]; x <= pr[k]; x++) TR(prow[k] - 1)[x] = RC_TRACK_H;
            TR(prow[k] - 1)[pl[k]] = RC_SAW_H;
        }
    }
    if (gen_dl >= 2 && rnd(2)) {
        y = 4 + rnd(7);
        x = 2 + rnd(14);
        if ((TR(y)[x] & 0x7F) == 0) TR(y)[x] = RC_BAT;
    }
    if (gen_dl >= 8 && rnd(3) == 0) {
        y = 4 + rnd(5);
        if (!TR(y)[0] && (TR(y)[1] & 0x7F) == 0) TR(y)[0] = RC_CANNON;
        else if (!TR(y)[17] && (TR(y)[16] & 0x7F) == 0) TR(y)[17] = RC_CANNON;
    }
    if (gen_dl >= 10 && rnd(3) == 0) {
        if ((TR(5)[2] & 0x7F) == 0) TR(5)[2] = RC_GHOST;
    }
    for (y = 0; y < 15; y++)
        for (x = 0; x < 18; x++) TR(y)[x] &= 0x7F;
    return 1;
}

/* ------------------------------------------------------------------ resolve to cells + spawns */
static void spawn(uint8_t type, uint8_t x, uint8_t y, uint8_t a, uint8_t b) {
    Spawn *s;
    if (tw_npend >= MAX_PEND) return;
    s = &TW->pend[tw_npend++];
    s->type = type;
    s->x = x;
    s->y = y;
    s->a = a;
    s->b = b;
}

static void resolve(uint8_t y0, uint8_t y1) {
    uint8_t x, y, c, k, *row, cx, cy, i;
    for (y = y0; y < y1; y++) {
        row = TW->cells[(uint8_t)(ring0 + y) & 31];
        row[0] = row[19] = y ? K_STONE : K_SLAB;
        for (x = 0; x < 18; x++) {
            c = TR(y)[x];
            cx = (x + 1) * 8;
            cy = y * 8;
            k = K_EMPTY;
            switch (c) {
            case RC_STONE: k = y ? K_STONE : K_SLAB; break;
            case RC_WOOD: k = K_WOOD; gen_hint = 1; break;
            case RC_PLANK: k = K_PLANK; break;
            case RC_SPIKE: k = K_SPIKE; break;
            case RC_SPIKE_D: k = K_SPIKE_D; break;
            case RC_CRUMBLE: k = K_CRUMBLE; break;
            case RC_SPRING: k = K_SPRING; break;
            case RC_COIN: k = K_COIN; break;
            case RC_COIN_MAYBE:
                if (rnd(2)) { k = K_COIN; TR(y)[x] = RC_COIN; } else TR(y)[x] = 0;
                break;
            case RC_GEM: k = K_GEM; break;
            case RC_HEART:
                if (tw_hearts < 3 && rnd(2)) { spawn(E_HEART, cx, cy, 0, 0); TR(y)[x] = 0; }
                else { k = K_COIN; TR(y)[x] = RC_COIN; }
                break;
            case RC_ICE:
                if (gen_zone == 3) k = K_ICE;
                else { k = K_STONE; TR(y)[x] = RC_STONE; }
                break;
            case RC_CANNON:
                k = K_CANNON;
                spawn(E_CANNON, cx, cy, x < 9 ? 0 : 1, rnd(60));
                break;
            case RC_ICICLE: spawn(E_ICICLE, cx, cy, 0, 0); TR(y)[x] = 0; break;
            case RC_HELPER:
                if (gen_dl < 3) { k = K_STONE; TR(y)[x] = RC_STONE; } else TR(y)[x] = 0;
                break;
            case RC_SPIKE_D2:
            case RC_SPIKE_D4:
                if (gen_dl >= (c == RC_SPIKE_D2 ? 2 : 4)) { k = K_SPIKE; TR(y)[x] = RC_SPIKE; } else TR(y)[x] = 0;
                break;
            case RC_SLIME_D3:
                if (gen_dl < 3) { TR(y)[x] = 0; break; }
                /* fall through */
            case RC_SLIME: spawn(E_SLIME, cx - 4, cy + 8, 0, 0); TR(y)[x] = 0; break;
            case RC_BAT_D2:
                if (gen_dl < 2) { TR(y)[x] = 0; break; }
                /* fall through */
            case RC_BAT:
                if (gen_zone == 5 && gen_dl >= 8 && rnd(3) == 0) spawn(E_GHOST, cx - 4, cy, 0, 0);
                else spawn(E_BAT, cx - 4, cy, 0, 0);
                TR(y)[x] = 0;
                break;
            case RC_GHOST:
                if (gen_dl >= 8) spawn(E_GHOST, cx - 4, cy, 0, 0);
                TR(y)[x] = 0;
                break;
            case RC_SAW_H: {
                uint8_t a = x, b = x;
                while (a > 0 && (TR(y)[a - 1] == RC_TRACK_H || TR(y)[a - 1] == RC_SAW_H)) a--;
                while (b < 17 && (TR(y)[b + 1] == RC_TRACK_H || TR(y)[b + 1] == RC_SAW_H)) b++;
                spawn(E_SAW, cx + 4, cy + 4, (a + 1) * 8 + 4, (b + 1) * 8 + 4);
                break;
            }
            case RC_SAW_V: {
                uint8_t a = y, b = y;
                while (a > 1 && (TR(a - 1)[x] == RC_TRACK_V || TR(a - 1)[x] == RC_SAW_V)) a--;
                while (b < 14 && (TR(b + 1)[x] == RC_TRACK_V || TR(b + 1)[x] == RC_SAW_V)) b++;
                spawn(E_SAWV, cx + 4, cy + 4, a * 8 + 4, b * 8 + 4);
                break;
            }
            case RC_PLATFORM: {
                uint8_t b = x;
                while (b < 17 && (TR(y)[b + 1] == RC_TRACK_P || TR(y)[b + 1] == RC_PLATFORM)) b++;
                for (i = x; i > 0 && TR(y)[i - 1] == RC_TRACK_P; i--);
                spawn(E_PLAT, cx, cy, (i + 1) * 8, (b + 1) * 8 - 16);
                break;
            }
            case RC_FIRE: k = K_FIRE; if (y) TR(y - 1)[x] = D_CAMP; break;
            case RC_FIRE_R: k = K_FIRE; if (y) TR(y - 1)[x] = D_CAMP + 1; break;
            default: break;
            }
            row[x + 1] = k;
        }
    }
}

/* ------------------------------------------------------------------ decoration */
static uint8_t empty_rect(uint8_t x0, uint8_t y0, uint8_t w, uint8_t h) {
    uint8_t x, y;
    if (x0 + w > 18 || y0 + h > 15) return 0;
    for (y = y0; y < y0 + h; y++)
        for (x = x0; x < x0 + w; x++)
            if (TR(y)[x]) return 0;
    return 1;
}

static void decorate(void) {
    uint8_t i, x, y, n;
    /* window */
    for (i = 0; i < 6; i++) {
        x = 1 + rnd(15);
        y = 2 + rnd(8);
        if (empty_rect(x - 1, y - 1, 4, 4)) {
            TR(y)[x] = D_WINDOW;
            TR(y)[x + 1] = D_WINDOW + 1;
            TR(y + 1)[x] = D_WINDOW + 2;
            TR(y + 1)[x + 1] = D_WINDOW + 3;
            break;
        }
    }
    /* banner hanging from the ceiling */
    if (rnd(3)) {
        for (i = 0; i < 6; i++) {
            x = rnd(17);
            if (empty_rect(x, 1, 2, 4)) {
                for (n = 0; n < 6; n++) TR(1 + n / 2)[x + (n & 1)] = D_BANNER + n;
                break;
            }
        }
    }
    /* torches */
    for (n = 0, i = 0; i < 10 && n < 2; i++) {
        x = rnd(18);
        y = 3 + rnd(9);
        if (empty_rect(x, y - 1, 1, 3) && (x == 0 || !TR(y)[x - 1]) && (x == 17 || !TR(y)[x + 1])) {
            TR(y)[x] = D_FLAME;
            TR(y + 1)[x] = D_TORCH;
            n++;
        }
    }
    /* a chain */
    if (rnd(2)) {
        x = rnd(18);
        n = 2 + rnd(3);
        if (empty_rect(x, 1, 1, n + 1))
            for (y = 1; y <= n; y++) TR(y)[x] = D_CHAIN;
    }
}

void tw_gen_begin(uint16_t floor, uint8_t entry, uint8_t r0) BANKED {
    ring0 = r0;
    gen_floor = floor;
    gen_entry = entry;
    gen_zone = tw_zone_of(floor);
    gen_dl = tw_difficulty(floor);
    gen_hint = 0;
    tw_npend = 0;
    tw_next_flags = 0;
    tw_gen_proc = 0;
    tw_gen_row = 15;
    gs = (uint16_t)tt_random() | 1;
    gen_tries = 0;
    gen_proc_try = (floor >= 6 && floor % 10) ? rnd(100) < (floor >= 41 ? 60 : floor >= 21 ? 50 : 35) : 0;
    gen_phase = 1;
}

void tw_gen_work(void) BANKED {
    uint8_t id, mirror = 0;
    switch (gen_phase) {
    case 1:
        if (gen_floor == 1) {
            decode(TW_ROOM_ENTRANCE, 0);
            tw_next_exit = tw_room_info[TW_ROOM_ENTRANCE * 4 + 2];
        } else if (gen_floor % 10 == 0) {
            decode(TW_ROOM_REST, 0);
            tw_next_exit = tw_room_info[TW_ROOM_REST * 4 + 2];
            tw_next_flags |= 2;
        } else if (gen_proc_try && gen_tries < 4) {
            gen_tries++;
            if (proc_build(gen_entry)) {
                tw_gen_proc = 1;
                gen_phase = 2;
            }
            return;
        } else {
            id = pick_template(gen_floor, gen_entry, &mirror);
            decode(id, mirror);
            tw_next_exit = tw_room_info[id * 4 + 2];
            if (mirror) tw_next_exit = 2 - tw_next_exit;
        }
        gen_phase = 2;
        return;
    case 2:
        resolve(0, 8);
        gen_phase = 3;
        return;
    case 3:
        resolve(8, 15);
        gen_phase = 4;
        return;
    case 4:
        decorate();
        if (gen_hint) tw_next_flags |= 1;
        tw_gen_row = 0;
        gen_phase = 0;
        return;
    }
}

void tw_gen_room(uint16_t floor, uint8_t entry, uint8_t r0) BANKED {
    tw_gen_begin(floor, entry, r0);
    while (gen_phase) tw_gen_work();
}

/* ------------------------------------------------------------------ tiles */
static uint8_t tile_out, attr_out;

static uint8_t *ru, *rc, *rd;   /* cell rows above, at and below the tile being drawn */

static void set_rows(uint8_t ring) {
    ru = TW->cells[(uint8_t)(ring - 1) & 31];
    rc = TW->cells[ring & 31];
    rd = TW->cells[(uint8_t)(ring + 1) & 31];
}

static uint8_t nb(int8_t x, const uint8_t *r) {
    return (uint8_t)x > 19 ? K_STONE : r[(uint8_t)x];
}

static uint8_t stone_family(uint8_t k) {
    return k == K_STONE || k == K_SLAB;
}

static void wall_tile(uint8_t x, uint8_t ring) {
    uint8_t v = (x & 1) | ((ring & 1) << 1);
    tile_out = TW_T_WALL0 + v;
    if (v == 0 && (uint8_t)(x * 7 + ring * 13) % 23 == 0) tile_out = TW_T_WALL_CRACK;
    else if (v == 3 && (uint8_t)(x * 5 + ring * 3) % 19 == 0) tile_out = TW_T_WALL_SPECK;
    attr_out = P_WALL | TB;
}

static void kind_tile(uint8_t x, uint8_t ring, uint8_t k, uint8_t slab_row) {
    uint8_t e = 0, n;
    switch (k) {
    case K_SLAB:
        tile_out = TW_T_SLAB0 + (x & 1);
        attr_out = P_STONE | TB;
        return;
    case K_STONE:
    case K_WOOD:
        if (slab_row) {
            tile_out = TW_T_SLAB0 + (x & 1);
            attr_out = P_STONE | TB;
            return;
        }
        if (k == K_STONE) {
            if (!stone_family(nb(x, ru))) e |= 1;
            if (!stone_family(nb(x, rd))) e |= 2;
            if (!stone_family(nb(x - 1, rc))) e |= 4;
            if (!stone_family(nb(x + 1, rc))) e |= 8;
            tile_out = TW_T_STONE + e;
            attr_out = P_STONE | TB;
        } else {
            if (nb(x, ru) != K_WOOD) e |= 1;
            if (nb(x, rd) != K_WOOD) e |= 2;
            if (nb(x - 1, rc) != K_WOOD) e |= 4;
            if (nb(x + 1, rc) != K_WOOD) e |= 8;
            tile_out = TW_T_WOOD + e;
            attr_out = P_WOOD | TB;
        }
        return;
    case K_ICE:
        tile_out = K_SOLID(nb(x, ru)) ? TW_T_ICE : TW_T_ICE_TOP;
        attr_out = P_MISC | TB;
        return;
    case K_CRUMBLE: tile_out = TW_T_CRUMBLE0; attr_out = P_STONE | TB; return;
    case K_CANNON: tile_out = TW_T_CANNON; attr_out = P_METAL | TB | (x >= 10 ? FX : 0); return;
    case K_PLANK:
        n = (nb(x - 1, rc) == K_PLANK ? 1 : 0) | (nb(x + 1, rc) == K_PLANK ? 2 : 0);
        tile_out = n == 2 ? TW_T_PLANK_L : n == 1 ? TW_T_PLANK_R : TW_T_PLANK_M;
        attr_out = P_WOOD | TB;
        return;
    case K_SPRING: tile_out = TW_T_SPRING; attr_out = P_MISC | TB; return;
    case K_SPIKE: tile_out = TW_T_SPIKE; attr_out = P_METAL | TB; return;
    case K_SPIKE_D: tile_out = TW_T_SPIKE; attr_out = P_METAL | TB | FY; return;
    case K_COIN: tile_out = TW_T_COIN; attr_out = P_COIN | TB; return;
    case K_GEM: tile_out = TW_T_GEM; attr_out = P_MISC | TB; return;
    case K_FIRE: tile_out = TW_T_CAMP + 2 + (nb(x - 1, rc) == K_FIRE ? 1 : 0); attr_out = P_DECOR | TB; return;
    default: wall_tile(x, ring); return;
    }
}

static void deco_tile(uint8_t c, uint8_t x, uint8_t ring, uint8_t tx, uint8_t ty) {
    if (c >= D_WINDOW && c < D_WINDOW + 4) { tile_out = TW_T_WINDOW + (c - D_WINDOW); attr_out = P_WALL | TB; }
    else if (c >= D_BANNER && c < D_BANNER + 6) { tile_out = TW_T_BANNER + (c - D_BANNER); attr_out = P_DECOR | TB; }
    else if (c == D_TORCH) { tile_out = TW_T_TORCH; attr_out = P_DECOR | TB; }
    else if (c == D_FLAME) { tile_out = TW_T_FLAME; attr_out = P_DECOR | TB; }
    else if (c == D_CHAIN) { tile_out = TW_T_CHAIN; attr_out = P_WALL | TB; }
    else if (c == D_CAMP || c == D_CAMP + 1) { tile_out = TW_T_CAMP + (c - D_CAMP); attr_out = P_DECOR | TB; }
    else if (c == RC_TRACK_H || c == RC_SAW_H) {
        uint8_t l = tx > 0 && (TR(ty)[tx - 1] == RC_TRACK_H || TR(ty)[tx - 1] == RC_SAW_H);
        uint8_t r = tx < 17 && (TR(ty)[tx + 1] == RC_TRACK_H || TR(ty)[tx + 1] == RC_SAW_H);
        tile_out = (l && r) ? TW_T_RAIL_H : TW_T_RAIL_END;
        attr_out = P_WALL | TB;
    } else if (c == RC_TRACK_V || c == RC_SAW_V) {
        uint8_t u = ty > 0 && (TR(ty - 1)[tx] == RC_TRACK_V || TR(ty - 1)[tx] == RC_SAW_V);
        uint8_t d = ty < 14 && (TR(ty + 1)[tx] == RC_TRACK_V || TR(ty + 1)[tx] == RC_SAW_V);
        tile_out = (u && d) ? TW_T_RAIL_V : TW_T_RAIL_END;
        attr_out = P_WALL | TB;
    } else if (c == RC_TRACK_P || c == RC_PLATFORM) { tile_out = TW_T_TRACK; attr_out = P_WALL | TB; }
    else wall_tile(x, ring);
}

#ifndef TT_HOST
#define row_t (TW->row_t)
#define row_a (TW->row_a)

static void put_row(uint8_t ring) {
    VBK_REG = 1;
    set_bkg_tiles(0, ring & 31, 20, 1, row_a);
    VBK_REG = 0;
    set_bkg_tiles(0, ring & 31, 20, 1, row_t);
}

void tw_gen_step(void) BANKED {
    uint8_t x, y, ring, k, c;
    const uint8_t *trow;
    if (gen_phase || tw_gen_row >= 15) return;
    y = tw_gen_row++;
    ring = (uint8_t)(ring0 + y) & 31;
    set_rows(ring);
    trow = TR(y);
    for (x = 0; x < 20; x++) {
        k = rc[x];
        c = (x >= 1 && x <= 18) ? trow[x - 1] : 0;
        if (k == K_EMPTY && c) deco_tile(c, x, ring, x - 1, y);
        else kind_tile(x, ring, k, y == 0);
        row_t[x] = tile_out;
        row_a[x] = attr_out;
    }
    put_row(ring);
}

void tw_gen_finish(void) BANKED {
    while (gen_phase) tw_gen_work();
    while (tw_gen_row < 15) tw_gen_step();
}

void tw_redraw(uint8_t x, uint8_t ring) BANKED {
    set_rows(ring);
    kind_tile(x, ring, rc[x], 0);
    VBK_REG = 1;
    set_bkg_tile_xy(x, ring & 31, attr_out);
    VBK_REG = 0;
    set_bkg_tile_xy(x, ring & 31, tile_out);
}

/* The very first floor of a run sits on a plain slab with planks in the start lane. */
void tw_draw_slab(uint8_t ring, uint8_t lane) BANKED {
    uint8_t x;
    for (x = 0; x < 20; x++) {
        TW->cells[ring & 31][x] = (x >= lane_x[lane] + 1 && x < lane_x[lane] + 5) ? K_PLANK : K_SLAB;
    }
    set_rows(ring);
    for (x = 0; x < 20; x++) {
        kind_tile(x, ring, TW->cells[ring & 31][x], 1);
        if (TW->cells[ring & 31][x] == K_PLANK) kind_tile(x, ring, K_PLANK, 0);
        row_t[x] = tile_out;
        row_a[x] = attr_out;
    }
    put_row(ring);
}
#endif
