#ifndef TABLETOP_H
#define TABLETOP_H
#include <stdint.h>
#ifdef TT_HOST
#define BANKED
#define NONBANKED
#else
#include <gbdk/platform.h>
#endif

/* ------------------------------------------------------------------ states */
#define TT_MENU 0
#define TT_PUZZLE 1
#define TT_GOMOKU 2
#define TT_SETTINGS 3
#define TT_MATCH 4
#define TT_PICK 5
#define TT_GMODE 6
#define TT_PAUSE 7
#define TT_CONFIRM 8
#define TT_RESULT 9
#define TT_HELP 10
#define TT_TITLE 11

/* ------------------------------------------------------------------ buttons */
#define TT_UP 4
#define TT_DOWN 8
#define TT_LEFT 2
#define TT_RIGHT 1
#define TT_A 16
#define TT_B 32
#define TT_SELECT 64
#define TT_START 128
#define TT_DPAD (TT_UP | TT_DOWN | TT_LEFT | TT_RIGHT)

/* ------------------------------------------------------------------ save data
   Layout is unchanged from the first cartridge so existing saves load. */
typedef struct {uint16_t wins,losses;uint8_t opening,result;} RivalRecord;
typedef struct {
 uint8_t cells[16],previous[16];uint32_t score,previous_score,best;
 uint16_t moves,previous_moves;uint8_t active,undo,won;
} Puzzle;
typedef struct {uint32_t rng;uint16_t rating;uint8_t sound,music,last_rival;RivalRecord rivals[10];Puzzle puzzle;uint8_t rival_first[10],rival_transform[10],rival_reply[10],rival_reply_bag[10][3];} SaveData;
extern SaveData tt_save;
extern uint8_t tt_state,tt_old_state,tt_pressed,tt_held,tt_dirty,tt_saving,tt_save_available;
extern uint16_t tt_clock;

/* ------------------------------------------------------------------ core / save */
uint32_t tt_random(void) BANKED;
uint16_t tt_random_below(uint16_t n) BANKED;
void tt_entropy(uint8_t buttons,uint8_t div,uint16_t frame) BANKED;
void tt_defaults(void) BANKED;
void tt_save_request(void) BANKED;
void tt_save_init(void) BANKED;
void tt_save_tick(void) BANKED;
void tt_save_reset(void) BANKED;

/* ------------------------------------------------------------------ audio */
#define SFX_MOVE 0
#define SFX_OK 1
#define SFX_BACK 2
#define SFX_CLACK 3
#define SFX_SLIDE 4
#define SFX_MERGE 5
#define SFX_ERROR 6
#define SFX_BLIP 7
#define SFX_TICK 8
#define SFX_UNDO 9
#define SFX_POP 10
#define SFX_WHOOSH 11
#define SONG_TITLE 0
#define SONG_PUZZLE 1
#define SONG_GOMOKU 2
#define JINGLE_WIN 3
#define JINGLE_LOSE 4
#define JINGLE_2048 5
#define JINGLE_FOUND 6
#define JINGLE_BEST 7
void tt_sound(uint8_t effect) BANKED;
void tt_sound_pitch(uint8_t effect,uint8_t pitch) BANKED;
void tt_music(uint8_t song) BANKED;
void tt_music_tick(void) BANKED;
void tt_audio_init(void) BANKED;
void tt_audio_apply(void) BANKED;
uint8_t tt_music_on(void) BANKED;

/* ------------------------------------------------------------------ flow */
void tt_menu(void) BANKED;
void tt_title(void) BANKED;
void puzzle_new(void) BANKED;
void puzzle_enter(void) BANKED;
void puzzle_tick(void) BANKED;
uint8_t puzzle_move(uint8_t direction) BANKED;
uint8_t puzzle_can_move(void) BANKED;
void puzzle_undo(void) BANKED;
void puzzle_settle(void) BANKED;
void gomoku_menu(void) BANKED;
void gomoku_enter(uint8_t mode,uint8_t rival) BANKED;
void gomoku_tick(void) BANKED;
void gomoku_render(void) BANKED;
void gomoku_resume(void) BANKED;
void gomoku_pause_tick(void) BANKED;
void tabletop_init(void) BANKED;
void tabletop_update(void) BANKED;

/* 2048 view hooks (tt_puzzle_view.c; stubbed by the desktop tests) */
void puzzle_view_enter(void) BANKED;
void puzzle_view_board(void) BANKED;
void puzzle_view_stats(void) BANKED;
void puzzle_view_tick(void) BANKED;
void puzzle_view_moved(void) BANKED;
void puzzle_view_settled(void) BANKED;

/* 2048 animation state shared between rules and view */
extern uint8_t pz_anim,pz_spawn_cell,pz_spawn_age,pz_notice,pz_over,pz_merges,pz_best_before;
extern uint8_t pz_motion_from[16],pz_motion_to[16],pz_motion_exp[16],pz_motion_count;
extern uint8_t pz_merge_cells[16];
extern uint32_t pz_gain;
#endif
