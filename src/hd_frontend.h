#ifndef HD_FRONTEND_H
#define HD_FRONTEND_H

#include "xport.h"
#include "vid.h"
#include "game/common/car.h"
#include "game/common/renderview.h"
#include "game/common/transformer.h"
#include "game/psx/draw.h"

void hd_frontend_begin_frame(void);
void hd_frontend_end_frame(void);
void hd_frontend_reset_history(void);
void hd_frontend_sample(float phase);
void hd_frontend_begin(sint32 player, sint32 x, sint32 y);
void hd_frontend_capture(Car::tObj *car, const Draw::CarCache *state);
void hd_frontend_capture_halo(sint32 part, const Transformer::zObj *object, sint32 type, sint32 overlay, sint32 sortOffset);
void hd_frontend_capture_halo_tables(void);
void hd_frontend_mark_car(void);
bool hd_frontend_page_ui(uint32 token, uint16 *page);
bool hd_frontend_ready_ui(void);
bool hd_frontend_prepare_ui(void);
void hd_frontend_begin_ui(void);
bool hd_frontend_draw_ui(uint32 token, const float mapping[4]);
uint32 hd_frontend_halo_geometry(VID_VERTEX2D *vertices, const COORD16 *points, sint16 flag, sint32 type, const float matrix[3][4], bool mirror, float x, float y, float depthBias);
void hd_frontend_capture_shadow(const Car::tObj *car, const DRender::tView *view, const Draw::tPixMap *map, uint32 color);
void hd_frontend_capture_spot(const matrixtdef *matrix, const coorddef *position, const COORD16 *vertices, uint32 color);
void hd_frontend_capture_ring(const matrixtdef *matrix, const coorddef *position, const COORD16 *vertices, const Draw::tPixMap *map, const sint8 *states, const uint32 *colors);
void hd_frontend_forget_car(Car::tObj *car);
void hd_frontend_invalidate(void);
void hd_frontend_release(void);
bool hd_frontend_preview(void);
bool hd_frontend_preview_at(uint32 phase);

#endif
