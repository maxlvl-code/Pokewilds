"""0.3.1 UI/overworld regression on a genuine, synthetic 0.3 flash save.
All changes to the running ROM are made with normal buttons; RAM is read-only.
"""
from emulator import Emulator
from pathlib import Path
import gzip,json,ctypes
from collections import deque
out=Path('validation04');out.mkdir(exist_ok=True)
save=out/'controls.sav';save.write_bytes(gzip.decompress(Path('tests/fixtures/v03-camp.sav.gz').read_bytes()))
e=Emulator('../engine/pokeemerald.gba',save);e.run(180)
checks=[]
def check(label,condition=True):
 assert condition,label
 checks.append(label);print('PASS',label,flush=True)
def ui():return e.read('sUi',1)
def sel():return e.read('sSelection',1)
def count():return e.read('gPartiesCount',1)
def pos():
 p=e.symbols['gSaveblock3'];return tuple(ctypes.c_int16(e.read(p+d,2)).value for d in (16,18))
def signed(sym):return ctypes.c_int16(e.read(sym,2)).value
def modal():return e.read('sNoticeModal',1) and e.read('sNoticeFrames',2)
def dismiss():
 if modal():e.tap('B')
def back_world():
 for _ in range(6):
  if ui()==2:return
  e.tap('B')
 assert ui()==2

def menu(row):
 if ui()==2:e.tap('START')
 assert ui()==3,(ui(),row)
 for _ in range((row-sel())%9):e.tap('DOWN')
 e.tap('A',30)
def snap(name):e.screenshot(out/(name+'.png'))
last_actors=None
def observe_actors():
 global last_actors
 current=[]
 for i in range(5):
  a=e.symbols['sWild']+20*i
  current.append((ctypes.c_int16(e.read(a,2)).value,ctypes.c_int16(e.read(a+2,2)).value,e.read(a+4,2),e.read(a+7,1)))
 if last_actors:
  for old,new in zip(last_actors,current):
   if old[3] and abs(old[0]-pos()[0])<=8 and abs(old[1]-pos()[1])<=5:
    assert new[3] and new[2]==old[2] and abs(new[0]-old[0])+abs(new[1]-old[1])<=1,('visible actor teleported',old,new)
 last_actors=current
def step(key,track=False):
 old=pos();started=False
 for _ in range(90):
  e.run(1,key if not started else 0)
  if track:observe_actors()
  if pos()!=old:started=True
  if started and e.read('sMoveFrames',1)==0:break
 e.run(3)
 assert started,('move failed',old,key)
check('0.3 flash save recognized',e.read('sHasSave',1)==1)
e.tap('A',140)
check('0.3 world and party load intact',ui()==2 and pos()==(0,0) and count()==5 and e.read(e.symbols['gSaveblock3']+4)==3)
snap('01-loaded-v03-save')
# Every ordinary submenu must return to the item that opened it.
for row,screen in [(0,5),(1,9),(2,4),(3,8),(4,6),(5,11),(7,10),(8,7)]:
 menu(row);check('open camp menu item '+str(row),ui()==screen)
 if row==0:
  e.tap('DOWN');chosen=sel();e.tap('A',160);snap('02-summary');e.tap('B',160)
  check('summary returns to party and selected Pokemon',ui()==5 and sel()==chosen)
 if row==2:
  e.tap('A');check('craft failure gives a modal message',modal())
  selection=sel();e.tap('DOWN');check('message blocks hidden menu actions',sel()==selection)
  snap('03-craft-message');e.tap('A');check('A dismisses message without recrafting',not modal() and ui()==4)
 if row==3:snap('04-build-materials')
 e.tap('B');check('Back restores camp menu item '+str(row),ui()==3 and sel()==row)
snap('05-scrolling-menu');e.tap('START');check('START closes camp menu',ui()==2)
e.tap('SELECT');e.tap('B');check('direct map Back returns directly to world',ui()==2)
e.tap('SELECT');e.tap('SELECT');check('SELECT toggles direct map closed',ui()==2)
e.tap('L');e.tap('R');check('skills R enters build mode',ui()==8);e.tap('B')
check('build from skills returns to skills',ui()==15)
e.tap('A');check('skills A actually performs the field action',ui()==2)
snap('06-world-field-action')
# Short turns and simultaneous direction+action must not accidentally walk.
origin=pos();e.tap('UP');check('short direction tap turns in place',pos()==origin and e.read(e.symbols['gSaveblock3']+36,1)==0)
step('UP');check('holding direction moves after turning',pos()==(0,-1))
step('DOWN');check('movement returns to origin',pos()==origin)
# Storage: navigation+selection in the same frame acts on the NEW cell.
menu(5);e.tap('START');identity=e.read(e.symbols['gParties']+100);e.tap('SELECT')
check('storage deposit succeeds',count()==4);dismiss()
e.tap('RIGHT+A');check('RIGHT+A does not withdraw the previous slot',count()==4 and e.read('sBoxSlot',1)==1)
dismiss();e.tap('LEFT+A');check('LEFT+A withdraws the selected slot',count()==5 and e.read(e.symbols['gParties']+400)==identity)
dismiss();e.tap('SELECT');check('depositing lead updates party',count()==4)
# The new lead is Geodude; its actual OBJ palette must replace Machop's.
expected=Path('assets/actors.gbapal').read_bytes()[74*32:75*32]
pal=e.symbols['gPlttBufferUnfaded']+448*2
actual=b''.join(e.read(pal+i*2,2).to_bytes(2,'little') for i in range(16))
check('depositing lead reloads the following sprite',actual==expected)
dismiss();e.tap('A');check('deposited lead can be recovered',count()==5);dismiss();back_world()
snap('07-follower-after-storage')
# Placement is a preview, not an immediate side effect of START.
menu(0);e.tap('DOWN');chosen=sel();before=count();e.tap('START')
check('party START opens placement preview without removing Pokemon',ui()==16 and count()==before)
e.tap('B');check('cancel placement restores party selection',ui()==5 and sel()==chosen and count()==before)
e.tap('START')
for axis,target,neg,poskey in [('sCursorX',1,'LEFT','RIGHT'),('sCursorY',0,'UP','DOWN')]:
 for _ in range(10):
  n=signed(axis)
  if n==target:break
  e.tap(neg if n>target else poskey)
snap('08-placement-preview');e.tap('A')
check('confirm placement removes exactly one party member',ui()==2 and count()==before-1)
e.tap('UP');before_pos=pos();e.tap('RIGHT+A')
check('direction and A interact with the newly faced tile',ui()==14 and pos()==before_pos)
e.tap('A');check('unready habitat shows an explicit message',modal());e.tap('B');e.tap('DOWN');e.tap('A')
check('placed Pokemon can be picked up',ui()==2 and count()==before)
# Back and Start while movement is still finishing must remain buffered.
e.run(1,'RIGHT');e.run(1,'START');e.run(30)
check('START during a step opens the menu',ui()==3);e.tap('B')
# Save from the new menus, restart, and verify the same world still loads.
menu(6);e.run(1600);check('save action completes',ui()==2 and e.read('sHasSave',1)==1)
expected_pos=pos();expected_ids=[e.read(e.symbols['gParties']+100*i) for i in range(count())]
e.close();e=Emulator('../engine/pokeemerald.gba',save);e.run(180);e.tap('A',140)
check('repaired menus still save and reload exact party identities',pos()==expected_pos and expected_ids==[e.read(e.symbols['gParties']+100*i) for i in range(count())])
snap('09-reloaded');e.close()
fresh=out/'fresh-controls.sav';fresh.unlink(missing_ok=True)
e=Emulator('../engine/pokeemerald.gba',fresh);e.run(180);e.tap('A')
initial=e.read('sSeed')
for _ in range(7):e.tap('RIGHT')
check('seed cursor selects the eighth displayed digit',e.read('sDigit',1)==7)
for _ in range(7):e.tap('UP')
check('seed digit wraps without changing neighboring digits',e.read('sSeed')==(initial&~15))
e.tap('DOWN');check('seed digit wraps backward independently',e.read('sSeed')==(initial|15))
for _ in range(10):e.tap('UP')
snap('10-seed-editor');e.tap('B');check('seed cancel returns to title',ui()==0)
e.tap('A');e.tap('START',220)
e.run(1,'DOWN+START');e.run(40,'DOWN')
check('held movement does not scroll a newly opened menu',ui()==3 and sel()==0)
e.run(3);e.tap('DOWN');check('menu navigation resumes after direction release',sel()==1);e.tap('B')
for i in range(60):step('UP' if i%2==0 else 'DOWN',track=True)
check('visible wild Pokemon remain stable across more than 48 steps',ui()==2 and not e.read('sBattleReturning',1))
snap('11-stable-overworld')
# Plan on the terrain already loaded around camp. Observe it; never modify RAM.
ground=set()
for i in range(9):
 a=e.symbols['sGame']+12+264*i
 if not e.read(a+5,1):continue
 x=ctypes.c_int16(e.read(a,2)).value*16;y=ctypes.c_int16(e.read(a+2,2)).value*16
 for yy in range(16):
  for xx in range(16):
   if e.read(a+6+yy*16+xx,1) in {0,3,6,7,8,10,13,15,16,18}:ground.add((x+xx,y+yy))
def walk(goal):
 for _ in range(200):
  if pos()==goal:return
  occupied=set()
  for i in range(5):
   a=e.symbols['sWild']+20*i
   if e.read(a+7,1):
    x=ctypes.c_int16(e.read(a,2)).value;y=ctypes.c_int16(e.read(a+2,2)).value
    dx=ctypes.c_int16(e.read(a+12,2)).value;dy=ctypes.c_int16(e.read(a+14,2)).value
    occupied.add((x,y));occupied.add((x+(dx>0)-(dx<0),y+(dy>0)-(dy<0)))
  queue=deque([(pos(),[])]);seen={pos()};route=None
  while queue:
   p,path=queue.popleft()
   if p==goal:route=path;break
   for dx,dy,key in [(0,-1,'UP'),(0,1,'DOWN'),(-1,0,'LEFT'),(1,0,'RIGHT')]:
    n=(p[0]+dx,p[1]+dy)
    if n in ground and n not in occupied and n not in seen:
     seen.add(n);queue.append((n,path+[key]))
  assert route,('no camp route',pos(),goal)
  step(route[0],track=True)
 raise AssertionError('camp route timed out')
def guides_present():
 expected=[(-3,2,43),(4,3,74),(3,-3,10),(-4,-3,16)]
 for i,(x,y,species) in enumerate(expected):
  a=e.symbols['sWild']+20*i
  if not (e.read(a+7,1) and ctypes.c_int16(e.read(a,2)).value==x and ctypes.c_int16(e.read(a+2,2)).value==y and e.read(a+4,2)==species):return False
 return True
walk((-14,-10));e.run(100)
check('unrecruited helpers remain at camp during exploration',guides_present())
menu(6);e.run(1600);e.close()
e=Emulator('../engine/pokeemerald.gba',fresh);e.run(180);e.tap('A',140);last_actors=None
check('distant save reload preserves camp helpers',pos()==(-14,-10) and guides_present())
walk((0,0));check('returning to camp finds the same four helpers',guides_present());e.close()
(out/'controls.json').write_text(json.dumps({'checks':checks,'fixture':'synthetic save produced by released 0.3 ROM','input_method':'buttons only; no RAM writes'},indent=2))
print('COMPLETE',len(checks),'control checks',flush=True)
