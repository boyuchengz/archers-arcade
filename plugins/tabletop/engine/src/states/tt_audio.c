#pragma bank 255
/* 4-channel step sequencer for the soundtrack + sound effects that borrow
   channel 2 (tones) and channel 4 (noise) for a few frames. */
#include <gb/gb.h>
#include "states/tabletop.h"
#include "states/tt_music_data.h"

#define NONE 255
static uint8_t cur = NONE, want = NONE, step, wait;
static uint8_t sfx2, sfx4, next_at, next_note, next_env;

static const uint8_t bass_wave[16] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                                      0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10};
static const uint8_t drum_env[5] = {0, 0xC1, 0xA2, 0x41, 0x61};
static const uint8_t drum_poly[5] = {0, 0x71, 0x52, 0x10, 0x1B};

uint8_t tt_music_on(void) BANKED {
    return (tt_save.music & 0x80) ? (tt_save.music & 1) : 1;
}

static void silence(void) {
    NR12_REG = 0; NR14_REG = 0x80;
    if (!sfx2) { NR22_REG = 0; NR24_REG = 0x80; }
    NR32_REG = 0;
    if (!sfx4) { NR42_REG = 0; NR44_REG = 0x80; }
}

static void load_wave(void) {
    uint8_t i;
    NR30_REG = 0;
    for (i = 0; i < 16; i++) ((volatile uint8_t *)0xFF30)[i] = bass_wave[i];
    NR30_REG = 0x80;
}

void tt_audio_init(void) BANKED {
    NR52_REG = 0x80;
    NR50_REG = 0x77;
    NR51_REG = tt_save.sound ? 0xFF : 0x00;
    NR10_REG = 0;
    load_wave();
    cur = want = NONE;
}

static void start(uint8_t song) {
    cur = song;
    step = 0;
    wait = 1;
    silence();
}

void tt_music(uint8_t song) BANKED {
    if (song < JINGLE_WIN) {
        want = song;
        if (!tt_save.sound || !tt_music_on()) { cur = NONE; silence(); return; }
        if (cur == song) return;
        start(song);
    } else {
        if (!tt_save.sound) return;
        start(song);
    }
}

void tt_audio_apply(void) BANKED {
    NR51_REG = tt_save.sound ? 0xFF : 0x00;
    if (!tt_save.sound || !tt_music_on()) {
        cur = NONE;
        silence();
    } else if (want != NONE && cur != want) {
        start(want);
    }
}

static void trigger_ch1(uint8_t note, const Song *s) {
    uint16_t p = note_period[note];
    NR10_REG = 0;
    NR11_REG = s->d1;
    NR12_REG = s->e1;
    NR13_REG = (uint8_t)p;
    NR14_REG = 0x80 | (uint8_t)(p >> 8);
}

static void trigger_ch2(uint8_t note, uint8_t duty, uint8_t env, uint8_t len) {
    uint16_t p = note_period[note];
    NR21_REG = duty | len;
    NR22_REG = env;
    NR23_REG = (uint8_t)p;
    NR24_REG = (len ? 0xC0 : 0x80) | (uint8_t)(p >> 8);
}

static void trigger_ch3(uint8_t note, uint8_t vol) {
    uint16_t p = note_period[note];
    NR30_REG = 0x80;
    NR32_REG = vol;
    NR33_REG = (uint8_t)p;
    NR34_REG = 0x80 | (uint8_t)(p >> 8);
}

static void trigger_ch4(uint8_t env, uint8_t poly, uint8_t len) {
    NR41_REG = len;
    NR42_REG = env;
    NR43_REG = poly;
    NR44_REG = len ? 0xC0 : 0x80;
}

void tt_music_tick(void) BANKED {
    const Song *s;
    uint8_t v;
    if (sfx2) sfx2--;
    if (sfx4) sfx4--;
    if (next_at && !--next_at) {
        trigger_ch2(next_note, 0x80, next_env, 0);
        sfx2 = 8;
    }
    if (cur == NONE) return;
    if (--wait) return;
    s = &songs[cur];
    wait = s->tempo;
    v = s->ch[0][step];
    if (v == 1) { NR12_REG = 0; NR14_REG = 0x80; }
    else if (v) trigger_ch1(v - 2, s);
    v = s->ch[1][step];
    if (!sfx2) {
        if (v == 1) { NR22_REG = 0; NR24_REG = 0x80; }
        else if (v) trigger_ch2(v - 2, s->d2, s->e2, 0);
    }
    v = s->ch[2][step];
    if (v == 1) NR32_REG = 0;
    else if (v) trigger_ch3(v - 2, s->v3);
    v = s->ch[3][step];
    if (v && !sfx4) trigger_ch4(drum_env[v], drum_poly[v], 0);
    if (++step >= s->len) {
        step = 0;
        if (!s->loop) {
            silence();
            cur = NONE;
            if (want != NONE && tt_save.sound && tt_music_on()) start(want);
        }
    }
}

/* ------------------------------------------------------------------ effects */
#define N(octave, semi) ((uint8_t)(((octave) - 2) * 12 + (semi)))

void tt_sound_pitch(uint8_t effect, uint8_t pitch) BANKED {
    if (!tt_save.sound) return;
    switch (effect) {
    case SFX_MOVE:
        trigger_ch2(N(6, 0), 0x80, 0x71, 56); sfx2 = 4; break;
    case SFX_OK:
        trigger_ch2(N(5, 4), 0x80, 0xA2, 0); sfx2 = 10;
        next_note = N(5, 11); next_env = 0xA3; next_at = 4; break;
    case SFX_BACK:
        trigger_ch2(N(5, 7), 0x80, 0x92, 0); sfx2 = 10;
        next_note = N(5, 0); next_env = 0x93; next_at = 4; break;
    case SFX_CLACK:
        trigger_ch4(0xB1, 0x33, 50); sfx4 = 6;
        trigger_ch2(N(7, 0), 0x00, 0x51, 60); sfx2 = 3; break;
    case SFX_SLIDE:
        trigger_ch4(0x62, 0x24, 52); sfx4 = 6; break;
    case SFX_MERGE: {
        uint8_t n = N(4, 0) + (pitch < 14 ? pitch : 14) * 2;
        trigger_ch2(n, 0x80, 0xB2, 0); sfx2 = 12;
        next_note = n + 7; next_env = 0xA3; next_at = 3; break;
    }
    case SFX_ERROR:
        trigger_ch2(N(3, 1), 0xC0, 0xA2, 0); sfx2 = 10;
        next_note = N(2, 11); next_env = 0x93; next_at = 5; break;
    case SFX_BLIP:
        trigger_ch2(pitch, 0x80, 0x61, 58); sfx2 = 3; break;
    case SFX_TICK:
        trigger_ch2(N(6, 7), 0x40, 0x81, 58); sfx2 = 3; break;
    case SFX_UNDO:
        trigger_ch2(N(5, 9), 0x40, 0x92, 0); sfx2 = 10;
        next_note = N(5, 2); next_env = 0x92; next_at = 3; break;
    case SFX_POP:
        trigger_ch2(N(6, 4), 0x80, 0x51, 60); sfx2 = 3; break;
    case SFX_WHOOSH:
        trigger_ch4(0x2B, 0x31, 40); sfx4 = 10; break;
    }
}

void tt_sound(uint8_t effect) BANKED {
    tt_sound_pitch(effect, 0);
}
