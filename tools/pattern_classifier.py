def won(a):
 return any(all(a[j]==1 for j in range(i,i+5)) for i in range(5) if i<=4<i+5)
def win_spots(a):
 r=[]
 for i,v in enumerate(a):
  if v:continue
  a[i]=1
  if won(a):r.append(i)
  a[i]=0
 return r
def classify(a):
 if won(a):return 6
 w=win_spots(a)
 if len(w)>1:return 5
 if w:return 4
 # An open three is one move away from a genuine two-ended four.
 for i,v in enumerate(a):
  if v:continue
  a[i]=1;n=len(win_spots(a));a[i]=0
  if n>1:return 3
 best=0
 for start in range(5):
  if not(start<=4<start+5):continue
  b=a[start:start+5]
  if 2 in b:continue
  n=b.count(1)
  if n>=3:best=max(best,2)
  elif n>=2 and (a[max(0,start-1)]==0 or a[min(8,start+5)]==0):best=max(best,1)
 return best
