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
static unsigned decision(uint8_t c,uint8_t rival){uint8_t board[225];uint16_t patterns[2][225];memcpy(board,go_cells,225);memcpy(patterns,go_patterns,sizeof(patterns));ai_begin(c,rival);unsigned f=0;while(!ai_done&&f<220){tt_clock++;ai_tick();f++;}CHECK(ai_done&&f<=180);CHECK(!memcmp(board,go_cells,225));CHECK(!memcmp(patterns,go_patterns,sizeof(patterns)));CHECK(ai_resign||(!go_cells[ai_move]&&ai_move<225));return f;}
int main(void){
 memset(test_sram,255,sizeof(test_sram));tt_save_init();CHECK(tt_save_available);
 for(int id=0;id<10;id++){uint8_t style[7];rival_style(id,style);CHECK(style[2]>=2&&style[2]<=4);CHECK(strlen(rivals[id].name)<=5);CHECK(rival_rating(id)>=600&&rival_rating(id)<=1800);for(int e=0;e<8;e++)for(int v=0;v<2;v++)CHECK(strlen(reactions[id][e][v])<=20);}
 for(int elo=450;elo<=1950;elo+=50){tt_save.rating=elo;tt_save.last_rival=255;for(int n=0;n<100;n++){int possible=0;for(int i=0;i<10;i++)if(abs((int)rival_rating(i)-elo)<=150)possible++;int p=rival_match();CHECK(p<10);if(possible)CHECK(abs((int)rival_rating(p)-elo)<=150);if(possible>1)CHECK(p!=tt_save.last_rival);tt_save.last_rival=p;}}
 CHECK(rival_elo_delta(1000,1000,1)==16);CHECK(rival_elo_delta(1000,1000,2)==-16);CHECK(rival_elo_delta(1000,1000,3)==0);
 tt_defaults();go_mode=GO_RANKED;go_rival=9;go_player_color=1;go_reset();go_opening=17;go_book_transform=2;
 for(int i=0;i<4;i++){CHECK(go_place(112+i));CHECK(go_place(97+i));}CHECK(go_place(116));CHECK(go_winner==1);rival_record_result();CHECK(tt_save.rivals[9].wins==1&&tt_save.rivals[9].result==1&&tt_save.rating==1032);CHECK(tt_save.rival_first[9]==112&&tt_save.rival_reply[9]==97);
 go_reset();go_place(112);rival_new_match();uint8_t reply=rival_book_move(9);CHECK(reply<225&&reply!=97&&go_opening!=17&&go_book_transform!=2);
 /* Opening replies rotate without repeating the first response within 20 games. */
 tt_defaults();go_rival=9;go_player_color=1;uint8_t seen[225]={0};for(int game=0;game<20;game++){go_reset();go_place(112);rival_new_match();reply=rival_book_move(9);CHECK(reply<225&&!seen[reply]);seen[reply]=1;}
 /* Load a previous layout without dropping its puzzle, rating or seed. */
 memset(test_sram,255,sizeof(test_sram));tt_defaults();tt_save.rating=1320;tt_save.puzzle.best=4096;tt_save.puzzle.cells[7]=9;tt_save.puzzle.active=1;
 unsigned old_size=offsetof(SaveData,rival_first);uint16_t crc=0xFFFF;for(unsigned i=0;i<old_size;i++){test_sram[12+i]=((uint8_t*)&tt_save)[i];crc=crc_byte(crc,test_sram[12+i]);}
 test_sram[0]='A';test_sram[1]='T';test_sram[2]=1;test_sram[3]=0xA5;test_sram[4]=1;test_sram[5]=0;test_sram[6]=old_size&255;test_sram[7]=old_size>>8;test_sram[8]=crc&255;test_sram[9]=crc>>8;
 tt_save_init();CHECK(tt_save.rating==1320&&tt_save.puzzle.best==4096&&tt_save.puzzle.cells[7]==9);CHECK(tt_save.rival_first[0]==255&&tt_save.rival_reply_bag[0][0]==0);
 tt_save.rivals[3].wins=7;tt_save.rival_first[3]=112;tt_save.rival_reply_bag[3][1]=0x81;tt_save_request();for(int i=0;i<100;i++)tt_save_tick();uint32_t seed=tt_save.rng,next=tt_random();tt_save_init();CHECK(tt_save.rivals[3].wins==7&&tt_save.rival_first[3]==112&&tt_save.rival_reply_bag[3][1]==0x81);CHECK(tt_save.rng==seed&&tt_random()==next);
 printf("PASS rivals/saves: %u checks; 10 complete profiles, bounded dialogue, ranked matching, K32 Elo, grudge response, 20 unique first replies, legacy puzzle migration and persisted RNG/history.\n",checks);
}
