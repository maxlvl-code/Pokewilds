from pathlib import Path
from PIL import Image
import ctypes,subprocess
ROOT=Path(__file__).resolve().parents[1]
lib=ctypes.CDLL(str(ROOT/'validation/libemulator.so'))
lib.emu_open.argtypes=[ctypes.c_char_p,ctypes.c_char_p];lib.emu_read.argtypes=[ctypes.c_uint32,ctypes.c_int];lib.emu_read.restype=ctypes.c_uint32
lib.emu_write.argtypes=[ctypes.c_uint32,ctypes.c_uint32,ctypes.c_int];lib.emu_pixels.restype=ctypes.c_void_p
KEYS={'A':1,'B':2,'SELECT':4,'START':8,'RIGHT':16,'LEFT':32,'UP':64,'DOWN':128,'R':256,'L':512}
class Emulator:
 def __init__(self,rom,save):
  self.rom=Path(rom);self.save=Path(save)
  assert lib.emu_open(str(self.rom).encode(),str(self.save).encode())
  self.symbols={}
  for line in subprocess.check_output(['arm-none-eabi-nm','-S',str(self.rom.with_suffix('.elf'))],text=True).splitlines():
   p=line.split()
   if len(p)>=3:
    try:self.symbols[p[-1]]=int(p[0],16)
    except ValueError:pass
 def run(self,n=1,keys=0):
  if isinstance(keys,str):keys=sum(KEYS[k] for k in keys.split('+'))
  lib.emu_frames(n,keys)
 def tap(self,key,after=15):self.run(3,key);self.run(after)
 def read(self,addr,n=4):return lib.emu_read(self.symbols.get(addr,addr) if isinstance(addr,str) else addr,n)
 def write(self,addr,value,n=4):lib.emu_write(self.symbols.get(addr,addr) if isinstance(addr,str) else addr,value,n)
 def screenshot(self,path):
  b=ctypes.string_at(lib.emu_pixels(),240*160*lib.emu_pixel_size())
  Image.frombytes('RGBA',(240,160),b).convert('RGB').save(path)
 def close(self):lib.emu_close()
