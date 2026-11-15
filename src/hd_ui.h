#ifndef HD_UI_H
#define HD_UI_H

#include "xport.h"
#include "psx.h"
#include "hd_sampling.h"

class HdUiLayout
{
    uint8 previous;

    HdUiLayout(const HdUiLayout &);
    HdUiLayout &operator=(const HdUiLayout &);

public:
    enum Anchor
    {
        Center,
        Left,
        Right,
        Stretch
    };

    HdUiLayout(Anchor anchor);
    ~HdUiLayout(void);
};

class HdUiImage
{
    uint16 previous[5];

    HdUiImage(const HdUiImage &);
    HdUiImage &operator=(const HdUiImage &);

public:
    HdUiImage(uint16 page, sint32 x, sint32 y, sint32 width, sint32 height);
    ~HdUiImage(void);
};

void hd_ui_begin_frame(sint32 frame = -1);
void hd_ui_end_frame(void);
void hd_ui_set_view(sint32 view);
void hd_ui_world_view(sint32 player);
void hd_ui_pause_frame(void);
void hd_ui_frontend_frame(bool supported);
void hd_ui_frontend_3d(void);
void hd_ui_frontend_marker(uint32 token);
bool hd_ui_frontend_ready(void);
bool hd_ui_present(void);
bool hd_ui_pause_ready(void);
bool hd_ui_world_ready(sint32 views);
void hd_ui_page(uint16 page);
void hd_ui_area(const PSX_RECT *area);
bool hd_ui_quad(const uint32 position[4], const uint32 color[4], uint8 code, const uint32 texture[4] = 0);
bool hd_ui_colored(const uint32 position[4], const uint32 color[4], uint8 code);
bool hd_ui_flat(const uint32 position[4], uint32 color, uint8 code);
bool hd_ui_glyph(uint32 position, uint32 texture, uint32 color, uint32 size);
void hd_ui_sprite(const SPRT *sprite);
void hd_ui_ft4(const POLY_FT4 *quad);
void hd_ui_gt4(const POLY_GT4 *quad);
void hd_ui_f4(const POLY_F4 *quad);
void hd_ui_g4(const POLY_G4 *quad);
void hd_ui_queue(const SPRT *source);
void hd_ui_queue(const POLY_FT4 *source);
void hd_ui_queue(const POLY_GT4 *source);
void hd_ui_queue(const POLY_F4 *source);
void hd_ui_queue(const POLY_G4 *source);
void hd_ui_queue(const POLY_F3 *source);
void hd_ui_queue(const POLY_G3 *source);
void hd_ui_queue(const LINE_F2 *source);
void hd_ui_queue(const LINE_G2 *source);
void hd_ui_queue(const DR_MODE *source);
void hd_ui_tach(sint32 player, float rpm, const void *source, const Draw::tPixMap *pixmap);
void hd_ui_tach_sample(sint32 player, float rpm, const void *source, uint32 tick);
void hd_ui_offset_last(float x, float y);
void hd_ui_tach_reset(bool hold = false);
void hd_ui_invalidate(void);
void hd_ui_release(void);
bool hd_ui_prepare(sint32 width, sint32 height, float phase = 1);
bool hd_ui_render(bool surface);
bool hd_ui_preview(void);

#endif
