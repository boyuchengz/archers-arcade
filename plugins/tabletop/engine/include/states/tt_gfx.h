#ifndef TT_GFX_H
#define TT_GFX_H
#include "tabletop.h"
#include "tt_tiles.h"

/* Attribute byte: palette 0..7, +TT_BK1 selects VRAM bank 1 tiles (theme art). */
#define TT_BK1 8
#define TT_FLIPX 32
#define TT_FLIPY 64

/* Dynamic (software rendered) tiles live in VRAM bank 0 from here. */
#define TT_DYN 160

/* Sprite slots */
#define SPR_CURSOR 0      /* 4 corner brackets */
#define SPR_MARK 4        /* last-move marker */
#define SPR_DROP 5        /* falling stone */
#define SPR_ARROW_L 6
#define SPR_ARROW_R 7
#define SPR_PAGE 8        /* up to 10 page dots: 8..17 */
#define SPR_FX 18         /* particle pool: 18..39 */
#define SPR_FX_COUNT 22

/* OBJ palettes */
#define OP_CURSOR 0
#define OP_RIVAL 1
#define OP_SPARK 2
#define OP_CONF_A 3
#define OP_TEXT 4
#define OP_STONE 5
#define OP_MARK 6
#define OP_CONF_B 7

/* Palette RAM mirror: 0..31 background, 32..63 objects. */
extern uint16_t tt_pal[64];
extern uint8_t tt_bright,tt_win,tt_shake,tt_scx,tt_scy,tt_split_scx,tt_split_on,tt_pattern_on;

void tt_gfx_init(void) BANKED;
void tt_pal_set(uint8_t first,uint8_t count,const uint16_t *ram_colors) BANKED;
void tt_pal_obj_reset(void) BANKED;
void tt_pal_commit(void) BANKED;
void tt_frame(void) BANKED;
void tt_wait(uint8_t frames) BANKED;
void tt_fade_out(void) BANKED;
void tt_fade_in(void) BANKED;
void tt_screen_reset(void) BANKED;

void tt_put(uint8_t x,uint8_t y,uint8_t tile,uint8_t attr) BANKED;
void tt_fill(uint8_t x,uint8_t y,uint8_t w,uint8_t h,uint8_t tile,uint8_t attr) BANKED;
void tt_attr(uint8_t x,uint8_t y,uint8_t w,uint8_t h,uint8_t attr) BANKED;
void tt_rows(uint8_t x,uint8_t y,uint8_t w,uint8_t h,const uint8_t *tiles,const uint8_t *attrs) BANKED;
void tt_box(uint8_t x,uint8_t y,uint8_t w,uint8_t h,uint8_t style,uint8_t attr) BANKED;
void tt_number(uint8_t x,uint8_t y,uint32_t n,uint8_t digits,uint8_t attr) BANKED;
void tt_digits(uint8_t tile,uint8_t ntiles,uint32_t n,uint8_t ink,uint8_t shadow) BANKED;
void tt_clear(uint8_t attr) BANKED;

/* String helpers read the caller's bank (string literals live with the caller). */
void tt_text(uint8_t x,uint8_t y,const char *s,uint8_t attr) NONBANKED;
void tt_ctext(uint8_t y,const char *s,uint8_t attr) NONBANKED;
void tt_text_far(uint8_t x,uint8_t y,const char *s,uint8_t attr,uint8_t bank) BANKED;
uint8_t tt_strlen_far(const char *s,uint8_t bank) BANKED;

void tt_spr(uint8_t id,uint8_t x,uint8_t y,uint8_t tile,uint8_t prop) BANKED;
void tt_spr_hide(uint8_t first,uint8_t count) BANKED;

/* particles: tt_fx.c */
#define FX_SPARK 1
#define FX_CONFETTI 2
#define FX_FLOAT 3
#define FX_STILL 4
void fx_clear(void) BANKED;
void fx_tick(void) BANKED;
void fx_spawn(uint8_t kind,uint8_t x,uint8_t y,int8_t vx,int8_t vy,uint8_t tile,uint8_t prop,uint8_t life) BANKED;
void fx_burst(uint8_t x,uint8_t y,uint8_t count) BANKED;
void fx_confetti(uint8_t count) BANKED;
void fx_text(uint8_t x,uint8_t y,const char *ram_text) BANKED;

/* menu / screens: tt_menu.c */
void tt_pattern_tick(void) BANKED;
void tt_pattern_load(void) BANKED;
void tt_menu_bg(void) BANKED;

/* portraits: tt_portraits.c */
void portrait_load(uint8_t id,uint8_t tile_base) BANKED;
void portrait_draw(uint8_t id,uint8_t expr,uint8_t x,uint8_t y,uint8_t tile_base,uint8_t pal) BANKED;
void portrait_palettes(uint8_t id,uint8_t pal) BANKED;
#endif
