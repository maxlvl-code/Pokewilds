"""Short input, held movement, day/night and palette reload regression checks."""
from emulator import Emulator
from pathlib import Path
import ctypes,json,shutil
out=Path('validation03');shutil.copyfile(out/'acceptance.sav',out/'session.sav')
e=Emulator('../engine/pokeemerald.gba',out/'session.sav');e.run(180);e.tap('A',150)
checks=[]
def check(label,ok=True):
 assert ok,label
 checks.append(label);print('PASS',label,flush=True)
def pos():
 p=e.symbols['gSaveblock3'];return (ctypes.c_int16(e.read(p+16,2)).value,ctypes.c_int16(e.read(p+18,2)).value)
def seconds():return e.read(e.symbols['gSaveblock3']+1320)
check('session starts at camp',pos()==(0,0) and e.read('sUi',1)==2)
# An action pressed during an unfinished step must survive until movement ends.
e.run(1,'UP');e.run(1,'L');e.run(30)
check('one-frame skill tap buffered during a step',e.read('sUi',1)==15);e.tap('B')
for i in range(12):
 e.run(1,'START');e.run(12);assert e.read('sUi',1)==3,(i,'START missed')
 e.run(1,'B');e.run(12);assert e.read('sUi',1)==2,(i,'B missed')
check('12 repeated one-frame menu open/close taps')
# Return one step to camp, then hold a direction continuously in both directions.
e.tap('DOWN');check('return to movement test origin',pos()==(0,0))
e.run(32,'UP');e.run(8);check('held movement crosses four tiles without new key edges',pos()==(0,-4))
e.run(32,'DOWN');e.run(8);check('held direction reversal returns to origin',pos()==(0,0))
a=e.read('sFrames');e.run(300);ticks=e.read('sFrames')-a
check('idle world advances without sustained frame stalls',ticks>=295)
while seconds()<540:
 e.run(min(1800,(540-seconds())*60+60))
check('world clock reaches night during continuous play',540<=seconds()<570)
faded=e.symbols['gPlttBufferFaded'];plain=e.symbols['gPlttBufferUnfaded']
check('night dims terrain but leaves menus readable',any(e.read(faded+i*2,2)!=e.read(plain+i*2,2) for i in range(1,224)) and all(e.read(faded+i*2,2)==e.read(plain+i*2,2) for i in range(240,256)))
e.screenshot(out/'17-night.png')
e.tap('START');e.tap('A');e.tap('DOWN');e.tap('SELECT',30)
# Reloading a partner's graphics must not remove its night tint.
check('changing follower retains night tint',any(e.read(faded+i*2,2)!=e.read(plain+i*2,2) for i in range(448,464)))
e.tap('B');e.tap('B');e.screenshot(out/'18-night-follower.png')
e.close();(out/'session.json').write_text(json.dumps({'checks':checks,'idle_ticks_in_300_frames':ticks,'clock':'at least nine simulated minutes','input_method':'normal buttons, no memory writes'},indent=2))
