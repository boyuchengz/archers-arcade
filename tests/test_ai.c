#define TT_HOST
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "states/tt_gomoku.h"
uint8_t tt_game_cache[512],tt_scratch[512],test_sram[8192],tt_state,tt_old_state,tt_pressed,tt_held;
uint16_t tt_clock;uint8_t go_mode,go_rival,go_player_color,go_cursor,go_opening;
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
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
static unsigned decision(uint8_t c,uint8_t rival){uint8_t board[225],strengths[450];uint16_t patterns[2][225];memcpy(board,go_cells,225);memcpy(strengths,tt_game_cache,450);memcpy(patterns,go_patterns,sizeof(patterns));ai_begin(c,rival);unsigned f=0;while(!ai_done&&f<220){tt_clock++;ai_tick();f++;}CHECK(ai_done&&f<=180);CHECK(!memcmp(board,go_cells,225));CHECK(!memcmp(patterns,go_patterns,sizeof(patterns)));CHECK(!memcmp(strengths,tt_game_cache,450));CHECK(ai_resign||(!go_cells[ai_move]&&ai_move<225));return f;}
int main(void){tt_defaults();CHECK(sizeof(AIWork)<=512);
 go_reset();for(int x=3;x<7;x++)go_set(7*15+x,1);decision(1,9);CHECK(!ai_resign&&go_wins_at((go_set(ai_move,1),ai_move),1));
 go_reset();for(int x=3;x<7;x++)go_set(7*15+x,2);go_set(7*15+2,1);decision(1,9);CHECK(!ai_resign&&ai_move==7*15+7);
 go_reset();for(int x=3;x<7;x++)go_set(7*15+x,2);decision(1,9);CHECK(ai_resign);
 go_reset();go_set(111,1);go_set(112,1);go_set(113,1);go_set(0,2);go_set(15,2);go_count=5;go_turn=2;go_opening=255;decision(2,9);CHECK(!ai_resign);
 go_reset();go_place(112);go_place(113);go_place(97);ai_begin(2,9);for(int i=0;i<20;i++){tt_clock++;ai_tick();}ai_cancel();for(int i=0;i<50&&!ai_done;i++){tt_clock++;ai_tick();}CHECK(ai_done&&ai_cancelled&&!memcmp(go_live_cells,go_cells,225));
 unsigned wins[3]={0},max_f=0,moves=0;uint8_t sequences[20][225],lengths[20];clock_t began=clock();
 tt_save.rng=0x124513;for(int game=0;game<20;game++){
  go_reset();go_mode=GO_RANKED;go_rival=9;go_player_color=2;rival_new_match();int turn;
  for(turn=0;turn<100&&!go_winner;turn++){
   tt_entropy(TT_A,(game*37+turn*19)&255,tt_clock);unsigned f=decision(go_turn,go_turn==1?9:0);if(f>max_f)max_f=f;
   if(ai_resign){go_winner=3-go_turn;break;}CHECK(go_place(ai_move));moves++;
  }
  if(go_winner<3)wins[go_winner]++;if(go_winner){go_rival=9;rival_record_result();}lengths[game]=go_count;memcpy(sequences[game],go_history,go_count);for(int prev=0;prev<game;prev++)CHECK(lengths[prev]!=go_count||memcmp(sequences[prev],go_history,go_count));
 }
 printf("PASS AI: %u checks; %u legal self-play moves, strongest vs beginner W/L/draw-or-limit %u/%u/%u; max %u sliced ticks; 20 unique sequences; host CPU %.3fs; workspace %zu bytes.\n",checks,moves,wins[1],wins[2],20-wins[1]-wins[2],max_f,(double)(clock()-began)/CLOCKS_PER_SEC,sizeof(AIWork));
}
