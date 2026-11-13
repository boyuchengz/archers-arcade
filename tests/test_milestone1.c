#define TT_HOST
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "states/tabletop.h"
uint8_t tt_game_cache[512],tt_scratch[512],test_sram[8192],tt_state,tt_old_state,tt_pressed,tt_held;uint16_t tt_clock;
void tt_text(uint8_t x,uint8_t y,const char*s,uint8_t p){}
void tt_number(uint8_t x,uint8_t y,uint32_t n,uint8_t d,uint8_t p){} void tt_panel_number(uint8_t y,uint32_t n){}
void tt_map(uint8_t x,uint8_t y,uint8_t w,uint8_t h,const uint8_t*t,const uint8_t*p){}
void tt_fill(uint8_t x,uint8_t y,uint8_t w,uint8_t h,uint8_t t,uint8_t p){} void tt_hint(uint8_t x,uint8_t y,uint8_t k,const char*s,uint8_t p){} void tt_puzzle_theme(void){} void tt_pause_card(void){} void tt_palette_reset(void){} void tt_clear(uint8_t p){} void tt_popup(const char*a,const char*b){} void tt_sound(uint8_t e){} void tt_menu(void){}
void puzzle_view_enter(void){} void puzzle_view_board(void){} void puzzle_view_stats(void){} void puzzle_view_tick(void){} void puzzle_view_moved(void){} void puzzle_view_settled(void){}
#include "tt_save.c"
#include "tt_puzzle.c"
static unsigned count;
#define CHECK(x) do{count++;if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
static void row(uint8_t a,uint8_t b,uint8_t c,uint8_t d){memset(&tt_save.puzzle,0,sizeof(Puzzle));tt_save.puzzle.active=1;tt_save.puzzle.cells[0]=a;tt_save.puzzle.cells[1]=b;tt_save.puzzle.cells[2]=c;tt_save.puzzle.cells[3]=d;}
static void finish(void){tt_pressed=0;for(int i=0;i<6;i++){tt_clock++;puzzle_tick();}}
int main(void){
 memset(test_sram,255,sizeof(test_sram));tt_save_init();CHECK(tt_save_available&&tt_save.rating==1000);
 row(1,1,1,1);CHECK(puzzle_move(TT_LEFT));CHECK(tt_save.puzzle.cells[0]==2&&tt_save.puzzle.cells[1]==2&&!tt_save.puzzle.cells[2]);CHECK(tt_save.puzzle.score==8);
 puzzle_undo();CHECK(tt_save.puzzle.cells[0]==1&&tt_save.puzzle.score==0&&!tt_save.puzzle.undo&&tt_save.puzzle.best==8);
 row(1,1,2,0);puzzle_move(TT_LEFT);CHECK(tt_save.puzzle.cells[0]==2&&tt_save.puzzle.cells[1]==2&&!tt_save.puzzle.cells[2]);
 row(1,1,1,1);puzzle_move(TT_RIGHT);CHECK(tt_save.puzzle.cells[2]==2&&tt_save.puzzle.cells[3]==2&&!tt_save.puzzle.cells[0]);
 row(1,2,3,4);CHECK(!puzzle_move(TT_LEFT));CHECK(tt_save.puzzle.moves==0&&!tt_save.puzzle.undo);
 for(int i=0;i<16;i++)tt_save.puzzle.cells[i]=(i+i/4)%2+1;CHECK(!puzzle_can_move());tt_save.puzzle.cells[0]=0;CHECK(puzzle_can_move());
 row(10,10,0,0);puzzle_move(TT_LEFT);finish();CHECK(tt_save.puzzle.won&&tt_save.puzzle.score==2048);
 row(30,30,30,30);puzzle_move(TT_LEFT);CHECK(tt_save.puzzle.cells[0]==31&&tt_save.puzzle.cells[1]==31&&tt_save.puzzle.score==UINT32_MAX);puzzle_undo();CHECK(tt_save.puzzle.cells[0]==30);
 row(1,1,0,0);tt_save.puzzle.score=UINT32_MAX-2;puzzle_move(TT_LEFT);CHECK(tt_save.puzzle.score==UINT32_MAX);
 unsigned twos=0,fours=0;for(int r=0;r<10000;r++){memset(tt_save.puzzle.cells,0,16);spawn();int cells=0;for(int i=0;i<16;i++)if(tt_save.puzzle.cells[i]){cells++;if(tt_save.puzzle.cells[i]==1)twos++;else if(tt_save.puzzle.cells[i]==2)fours++;else CHECK(0);}CHECK(cells==1);}CHECK(twos>8750&&twos<9250&&fours>750&&fours<1250);
 /* Every 4-cell exponent combination agrees with a separate one-pass merge model. */
 for(int dir=0;dir<4;dir++)for(int bits=0;bits<625;bits++){
  uint8_t expected[4]={0},input[4],v[4],n=0,d=0;int t=bits;row(0,0,0,0);
  uint8_t key=(uint8_t[]){TT_LEFT,TT_RIGHT,TT_UP,TT_DOWN}[dir];
  for(int p=0;p<4;p++){input[p]=t%5;t/=5;tt_save.puzzle.cells[cell_at(key,0,p)]=input[p];if(input[p])v[n++]=input[p];}
  for(int p=0;p<n;p++){if(p+1<n&&v[p]==v[p+1]){expected[d++]=v[p]+1;p++;}else expected[d++]=v[p];}
  puzzle_move(key);for(int p=0;p<4;p++)CHECK(tt_save.puzzle.cells[cell_at(key,0,p)]==expected[p]);
 }
 /* A committed save remains readable through every interrupted progressive write. */
 tt_defaults();tt_save.rating=1111;tt_save_request();for(int i=0;i<100;i++)tt_save_tick();uint8_t baseline[8192];memcpy(baseline,test_sram,8192);
 for(int cut=0;cut<40;cut++){
  memcpy(test_sram,baseline,8192);tt_saving=tt_dirty=0;tt_save_init();CHECK(tt_save.rating==1111);tt_save.rating=1420;tt_save_request();for(int i=0;i<cut;i++)tt_save_tick();tt_saving=tt_dirty=0;tt_save_init();CHECK(tt_save.rating==1111||tt_save.rating==1420);
 }
 printf("PASS milestone 1: %u checks; merge directions, single-merge rule, undo, 2048 detection, spawn distribution %u/%u and save interruption recovery.\n",count,twos,fours);
}
