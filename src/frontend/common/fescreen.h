#ifndef FESCREEN_H
#define FESCREEN_H

#include "xport.h"
#include "frontend/common/femenu.h"
#include "frontend/psx/fetexture.h"

extern tTexture::ShapeInfo *gCurrentShapes;

struct tShapeInformation
{
    tTexture::ShapeInfo *fShapes;
    char *fFile;
    char *fDestFile;
    uint32 async_handle;
    uint16 fNumShapes;
    uint16 fFlags;
    char fFilename[16];
    bool fLoadCancelled;
};

class tScreen
{
public:

    static sint32 fSuppressLoadingText;

    enum TransitionType
    {
        kScreen_TransitionTypeItem = 0,
        kScreen_TransitionTypeMenu = 1,
        kScreen_TransitionTypeScreen = 2
    };

    tShapeInformation fPermShapes;
    tShapeInformation fSwapShapes;
    sint32 fTransitionTicks;
    bool fTransitionOff;
    sint32 fInternalScreenFadeVal;
    sint16 fScreenFadeVal;
    virtual void GetShapeInfo(sint16 &numPermShapes, sint16 &numSwapShapes, char **permFileName, char **swapFileName);

    virtual void DrawBackground(void);

    virtual void DrawForeground(void);

    virtual ~tScreen(void);
    virtual void PreLoad(void);
    virtual void Initialize(void);
    virtual void Cleanup(void);
    virtual bool TransitionIsFinished(void);
    virtual void ProcessInput(tPlayer fromPlayer, tInputKeyType &keyval, tMenuCommand &command);
    static void DisplayLoadingText(void);
    static void GoNonInterlaced(void);
    void DrawBackgroundImage(sint32 startShape, sint32 numShapes, tTexture::ShapeInfo *shapes, sint32 flip_axis);
    void AsyncLoadPermanentShapeFile(char *fileName);
    void AsyncLoadSwapShapeFile(char *fileName);
    bool IsShapeFileLoaded(tShapeInformation &shapes);
    void UploadPermanentShapes(sint32 numPermanentShapes);
    void UploadSwapShapes(sint32 numSwapShapes);
    tScreen(void);
    void Draw(bool drawBackground);
    void AsyncLoadShapeFile(char *name, tShapeInformation &data);
    void CancelAsyncLoad(tShapeInformation &data);
    void InitializeShapes(tShapeInformation &data, uint32 numShapes);
    void FreeShapes(tShapeInformation &data);
    void UploadShapes(tShapeInformation &data, sint16 x, sint16 y, sint16 numShapes, sint16 index);
    void TransitionOff(TransitionType type, tMenu *);
    void TransitionOn(TransitionType type, tMenu *);
    void UpdateTransition(void);
};

#endif
