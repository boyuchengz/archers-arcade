#pragma bank 255
#include <string.h>
#include "states/tt_rivals.h"
#define INF 3000000L
#define PH_GUARD 0
#define PH_VCF 1
#define PH_SEARCH 2
#define PH_UNWIND 3
#define PH_WAIT 4
#define MAX_TOP 8
uint8_t ai_done,ai_move,ai_resign,ai_hint,ai_cancelled;
uint16_t ai_started,ai_finished,ai_nodes,ai_max_elapsed,ai_max_slice,ai_visible_at,ai_max_visible;
uint8_t ai_completed_depth;
static const uint8_t three_pairs[256]={0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0};
typedef struct {int32_t alpha,beta,best;uint8_t moves[8],n,next,remaining,side,parent_move,terminal,ready;} SearchFrame;
typedef struct {uint8_t moves[8],n,next,move,block,stage;} ForceFrame;
typedef struct {
 union {SearchFrame search[5];ForceFrame force[5];} frames;
 uint8_t root[8],n,valid,iter_valid;int32_t root_score[8],iter_score[8];
 uint8_t temp_moves[8];uint16_t temp_scores[8];
} AIWork;
extern uint8_t tt_scratch[512];
#define W ((AIWork *)tt_scratch)
static uint8_t phase,side,profile,sp,iteration,force_side,force_pass,force_success,force_root,stop_mode;
static uint8_t force_return,force_target,attack_w,defense_w,max_depth,temperature,radius,lapse,pace;
static uint8_t unwind_vcf,unwind_stage;
static uint16_t ready_at,deadline,hint_at;
static uint8_t hint_index,guard_stage,gen_active,gen_pos,gen_n,gen_turn,gen_forced,gen_human;
static uint8_t *gen_out,*gen_count;
static int32_t returned;
static uint8_t has(uint16_t bits,uint8_t category){uint8_t d;if(category==6)return (bits&(bits>>1)&0x2222)!=0;if(category==4)return (bits&0x4444)!=0;for(d=0;d<4;d++){if((bits&15)>=category)return 1;bits>>=4;}return 0;}
static uint8_t near_last(uint8_t p){int8_t x=p%15,y=p/15,dx,dy;uint8_t i,q;for(i=0;i<2&&i<go_count;i++){q=go_history[go_count-1-i];dx=x-(q%15);dy=y-(q/15);if(dx<0)dx=-dx;if(dy<0)dy=-dy;if((uint8_t)dx<=radius&&(uint8_t)dy<=radius)return 1;}return !go_count;}
static int32_t ordering(uint8_t p,uint8_t turn,uint8_t human){
 uint16_t a=go_strength(turn-1)[p],b=go_strength(2-turn)[p];int32_t v;
 if(profile==4){uint16_t bits=go_patterns[turn-1][p];if(three_pairs[(uint8_t)bits]+three_pairs[bits>>8]>=2)a+=80;}v=a*attack_w+b*defense_w;
 if(human&&!has(go_patterns[turn-1][p],4)&&!has(go_patterns[2-turn][p],4)&&!near_last(p)&&tt_random_below(100)<lapse)v>>=3;
 return v;
}
static void candidate_begin(uint8_t *out,uint8_t turn,uint8_t forced,uint8_t human,uint8_t *count){gen_out=out;gen_count=count;gen_turn=turn;gen_forced=forced;gen_human=human;gen_n=gen_pos=0;gen_active=1;*count=0;}
static void candidate_tick(void){
 uint8_t p,i,j,end;uint16_t v;
 if(!go_count&&!go_cells[112]){gen_out[0]=112;*gen_count=1;gen_active=0;return;}
 end=gen_pos>201?225:gen_pos+24;
 while(gen_pos<end){p=gen_pos++;if(go_cells[p])continue;if(gen_forced&&!has(go_patterns[gen_turn-1][p],4))continue;
  if(!gen_forced&&!go_patterns[0][p]&&!go_patterns[1][p])continue;
  v=ordering(p,gen_turn,gen_human);i=0;while(i<gen_n&&W->temp_scores[i]>=v)i++;if(i>=8)continue;
  if(gen_n<8)gen_n++;for(j=gen_n-1;j>i;j--){W->temp_scores[j]=W->temp_scores[j-1];gen_out[j]=gen_out[j-1];}W->temp_scores[i]=v;gen_out[i]=p;
 }
 if(gen_pos==225){if(!gen_n&&!gen_forced)for(p=0;p<225;p++)if(!go_cells[p]){gen_out[gen_n++]=p;break;}*gen_count=gen_n;gen_active=0;}
}
static int32_t position_value(uint8_t turn){uint8_t p,best_a=0,best_b=0,a,b;uint8_t *mine=go_strength(turn-1),*other=go_strength(2-turn);for(p=0;p<225;p++)if(!go_cells[p]){a=mine[p];b=other[p];if(a>best_a)best_a=a;if(b>best_b)best_b=b;}return (int16_t)best_a-best_b;}
static uint8_t book_safe(void){uint8_t p;for(p=0;p<225;p++)if(!go_cells[p]&&(has(go_patterns[0][p],4)||has(go_patterns[1][p],4)))return 0;return 1;}
static uint8_t winning_points(uint8_t turn,uint8_t *one){uint8_t p,n=0;for(p=0;p<225;p++)if(!go_cells[p]&&has(go_patterns[turn-1][p],6)){*one=p;if(++n>=2)return n;}return n;}
static void start_wait(void){phase=PH_WAIT;}
static void choose_soft(void){
 uint8_t i,n=W->valid?W->n:W->iter_valid,chosen=0;int32_t best=-INF,diff,margin=temperature>>1;uint16_t total=0,weight;
 if(!n){ai_move=W->root[0];return;}
 for(i=0;i<n;i++){int32_t v=W->valid?W->root_score[i]:W->iter_score[i];if(v>best){best=v;chosen=i;}}
 for(i=0;i<n;i++){int32_t v=W->valid?W->root_score[i]:W->iter_score[i];diff=best-v;if(diff>margin)continue;weight=1+(uint16_t)(margin-diff);total+=weight;if(tt_random_below(total)<weight)chosen=i;}
 ai_move=W->root[chosen];
}
static void search_init(void){
 SearchFrame *f;sp=0;iteration=2;memset(&W->frames,0,sizeof(W->frames));f=&W->frames.search[0];f->remaining=iteration;f->side=side;f->alpha=-INF;f->beta=INF;f->best=-INF;f->n=W->n;memcpy(f->moves,W->root,W->n);f->ready=1;phase=PH_SEARCH;W->iter_valid=0;
}
static void force_init(uint8_t turn){
 ForceFrame *f;sp=0;force_side=turn;force_success=0;force_return=0;memset(&W->frames,0,sizeof(W->frames));f=&W->frames.force[0];candidate_begin(f->moves,turn,1,0,&f->n);phase=PH_VCF;
}
static void finish_force(void){
 if(force_success){
  if(force_side==side){ai_move=force_root;start_wait();return;}
  /* Opponent-to-move proof is a defensive hint, not proof we must resign now. */
  {uint8_t j,k=255;for(j=0;j<W->n;j++)if(W->root[j]==force_root)k=j;
   if(k==255){if(W->n<8)W->n++;k=W->n-1;}for(j=k;j;j--)W->root[j]=W->root[j-1];W->root[0]=force_root;}
  rival_line(3);search_init();return;
 }
 if(!force_pass){force_pass=1;force_init(3-side);}else search_init();
}
static void force_step(void){
 ForceFrame *f=&W->frames.force[sp];uint8_t p,count,other,one=255;
 if(force_return){
  if(f->stage==4){go_set(f->block,0);f->stage=5;return;}
  if(f->stage==5||f->stage==1){go_set(f->move,0);f->stage=0;if(force_success){if(!sp){finish_force();return;}sp--;force_return=1;return;}force_return=0;return;}
 }
 if(f->stage==0){
  if(f->next>=f->n){if(!sp){finish_force();return;}sp--;force_return=1;return;}
  p=f->moves[f->next++];f->move=p;f->stage=1;if(!sp)force_root=p;go_set(p,force_side);ai_nodes++;return;
 }
 if(f->stage==1){
  count=winning_points(force_side,&one);other=winning_points(3-force_side,&p);
  if(go_wins_at(f->move,force_side)||(!other&&count>=2)){force_success=1;force_return=1;return;}
  if(other||count!=1||sp>=3){force_return=1;return;}f->block=one;f->stage=2;
 }
 if(f->stage==2){go_set(f->block,3-force_side);f->stage=4;sp++;f=&W->frames.force[sp];memset(f,0,sizeof(*f));candidate_begin(f->moves,force_side,1,0,&f->n);return;}
}
static void complete_node(int32_t result){
 SearchFrame *f=&W->frames.search[sp],*parent;uint8_t move;
 if(!sp){
  memcpy(W->root_score,W->iter_score,sizeof(W->root_score));W->valid=1;ai_completed_depth=iteration;
  if(iteration>=max_depth){choose_soft();start_wait();return;}iteration++;memset(&W->frames,0,sizeof(W->frames));f=&W->frames.search[0];f->side=side;f->remaining=iteration;f->alpha=-INF;f->beta=INF;f->best=-INF;f->n=W->n;memcpy(f->moves,W->root,W->n);f->ready=1;W->iter_valid=0;return;
 }
 move=f->parent_move;go_set(move,0);sp--;parent=&W->frames.search[sp];result=-result;
 if(!sp){W->iter_score[parent->next-1]=result;W->iter_valid=parent->next;}
 if(result>parent->best)parent->best=result;if(result>parent->alpha)parent->alpha=result;
 if(parent->alpha>=parent->beta)parent->next=parent->n;
}
static void search_step(void){
 SearchFrame *f=&W->frames.search[sp],*child;uint8_t move,win;
 if(f->terminal){complete_node(-INF+1000*(4-f->remaining));return;}
 if(!f->remaining){complete_node(position_value(f->side));return;}
 if(!f->ready){candidate_begin(f->moves,f->side,0,0,&f->n);f->ready=1;return;}
 if(f->next>=f->n){complete_node(f->best==-INF?position_value(f->side):f->best);return;}
 move=f->moves[f->next++];win=has(go_patterns[f->side-1][move],6);go_set(move,f->side);ai_nodes++;
 sp++;child=&W->frames.search[sp];memset(child,0,sizeof(*child));child->remaining=f->remaining-1;child->side=3-f->side;child->alpha=-f->beta;child->beta=-f->alpha;child->best=-INF;child->parent_move=move;child->terminal=win;
}
static void unwind_step(void){
 if(unwind_vcf){ForceFrame *f=&W->frames.force[sp];
  if(f->stage>=2&&!unwind_stage){go_set(f->block,0);unwind_stage=1;return;}
  if(f->stage>=1){go_set(f->move,0);f->stage=0;unwind_stage=0;return;}
  if(sp){sp--;return;}
 }else if(sp){go_set(W->frames.search[sp].parent_move,0);sp--;return;}
 if(!ai_cancelled)choose_soft();start_wait();
}
void ai_cancel(void) BANKED {if(ai_done)return;ai_cancelled=1;gen_active=0;unwind_vcf=phase==PH_VCF;unwind_stage=0;phase=PH_UNWIND;ready_at=tt_clock;}
void ai_delay(uint16_t frames) BANKED {ai_started+=frames;ready_at+=frames;deadline+=frames;}
void ai_begin(uint8_t color,uint8_t rival) BANKED {
 uint8_t style[7];side=color;profile=rival%10;rival_style(profile,style);attack_w=style[0];defense_w=style[1];max_depth=style[2];temperature=style[3];radius=style[4];lapse=style[5];pace=style[6];
 ai_done=ai_resign=ai_cancelled=ai_completed_depth=0;ai_nodes=0;ai_started=tt_clock;hint_at=tt_clock+18;hint_index=0;deadline=ai_started+140;ready_at=ai_started+60+tt_random_below(51+pace*20);phase=PH_GUARD;sp=force_pass=guard_stage=gen_active=0;memset(W,0,sizeof(AIWork));ai_hint=112;ai_move=112;
}
void ai_tick(void) BANKED {
 uint8_t own,opponent,one=255,i,book;uint16_t now;
 if(ai_done)return;
#ifdef TT_HOST
 now=tt_clock;
#else
 now=sys_time;
#endif
 if(phase<PH_UNWIND&&(int16_t)(now-deadline)>=0){gen_active=0;unwind_vcf=phase==PH_VCF;unwind_stage=0;phase=PH_UNWIND;}
 if(gen_active)candidate_tick();
 else if(phase==PH_GUARD){
  if(guard_stage==0){own=winning_points(side,&one);if(own){rival_line(2);ai_move=one;ai_hint=one;ready_at=ai_started+18+tt_random_below(23);start_wait();}else guard_stage=1;}
  else if(guard_stage==1){opponent=winning_points(3-side,&one);if(opponent){ai_move=one;ai_hint=one;if(opponent>=2){ai_resign=1;rival_line(3);}ready_at=ai_started+18+tt_random_below(23);start_wait();}else guard_stage=2;}
  else if(guard_stage==2){if(go_count<6&&book_safe()){rival_book_begin(profile);guard_stage=3;}else{candidate_begin(W->root,side,0,1,&W->n);guard_stage=4;}}
  else if(guard_stage==3){book=rival_book_step();if(book<225){ai_move=ai_hint=book;start_wait();}else if(book==255){candidate_begin(W->root,side,0,1,&W->n);guard_stage=4;}}
  else {ai_move=W->root[0];ai_hint=ai_move;for(i=0;i<W->n;i++)W->iter_score[i]=ordering(W->root[i],side,0);W->iter_valid=W->n;force_init(side);}
 }else if(phase==PH_VCF)force_step();else if(phase==PH_SEARCH)search_step();else if(phase==PH_UNWIND)unwind_step();
 if(W->n&&(int16_t)(now-hint_at)>=0){if(++hint_index>=W->n)hint_index=0;ai_hint=W->root[hint_index];hint_at=now+18;}
 if(phase==PH_WAIT&&(ai_cancelled||(int16_t)(now-ready_at)>=0)){
  ai_done=1;ai_finished=now;if((uint16_t)(now-ai_started)>ai_max_elapsed)ai_max_elapsed=now-ai_started;
  if(!ai_resign&&!ai_cancelled&&(ai_move>=225||go_cells[ai_move])){for(i=0;i<225;i++)if(!go_cells[i]){ai_move=i;break;}}
 }
#ifndef TT_HOST
 {uint16_t cost=sys_time-now;if(cost>ai_max_slice)ai_max_slice=cost;}
#endif

}
