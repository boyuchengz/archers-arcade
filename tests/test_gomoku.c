#define TT_HOST
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "states/tt_gomoku.h"
#include "tt_pattern0.c"
#include "tt_pattern1.c"
#include "tt_pattern2.c"
#include "tt_pattern3.c"
#include "tt_pattern4.c"
#include "tt_pattern5.c"
#include "tt_pattern6.c"
#include "tt_pattern7.c"
#include "tt_patterns.c"
uint8_t tt_game_cache[512];
#include "tt_gocore.c"
static unsigned checks;static unsigned rng=123456789;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
static unsigned rand32(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static unsigned strength_ref(uint16_t bits){static const unsigned weights[]={0,1,4,16,64,160,255};unsigned total=0,threes=0;for(int d=0;d<4;d++){unsigned c=(bits>>(d*4))&15;CHECK(c<7);total+=weights[c];threes+=c==3;}if(threes>=2)total+=64;return total>255?255:total;}
int main(void){
 go_reset();CHECK(go_turn==1&&go_count==0);CHECK(go_place(112));CHECK(!go_place(112)&&go_count==1);CHECK(!go_place(225));go_undo_pair();CHECK(go_count==0&&go_turn==1&&!go_cells[112]);
 int dirs[4]={1,15,16,14};for(int d=0;d<4;d++){go_reset();int start=d==3?5:0;for(int k=0;k<4;k++)go_set(start+k*dirs[d],1);CHECK(!go_wins_at(start,1));go_set(start+4*dirs[d],1);CHECK(go_wins_at(start+2*dirs[d],1));go_set(start+5*dirs[d],1);CHECK(go_wins_at(start+2*dirs[d],1));}
 go_reset();for(int p=13;p<=17;p++)go_set(p,1);CHECK(!go_wins_at(15,1));
 go_reset();for(int turn=0;turn<80;turn++){int p;do{p=rand32()%225;}while(go_cells[p]);go_set(p,1+(turn&1));uint16_t before[2][225];memcpy(before,go_patterns,sizeof(before));for(int q=0;q<225;q++){uint16_t expected[2]={0,0};for(int d=0;d<4;d++){int j=0;uint16_t code=0;for(int k=-4;k<=4;k++){if(!k)continue;int xx=q%15+dx[d]*k,yy=q/15+dy[d]*k;int v=(xx<0||xx>=15||yy<0||yy>=15)?3:go_cells[yy*15+xx];code|=v<<(2*j++);}CHECK(go_line_codes[q][d]==code);int packed=patterns_lookup(code);expected[0]|=(packed&15)<<(4*d);expected[1]|=(packed>>4)<<(4*d);}if(go_cells[q])expected[0]=expected[1]=0;CHECK(expected[0]==go_patterns[0][q]);CHECK(expected[1]==go_patterns[1][q]);}}
 for(int color=0;color<2;color++)for(int q=0;q<225;q++)CHECK(go_strength(color)[q]==strength_ref(go_patterns[color][q]));
 go_reset();for(int k=0;k<4;k++)go_set(7*15+4+k,1);CHECK((go_patterns[0][7*15+8]&15)==6);CHECK((go_patterns[0][7*15+3]&15)==6);
 uint8_t strengths[450];memcpy(strengths,tt_game_cache,450);go_set(122,2);go_set(122,0);CHECK(!memcmp(strengths,tt_game_cache,450));
 printf("PASS Gomoku core: %u checks; all directions, overlines, edges, illegal moves, undo and incremental cache equals full recomputation.\n",checks);
}
