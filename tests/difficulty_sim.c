#define TT_HOST
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "states/tt_gomoku.h"
uint8_t tt_scratch[512] __attribute__((aligned(16)));
uint8_t tt_game_cache[512],test_sram[8192],tt_state,tt_old_state,tt_pressed,tt_held;
uint16_t tt_clock;
uint8_t go_mode,go_rival,go_player_color,go_cursor,go_opening;
#include "tt_save.c"
#include "tt_pattern0.c"
#include "tt_pattern1.c"
#include "tt_pattern2.c"
#include "tt_pattern3.c"
#include "tt_pattern4.c"
#include "tt_pattern5.c"
#include "tt_pattern6.c"
#include "tt_pattern7.c"
#include "tt_patterns.c"
#include "tt_gocore.c"
#include "tt_rivals.c"
#include "tt_ai.c"

/* Simulated human policies.
   level 0 "newcomer": builds own lines, blocks a four only half the time, random among top 5.
   level 1 "casual":   always takes a win and blocks a four, otherwise greedy among top 3.
   level 2 "club":     plays the grandmaster AI (SOREN settings) itself. */
static int ai_pick(uint8_t color, uint8_t rival, uint8_t *resigned) {
    ai_begin(color, rival);
    for (int f = 0; f < 400 && !ai_done; f++) { tt_clock++; ai_tick(); }
    *resigned = ai_resign;
    return ai_move;
}
static int human_pick(uint8_t color, int level) {
    uint8_t one, r;
    if (level == 2) return ai_pick(color, 9, &r) | (r ? 0x100 : 0);
    if (winning_points(color, &one)) return one;
    if (winning_points(3 - color, &one) && (level >= 1 || rand() % 2)) return one;
    /* sees a move that makes an open four / double four (notice rate by level) */
    int notice = level == 0 ? 40 : 85, guard = level == 0 ? 35 : 75, threat = -1;
    for (int p = 0; p < 225 && go_count; p++) {
        if (go_cells[p] || (!go_patterns[0][p] && !go_patterns[1][p])) continue;
        uint8_t t;
        go_set(p, color); int mine = winning_points(color, &t); go_set(p, 0);
        if (mine >= 2 && rand() % 100 < notice) return p;
        if (threat < 0) { go_set(p, 3 - color); int theirs = winning_points(3 - color, &t); go_set(p, 0); if (theirs >= 2) threat = p; }
    }
    if (threat >= 0 && rand() % 100 < guard) return threat;
    int k = level == 0 ? 5 : 3, best[5] = {0}, score[5] = {0}, n = 0;
    for (int p = 0; p < 225; p++) {
        if (go_cells[p]) continue;
        if (go_count && !go_patterns[0][p] && !go_patterns[1][p]) continue;
        int s = go_strength(color - 1)[p] * 10 + go_strength(2 - color)[p] * (level == 0 ? 6 : 9) + rand() % 8;
        int i = n < k ? n++ : k - 1;
        if (n == k && i == k - 1 && s <= score[i]) continue;
        while (i > 0 && score[i - 1] < s) { score[i] = score[i - 1]; best[i] = best[i - 1]; i--; }
        score[i] = s; best[i] = p;
    }
    if (!go_count) return 112;
    if (!n) { for (int p = 0; p < 225; p++) if (!go_cells[p]) return p; }
    return best[rand() % (n < k ? n : k)];
}
int main(int argc, char **argv) {
    int games = argc > 1 ? atoi(argv[1]) : 40;
    srand(12345);
    tt_defaults();
    const char *lvl[3] = {"newcomer", "casual", "club(AI as player)"};
    for (int level = 0; level < 3; level++) {
        printf("\nPlayer: %s\n", lvl[level]);
        for (int rival = 0; rival < 10; rival++) {
            int w = 0, l = 0, d = 0, wb = 0, gb = 0;
            for (int g = 0; g < games; g++) {
                go_mode = GO_PRACTICE; go_rival = rival; go_player_color = 1 + (g & 1);
                tt_save.rng ^= (uint32_t)rand() * 2654435761u;
                go_reset(); rival_new_match();
                int winner = 0;
                while (!go_winner && go_count < 225) {
                    int p; uint8_t res = 0;
                    if (go_turn == go_player_color) { p = human_pick(go_turn, level); if (p & 0x100) { winner = 3 - go_turn; break; } }
                    else { p = ai_pick(go_turn, rival, &res); if (res) { winner = go_player_color; break; } }
                    if (!go_place((uint8_t)p)) { for (int q = 0; q < 225; q++) if (go_place(q)) break; }
                }
                if (!winner) winner = go_winner;
                if (go_player_color == 1) gb++;
                if (winner == go_player_color) { w++; if (go_player_color == 1) wb++; }
                else if (winner == 3 || winner == 0) d++; else l++;
            }
            printf("  vs %-5s (%4d): player wins %3d%%  (as black %3d%%, as white %3d%%)  draws %d\n",
                   rivals[rival].name, rivals[rival].rating, w * 100 / games, wb * 100 / gb, (w - wb) * 100 / (games - gb), d);
        }
    }
    return 0;
}
