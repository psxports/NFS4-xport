#ifndef COLLIDE_H
#define COLLIDE_H

#include "xport.h"
#include "game/common/bo.h"
#include "game/common/car.h"
#include "game/common/coorddef.h"

class Collide
{
public:
    static BO::tNewtonObj *gRegistry[16];

    static sint32 gRaiseUp;
    static sint32 gNumRegistered;

    static void DoObjectFixedObjectCollision(BO::tNewtonObj *o, coorddef *p, coorddef *v, coorddef *n);
    static void LimitAngularVel(BO::tNewtonObj *o);
    static void TestWithPlane(BO::tNewtonObj *o, coorddef *normal, coorddef *samplePoint);
    static sint32 DoObjectObjectCollision(BO::tNewtonObj *o0, BO::tNewtonObj *o1, coorddef *p, coorddef *normal);
    static sint32 DoActualObjectCollisionCheck(BO::tNewtonObj *o0, BO::tNewtonObj *o1, coorddef *p, coorddef *normal);
    static sint32 TestObjectVertices(BO::tNewtonObj *o0, BO::tNewtonObj *o1, coorddef *p, coorddef *normal);
    static sint32 CheckForCollisionBetween(BO::tNewtonObj *o0, BO::tNewtonObj *o1);
    static void ClearCollisionRegistry(void);
    static sint32 CheckAccuratePointRadiusCollision(BO::tNewtonObj *newObj, coorddef *point, sint32 radius);
    static void CheckMeForCollisions(BO::tNewtonObj *newObj);
};

bool CollisionHumanFlag(const BO::tNewtonObj *obj);
bool CollisionCopFlag(const BO::tNewtonObj *obj);

#endif
