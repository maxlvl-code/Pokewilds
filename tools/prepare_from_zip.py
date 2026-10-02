#!/usr/bin/env python3
import sys, shutil
from pathlib import Path
from zipfile import ZipFile
if len(sys.argv) != 3:
    raise SystemExit('usage: prepare_from_zip.py <pokewilds-windows64.zip> <project-root>')
source = Path(sys.argv[1]).resolve()
root = Path(sys.argv[2]).resolve()
work = root / 'build-input'
shutil.rmtree(work, ignore_errors=True)
work.mkdir(parents=True)
with ZipFile(source) as z: z.extractall(work)
jars = list(work.rglob('pokewilds.jar'))
if not jars: raise SystemExit('pokewilds.jar not found in input zip')
jar = jars[0]
assets = root / 'app/src/main/assets'
shutil.rmtree(assets, ignore_errors=True)
assets.mkdir(parents=True)
exclude={'META-INF','Script-Directory','com','gme','javazoom','kotlin','leakcanary','linux','linux64','macos','macosx64','macosxarm64','net','okio','org','oshi','shark','windows','windows32','windows64'}
with ZipFile(jar) as z:
    count=total=0
    for info in z.infolist():
        n=info.filename
        if info.is_dir() or n.endswith('.class'): continue
        if n.split('/',1)[0] in exclude: continue
        if n.lower().endswith(('.dll','.so','.dylib','.jnilib','.exe')): continue
        dest=assets/n; dest.parent.mkdir(parents=True,exist_ok=True)
        with z.open(info) as src, open(dest,'wb') as dst: shutil.copyfileobj(src,dst)
        count += 1; total += info.file_size
print(f'Extracted {count} Android assets ({total} bytes) from {jar}')
print(jar)
