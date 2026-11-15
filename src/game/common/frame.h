#ifndef FRAME_H
#define FRAME_H

#include "xport.h"

class FramePresentationHold
{
    bool active;

    FramePresentationHold(const FramePresentationHold &);
    FramePresentationHold &operator=(const FramePresentationHold &);

public:
    FramePresentationHold(void);
    ~FramePresentationHold(void);
};

class FramePresentation
{
    bool owner;
    bool requestedHd;
    bool changed;
    float phase;

    FramePresentation(const FramePresentation &);
    FramePresentation &operator=(const FramePresentation &);

public:
    FramePresentation(void);
    ~FramePresentation(void);
    void Finish(void);
    static bool Active(void);
    static bool WantsHd(void);
    static bool Handoff(void);
    static void SetPhase(float value);
    static void FinishActive(void);
    static void SubmitOriginal(sint32 frame, sint32 view = -1, sint32 environmentFrame = -1);
    static void BeginOriginal(void);
    static bool DeferGlyph(void *marker, uint32 position, uint32 texture, uint32 color, uint32 size);
    static void BuildOriginalGlyphs(void);
    static bool DeferOriginalLine(void *marker, const uint32 position[4], const uint32 color[4], uint8 code);
    static bool DeferOriginalPolygon(void *marker, const uint32 position[4], const uint32 color[4], uint8 code, const uint32 texture[4] = 0);
    static bool DeferQuad(void *marker, const uint32 position[4], const uint32 color[4], uint8 code, const uint32 texture[4] = 0);
    static bool DeferColored(void *marker, const uint32 position[4], const uint32 color[4], uint8 code);
    static bool DeferFlat(void *marker, const uint32 position[4], uint32 color, uint8 code);
    static void BuildOriginalQuads(void);
    static bool RestoreOriginal(void);
    static void ReleaseOriginal(void);
};

#endif
