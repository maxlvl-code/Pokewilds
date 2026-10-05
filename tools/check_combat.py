"""Native capture, summary, storage and restart regression."""
from emulator import Emulator
from pathlib import Path
import ctypes,json
out=Path('validation04');out.mkdir(exist_ok=True)
save=out/'combat.sav';save.unlink(missing_ok=True)
e=Emulator('../engine/pokeemerald.gba',save)
checks=[]
def check(label,ok=True):
 assert ok,label
 checks.append(label);print('PASS',label,flush=True)
def pos():
 p=e.symbols['gSaveblock3'];return [ctypes.c_int16(e.read(p+16,2)).value,ctypes.c_int16(e.read(p+18,2)).value]
def menu(n):
 e.tap('START')
 for _ in range(n):e.tap('DOWN')
 e.tap('A',50)
def back_world():
 for _ in range(5):
  if e.read('sUi',1)==2:return
  e.tap('B')
 assert e.read('sUi',1)==2
def step(key):
 old=pos();started=False
 for _ in range(90):
  e.run(1,key if not started else 0)
  if pos()!=old:started=True
  if started and not e.read('sMoveFrames',1):break
 assert started
 e.run(3)
e.run(180);e.tap('A',30);e.tap('START',220)
for k in ['UP','UP','RIGHT','RIGHT','RIGHT']:step(k)
e.tap('UP')
e.tap('A');e.tap('DOWN');e.tap('A',400)
check('friendly interaction can start a native battle',e.read('sBattleReturning',1)==1)
e.screenshot(out/'12-battle-intro.png')
start=pos();p=e.symbols['gParties']
check('starter has valid level and battle HP',e.read(p+84,1)==7 and 0<e.read(p+86,2)<=e.read(p+88,2))
# Advance text, open Bag, select the Ball pocket, and use one ball.
for key,frames in [('A',140),('A',100),('RIGHT',20),('A',100),('RIGHT',60),('A',30),('A',450),('A',450)]:e.tap(key,frames)
e.screenshot(out/'13-caught.png')
for key,frames in [('A',450),('A',350),('A',250),('A',250),('B',300)]:e.tap(key,frames)
check('capture and return to exact wilderness position',e.read('sUi',1)==2 and not e.read('sBattleReturning',1) and e.read('gBattleOutcome',1)==7 and pos()==start and e.read('gPartiesCount',1)==2)
identity=e.read(p+100);e.screenshot(out/'14-capture-return.png')
menu(5);e.tap('START');e.tap('SELECT',60)
check('deposit captured Pokemon in storage',e.read('gPartiesCount',1)==1)
back_world();menu(6);e.run(1800);check('save after deposit',e.read('sUi',1)==2)
e.close();e=Emulator('../engine/pokeemerald.gba',save);e.run(180)
check('flash save detected after emulator restart',e.read('sHasSave',1)==1)
e.tap('A',150);menu(5);e.tap('A',60)
check('withdraw exact captured Pokemon after restart',e.read('gPartiesCount',1)==2 and e.read(e.symbols['gParties']+100)==identity and e.read(e.symbols['gParties']+186,2)>0)
e.screenshot(out/'15-storage-withdraw.png');back_world()
menu(0);e.tap('DOWN');e.tap('A',180);e.screenshot(out/'16-native-summary.png');e.tap('B',180)
check('native summary returns to party',e.read('sUi',1)==5 and pos()==start);back_world()
menu(7);e.tap('DOWN');e.tap('DOWN');e.tap('A',1800)
check('save and quit returns to title',e.read('sUi',1)==0 and e.read('sHasSave',1)==1)
e.tap('A',150)
check('continue after save and quit restores captured party',e.read('sUi',1)==2 and e.read('gPartiesCount',1)==2 and e.read(e.symbols['gParties']+100)==identity)
for _ in range(2):
 menu(7);e.tap('DOWN');e.tap('DOWN');e.tap('A',1800)
 assert e.read('sUi',1)==0
 ticks=e.read('sFrames');e.run(60);assert e.read('sFrames')-ticks>=55
 e.tap('A',150)
 assert e.read('sUi',1)==2 and e.read(e.symbols['gParties']+100)==identity
check('repeated save and quit cycles keep the title responsive')
e.close();(out/'combat.json').write_text(json.dumps({'checks':checks,'capture_position':start,'input_method':'normal buttons, no memory writes'},indent=2))
