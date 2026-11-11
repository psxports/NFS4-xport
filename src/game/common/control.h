#ifndef CONTROL_H
#define CONTROL_H

#include "game/common/car.h"

class Control
{
public:
    static void AI(Car::tObj *carObj);
    static void Human(Car::tObj *carObj);
};

#endif
