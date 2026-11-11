#ifndef EACLIB_H
#define EACLIB_H

#include <stdbool.h>
#include <stdio.h>

#include "xport.h"
#include "eaclib/psx/callback.h"
#include "game/common/coorddef.h"
#include "game/common/matrixtdef.h"

#ifdef __cplusplus
#include "eaclib/psx/nasync.h"
#include "eaclib/psx/nfile.h"
#include "eaclib/psx/spchdata.h"
#include "eaclib/psx/spchinit.h"
#include "eaclib/psx/spktplay.h"
#include "eaclib/psx/sst.h"
#include "eaclib/psx/ssysinit.h"
#include "eaclib/psx/stream.h"

#endif

struct shapetbl;

struct SNDPLAYOPTS;

struct SNDLIMITS;

struct SNDSAMPLEFORMAT;

struct SoundStreamPrefix;

struct SoundPlatformChannel;

struct SoundStreamRequestPrefix;

struct NativeStreamControl;

struct NativeStreamHandle;

struct SoundAttributesPrefix;

struct SoundTimbreParameters;

struct NativeAsyncRequest;

struct NativeAsyncQueue;

#ifdef __cplusplus

struct SNDSYSOPTS;

struct SNDSTREAMSTATUS;

struct SNDREQUESTSTATUS;

extern "C" {
#endif

void *reservememadr(const char *name, sint32 length, uint32 flags);
void *resizememadr(void *stream, sint32 length);
uint32 purgememadr(void *memory);
sint32 largestunused(void);
sint32 getblocksize(const void *memory);
char *getblockname(void *memory);
sint32 SNDstop(sint32 handle);
sint32 SNDpitchbend(sint32 handle, sint32 bend);
sint32 iSNDunsafebend(sint32 handle, sint32 bend);
sint32 elapsedticks(void);

struct SoundStreamRequestPrefix *iSNDstreamgetrequestptr(sint32 requestId);
sint32 SNDpitchmult(sint32 handle, sint32 multiplier);
sint32 SNDpan(sint32 handle, sint32 pan);
sint32 SND3dpos(sint32 handle, sint32 azimuth, sint32 elevation);
sint32 SNDmastervol(sint32 volume);
sint32 SNDcdvol(sint32 volume);
sint32 iSNDplatformcdpanvol(sint32 pan, sint32 volume);
void SNDstopall(void);
void StopSoundVoicesExcept(sint32 preservedHandle);
sint32 SNDautovol(sint32 handle, sint32 ticks, sint32 volume);
sint32 SNDtimeremaining(sint32 handle);
uint32 iSNDtimeremaining(sint32 channel);
void iSNDpacketfreeframes(sint32 handle, sint32 channel, uint32 frames);
uint32 SNDover(sint32 handle);
sint32 SNDvol(sint32 handle, sint32 volume);
sint32 SNDgetvol(sint32 handle);
uint32 systemtask(sint32 context);
bool iscurrentthread(sint32 thread);
void yieldthread(sint32 thread);
void iSNDserveradd100hzclient(void (*callback)(void));
void iSNDserverremove100hzclient(void (*callback)(void));
#ifdef __cplusplus
void iSNDserveraddclient(void (*callback)(...));
void iSNDserverremoveclient(void (*callback)(...));
#endif
void delsystemtask(sint32 (*callback)(sint32, sint32));
void addsystemtask(sint32 (*callback)(sint32, sint32), sint32 interval, sint32 delay);
sint32 iSNDinit(void);
sint32 iSNDrestore(void);
void iSNDmeminit(void *memory, sint32 size);
uint32 iSNDmemrestore(void);
void iSNDmemconstrain(sint32 *offset, sint32 *length);
void *iSNDmalloc(sint32 size);
sint32 iSNDplatformoutputcaps(void);
sint32 iSNDplatformoutputset(void);
sint32 SNDgetlimits(struct SNDLIMITS *limits);
sint32 SNDsetlimits(struct SNDLIMITS *limits);
char *loadbigfileheader(const char *filename, uint32 flags);
sint32 typeofbigfile(const void *header);
sint32 sizeofbigfileheader(const void *header);
sint32 locatebigoffset(char *header, const char *name);
sint32 bigcount(const void *header);
char *locatebig(char *header, const char *name);
sint32 gettick(void);
sint32 settrans(sint32 mode);
void movfxya(struct shapetbl *shape, sint32 x, sint32 y);
void resettick(void);
void restoretimer(void);
sint32 tmrint(void);
sint32 initmemadr(void *memory, sint32 size);
void inittimer(sint32 frequency);
void addexit(GameTimerCallback callback);
void iSNDpsxkeyon(uint32 channels);
void iSNDpsxkeyoff(uint32 channels);
void iSNDpsxeffecton(uint32 channels);
void iSNDpsxeffectoff(uint32 channels);
void iSNDpsxeffectvol(sint32 left, sint32 right);
void iSNDsetslot(sint32 channel, sint32 address, sint32 pitch);
void iSNDsetvol(sint32 channel, sint32 left, sint32 right);
sint32 iSNDplatformfxmasterlevel(sint32 effect, sint32 level);
void iSNDpsxfxinit(sint32 mode);
sint32 iSNDplatformfxinit(sint32 effect, sint32 mode);
sint32 GetFilterLength(void);
sint32 GetFilterPriority(void);
char *locatebigentryz(char *header, const char *name, sint32 index, sint32 *offset, sint32 *size);
char *locatebigentry(char *header, const char *name, sint32 index, sint32 *offset, sint32 *size);
sint32 wildcard(const char *name, char *pattern);
sint32 SNDmemlargestunused(sint32 *address);
sint32 SNDbankadd(sint32 *handle, void *bank);
sint32 SNDbankheadersize(sint32 handle);
sint32 SNDbankheadercopy(void *destination, sint32 handle);
void SNDfxinitbus(sint32 bus, sint32 level, sint32 effect, sint32 parameter0, sint32 parameter1);
sint32 SNDfxmasterlevel(sint32 bus, sint32 level);
sint32 SNDfxlevel(sint32 voice, sint32 bus, sint32 level);
const char *strrstr(const char *text, const char *separators);
sint32 SNDplaysetdef(struct SNDPLAYOPTS *options);
sint32 SNDplay(struct SNDPLAYOPTS *options);
sint32 cSNDplay(struct SNDPLAYOPTS *options, sint32 unused);
sint32 iSNDplaytaggedpatch(uint8 *patch, struct SNDPLAYOPTS *options);
void timedwait(sint32 duration);
void *loadshapeadr(const char *name, uint32 flags);
sint32 filesize(const char *name);

void *loadfileatadrz(const char *name, void *destination);
void *loadfileatadr(const char *name, void *destination);
void *loadfileadr(const char *name, uint32 flags);
void *loadfileadrz(const char *name, uint32 flags);
void *loadpackadrz(const char *name, uint32 flags);
void addtimer(void (*callback)(void));
void *initlinkmode(sint32 mode, sint32 maxprims, uint32 flags);
void waitdraw(void);
void deltimer(void (*callback)(void));
sint32 fixedsin(sint32 angle);
sint32 fixedcos(sint32 angle);
sint32 fixedatan(sint32 x, sint32 z);
uint32 isqrt(uint32 value);
sint32 iSNDpacketplayoverhead(sint32 chunks);
sint32 iSNDplatformpacketoverhead(void);
sint32 iSNDplatformcalcdatarate(const struct SNDSAMPLEFORMAT *format);
sint32 iSNDabs(sint32 value);
sint32 SNDattributessetdef(struct SoundAttributesPrefix *attributes);
sint32 iSNDpatchtohdr(uint8 *patch, struct SNDSAMPLEFORMAT *format, struct SoundAttributesPrefix *attributes, uint32 *frames);
sint32 iSNDgetdata(const uint8 *data, uint32 count);
sint32 iSNDgettag(uint8 **cursor, uint32 *tag, uint32 *value, uint8 **payload);
void iSNDdmservice(void);
void iSNDdmtransfer(void);
void iSNDdmcallback(void);
uint32 iSNDdmqueue(const uint8 *source, sint32 spuAddress, sint32 size, sint32 priority, sint32 notify);
uint32 iSNDdmqueuesplit(const uint8 *source, sint32 spuAddress, sint32 size, sint32 priority);
sint32 iSNDdmcomplete(uint32 request);
void iSNDpsxmemconstrain(uint32 *offset, uint32 *length);
sint32 iSNDpsxmalloc(sint32 size);
sint32 iSNDpsxfree(sint32 address);
sint32 VoxSentence_GetNumPhrases(const uint8 *sentence);
sint32 iSNDvalidbank(sint32 bank);
sint32 iSNDbankremovepat(sint32 bank, sint32 patchIndex, void *sampleTable);
sint32 iSNDremovepatches(sint32 bank, sint32 count);
sint32 cSNDbankremove(sint32 bank, sint32 unused);
sint32 iSNDbankalloc(void);
sint32 iSNDdownloadbank(uint8 *bank, const uint8 *sampleData);
void iSNDresetpatch(struct SoundTimbreParameters *parameters);
void iSNDresolveheader(struct SoundTimbreParameters *parameters, struct SoundTimbreParameters *bases);
sint32 iSNDplaytaggedtimbre(uint8 *sample, struct SNDPLAYOPTS *options, struct SoundTimbreParameters *parameters, struct SoundTimbreParameters *bases, sint32 key, sint32 velocity, sint32 randomPitch);
sint32 iSNDplatformplay(uint8 *sample, sint32 channel, sint32 volume, sint32 pitch, sint32 timeMultiplier, sint32 effectVolume, sint32 azimuth, sint32 elevation);
void iSNDresettimbre(struct SoundTimbreParameters *parameters, struct SoundTimbreParameters *bases);
sint32 iSNDresolvetaggedpatch(uint8 *patch, const uint8 *sampleData, void *sampleTable);
sint32 iSNDremovetaggedpatch(uint8 *patch, void *sampleTable);
sint32 iSNDplatformresolve(uint8 *tags, const uint8 *sampleData, void *sampleTable);
sint32 iSNDplatformremove(uint8 *tags, void *sampleTable);
uint32 iSNDrandom(void);
sint32 randrange(sint32 range);
void iSNDmulu64(uint32 *result, uint32 left, uint32 right);
uint32 iSNDdivu64(uint32 low, uint32 high, uint32 divisor);
sint32 filterchunk(struct NativeStreamControl *control, const uint32 *data);
sint32 validatehandle(struct NativeStreamHandle *handle, struct NativeStreamControl **control, struct NativeStreamHandle **filter);
sint32 inbetween(uintptr_t start, uintptr_t end, uintptr_t position);
void decbufferusage(struct NativeStreamControl *control, uint32 length);

struct NativeStreamRequest *getfreerequest(struct NativeStreamControl *control);
void queuerequest(struct NativeStreamControl *control, struct NativeStreamRequest *request);

void freerequest(struct NativeStreamControl *control, struct NativeStreamRequest *request);
sint32 parsechunks(struct NativeStreamControl *control);
void opencallback(uint32 operation, uint32 result, void *userData);
void closecallback(uint32 operation, uint32 result, void *userData);
void readcallback(uint32 operation, uint32 result, void *userData);
void startnextrequest(struct NativeStreamControl *control, sint32 priority);
void restartstream(struct NativeStreamControl *control, sint32 priority);

struct SoundStreamPrefix *iSNDstreamgetstreamptr(sint32 handle);
sint32 iSNDstreamcreate(struct SNDPLAYOPTS *options, sint32 streams, sint32 chunks, void *buffer, sint32 size, struct NativeStreamHandle *streamHandle, sint32 shared);
sint32 iSNDstreamqueue(sint32 handle, sint32 delay, const void *source, sint32 offset, sint32 mode);
sint32 iSNDstreamnumcreated(void);
sint32 iSNDstreamisheld(struct SoundStreamPrefix *stream);
#ifdef __cplusplus
void iSNDstreamservice(...);
#endif
#ifdef __cplusplus
sint32 iSNDstreamdestroyall(...);
#endif
void iSNDstreamremoverequest(uint32 requestId);
void iSNDstreamreleasecallback(void *data);
void iSNDstreamnotifycallback(sint32 handle, uint32 frames);
sint32 iSNDsystemtaskreal(sint32 context, sint32 lateness);
void iSNDserver(void);
void iSNDenteraudio(void);
void iSNDleaveaudio(void);
void iSNDpacketgetirq(void);
void iSNDpacketsetirq(void);
void iSNDpacketirqcallback(void);
void iSNDpsxenablespuirq(void);
void iSNDpsxdisablespuirq(void);
void iSNDpsxzerospu(uint32 *buffer, sint32 length);
void iSNDpacketpurgeframes(sint32 handle, uint32 position, uint32 frames);
void iSNDfillspuwithpackets(sint32 handle, uint32 packetIndex);
void iSNDpacketserve(void);
void iSNDstreamhotroddatachunks(void);
const uint8 *iSNDpacketget(sint32 handle, sint32 channel, uint32 *frames);
sint32 iSNDplatformpacketplaycreate(sint32 handle, void *buffer);
void iSNDplatformpacketplaydestroy(sint32 handle);
sint32 iSNDallocchan(uint32 mask, sint32 channels, sint32 priority, sint32 *playHandle);
sint32 iSNDischanreserved(sint32 channel, sint32 count);
sint32 iSNDfindfreekey(void);
void iSNDfreechan(sint32 channel);
sint32 iSNDdetunetolinear(sint32 detune);
void iSNDcalcpitch(sint32 channel);
void iSNDcalcvol(sint32 channel);
sint32 iSNDgetchan(sint32 handle);
sint32 iSNDpatchkey(sint32 channel, sint32 *cursor);
sint32 iSNDplatformpitch(sint32 channel, sint32 pitch);
sint32 iSNDplatformfxlevel(sint32 channel, sint32 effect, sint32 level);
uint32 iSNDstartvoice(sint32 channel);
void iSNDserve(void);
void iSNDpsxpacketstop(struct SoundPlatformChannel *channel);
sint32 iSNDvol(sint32 channel, sint32 volume);
sint32 iSNDunsafevol(sint32 handle, sint32 volume);
sint32 iSNDstop(sint32 channel);
sint32 iSNDplatform3dpos(sint32 channel, sint32 pan, sint32 elevation);
sint32 iSNDplatformpacketplay(sint32 handle, sint32 channel, sint32 pan, sint32 volume, sint32 pitch, sint32 time, sint32 effect, struct SNDSAMPLEFORMAT *format, void *attributes);
sint32 iSNDstreamparseend(struct SoundStreamPrefix *stream, void *chunk);
sint32 iSNDstreamparsechunk(struct SoundStreamPrefix *stream, uint32 *chunk);
sint32 iSNDsin(sint32 angle);
void iSNDlibatodlrv(sint32 angle, sint32 volume, sint32 *left, sint32 *right);
void iSNDatolrv(sint32 angle, sint32 volume, sint32 *left, sint32 *right);
void iSNDpvtolrv(sint32 pan, sint32 volume, sint32 *left, sint32 *right);
void iSNDatodlrv(sint32 angle, sint32 volume, sint32 *left, sint32 *right, sint32 previousLeft, sint32 previousRight);
sint32 iSNDstreamparsedata(struct SoundStreamPrefix *stream, uint32 *chunk);
sint32 iSNDstreamparseheader(struct SoundStreamPrefix *stream, uint32 *chunk);
sint32 iSNDstreamparsenumchunks(struct SoundStreamPrefix *stream, uint32 *chunk);
sint32 intatan(sint32 x, sint32 z);
sint32 intarcsin(sint32 value);
sint32 intarccos(sint32 value);

coorddef *crossproduct(coorddef *left, coorddef *right, coorddef *result);

uint32 asyncloadfile(const char *name, uint32 flags);
#ifdef __cplusplus
uint32 asyncloadfilecallback(const char *name, uint32 flags, NativeAsyncCallback callback);
void synccallback(uint32 operation, uint32 result, void *userData);
sint32 syncblockio(sint32 fileHandle, sint32 offset, void *destination, sint32 length, sint32 priority, NativeReadOperation reader);
void iFILE_perror(NativeFileOperation *operation);
NativeFileOperation *reserveop(void);
void freeop(NativeFileOperation *operation);
NativeFileSlot *reservehandle(void);
void freehandle(NativeFileSlot *file);
void iFILE_ExecCommand(NativeFileOperation *operation);
uint32 asyncloadfileatcallback(const char *name, void *destination, NativeAsyncCallback callback);
uint32 asyncloadsegmentcallback(sint32 offset, void *destination, sint32 length, NativeAsyncCallback callback);
#endif
void queueadd(struct NativeAsyncQueue *queue, struct NativeAsyncRequest *request);

struct NativeAsyncRequest *queuefetch(struct NativeAsyncQueue *queue);
uint32 newrequestid(struct NativeAsyncRequest *request);

void cancelrequest(struct NativeAsyncRequest *request);
void finishrequest(struct NativeAsyncRequest *request);
void loadfileclosecallback(uint32 operation, uint32 result, void *userData);
void loadfilereadcallback(uint32 operation, uint32 result, void *userData);
void loadfilesizecallback(uint32 operation, uint32 result, void *userData);
void loadfileopencallback(uint32 operation, uint32 result, void *userData);
void loadsegreadcallback(uint32 operation, uint32 result, void *userData);
sint32 asyncsystemtask(sint32 context, sint32 lateness);
void initasync(sint32 count, sint32 chunkSize, uint32 flags);
void initfileio(void);
uint32 initjoy(uint32 count);
sint32 openfile(const char *name, uint32 mode, FILE **handle);
sint32 closefile(FILE *handle);
sint32 readfile(FILE *handle, void *destination, sint32 offset, sint32 length);
sint32 writefile(FILE *handle, const void *source, sint32 offset, sint32 length);
sint32 getfilesize(FILE *handle);
uint32 asyncloadfileat(const char *name, void *destination);
void setasyncfile(const char *name);
uint32 asyncloadsegment(sint32 offset, void *destination, sint32 length);
void cancelasyncload(uint32 handle);
char *getasyncreadadr(uint32 handle);
sint32 getasyncreadstatus(uint32 handle);

void reorthogonalize(matrixtdef *matrix);

matrixtdef *addmatrix(matrixtdef *left, matrixtdef *right, matrixtdef *result);

matrixtdef *submatrix(matrixtdef *left, matrixtdef *right, matrixtdef *result);

matrixtdef *scalematrix(matrixtdef *matrix, sint32 scale, matrixtdef *result);
void transmult(matrixtdef *left, matrixtdef *right, matrixtdef *result);
void transpose(matrixtdef *source, matrixtdef *destination);
void transform(coorddef *vector, matrixtdef *matrix, coorddef *result);
void fixedxformx(matrixtdef *matrix, sint32 angle);
void *locateshape(void *file, const char *name);

uint8 *memcpyl(uint8 *destination, const uint8 *source, sint32 length);
void memcpyb(uint8 *destination, const uint8 *source, uint32 length);
uint8 *refcpy(uint8 *destination, uint32 distance, sint32 length);
sint32 unrefpack(const uint8 *data, uint8 *destination, sint32 unpack);
void chase(uint32 symbol);
sint32 unbtree(const uint8 *data, uint8 *destination);
uint32 unhuff(const uint8 *data, uint8 *destination, sint32 unpack);
void checkrect(struct PSX_RECT *rectangle);
void vramimage(struct PSX_RECT *rectangle, uint32 *pixels);
void vramfxya(struct shapetbl *shape, sint32 x, sint32 y, sint32 clutx, sint32 cluty);
sint32 shapetoclutid(struct shapetbl *shape);
void xformy(matrixtdef *matrix, sint32 angle);
void fixedxformy(matrixtdef *matrix, sint32 angle);
void fixedxformz(matrixtdef *matrix, sint32 angle);
void intsincos(sint32 angle, sint32 *sine, sint32 *cosine);
void fixedsincos(sint32 angle, sint32 *sine, sint32 *cosine);
void fastmovfxya(struct shapetbl *shape, sint32 x, sint32 y);
#ifdef __cplusplus
void iSND100hzserver(...);
#endif

#ifdef __cplusplus
}
#endif

#endif
