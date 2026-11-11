#ifndef FETV_H
#define FETV_H

#include "xport.h"
#include "frontend/common/screenpinkslips.h"

void DrawTVLines(tTVConfig &tv);
void DrawTV(tTVConfig &tv);
void InitTV(tTVConfig &tv, tTexture::ShapeInfo *textures, sint16 index);
void TurnOffTV(tTVConfig &tv);
void TurnOnTV(tTVConfig &tv);

#endif
