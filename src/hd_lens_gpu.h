#ifndef HD_LENS_GPU_H
#define HD_LENS_GPU_H

#include "vid.h"

bool hd_lens_gpu_prepare(VID_TEXTURE *vram);
void hd_lens_gpu_reset(bool hold = false);
void hd_lens_gpu_release(void);
VID_TARGET *hd_lens_gpu_target(void);
bool hd_lens_gpu_resolve(VID_TARGET *destination, bool offscreen, VID_RECT viewport, const float *clear, float x, float y, bool eligible, bool paused, sint32 tick);
void hd_lens_gpu_pass(sint32 pass, float phase, float centerX, float centerY);
void hd_lens_gpu_complete(bool success);

#endif
