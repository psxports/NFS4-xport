#ifndef INTERNAL_H
#define INTERNAL_H

#include <ctype.h>
#ifdef __cplusplus
#include <new>
#endif
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(_WIN32)
#include <strings.h>

#endif

#include "xport.h"
#include "psx_spu.h"
#include "eaclib/psx/callback.h"
#include "eaclib/psx/eaclib.h"
#include "eaclib/psx/globals.h"
#include "eaclib/psx/math_exports.h"
#include "eaclib/psx/memstd.h"
#include "eaclib/psx/pad.h"
#include "eaclib/psx/salloc.h"
#include "eaclib/psx/sdata.h"
#include "eaclib/psx/sdma.h"
#include "eaclib/psx/sdmemman.h"
#include "eaclib/psx/smemman.h"
#include "eaclib/psx/spat2hdr.h"
#include "eaclib/psx/spktplay.h"
#include "eaclib/psx/splysdef.h"
#include "eaclib/psx/ssine.h"
#include "eaclib/psx/sst.h"
#include "eaclib/psx/sstrstat.h"
#include "eaclib/psx/sststat.h"
#include "eaclib/psx/stagpat.h"
#include "eaclib/psx/stream.h"
#include "eaclib/psx/systask.h"
#include "eaclib/psx/textcrnt.h"
#include "eaclib/psx/unbtree.h"
#ifdef __cplusplus
#include "game/common/mathnfs.h"
#else
#include "game/common/coorddef.h"
#include "game/common/matrixtdef.h"
#endif
#include "game/psx/texture.h"
#include "game/psx/unpack.h"
#include "lib/psx/blkfill.h"
#include "lib/psx/blkmov.h"
#include "lib/psx/fixdmult.h"
#include "lib/psx/getm.h"
#include "lib/psx/math64.h"
#include "lib/psx/shpdepth.h"
#include "lib/psx/sinfunc.h"

#ifdef __cplusplus
extern "C" {
#endif
void WriteSoundRegister(uint32 offset, uint32 value);
uint32 ReadSoundRegister(uint32 offset);
#ifdef __cplusplus
}
#endif

#endif
