#ifndef HD_CAR_NORMALS_H
#define HD_CAR_NORMALS_H

#include "xport.h"
#include "game/common/transformer.h"

void hd_car_normals_capture(const Transformer::zScene *scene, sint32 part, const Transformer::zObj *object);
void hd_car_normals_forget(const Transformer::zScene *scene);
const VEC3F *hd_car_normals_read(const Transformer::zObj *object);

#endif
