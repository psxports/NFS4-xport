#ifndef SCREENCARSELECT_H
#define SCREENCARSELECT_H

#include "xport.h"
#include "psx.h"
#include "frontend/common/fedialog.h"
#include "frontend/common/screenpinkslips.h"
#include "frontend/common/tcarinfo.h"
#include "frontend/psx/fetexture.h"

typedef sint32 FEVECTOR[4];

typedef sint32 FEMATRIX[4][4];

struct CARDINFO_def;

void DrawCar(tCarInfo &carInfo, sint16 x, sint16 y, float camerax, float cameray, char brightness, bool reflection, uint32 rotate, tPlayer player);

struct tOverlay
{
    PSX_RECT location[2];
    sint16 transition;
    sint16 delta;
    sint16 direction;
    sint16 ID;
};

class tVideoWall
{
public:
    tTVConfig *fTVs;
    sint16 fFirstTVShape;
    sint16 fNumTVs;
    tTexture::ShapeInfo *fTVShapes;
    sint16 *tvOrder;
    uint32 fTVTicks;
    sint16 fTransitionDirection;
    sint16 fFlipAxis;
    sint16 fOffsetX;
    sint16 fOffsetY;
    sint16 fAvailableTextID;
    sint16 fAvailable;
    sint16 fAvailableBright;
    sint16 fValid;
    sint16 fAvailableX;
    sint16 fAvailableY;
    tTexture::ShapeInfo *fIconShapes;
    sint16 fIcon;
    sint16 fIconFrames;
    sint16 fIconX;
    sint16 fIconY;
    uint32 fUpdated;

    void Initialize(tTVConfig *tvs, tTexture::ShapeInfo *shapes, sint16 firstTV, sint16 numTVs, sint16 *tvOrdering, sint16 flip_axis);
    void UpdateImages(void);
    void SetAvailableText(sint16 textID, sint16 x, sint16 y);
    void SetAvailableIcon(sint16 icon, sint16 numFrames, sint16 x, sint16 y, tTexture::ShapeInfo *iconImages);
    void SetOffset(sint16 x, sint16 y);
    void SetAvailable(sint16 avail);
    void SetValid(sint16 valid);
    void UpdateTransition(void);
    void Draw(void);
    void TurnOff(void);
    void TurnOffInstant(void);
    void TurnOn(void);
};

class tScreenCarSelect : public tScreen
{
public:
    tScreenCarSelect(void);
    virtual ~tScreenCarSelect(void);
    virtual void Cleanup(void);
    void DrawOverlay(tOverlay *overlay);
    void SetState(sint32 state);
    void CalcSplinePosition(sint32 knot1, sint32 knot2, sint32 knot3, sint32 knot4, uint32 elapsed, sint32 &camY, sint32 &camZ, sint32 &screenX, sint32 &screenY, sint32 &camRot);
    virtual void GetShapeInfo(sint16 &numPermShapes, sint16 &numSwapShapes, char **permFileName, char **swapFileName);
    virtual void DrawVideoWall(sint16 y);
    virtual void InitializeVideoWall(void);
    virtual void UpdateVideoWall(tCarInfo &carInfo);
    virtual bool GetCar(tCarInfo &carInfo);
    virtual void AllocateAsyncBuffer(void);
    virtual void FreeAsyncBuffer(void);
    virtual void Initialize(void);
    virtual void ProcessInput(tPlayer, tInputKeyType &keyval, tMenuCommand &);
    void SetBrightness(sint16 bright, sint16 i);
    void UpdateBrightness(sint16 i);
    virtual void DrawBackground(void);
    void DrawSliders(tCarInfo &carInfo, sint16 x, sint16 y);
    virtual void DrawForeground(void);
    tOverlay fOverlays[7];
    tOverlay *fCurrentOverlays[4];
    sint16 fState;
    sint16 fPreviousCar;
    sint16 fPreviousCarID;
    sint16 fPreviousCountry;
    tTVConfig tvConfigs[10];
    tVideoWall fVideoWall[2];
    sint16 fBrightness[2];
    sint16 fDestBrightness[2];
    sint32 fFadeTicks[2];
    sint32 fTVsInitialized;
    bool fInShowroom;
    uint32 fShowroomTicks;
    uint32 fSpeechTicks;
    bool fSpeechPlayed;
    sint32 fSplineInterval;
    uint32 fCameraRotation;
};

class tScreenCarSelectDuel : public tScreenCarSelect
{
public:
    virtual void PreLoad(void);
    virtual void AllocateAsyncBuffer(void);
    virtual void FreeAsyncBuffer(void);
    virtual void InitializeVideoWall(void);
    virtual void Initialize(void);
    virtual void Cleanup(void);
    virtual void DrawVideoWall(sint16 y);
    virtual void DrawOpponentVideoWall(sint16 y);
    virtual void GetShapeInfo(sint16 &numPermShapes, sint16 &numSwapShapes, char **permFileName, char **swapFileName);
    virtual void UpdateVideoWall(tCarInfo &carInfo);
    void UpdateOpponentVideoWall(tCarInfo &carInfo);
    virtual void DrawBackground(void);
    virtual void DrawForeground(void);
    sint16 fPreviousOpponent;
    sint32 fOpponentTVsInitialized;
    tShapeInformation fOpponentShapes;
};

class tScreenCarSelectTwoPlayer : public tScreenCarSelect
{
public:
    tScreenCarSelectTwoPlayer(void);
    virtual ~tScreenCarSelectTwoPlayer(void)
    {
        FUNCTION_MARKER(0x8003F714, "FRONT.BIN");
    }

    virtual bool GetCar(tCarInfo &carInfo);
    virtual void DrawVideoWall(sint16 y);
    virtual void GetShapeInfo(sint16 &numPermShapes, sint16 &numSwapShapes, char **permFileName, char **swapFileName);
    virtual void UpdateVideoWall(tCarInfo &carInfo);
    virtual void TurnOffVideoWall(void);
    virtual void DrawBackground(void);
    virtual void DrawForeground(void);
    virtual void SetDialog(void);
    virtual void AllocateAsyncBuffer(void);
    virtual void InitializeVideoWall(void);
    virtual void Initialize(void);
    virtual void Cleanup(void);
    tDialogBackUpOnly CarDialog;
};

class tScreenPinkSlipsCarSelect : public tScreenCarSelectTwoPlayer
{
public:
    virtual ~tScreenPinkSlipsCarSelect(void)
    {
        FUNCTION_MARKER(0x8003F6D0, "FRONT.BIN");
    }

    virtual bool GetCar(tCarInfo &carInfo);
    virtual void DrawBackground(void);
    void DoMemCardStuff(void);

    virtual void Initialize(void);
    virtual void Cleanup(void);
    virtual void SetDialog(void);
    virtual void ProcessInput(tPlayer, tInputKeyType &keyval, tMenuCommand &);
    virtual void GetShapeInfo(sint16 &numPermShapes, sint16 &numSwapShapes, char **permFileName, char **swapFileName);
    sint32 waitfordialog;
    CARDINFO_def *pCI;
    sint32 fStartCheckTick;
    sint32 fCardFailed;
    bool fExitingScreen;
};

extern tScreenCarSelect *screenCarSelect;
extern tScreenCarSelectDuel *screenCarSelectDuel;
extern tScreenCarSelectTwoPlayer *screenCarSelectTwoPlayer;
extern tScreenCarSelectTwoPlayer *screenCarSelectPlayerTwo;
extern tScreenPinkSlipsCarSelect *screenPinkSlipsCarSelectTwoPlayer;
extern tScreenPinkSlipsCarSelect *screenPinkSlipsCarSelectPlayerTwo;

#endif
