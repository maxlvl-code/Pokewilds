#include <mgba/core/core.h>
#include <mgba/core/log.h>
#include <mgba/gba/core.h>
#include <mgba-util/vfs.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
static void quiet(struct mLogger *l,int cat,enum mLogLevel level,const char *fmt,va_list args) {(void)l;(void)cat;(void)level;(void)fmt;(void)args;}
static struct mLogger logger={.log=quiet};
static struct mCore *core;
static color_t pixels[240*160];
int emu_open(const char *rom,const char *save){
 mLogSetDefaultLogger(&logger);
 core=GBACoreCreate();if(!core||!core->init(core))return 0;
 mCoreInitConfig(core,"pokewilds-test");
 mCoreConfigSetIntValue(&core->config,"logLevel",0);
 core->setVideoBuffer(core,pixels,240);
 if(!mCoreLoadFile(core,rom))return 0;
 if(save&&save[0]){struct VFile *vf=VFileOpen(save,O_RDWR|O_CREAT);if(!vf||!core->loadSave(core,vf))return 0;}
 core->reset(core);return 1;
}
void emu_close(void){if(core){core->deinit(core);core=NULL;}}
void emu_frames(int count,int keys){core->setKeys(core,keys);for(int i=0;i<count;i++)core->runFrame(core);}
uint32_t emu_read(uint32_t a,int width){if(width==1)return core->busRead8(core,a);if(width==2)return core->busRead16(core,a);return core->busRead32(core,a);}
void emu_write(uint32_t a,uint32_t v,int width){if(width==1)core->busWrite8(core,a,v);else if(width==2)core->busWrite16(core,a,v);else core->busWrite32(core,a,v);}
const void *emu_pixels(void){return pixels;}
int emu_pixel_size(void){return sizeof(color_t);}
