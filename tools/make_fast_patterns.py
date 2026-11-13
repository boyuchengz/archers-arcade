from pathlib import Path
import runpy
# Reuse the independently tested line classifier without importing its output side effects.
s=Path(__file__).with_name('pattern_classifier.py').read_text();ns={};exec(s,ns);classify=ns['classify']
r=Path(__file__).resolve().parents[1]/'plugins/tabletop/engine'
values=[]
for code in range(65536):
 a=[(code>>(2*i))&3 for i in range(8)];a=a[:4]+[0]+a[4:]
 packed=0
 for color in (1,2):
  line=[0 if v==0 else 1 if v==color else 2 for v in a];line[4]=1;packed|=classify(line)<<((color-1)*4)
 values.append(packed)
for page in range(8):
 a=values[page*8192:(page+1)*8192]
 text='#pragma bank 255\n#include "states/tt_gomoku.h"\nstatic const uint8_t patterns_page_'+str(page)+'[8192]={\n'
 text+=',\n'.join(','.join(map(str,a[i:i+64])) for i in range(0,len(a),64))+'\n};\n'
 text+=f'uint8_t pattern_page_{page}(uint16_t code) BANKED {{return patterns_page_{page}[code];}}\n'
 (r/f'src/states/tt_pattern{page}.c').write_text(text)
text='#pragma bank 255\n#include "states/tt_gomoku.h"\n'
for page in range(8):text+=f'extern uint8_t pattern_page_{page}(uint16_t code) BANKED;\n'
text+='uint8_t patterns_lookup(uint16_t code) BANKED {switch(code>>13){\n'
for page in range(8):text+=f'case {page}:return pattern_page_{page}(code&8191);\n'
text+='}return 0;}\n';(r/'src/states/tt_patterns.c').write_text(text)
base=[]
for p in range(225):
 x,y=p%15,p//15;codes=[]
 for dx,dy in [(1,0),(0,1),(1,1),(1,-1)]:
  c=0;i=0
  for n in range(-4,5):
   if n==0:continue
   if not(0<=x+dx*n<15 and 0<=y+dy*n<15):c|=3<<(2*i)
   i+=1
  codes.append(c)
 base.append(codes)
(r/'include/states/tt_border_codes.h').write_text('static const uint16_t initial_line_codes[225][4]={\n'+',\n'.join('{'+','.join(map(str,b))+'}' for b in base)+'\n};\n')
print('Generated 65536 packed two-color pattern classifications in eight 8KiB pages; precomputed all border masks.')
