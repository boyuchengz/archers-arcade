#pragma bank 255
#include "states/tabletop.h"
#include <string.h>
#include <stddef.h>
#ifndef TT_HOST
#include <gb/gb.h>
#define SAVE_RAM ((volatile uint8_t *)0xA000u)
#else
extern uint8_t test_sram[8192];
#define SAVE_RAM test_sram
#define ENABLE_RAM_MBC5 ((void)0)
#define SWITCH_RAM_MBC5(n) ((void)0)
#endif
SaveData tt_save;
uint8_t tt_dirty,tt_saving,tt_save_available;
static uint8_t snapshot[sizeof(SaveData)],active_slot;
static uint16_t generation,write_pos,write_crc;
static uint8_t write_stage;
static uint16_t crc_byte(uint16_t c,uint8_t b){uint8_t k;c^=(uint16_t)b<<8;for(k=0;k<8;k++)c=(c&0x8000)?(c<<1)^0x1021:c<<1;return c;}
static uint16_t ram_crc(uint16_t off,uint16_t n){uint16_t i,c=0xFFFF;for(i=0;i<n;i++)c=crc_byte(c,SAVE_RAM[off+12+i]);return c;}
static uint8_t valid(uint16_t off){
 uint16_t c,n;if(SAVE_RAM[off]!='A'||SAVE_RAM[off+1]!='T'||SAVE_RAM[off+2]!=1||SAVE_RAM[off+3]!=0xA5)return 0;
 n=SAVE_RAM[off+6]|((uint16_t)SAVE_RAM[off+7]<<8);if(n!=sizeof(SaveData)&&n!=offsetof(SaveData,tw_best)&&n!=offsetof(SaveData,rival_first))return 0;
 c=SAVE_RAM[off+8]|((uint16_t)SAVE_RAM[off+9]<<8);return c==ram_crc(off,n);
}
void tt_defaults(void) BANKED {memset(&tt_save,0,sizeof(tt_save));tt_save.rating=1000;tt_save.rng=0x8A751E43;tt_save.sound=1;tt_save.last_rival=255;memset(tt_save.rival_first,255,10);memset(tt_save.rival_reply,255,10);}
uint32_t tt_random(void) BANKED {uint32_t x=tt_save.rng;if(!x)x=0x9e3779b9;x^=x<<13;x^=x>>17;x^=x<<5;tt_save.rng=x;return x;}
uint16_t tt_random_below(uint16_t n) BANKED {uint16_t v,threshold;if(!n)return 0;threshold=(uint16_t)(0-n)%n;do{v=(uint16_t)tt_random();}while(v<threshold);return v%n;}
void tt_entropy(uint8_t buttons,uint8_t div,uint16_t frame) BANKED {
 tt_save.rng^=((uint32_t)frame<<16)|((uint16_t)div<<8)|buttons;tt_random();tt_dirty=1;
}
void tt_save_init(void) BANKED {
 uint8_t a,b,old0,old1;uint16_t g0,g1,off,i,n;ENABLE_RAM_MBC5;SWITCH_RAM_MBC5(0);
 old0=SAVE_RAM[8190];old1=SAVE_RAM[8191];SAVE_RAM[8190]=0x35;SAVE_RAM[8191]=0xCA;
 tt_save_available=(SAVE_RAM[8190]==0x35&&SAVE_RAM[8191]==0xCA);
 SAVE_RAM[8190]=0xCA;SAVE_RAM[8191]=0x35;tt_save_available&=(SAVE_RAM[8190]==0xCA&&SAVE_RAM[8191]==0x35);
 SAVE_RAM[8190]=old0;SAVE_RAM[8191]=old1;tt_defaults();active_slot=1;generation=0;
 if(!tt_save_available)return;a=valid(0);b=valid(2048);if(!a&&!b)return;
 g0=SAVE_RAM[4]|((uint16_t)SAVE_RAM[5]<<8);g1=SAVE_RAM[2052]|((uint16_t)SAVE_RAM[2053]<<8);
 active_slot=b&&(!a||(int16_t)(g1-g0)>0);off=active_slot?2048:0;generation=active_slot?g1:g0;
 n=SAVE_RAM[off+6]|((uint16_t)SAVE_RAM[off+7]<<8);for(i=0;i<n;i++)((uint8_t *)&tt_save)[i]=SAVE_RAM[off+12+i];
 if(tt_save.rating<100||tt_save.rating>2500)tt_save.rating=1000;
 tt_dirty=tt_saving=write_stage=0;
}
void tt_save_request(void) BANKED {tt_dirty=1;}
void tt_save_tick(void) BANKED {
 uint8_t n=32;uint16_t off;
 if(!tt_save_available){tt_dirty=0;return;}ENABLE_RAM_MBC5;SWITCH_RAM_MBC5(0);
 if(!tt_saving){if(!tt_dirty)return;memcpy(snapshot,&tt_save,sizeof(SaveData));tt_dirty=0;tt_saving=1;write_pos=0;write_crc=0xFFFF;write_stage=0;}
 off=active_slot?0:2048;
 if(!write_stage){SAVE_RAM[off+3]=0;write_stage=1;}
 while(n--&&write_pos<sizeof(SaveData)){uint8_t v=snapshot[write_pos];SAVE_RAM[off+12+write_pos]=v;write_crc=crc_byte(write_crc,v);write_pos++;}
 if(write_pos==sizeof(SaveData)){
  generation++;SAVE_RAM[off]='A';SAVE_RAM[off+1]='T';SAVE_RAM[off+2]=1;
  SAVE_RAM[off+4]=(uint8_t)generation;SAVE_RAM[off+5]=generation>>8;SAVE_RAM[off+6]=sizeof(SaveData)&255;SAVE_RAM[off+7]=sizeof(SaveData)>>8;
  SAVE_RAM[off+8]=write_crc&255;SAVE_RAM[off+9]=write_crc>>8;SAVE_RAM[off+3]=0xA5;
  active_slot=!active_slot;tt_saving=0;
 }
}
void tt_save_reset(void) BANKED {tt_defaults();tt_dirty=1;}
