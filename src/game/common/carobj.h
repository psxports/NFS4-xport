#ifndef CAROBJ_H
#define CAROBJ_H

#include "xport.h"
#include "game/common/aidatarecord.h"
#include "game/common/aiperson.h"
#include "game/common/aiphysic.h"
#include "game/common/aiscript.h"
#include "game/common/bo.h"
#include "game/common/bworldsm.h"
#include "game/common/car.h"
#include "game/common/cars.h"
#include "game/common/donutmode.h"
#include "game/common/gmesetup.h"
#include "game/common/mathnfs.h"
#include "game/common/trk.h"
#include "game/common/udff.h"

struct Car::tWheel
{
    coorddef currentPos;
    coorddef roadNormal;
    sint32 actualHeight;
    sint32 wheelAcc;
    sint32 wheelInAir;
    sint32 rebound;
    sint32 impactCompression;
    sint32 roadSurfaceType;
};

enum speechModeType
{
    SPEECH_MODE_NONE = 0,
    SPEECH_MODE_PURSUIT = 1,
    SPEECH_MODE_PURSUIT_WRONG_SIDE = 2,
    SPEECH_MODE_PURSUIT_BUMP_COP = 3,
    SPEECH_MODE_WARNING = 4,
    SPEECH_MODE_WARNING_WRONG_SIDE = 5,
    SPEECH_MODE_WARNING_BUMP_COP = 6,
    SPEECH_MODE_BUST = 7,
    SPEECH_MODE_APPROACH_ROADBLOCK = 8,
    SPEECH_MODE_ARREST = 9,
    SPEECH_MODE_COP_KILLER = 10,
    SPEECH_MODE_SENTINAL = 11,
};

enum speechIntensityType
{
    SPEECH_INTENSITY_LOW = 0,
    SPEECH_INTENSITY_MED = 1,
    SPEECH_INTENSITY_HIGH = 2,
    SPEECH_INTENSITY_SENTINAL = 3,
};

enum speechSourceType
{
    SPEECH_SOURCE_REGULAR_COP = 0,
    SPEECH_SOURCE_SUPER_COP = 1,
    SPEECH_SOURCE_ROADBLOCK = 2,
    SPEECH_SOURCE_SENTINAL = 3,
};

class AISpeechInfo
{
public:
    struct AISpeechInfo_t
    {
        speechModeType speechMode;
        speechIntensityType speechIntensity;
        speechSourceType speechSource;
        sint32 playerCarIndex;
        sint32 copCarIndex;
        sint32 warningNumber;
    };
};

struct Cars::tRenderInfo
{
    sint16 colorIndex;
    char currentCountry;
    char newCountry;
    sint32 textureStartIndex;
    sint32 bodyPitch;
    sint32 bodyRoll;
    sint32 currentPitch;
    sint32 currentRoll;
    sint32 currentHeight;
    sint32 pitchFactor;
    sint32 rollFactor;
    sint32 world_otz;
    sint32 sub_otz;
    uint32 *sub_ot;
    sint32 sub_otSize;
    sint32 sub_otOffset;
    void *sort_carObj;
    sint16 sort_flag;
    sint16 damageParts;
    sint32 light;
    char *palCopy;
    sint16 palNum;
    sint16 palCopyNum[20];
    char medOnly;
    char upgradeFlags;
    sint16 headLight;
    sint16 brakeLight;
    sint16 signalLight[2];
    sint16 currentCarType;
    sint16 newCarType;
    sint16 inside;
    sint16 detail;
    sint16 VRamX;
    sint16 VRamY;
    sint16 textureOffsetU;
    sint16 textureOffsetV;
    uint8 licenseOffsetU[2];
    uint8 licenseOffsetV[2];
    sint32 rideHeight;
    sint32 upgradeHeight;
};

struct AIPhysic::BrakeInfo
{
    uint8 brakeTable_[128];
    sint32 deceleration_;
};

struct CCOORD16
{
    sint16 x;
    sint16 y;
    sint16 z;
    sint16 light;
};

struct BWorldSm::Pos
{
    sint16 slice;
    sint16 stripQuadInd;
    sint16 simRotFlag;
    coorddef quadPts[4];
    CCOORD16 quadPts16[4];
    char sliceChanged;
    char quadChanged;
    char offEdge;
    char triangleFlag;
    coorddef normal;
    coorddef forward;
    char quad;
    uint8 chunk;
    char lastRezRequested;
    char rez;

    Trk::NewSimQuad *simQuad;
    Trk::NewStrip *strip;
    Trk::NewSimSlice *simSlice;
};

struct BO::tNewtonCollisionInfo
{
    sint32 collided;
    sint32 impulse;
    BO::tNewtonObj *otherObj;
    sint32 sfxType;
    sint32 disableCollisionTimer;
    coorddef collisionPoint;
    sint32 lastCollision;
    BO::tNewtonObj *lastOtherObj;
    sint32 lastImpulse;
    sint32 lastTime;
};

struct BO::tNewtonObj
{
    sint32 objID;
    uint16 oldSlice;
    uint16 totalSlice;
    BWorldSm::Pos simRoadInfo;
    sint32 distToPlayer;
    uint8 simOptz;
    uint8 active;
    uint8 reOrthoCounter;
    sint32 gravityMult;
    sint32 cumulatedRot;
    sint32 lastUpdated;
    coorddef position;
    coorddef linearVel;
    sint32 mass;
    sint32 massInv;
    sint32 speedXZ;
    sint32 xRelRoadCenter;
    sint16 positionXZ;
    sint16 eIndexEnvMap;
    sint16 eIndexShadow;
    sint32 wheelRot[2];
    sint32 wheelFrontX;
    sint32 wheelFrontZ;
    sint32 wheelBackX;
    sint32 wheelBackZ;
    sint32 wheelWidthF;
    sint32 wheelWidthB;
    matrixtdef orientMat;
    coorddef angularVel;
    sint32 moInertia;
    sint32 moInertiaInv;
    coorddef orientationToGround;
    coorddef dimension;
    sint32 dimensionRadius;
    matrixtdef roadMatrix;
    coorddef roadCenterPoint;
    sint32 roadGravityModifier;
    sint32 roadYaw;
    uint16 flightTime;
    uint16 deadTimer;
    sint32 groundElevation;
    sint32 groundVel;
    sint32 objAltitude;
    BO::tNewtonCollisionInfo collision;
    sint32 groundSurfaceType;
    sint32 driveSurfaceType;
    matrixtdef shadowMat;
    coorddef shadowCoord[4];
    sint32 damage[10];
};

struct Car::tObj
{
    BO::tNewtonObj N;
    char carName[9];
    char carNameLocalized[9];
    sint32 carIndex;
    sint32 humanIndex;
    sint32 sortIndex;
    sint32 carFlags;
    sint32 lap;
    sint32 unlap;
    sint32 roadSpan;
    sint32 glue;
    sint32 blowout;
    sint32 pullOver;
    sint32 revLimit;
    Car::tObj *swapCar;
    sint32 swapTime;
    GameSetup::tCarData *carInfo;
    Car::tWheel wheel[4];
    Car::tStats stats;
    sint32 crash;
    sint32 wrongway;
    coorddef angularAcc;
    coorddef linearAcc;
    coorddef linearAcc_ch;
    coorddef linearVel_ch;
    coorddef angularVel_ch;
    coorddef linearAcc_rh;
    Car::tControl control;
    char RSGasLevel;
    char RSBrakeLevel;
    sint32 RSSteering;
    sint32 RSControl;
    Car::tSpecs *specs;
    sint32 flywheelRpm;
    sint32 wheelSpin;
    sint32 frontWheelSpin;
    sint32 wheelLock;
    sint32 slide;
    sint32 gTransferFront;
    sint32 gTransferRight;
    sint32 frontSkid;
    sint32 rearSkid;
    sint32 oldSkidState;
    coorddef oldSkidPoint[4];
    sint32 oldAudioSkidState;

    void (*funcUpdateRoadInfo)(BO::tNewtonObj *);
    void (*funcReplay)(Car::tObj *);
    void (*funcControl)(Car::tObj *);
    void (*funcStats)(Car::tObj *);
    void (*funcHandlingPhysics)(Car::tObj *);
    void (*funcGravityPhysics)(BO::tNewtonObj *);
    void (*funcQDPhysicsUpdateVel)(Car::tObj *);
    void (*funcQDPhysicsUpdateRot)(BO::tNewtonObj *);
    void (*funcTestMeForCollisions)(BO::tNewtonObj *);
    void (*funcDoPostCollisionStuff)(Car::tObj *);
    sint32 personalityIndex;
    AIPerson::AIPerson_t *personality;
    AIScript::AIScript_t script;
    AISpeechInfo::AISpeechInfo_t speechInfo;
    sint32 carInLane;
    sint32 desiredDirection;
    sint32 direction;
    sint32 desiredLatPos;
    sint32 desiredSpeed;
    sint32 originalDesiredSpeed;
    sint32 currentSpeed;
    sint32 speed;
    sint32 laneSlack;
    sint32 AIFlags;
    sint32 roadPosition;
    Car::tObj *basisCar;
    sint32 aiShiftDuration;
    sint32 aiShiftTimer;
    sint32 redLine;
    sint32 tailgateTimer;
    sint32 physicsModelTimer;
    sint32 trafficSpeedRandomizingFactor;
    sint32 laneChangeSpeed;
    sint32 topSpeeds[7];
    sint32 invTopSpeeds[7];
    sint16 accTable[112];
    AIDataRecord_AccTable_t *accelerationRecord;
    AIDataRecord_CurveSpeedTable_t *curveSpeedTable;
    AIPhysic::BrakeInfo *brakeInfo;
    sint32 speedFactor;
    sint32 barrierThinkHarder;
    sint32 laneIndex;
    sint32 copTopSpeed;
    sint32 copAccMult;
    coorddef desiredVector;
    sint32 aCarWRTDesired;
    coorddef angularAcc_ch;
    sint32 driveDirectionReverseTime;
    sint32 driveDirection;
    sint32 driveDirectionTimer;
    sint32 aCar;
    sint32 aDesired;
    sint32 aCarWRTRoad;
    sint32 lateralVelocity;
    coorddef targetPos;
    sint32 targetLatPos;
    sint32 rampDesiredLatPos;
    sint32 preferredLateralPosition;
    sint32 preferredLateralPositionPower;
    sint32 timeOffRoad;
    sint32 max_clacc;
    sint32 max_aa;
    sint32 aiGlue;
    sint32 drag;
    sint32 slackProb;
    sint32 accNitrous;
    sint32 speedNitrous;
    sint32 wipeOutStartTick;
    sint32 wipeOutEndTick;
    sint32 btcGlueModifier;
    donutMode_t donutMode;
    sint32 AIFishtailEndTick;
    sint32 lookAheadSlice;
    sint32 forceNoSimOptz;
    sint32 gripFactor;
    Car::tObj *fallBehindCar;
    Car::tObj *nextAIRacer;
    sint32 caravanFollowBehindDistanceMeters;
    sint32 caravanTimer;
    sint32 AISlot;
    sint32 damageMult;
    sint32 topSpeedUpgradeMult;
    sint32 accUpgradeMult;
    sint32 extraWallCollisionAllowance;
    Cars::tCollisionInfo collision;
    sint32 audioDamageScrape;
    sint32 audioCount;
    Cars::tAudio audio[7];
    Cars::tRenderInfo render;
    sint32 async_handle;
};

#endif
