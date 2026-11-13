#pragma bank 255
#include <string.h>
#include "states/tt_rivals.h"
#ifndef TT_HOST
#include <gb/gb.h>
#include <gb/cgb.h>
#endif
#include "states/tt_openings.h"
#include "states/tt_dialogue.h"
typedef struct {char name[6];uint16_t rating;uint8_t style[7],favorites[3];char role[19];} Rival;
static const Rival rivals[10]={
 {"PIP",600,{19,12,2,90,3,28,0},{0,5,12},"Quick & careless"},
 {"RUSH",780,{28,12,2,48,4,16,1},{1,6,18},"All-out attacker"},
 {"MARA",900,{12,29,2,34,5,12,1},{3,14,20},"Stubborn wall"},
 {"JUNE",1020,{20,20,2,28,5,10,1},{2,9,17},"Curious explorer"},
 {"VEX",1120,{27,18,3,24,6,8,1},{7,15,23},"Sets sly traps"},
 {"ORIN",1240,{17,25,3,18,6,6,1},{4,10,19},"Patient guardian"},
 {"NOVA",1380,{22,21,3,14,7,4,2},{8,13,22},"Opportunist"},
 {"ROOK",1500,{24,22,4,10,7,3,2},{6,16,21},"Tactical planner"},
 {"IRIS",1640,{21,25,4,7,8,2,2},{5,11,20},"Calm & balanced"},
 {"SOREN",1800,{24,25,4,3,9,1,2},{12,18,23},"The grandmaster"}
};
static const char memory_lines[10][21]={"You won last time!","I remember that loss","You broke my wall.","Last time was yours.","You read me before.","Let us revisit it.","You shone last time.","A new plan this time","You found the shape.","Your last win stays."};
static const uint8_t elo_expected[17]={128,110,92,76,61,49,39,30,23,18,14,10,8,6,4,3,3};
uint8_t go_book_transform,rival_rematch;
static uint16_t last_reaction;
void rival_style(uint8_t id,uint8_t *out) BANKED {memcpy(out,rivals[id%10].style,7);}
uint16_t rival_rating(uint8_t id) BANKED {return rivals[id%10].rating;}
uint8_t rival_match(void) BANKED {
 uint8_t id,chosen=0,n=0;uint16_t best=65535,d,total=0,weight;uint8_t candidates[10];
 for(id=0;id<10;id++){d=rivals[id].rating>tt_save.rating?rivals[id].rating-tt_save.rating:tt_save.rating-rivals[id].rating;if(d<=150)candidates[n++]=id;if(d<best){best=d;chosen=id;}}
 if(!n)return chosen;
 for(id=0;id<n;id++){uint8_t k=candidates[id];if(n>1&&k==tt_save.last_rival)continue;d=rivals[k].rating>tt_save.rating?rivals[k].rating-tt_save.rating:tt_save.rating-rivals[k].rating;weight=151-(d>>1);total+=weight;if(tt_random_below(total)<weight)chosen=k;}
 return chosen;
}
void rival_new_match(void) BANKED {go_opening=255;go_book_transform=tt_random_below(8);rival_rematch=0;last_reaction=tt_clock-120;}
static uint16_t book_tries;
static uint8_t book_id,book_ply,book_remember,book_reply;
static int8_t book_ax,book_ay;
void rival_book_begin(uint8_t id) BANKED {
 uint8_t first=255;RivalRecord *record=&tt_save.rivals[id%10];
 book_id=id%10;book_ply=go_count;book_tries=0;book_ax=book_ay=7;book_reply=book_ply==(go_player_color==1?1:2);
 if(book_reply&&tt_save.rival_reply_bag[book_id][0]==255&&tt_save.rival_reply_bag[book_id][1]==255&&tt_save.rival_reply_bag[book_id][2]==255)memset(tt_save.rival_reply_bag[book_id],0,3);
 if(go_count){book_ax=go_history[0]%15;book_ay=go_history[0]/15;first=go_history[go_player_color==1?0:go_count>1?1:0];}
 book_remember=record->result==1&&first==tt_save.rival_first[book_id];
 if(go_opening==255){go_opening=tt_random_below(100)<65?rivals[book_id].favorites[tt_random_below(3)]:tt_random_below(24);if(record->result==1&&go_opening==record->opening)go_opening=(go_opening+1+tt_random_below(23))%24;if(book_remember)go_book_transform=(tt_save.rival_transform[book_id]+1+tt_random_below(7))%8;}
}
uint8_t rival_book_step(void) BANKED {
 uint8_t n,p,slot;int8_t x,y,t;
 if(book_ply>=6)return 255;
 for(n=0;n<8&&book_tries<384;n++,book_tries++){
  x=opening_book[go_opening][book_ply][0];y=opening_book[go_opening][book_ply][1];if(go_book_transform&4)x=-x;
  for(t=0;t<(go_book_transform&3);t++){int8_t a=x;x=-y;y=a;}x+=book_ax;y+=book_ay;
  if(x>=0&&x<15&&y>=0&&y<15){p=y*15+x;if(!go_cells[p]&&(!book_remember||p!=tt_save.rival_reply[book_id])){
    if(book_reply){slot=(y-book_ay+2)*5+x-book_ax+2;if(slot>12)slot--;if(slot<24){if(tt_save.rival_reply_bag[book_id][slot>>3]&(1<<(slot&7)))goto next_shape;tt_save.rival_reply_bag[book_id][slot>>3]|=1<<(slot&7);}}
    return p;
   }}
  next_shape:go_book_transform=(go_book_transform+1)&7;if((book_tries&7)==7)go_opening=(go_opening+1)%24;if(book_tries==191)memset(tt_save.rival_reply_bag[book_id],0,3);
 }
 return book_tries>=384?255:254;
}
uint8_t rival_book_move(uint8_t id) BANKED {uint8_t p;rival_book_begin(id);do{p=rival_book_step();}while(p==254);return p;}
char rival_msg[2][21];
uint8_t rival_msg_seq,rival_mood;
static const uint8_t event_mood[8]={1,2,1,2,0,3,1,1};
static void copy_line(char *dst,const char *src){uint8_t i=0;while(i<20&&src[i]){dst[i]=src[i];i++;}dst[i]=0;}
void rival_info(uint8_t id,char *name,char *role) BANKED {id%=10;copy_line(name,rivals[id].name);copy_line(role,rivals[id].role);}
void rival_line(uint8_t event) BANKED {
 uint8_t id=go_rival%10;event&=7;
 if(event==0&&tt_save.rivals[id].result==1){copy_line(rival_msg[0],memory_lines[id]);rival_msg[1][0]=0;rival_mood=0;}
 else{copy_line(rival_msg[0],reactions[id][event][0]);copy_line(rival_msg[1],reactions[id][event][1]);rival_mood=event_mood[event];}
 rival_msg_seq++;last_reaction=tt_clock;
}
void rival_react(uint8_t color) BANKED {
 uint8_t p,winning=0,pressure=0;uint16_t bits;
 if((uint16_t)(tt_clock-last_reaction)<60)return;
 for(p=0;p<225;p++)if(!go_cells[p]){bits=go_patterns[color-1][p];if(bits&(bits>>1)&0x2222)winning++;if(bits&0x4444)pressure=1;}
 if(winning>=2)rival_line(1);else if(pressure)rival_line(3);else if(tt_random_below(100)<18)rival_line(4);
}
int16_t rival_elo_delta(uint16_t player,uint16_t opponent,uint8_t result) BANKED {
 uint16_t diff=player>opponent?player-opponent:opponent-player,expected,idx,frac;int16_t error;
 if(diff>800)diff=800;idx=diff/50;frac=diff%50;expected=elo_expected[idx];if(idx<16)expected-=((elo_expected[idx]-elo_expected[idx+1])*frac)/50;
 if(player>opponent)expected=256-expected;error=(result==1?256:result==3?128:0)-(int16_t)expected;
 return error>=0?(error+4)/8:-((-error+4)/8);
}
void rival_record_result(void) BANKED {
 uint8_t id=go_rival%10,result=go_winner==3?3:go_winner==go_player_color?1:2,first,reply;int16_t delta;RivalRecord *r=&tt_save.rivals[id];
 if(result==1&&r->wins<65535)r->wins++;if(result==2&&r->losses<65535)r->losses++;r->result=result;r->opening=go_opening;
 first=go_player_color==1?0:1;reply=go_player_color==1?1:2;tt_save.rival_first[id]=go_count>first?go_history[first]:255;tt_save.rival_reply[id]=go_count>reply?go_history[reply]:255;tt_save.rival_transform[id]=go_book_transform;
 if(go_mode==GO_RANKED){delta=rival_elo_delta(tt_save.rating,rivals[id].rating,result);tt_save.rating+=delta;if(tt_save.rating<100)tt_save.rating=100;if(tt_save.rating>2500)tt_save.rating=2500;}
 rival_rematch=result==1&&tt_random_below(100)<(id==0?75:id==1?65:45);tt_save_request();
}
