"""ROM acceptance playthrough. Controls only; memory is observed, never patched."""
from emulator import Emulator
from pathlib import Path
from collections import deque
import ctypes, json, subprocess
OUT=Path('validation04');OUT.mkdir(exist_ok=True)
subprocess.run(['cc','-shared','-fPIC','-O2','-Iinclude','src/pokewilds/world.c','-o',str(OUT/'world.so')],check=True)
world=ctypes.CDLL(str(OUT.resolve()/'world.so'));world.PwWorld_BaseTile.restype=ctypes.c_ubyte
save=OUT/'acceptance.sav';save.unlink(missing_ok=True)
e=Emulator('../engine/pokeemerald.gba',save)
report={'input_method':'GBA buttons only; read-only RAM observations','checks':[],'movement':[]}
dirs=[(0,-1,'UP'),(0,1,'DOWN'),(-1,0,'LEFT'),(1,0,'RIGHT')]
passable={0,3,6,7,8,10,13,15,16,18}
def p16(a):return ctypes.c_int16(e.read(a,2)).value
def pos():return p16(e.symbols['gSaveblock3']+16),p16(e.symbols['gSaveblock3']+18)
def sv(o,n=2):return e.read(e.symbols['gSaveblock3']+o,n)
def ui():return e.read('sUi',1)
def check(label,ok=True):
 assert ok,label
 report['checks'].append(label);print('PASS',label,flush=True)
def snap(n):e.screenshot(OUT/(n+'.png'))
def actors():
 out=[]
 for i in range(5):
  a=e.symbols['sWild']+20*i
  if e.read(a+7,1):out.append({'i':i,'x':p16(a),'y':p16(a+2),'sp':e.read(a+4,2),'friendly':e.read(a+11,1)})
 return out
def edits():
 out={}
 for i in range(sv(28)):
  a=e.symbols['gSaveblock3']+166+i*6
  out[(p16(a),p16(a+2))]=e.read(a+4,1)
 return out
def tile(x,y):return edits().get((x,y),world.PwWorld_BaseTile(x,y))
def occupied():
 out={(a['x'],a['y']) for a in actors()}
 for i in range(5):
  a=e.symbols['sWild']+20*i
  if e.read(a+7,1) and e.read(a+16,1):
   dx=p16(a+12);dy=p16(a+14)
   out.add((p16(a)+(dx>0)-(dx<0),p16(a+2)+(dy>0)-(dy<0)))
 for i in range(6):
  a=e.symbols['gSaveblock3']+1332+i*8
  if e.read(a+6,1):out.add((p16(a),p16(a+2)))
 return out
def camera():
 # GBA scroll registers are write-only. Observe the engine's register buffer,
 # not open-bus values returned by reads of the hardware addresses.
 a=e.symbols['sGpuRegBuffer']
 return e.read(a+0x18,2),e.read(a+0x1A,2)
def step(key,run=False,blocked=False):
 old=pos();ticks=e.read('sFrames');cam=camera();started=False;delta=[]
 for f in range(90):
  e.run(1,(key+('+B' if run else '')) if not started else 0)
  assert not e.read('sBattleReturning',1),('unexpected battle',pos())
  newcam=camera()
  for a,b,mod in zip(cam,newcam,(512,256)):
   d=min((a-b)%mod,(b-a)%mod);delta.append(d)
   assert d<=(4 if run else 2),('camera jumped',old,pos(),cam,newcam,d)
  cam=newcam
  if pos()!=old:started=True
  if blocked and f>=2:break
  if started and not e.read('sMoveFrames',1) and e.read('sFrames')>ticks+1:break
 e.run(2)
 if blocked:assert pos()==old,('expected blocked tile',old,pos(),key)
 else:
  assert started,('blocked movement',old,key,actors())
  report['movement'].append({'start':old,'end':pos(),'frames':f+1,'max_camera_delta':max(delta),'running':run})
def walk(goal,run=False):
 for attempt in range(600):
  start=pos()
  if start==goal:return
  obstacles=occupied();cells=edits();q=deque([(start,[])]);seen={start};route=None
  for _ in range(18000):
   if not q:break
   p,path=q.popleft()
   if p==goal:route=path;break
   for dx,dy,k in dirs:
    n=(p[0]+dx,p[1]+dy)
    if n in seen or n in obstacles or abs(n[0]-start[0])>65 or abs(n[1]-start[1])>65:continue
    if cells.get(n,world.PwWorld_BaseTile(*n)) not in passable:continue
    seen.add(n);q.append((n,path+[k]))
  if route is None and goal in occupied():
   e.run(90);continue  # A wandering creature may temporarily occupy the goal.
  assert route,('no route',start,goal)
  step(route[0],run)
 raise AssertionError('navigation timed out')
def menu(row):
 assert ui()==2,('menu source',ui())
 e.tap('START')
 for _ in range(row):e.tap('DOWN')
 e.tap('A',30)
def back_world():
 for _ in range(3):
  if ui()==2:return
  e.tap('B')
 assert ui()==2

def recruit(sp):
 a=next(a for a in actors() if a['sp']==sp)
 for dx,dy,k in dirs:
  stand=(a['x']-dx,a['y']-dy)
  if tile(*stand) in passable and stand not in occupied():break
 walk(stand);step(k,blocked=True);before=e.read('gPartiesCount',1)
 e.tap('A');assert ui()==13
 snap('03-befriend-'+str(sp));e.tap('A')
 check('befriend species '+str(sp),e.read('gPartiesCount',1)==before+1 and ui()==2)
def setup_facing(target,key):
 dx,dy=next((x,y) for x,y,k in dirs if k==key)
 stand=(target[0]-dx,target[1]-dy);behind=(stand[0]-dx,stand[1]-dy)
 walk(behind);step(key);assert pos()==stand

def drop(index,target):
 setup_facing(target,'DOWN');menu(0)
 for _ in range(index):e.tap('DOWN')
 before=e.read('gPartiesCount',1);identity=e.read(e.symbols['gParties']+100*index)
 e.tap('START',30);assert ui()==16;e.tap('A')
 check('place habitat Pokemon at '+str(target),ui()==2 and e.read('gPartiesCount',1)==before-1 and target in occupied())
 return identity

e.run(180);snap('01-title');e.tap('A',30);e.tap('START',220)
assert ui()==2;world.PwWorld_Init(sv(8,4));snap('02-world')
walk((-2,-1));step('LEFT',blocked=True);before=sv(30);e.tap('A')
check('CUT needs a Grass helper',sv(30)==before and tile(-3,-1)==2)
for sp in (43,74,10,16):recruit(sp)
e.tap('L');assert ui()==15;snap('04-field-skills');e.tap('B')
for stand,key,target in [((-2,-1),'LEFT',(-3,-1)),((2,-1),'RIGHT',(3,-1)),((-3,1),'LEFT',(-4,1))]:
 walk(stand);step(key,blocked=True);before=sv(30);e.tap('A')
 check('CUT tree '+str(target),sv(30)==before+3 and tile(*target)==0)
walk((1,2));step('RIGHT',blocked=True);before=sv(32);e.tap('A')
check('SMASH rock',sv(32)==before+2 and tile(2,2)==0)
cat=drop(3,(-1,1));bird=drop(3,(1,1));snap('05-habitats')
setup_facing((1,-2),'UP');e.tap('A');assert tile(1,-2)==15
e.tap('A');check('DIG and plant seed',tile(1,-2)==16 and sv(1324)==2)
snap('06-planted')
# Build floor and roof through the same cursor as a player.
setup_facing((-3,-1),'LEFT');e.tap('R');e.tap('A');check('place floor',tile(-3,-1)==8)
e.tap('L');e.tap('A');check('place roof on floor',tile(-3,-1)==18);snap('07-roof');e.tap('B')
# Let time pass naturally, then harvest and collect both bed ingredients.
e.run(7400);check('berry crop matures',tile(1,-2)==17)
setup_facing((1,-2),'UP');before=sv(1326);e.tap('A')
check('harvest yields berries and seeds',sv(1326)==before+3 and sv(1324)==4 and tile(1,-2)==15)
for target,offset in [((-1,1),1328),((1,1),1330)]:
 setup_facing(target,'DOWN');e.tap('A');assert ui()==14;e.tap('A')
 check('happy habitat produces material '+str(offset),sv(offset)==2);snap('08-habitat-'+str(offset));back_world()
setup_facing((-1,-2),'UP');e.tap('R')
# Previous roof index 7 -> bed index 4.
for _ in range(3):e.tap('L')
e.tap('A');check('craft bed from wood, thread and feathers',tile(-1,-2)==12 and sv(1328)==0 and sv(1330)==0)
snap('09-built-bed');e.tap('B');e.tap('A')
check('rest at bed keeps full party HP',all(e.read(e.symbols['gParties']+100*i+86,2)==e.read(e.symbols['gParties']+100*i+88,2) for i in range(3)))
# Save includes two residents, crop, roof, bed and exact party identities.
menu(6);e.run(1800);assert ui()==2
expected={'pos':pos(),'resources':[sv(o) for o in (30,32,34,1324,1326,1328,1330)],'edits':edits(),'party':[e.read(e.symbols['gParties']+100*i) for i in range(3)]}
e.close();e=Emulator('../engine/pokeemerald.gba',save);e.run(180)
assert e.read('sHasSave',1);e.tap('A',150)
check('native flash reload restores position/resources/buildings',pos()==expected['pos'] and [sv(o) for o in (30,32,34,1324,1326,1328,1330)]==expected['resources'] and edits()==expected['edits'])
check('native flash reload preserves party identity',expected['party']==[e.read(e.symbols['gParties']+100*i) for i in range(3)])
for target,identity in [((-1,1),cat),((1,1),bird)]:
 setup_facing(target,'DOWN');e.tap('A');e.tap('DOWN');e.tap('A')
 check('pick up saved resident '+str(target),ui()==2 and e.read(e.symbols['gParties']+100*(e.read('gPartiesCount',1)-1))==identity and target not in occupied())
snap('10-reloaded-camp')
# Cross multiple positive and negative chunk boundaries, reversing direction.
# Only use reachable dry ground; normal collision and encounters remain active.
for goal in [(-17,-10),(-1,0),(17,-2),(0,0)]:
 walk(goal,run=True)
 check('stream terrain to '+str(goal),pos()==goal)
e.tap('SELECT');assert ui()==6;snap('11-explored-map');back_world()
menu(6);e.run(1800);e.close()
report['hardware_frame_samples']=len(report['movement'])
report['worst_walk_frames']=max(m['frames'] for m in report['movement'] if not m['running'])
report['worst_run_frames']=max(m['frames'] for m in report['movement'] if m['running'])
(OUT/'acceptance.json').write_text(json.dumps(report,indent=2))
print('COMPLETE',report['hardware_frame_samples'],'movement samples; max frames',report['worst_walk_frames'],report['worst_run_frames'],flush=True)
