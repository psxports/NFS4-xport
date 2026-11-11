#ifndef CAR_H
#define CAR_H

#include "xport.h"

class Car
{
public:
    struct tObj;

    struct tStats
    {
        sint32 extractSlice;
        sint32 sliceTotal;
        sint32 sliceTime;
        sint32 slice;
        sint32 lastSlice;
        sint32 lap;
        sint32 lapTime;
        sint32 time[4];
        sint32 topSpeed[4];
        sint32 position;
        sint32 fatalCrashes;
        sint32 finishType;
        sint32 checkpointDifference;
        sint32 checkpointUpdate;
        sint32 checkpointDisplay;
        sint32 numWarnings;
        sint32 numFines;
        sint32 numArrests;
        sint32 carType;
        sint32 carClass;
        sint32 carNameIndex;
        sint32 carFlags;
        sint32 finalDamage;
        sint32 finalNumWarnings;
        sint32 finalNumFines;
        sint32 finalNumArrests;
        sint32 finalFinishType;
        sint32 finalPosition;
        sint32 finalTotalTime;
        sint32 finalLapTime[4];
        sint32 finalBestLap;
    };

    struct tControl
    {
        char desiredGasLevel;
        char desiredBrakeLevel;
        char desiredGear;
        char gasLevel;
        char brakeLevel;
        char lastGear;
        char gear;
        char downShifting;
        char gearShiftTimer;
        char handBrake;
        char horn;
        char lights;
        char lookBehind;
        char abort;
        char event;
        char queuedEvent;
        sint32 desiredSteering;
        sint32 steering;
        sint32 hanno;
    };

    struct tSpecs
    {
        sint32 mass;
        sint32 numGears;
        sint32 gearShiftDelay;
        sint32 velToRpmRatio[8];
        sint32 gearEfficiency[8];
        sint32 torqueCurve[41];
        sint32 redline;
        sint32 maxSpeed;
        sint32 frontDriveRatio;
        sint32 maxBrakeAcc;
        sint32 frontBrakeRatio;
        sint32 wheelBase;
        sint32 frontGripBias;
        sint32 maxSteeringAcc;
        sint32 steeringRamp;
        sint32 lateralGripMult;
        sint32 frontAeroDownForce;
        sint32 rearAeroDownForce;
        sint32 gasOffFactor;
        sint32 gTransferFactor;
        sint32 slideMultiplier;
        sint32 spinVelCap;
        sint32 dampingPitchRate;
        sint32 dampingRollRate;
        sint32 suspensionStiffness;
        sint32 pitchAngularVelCap;
        sint32 rollAngularVelCap;
        sint32 damageFactor;
        sint32 bodyPitchFactor;
        sint32 bodyRollFactor;
        sint32 tireRange;
        sint32 rideOffset;
        sint32 dragFactor;
        sint32 redlineInv;
        sint32 gearVelInv[8];
        sint32 velToRpmRatioInv[8];
        sint32 gearAccCoeff[8];
        sint32 dragCoeff;
        sint32 alphaToAccRotInertia;
        sint32 accToAlphaRotInertia;
        sint32 lateralGripMultInv;
    };

    struct tWheel;

    typedef sint16 tAccTable[112];
    static void TireSkiddingStuff(tObj *carObj);
    static void DoSkiddingStuff(tObj *carObj);
    static void DoPostCollisionStuff(tObj *carObj);
};

#endif
