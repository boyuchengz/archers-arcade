#pragma bank 255
#include <string.h>
#include "states/tt_gomoku.h"
#include "states/tt_border_codes.h"
uint8_t go_cells[225],go_live_cells[225],go_history[225],go_count,go_turn,go_winner;
uint16_t go_patterns[2][225],go_line_codes[225][4],go_max_update_frames;
static const int8_t dx[4]={1,0,1,1},dy[4]={0,1,1,-1};
static const uint8_t neighbor_shift[9]={14,12,10,8,0,6,4,2,0};
static const uint16_t strength_pairs[256]={0,1,4,16,64,160,255,0,0,0,0,0,0,0,0,0,1,2,5,17,65,161,256,1,1,1,1,1,1,1,1,1,4,5,8,20,68,164,259,4,4,4,4,4,4,4,4,4,16,17,20,32,80,176,271,16,16,16,16,16,16,16,16,16,64,65,68,80,128,224,319,64,64,64,64,64,64,64,64,64,160,161,164,176,224,320,415,160,160,160,160,160,160,160,160,160,255,256,259,271,319,415,510,255,255,255,255,255,255,255,255,255,0,1,4,16,64,160,255,0,0,0,0,0,0,0,0,0,0,1,4,16,64,160,255,0,0,0,0,0,0,0,0,0,0,1,4,16,64,160,255,0,0,0,0,0,0,0,0,0,0,1,4,16,64,160,255,0,0,0,0,0,0,0,0,0,0,1,4,16,64,160,255,0,0,0,0,0,0,0,0,0,0,1,4,16,64,160,255,0,0,0,0,0,0,0,0,0,0,1,4,16,64,160,255,0,0,0,0,0,0,0,0,0,0,1,4,16,64,160,255,0,0,0,0,0,0,0,0,0,0,1,4,16,64,160,255,0,0,0,0,0,0,0,0,0};
static const uint8_t strength_threes[256]={0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0};
static void refresh_strength(uint8_t p){uint8_t color,lo,hi;uint16_t bits,v;for(color=0;color<2;color++){bits=go_patterns[color][p];lo=(uint8_t)bits;hi=bits>>8;v=strength_pairs[lo]+strength_pairs[hi];if(strength_threes[lo]+strength_threes[hi]>=2)v+=64;go_strength(color)[p]=v>255?255:(uint8_t)v;}}
static void refresh_direction(uint8_t p,uint8_t d){
 uint8_t packed;uint16_t mask;uint8_t shift=d<<2;
 if(go_cells[p]){go_patterns[0][p]=go_patterns[1][p]=0;go_strength(0)[p]=go_strength(1)[p]=0;return;}
 packed=patterns_lookup(go_line_codes[p][d]);mask=~((uint16_t)15<<shift);
 go_patterns[0][p]=(go_patterns[0][p]&mask)|((uint16_t)(packed&15)<<shift);
 go_patterns[1][p]=(go_patterns[1][p]&mask)|((uint16_t)(packed>>4)<<shift);refresh_strength(p);
}
void go_reset(void) BANKED {
 memset(go_cells,0,225);memset(go_live_cells,0,225);memset(go_patterns,0,sizeof(go_patterns));memset(tt_game_cache,0,450);memcpy(go_line_codes,initial_line_codes,sizeof(go_line_codes));go_count=go_winner=0;go_turn=1;go_max_update_frames=0;
}
void go_refresh_cell(uint8_t p) BANKED {uint8_t d;for(d=0;d<4;d++)refresh_direction(p,d);}
void go_set(uint8_t p,uint8_t color) BANKED {
 uint8_t d,q,shift;int8_t n,x,y,xx,yy;uint16_t mask;
#ifndef TT_HOST
 uint16_t began=sys_time,elapsed;
#endif
 if(p>=225)return;go_cells[p]=color;go_refresh_cell(p);x=p%15;y=p/15;
 for(d=0;d<4;d++)for(n=-4;n<=4;n++){
  if(!n)continue;xx=x+dx[d]*n;yy=y+dy[d]*n;if(xx<0||xx>=15||yy<0||yy>=15)continue;
  q=yy*15+xx;shift=neighbor_shift[n+4];mask=~((uint16_t)3<<shift);
  go_line_codes[q][d]=(go_line_codes[q][d]&mask)|((uint16_t)color<<shift);refresh_direction(q,d);
 }
#ifndef TT_HOST
 elapsed=sys_time-began;if(elapsed>go_max_update_frames)go_max_update_frames=elapsed;
#endif
}
uint8_t go_wins_at(uint8_t p,uint8_t color) BANKED {
 uint8_t d,n;int8_t x,y,xx,yy,sign;
 if(!color||p>=225)return 0;x=p%15;y=p/15;
 for(d=0;d<4;d++){n=1;for(sign=-1;sign<=1;sign+=2){xx=x+dx[d]*sign;yy=y+dy[d]*sign;while(xx>=0&&xx<15&&yy>=0&&yy<15&&go_cells[yy*15+xx]==color){if(++n>=5)return 1;xx+=dx[d]*sign;yy+=dy[d]*sign;}}}return 0;
}
uint8_t go_place(uint8_t p) BANKED {
 if(p>=225||go_cells[p]||go_winner||go_count>=225)return 0;go_set(p,go_turn);go_live_cells[p]=go_turn;go_history[go_count++]=p;
 if(go_wins_at(p,go_turn))go_winner=go_turn;else if(go_count==225)go_winner=3;go_turn=3-go_turn;return 1;
}
void go_undo(uint8_t n) BANKED {if(n>go_count)n=go_count;while(n--){go_set(go_history[--go_count],0);go_live_cells[go_history[go_count]]=0;go_turn=3-go_turn;}go_winner=0;}

void go_undo_pair(void) BANKED {go_undo(2);}
