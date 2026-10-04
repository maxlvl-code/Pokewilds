#!/usr/bin/env python3
"""Pack selected images from the supplied Pokewilds 0.8.11 JAR for GBA tile hardware."""
from pathlib import Path
from zipfile import ZipFile
from PIL import Image
import io,sys,json,re
jar=ZipFile(sys.argv[1]);out=Path(__file__).resolve().parents[1]/'assets';out.mkdir(exist_ok=True)
def img(n):return Image.open(io.BytesIO(jar.read(n))).convert('RGBA')
def crop(n,box):return img(n).crop(box)
def composite(base,over):
 b=base.copy();b.alpha_composite(over);return b
def tone(im):
 # Bake the desktop grass tint; avoid the untinted lime/white shader inputs.
 im=im.copy()
 im.putdata([(min(255,int(r*.85+20)),min(255,int(g*.72+24)),min(255,int(b*.7+40)),a)
             if g>r*1.1 and g>b*1.1 else (r,g,b,a) for r,g,b,a in im.getdata()])
 return im
bg=tone(img('tiles/grass2_under.png'))
floor=img('tiles/buildings/house5_floor1.png')
sand=img('tiles/desert1.png')
dirt=img('tiles/desert4.png')
def ground(over,base=bg):return composite(base,over)
def parts(im,base=bg):
 canvas=Image.new('RGBA',(16,32));canvas.alpha_composite(im,(0,32-im.height))
 return ground(canvas.crop((0,16,16,32)),base),canvas.crop((0,0,16,16))
tree,tree_top=parts(tone(img('tiles/tree2.png')))
bed,bed_top=parts(img('tiles/buildings/house_bed1.png'),floor)
fire,fire_top=parts(crop('tiles/campfire1.png',(0,0,16,20)))
fire2,fire2_top=parts(crop('tiles/campfire1.png',(16,0,32,20)))
berry,berry_top=parts(tone(crop('tiles/berrytree_cheri.png',(80,0,96,32))))
flower=img('tiles/flower1.png')
flower.putdata([(0,0,0,0) if r==255 else (96,144,64,255) if r==208 else (232,120,152,255) for r,g,b,a in flower.getdata()])
water=crop('tiles/water2.png',(0,0,16,16))
water2=crop('tiles/water2.png',(16,0,32,16))
for im in [water,water2]:
 im.putdata([(88,160,192,255) if r<200 else (168,216,216,255) for r,g,b,a in im.getdata()])
specs=[('plain',bg),('speckle',tone(img('tiles/green1.png'))),('tall',ground(tone(img('tiles/grass2_over.png')))),
 ('tree_base',tree),('tree_top',tree_top),('sand',sand),('water',water),('water2',water2),
 ('rock',ground(img('tiles/rock1_color.png'),dirt)),('flower',ground(flower)),('dirt',dirt),
 ('floor',floor),('wall',ground(img('tiles/buildings/house5_middle1.png'),floor)),
 ('door',ground(crop('tiles/buildings/house5_door1.png',(0,0,16,16)),floor)),
 ('fire',fire),('fire2',fire2),('fire_top',fire_top),('fire2_top',fire2_top),
 ('bed',bed),('bed_top',bed_top),('bridge',ground(crop('tiles/bridge1.png',(0,0,16,16)),water)),
 ('fence',ground(img('tiles/fence1.png'))),('soil',dirt),('sprout',ground(tone(img('tiles/grass_planted.png')),dirt)),
 ('berry',berry),('berry_top',berry_top),('roof',img('tiles/buildings/house5_roof_middle1.png'))]
# Shoreline edges occupy a few pixels inside a water tile. The mask is N/S/W/E.
for mask in range(16):
 shore=water.copy()
 for y in range(16):
  for x in range(16):
   if (mask&1 and y<3) or (mask&2 and y>12) or (mask&4 and x<3) or (mask&8 and x>12):
    shore.putpixel((x,y),sand.getpixel((x,y)))
 specs.append(('shore_'+str(mask),shore))
palette=[(0,0,0)]; tiles=bytearray(64);meta={}
def rgb15(c):return (c[0]>>3)|((c[1]>>3)<<5)|((c[2]>>3)<<10)
for name,im in specs:
 meta[name]=dict(tile=len(tiles)//64,width=im.width//8,height=im.height//8)
 for ty in range(0,im.height,8):
  for tx in range(0,im.width,8):
   for y in range(ty,ty+8):
    for x in range(tx,tx+8):
     r,g,b,a=im.getpixel((x,y));c=(r,g,b)
     if a<128:v=0
     else:
      if c not in palette:palette.append(c)
      v=palette.index(c)
     tiles.append(v)
assert len(palette)<224,(len(palette))
assert len(tiles)<=16384, 'Terrain overlaps the UI character block'
(out/'terrain.8bpp').write_bytes(tiles)
(out/'terrain.gbapal').write_bytes(b''.join(rgb15(c).to_bytes(2,'little') for c in palette+[(0,0,0)]*(224-len(palette))))
header=['/* Imported from the user-supplied Pokewilds v0.8.11 game assets. */']
for n,m in meta.items():header+=['#define PW_GFX_'+n.upper()+' '+str(m['tile'])]
(out/'terrain_ids.h').write_text('\n'.join(header)+'\n')
# Each character gets its own 16-color OBJ palette and 6/8 animation frames.
species=list(range(1,152))+[169,182,186,208,230,462]
sheet=img('pokemon/overworlds_sheet.png');actors=[('player',[img('player/gold-walking.png').crop((i*16,0,i*16+16,16)) for i in range(8)])]
# Sprite atlas order is NOT National Dex order. The supplied game uses this
# name table plus explicit index overrides, then L/L/U/U/D/D frame ordering.
def key(name):return re.sub(r'[^a-z0-9]','',name.lower())
dex=jar.read('pokemon/pokemon_to_index.txt').decode('cp1252').splitlines()
name_rows=jar.read('pokemon/pokemon_names.asm').decode('cp1252').splitlines()
atlas={key(re.search(r'db "([^"]+)"',line).group(1).replace('@','')):i for i,line in enumerate(name_rows) if 'db "' in line}
overrides=jar.read('pokemon/pokemon_overworld_adjustments.asm').decode('cp1252')
for name,index in re.findall(r'(?m)^([^:\r\n]+):\s+db i, (\d+)',overrides):atlas[key(name)]=int(index)
resolved={}
crystal=img('pokemon/crystal-overworld-sprites1.png')
for s in species:
 name=key(dex[s-1])
 frames=[]
 if name in atlas:
  index=atlas[name];resolved[str(s)]={'name':dex[s-1],'atlas_index':index}
  for f in [5,4,3,2,1,0]:
   n=index*6+f;x=n%156*16;y=n//156*16
   assert y+16<=sheet.height,(s,name,index)
   frames.append(sheet.crop((x,y,x+16,y+16)))
 elif s <= 251:
  # The desktop game itself falls back to this two-frame Crystal sheet for
  # species absent from the directional atlas. Never substitute another mon.
  resolved[str(s)]={'name':dex[s-1],'crystal_index':s-1}
  x=1+(s-1)%15*34;y=31+(s-1)//15*25
  frames=[crystal.crop((x+f*17,y,x+f*17+16,y+16)) for f in [0,1,0,1,0,1]]
 else:
  # Later evolutions in the encounter families need a real species sprite too.
  # Magnezone has no valid image in the supplied desktop atlas. Its included
  # fallback is the pinned expansion engine's credited follower sheet.
  native=Image.open(out/'sources'/f'{name}.png')
  rgba=native.convert('RGBA');rgba.putalpha(native.point(lambda p:0 if p==0 else 255,'L'))
  frames=[rgba.crop((f*32,0,f*32+32,32)).resize((16,16),Image.Resampling.NEAREST) for f in range(6)]
  resolved[str(s)]={'name':dex[s-1],'source':'pokeemerald-expansion follower'}
 actors.append((str(s),frames))
(out/'actor_mapping.json').write_text(json.dumps(resolved,indent=2)+'\n')
allbytes=bytearray();pals=bytearray()
for name,frames in actors:
 pal=[(255,0,255)]
 for im in frames:
  for r,g,b,a in im.getdata():
   if a>=128 and (r,g,b) not in pal:pal.append((r,g,b))
 if len(pal)>16:raise ValueError((name,len(pal)))
 for im in frames+frames[:8-len(frames)]:
  for ty in (0,8):
   for tx in (0,8):
    for y in range(ty,ty+8):
     row=[]
     for x in range(tx,tx+8):
      r,g,b,a=im.getpixel((x,y));row.append(pal.index((r,g,b)) if a>=128 else 0)
     allbytes.extend(row[i]|row[i+1]<<4 for i in range(0,8,2))
 pals.extend(b''.join(rgb15(c).to_bytes(2,'little') for c in pal+[(0,0,0)]*(16-len(pal))))
(out/'actors.4bpp').write_bytes(allbytes);(out/'actors.gbapal').write_bytes(pals)
(out/'species_ids.h').write_text('static const u16 sActorSpecies[] = {0,'+','.join(map(str,species))+'};\n')
(out/'ASSET-SOURCES.md').write_text('Terrain, Gold player and selected overworld sprites from the user-supplied Pokewilds v0.8.11 Windows archive, app/pokewilds.jar. Sprite indices follow the original name table and overrides, with its Crystal sheet fallback. Magnezone uses the follower sheet from pokeemerald-expansion commit e8bd1cd7b03fc032ea37e3ecd38b379b5d01a1e7, reduced to 16 pixels. Imported and packed for GBA hardware by tools/import_assets.py. Pokemon/Nintendo/Game Freak, Pokewilds and contributing artists retain their respective rights. These are not newly authored art. The engine keeps its own attribution documentation.\n')
print('Terrain:',len(tiles),'bytes;',len(palette),'colors; actors:',len(actors))
