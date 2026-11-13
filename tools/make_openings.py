"""Opening book for the rivals: 24 shapes, distinct under rotation/mirroring."""
from pathlib import Path
r = Path(__file__).resolve().parents[1]
# 24 opening shapes, canonicalized under rotations and mirrors.
import random
random.seed(9182);book=[];seen=set()
def transforms(points):
 for mirror in(1,-1):
  for rot in range(4):
   a=[]
   for x,y in points:
    x*=mirror
    for _ in range(rot):x,y=-y,x
    a.append((x,y))
   yield tuple(a)
while len(book)<24:
 points=[(0,0),random.choice([(1,0),(1,1),(2,0),(2,1),(2,2)])]
 while len(points)<6:
  p=(random.randrange(-2,3),random.randrange(-2,3))
  if p not in points:points.append(p)
 key=min(transforms(points))
 if key in seen:continue
 seen.add(key);book.append(points)
text='static const int8_t opening_book[24][6][2]={'+',\n'.join('{'+','.join('{%d,%d}'%p for p in a)+'}' for a in book)+'};\n'
(r/'plugins/tabletop/engine/include/states/tt_openings.h').write_text(text)
print('24 opening shapes distinct under rotation/mirroring.')
