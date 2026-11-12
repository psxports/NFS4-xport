#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "xport.h"
#include "psx.h"
#include "eaclib/psx/eaclib.h"
#include "eaclib/psx/globals.h"
#include "eaclib/psx/pad.h"
#include "frontend/common/feapp.h"
#include "frontend/common/feaudio.h"
#include "frontend/common/fecars.h"
#include "frontend/common/fedialog.h"
#include "frontend/common/feinput.h"
#include "frontend/common/fememcard.h"
#include "frontend/common/femenudefs.h"
#include "frontend/common/fetextrender.h"
#include "frontend/common/fetools.h"
#include "frontend/common/front.h"
#include "frontend/common/screenusername.h"
#include "frontend/common/statchk.h"
#include "frontend/psx/fetexture.h"
#include "frontend/psx/movie.h"
#include "frontend/psx/psxfront.h"
#include "game/common/audiocmn.h"
#include "game/common/audiomus.h"
#include "game/common/car.h"
#include "game/common/cars.h"
#include "game/common/clock.h"
#include "game/common/gmesetup.h"
#include "game/common/nfs3.h"
#include "game/common/textsys.h"
#include "game/psx/audio.h"
#include "game/psx/drawc.h"
#include "game/psx/textpix.h"
#include "game/psx/texture.h"

struct tActiveLine
{
    sint16 startTick;
    sint16 endTick;
    sint16 x1;
    sint16 y1;
    char type;
    sint16 data;
};

class FE3d
{
public:
    typedef CVECTOR zColor;

    typedef SVECTOR zNormal;

    typedef COORD16 zVertex;

    struct zUV
    {
        uint8 u;
        uint8 v;
    };

    struct zFacet
    {
        uint16 vertexId0, vertexId1, vertexId2;
        uint16 uvId0, uvId1, uvId2;
        SVECTOR normal[3];
        CVECTOR color;
        uint8 flag, pad1, pad2, pad3;
    };

    struct zObj
    {
        sint32 numVertex, numUV, numFacet;
        uint8 textureId, textureWidth, textureHeight, pad1;
        COORD16 *vertex;
        zUV *uv;
        COORD16 *Nvertex;
        zFacet *facet;
    };
};

static sint32 currentVideo;
uint32 gLargestUnused;
tFEApplication *FEApp;

tFEApplication::tFEApplication(void)
{
    FUNCTION_MARKER(0x80012FA0, "FRONT.BIN");
    tDialogBase *dialogs[3] = { &messagePopup, &helpPopup, &MemCardDialog };
    for (sint32 i = 0; i < 3; ++i)
    {
        tDialogBase *dialog = dialogs[i];
        dialog->currentlyOn = false;
        dialog->reservedheight = 0;
        dialog->MaxH = 0;
        dialog->OffsetY = 0;
        dialog->OffsetX = 0;
        dialog->height = 0;
        dialog->width = 0;
        dialog->top = 0;
        dialog->left = 0;
        dialog->MaxW = 288;
        dialog->specificPlayer = -1;
        dialog->fDefault = 0;
        dialog->timeOutTicks = 0;
    }
    messagePopup.Centerit = false;
    messagePopup.fFullyOpen = 0;
    messagePopup.fFadeText = 128;
    helpPopup.variant = -1;
    helpPopup.timeOutTicks = 1400;
    MemCardDialog.Centerit = false;
    MemCardDialog.fFullyOpen = 0;
    MemCardDialog.fFadeText = 128;
    MemCardDialog.timeOutTicks = 1152;
    for (sint32 i = 0; i < 2; ++i)
    {
        gotName[i] = 0;
        needName[i] = 0;
        speechToPlay[i] = -1;
    }
}

tFEApplication::~tFEApplication(void)
{
    FUNCTION_MARKER(0x800131C4, "FRONT.BIN");
}

void tFEApplication::PerformMenuInitialization(void)
{
    FUNCTION_MARKER(0x80013234, "FRONT.BIN");
    fCurrentMusic = 0;
    AudioMus::Volume(((uint8)frontEnd.musicVolume * 35) >> 6);
    tDialogBase::InitializeClass();
    Clock::SystemStartUp();
    Draw::gDoVSync = 1;
    FETextRender::SetABR(0, false);
}

void tFEApplication::PerformMenuDestruction(void)
{
    FUNCTION_MARKER(0x8001329C, "FRONT.BIN");

    Clock::SystemCleanUp();
    for (sint16 i = 0; i < 2; ++i)
    {
        if (fCurrentScreen[i] != 0)
            fCurrentScreen[i]->Cleanup();
        fCurrentScreen[i] = 0;
    }
    AudioMus::StopSong(1000);
    Draw::gDoVSync = 0;
    FETextRender::SetABR(0, false);
}

void tFEApplication::DrawHelpIcons(void)
{
    FUNCTION_MARKER(0x80013344, "FRONT.BIN");
    XASSERT(fCurrentMenu[0] != 0);
    tDrawShapeExtended flags;
    sint32 Col = 0x786e14;
    flags.tint[0] = Col;
    sint32 x = 14;
    sint32 y = (screenheight - 25);
    if (fCurrentMenu[0] != 0 && fCurrentMenu[0]->VertHelp != 0)
    {
        char string[12];
        char string2[2];
        string2[1] = 0;
        strcpy(string, TextSys::Word(252));
        for (sint32 i = (sint32)strlen(string) - 1; i >= 0; --i)
        {
            sint16 textY = (sint16)y;
            y = (y - 8);
            string2[0] = string[i];
            FETextRender::FullTextRGB(string2, 14, textY, Col, 0, 0);
        }
        y = (y + 8);
        if ((gPadinfo.buf[0].nopad == 0 && gPadinfo.buf[0].ID != 0x23) || (gPadinfo.buf[4].nopad == 0 && gPadinfo.buf[4].ID != 0x23))
        {
            y = (y - 8);
            DrawShapeExtended(53, 0x18, x, y, 0, 0, &flags);
        }
        if (gPadinfo.buf[0].ID == 0x23 || gPadinfo.buf[4].ID == 0x23)
            DrawShapeExtended(54, 0x18, 13, ((y - 9)), 0, 0, &flags);
    }
    else
    {
        if ((gPadinfo.buf[0].nopad == 0 && gPadinfo.buf[0].ID != 0x23) || (gPadinfo.buf[4].nopad == 0 && gPadinfo.buf[4].ID != 0x23))
        {
            DrawShapeExtended(53, 0x18, x, y, 0, 0, &flags);
            x += 14;
        }
        if (gPadinfo.buf[0].ID == 0x23 || gPadinfo.buf[4].ID == 0x23)
        {
            DrawShapeExtended(54, 0x18, x, y, 0, 0, &flags);
            x += 15;
        }
        FETextRender::FullTextRGB(TextSys::Word(252), x, y, Col, 0, 0);
        sint32 pixels = textpixels(TextSys::Word(252));
        sint32 length = (sint32)strlen(TextSys::Word(252));
        x = (((x + 5) + pixels) - length);
        if (fCurrentMenu[0]->fOptionsMenu != 0 || (fCurrentMenu[1] != 0 && fCurrentMenu[1]->fOptionsMenu != 0))
        {
            if ((gPadinfo.buf[0].nopad == 0 && gPadinfo.buf[0].ID != 0x23) || (gPadinfo.buf[4].nopad == 0 && gPadinfo.buf[4].ID != 0x23))
            {
                DrawShapeExtended(55, 0x18, x, y, 0, 0, &flags);
                x = (x + 14);
            }
            if (gPadinfo.buf[0].ID == 0x23 || gPadinfo.buf[4].ID == 0x23)
            {
                DrawShapeExtended(56, 0x18, x, y, 0, 0, &flags);
                x = (x + 15);
            }
            FETextRender::FullTextRGB(TextSys::Word(253), x, y, Col, 0, 0);
        }
    }
}

void tFEApplication::Redraw(void)
{
    FUNCTION_MARKER(0x8001373C, "FRONT.BIN");
    char saveFPlayer = fPlayer;
    FeAudio::systemtask(0);
    Draw::StartFrameRender();
    Draw::StartRenderingView(Draw::gPlayer1View);
#if 0
    char buffer[32];
    sprintf(buffer, "%d", (sint32)largestunused());
    FETextRender::FullText(buffer, 256, 215, textType_FramedInfo, textState_Selected, 0);
    sint32 buffered = AudioMus::Buffered();
    sint32 threshold = AudioMus::Threshold();
    sprintf(buffer, "%d/%d", buffered, threshold);
    FETextRender::FullText(buffer, 16, 215, textType_FramedInfo, textState_Hilighted, 0);
#endif
    DRAWENV *drenv = Draw::GetDRAWENV(Draw::gPlayer1View, gFlip);
    sint16 height = (sint16)(fCurrentMenu[1] != 0 ? screenheight / 2 : screenheight);
    tDialogBase::DrawAllDialogs();
    DrawHelpIcons();
    tMenuCommand emptycommand;
    tGlobalMenuDefs *defs = menuDefs;
    if (gPadinfo.buf[0].nopad == 0 && gPadinfo.buf[4].nopad == 0)
        defs->itemMainTwoPlayerRace.fFlags &= ~1;
    else
    {
        defs->itemMainTwoPlayerRace.fFlags |= 1;
        tInputKeyType JustOneToPass = kInput_KeyType_Up;
        if (fCurrentMenu[0]->fItemList[fCurrentMenu[0]->fCurrentItem] == &defs->itemMainTwoPlayerRace)
            defs->menuMain.tMenuNFS4::ProcessInput(kPlayerOne, JustOneToPass, emptycommand);
    }
    defs = menuDefs;
    if (gPadinfo.buf[0].nopad == 0)
        defs->itemMainOnePlayerRace.fFlags &= ~1;
    else
    {
        defs->itemMainOnePlayerRace.fFlags |= 1;
        tInputKeyType JustOneToPass = kInput_KeyType_Down;
        if (fCurrentMenu[0]->fItemList[fCurrentMenu[0]->fCurrentItem] == &defs->itemMainOnePlayerRace)
            defs->menuMain.tMenuNFS4::ProcessInput(kPlayerOne, JustOneToPass, emptycommand);
    }
    PSX_RECT r;
    r.x = 0;
    r.y = drenv->clip.y;
    r.w = 512;
    r.h = (sint16)screenheight;
    Draw::PrimStruct *cprim = &DrawC::carCache.head.cprim;
    DR_AREA *daprim = reinterpret_cast<DR_AREA *>(cprim->PrimPtr);
    setaddr(daprim, getaddr(cprim->LastPrim));
    cprim->PrimPtr = reinterpret_cast<char *>(daprim + 1);
    setaddr(cprim->LastPrim, daprim);
    SetDrawArea(daprim, &r);
    for (sint16 i = 1; i >= 0; --i)
    {
        fPlayer = (char)i;
        fYOffset = (sint16)(i * height);
        if (fCurrentMenu[(uint8)fPlayer] != 0)
            fCurrentMenu[(uint8)fPlayer]->UpdateTransition();
        if (fParentMenu[(uint8)fPlayer] != 0)
            fParentMenu[(uint8)fPlayer]->UpdateTransition();
        if (fCurrentScreen[(uint8)fPlayer] != 0)
            fCurrentScreen[(uint8)fPlayer]->UpdateTransition();
        if (fCurrentMenu[(uint8)fPlayer] != 0 && fCurrentMenu[(uint8)fPlayer]->IsSubMenu())
            fCurrentMenu[(uint8)fPlayer]->Draw();
        if (fCurrentScreen[(uint8)fPlayer] != 0)
            fCurrentScreen[(uint8)fPlayer]->Draw(false);
        if (fCurrentMenu[(uint8)fPlayer] != 0)
        {
            bool drawMenu = waitingForOtherPlayer[(uint8)fPlayer] == 0;
            if (!drawMenu && !fCurrentMenu[(uint8)fPlayer]->TransitionIsFinished())
                drawMenu = !fCurrentMenu[(uint8)fPlayer]->IsSubMenu();
            if (drawMenu)
                fCurrentMenu[(uint8)fPlayer]->Draw();
            if (fParentMenu[(uint8)fPlayer] != 0)
                fParentMenu[(uint8)fPlayer]->Draw();
        }
        if (fCurrentScreen[(uint8)fPlayer] != 0)
            fCurrentScreen[(uint8)fPlayer]->Draw(true);
        r.x = 0;
        r.y = (sint16)((uint16)drenv->clip.y + (uint16)fYOffset);
        r.w = 512;
        r.h = height;
        daprim = reinterpret_cast<DR_AREA *>(cprim->PrimPtr);
        setaddr(daprim, getaddr(cprim->LastPrim));
        cprim->PrimPtr = reinterpret_cast<char *>(daprim + 1);
        setaddr(cprim->LastPrim, daprim);
        SetDrawArea(daprim, &r);
    }
    fPlayer = saveFPlayer;
    Draw::StopRenderingView(Draw::gPlayer1View);
    Draw::StopFrameRender();
}

static void FreeHelpShapeCluts(void)
{
    FUNCTION_MARKER(0x80013F98, "FRONT.BIN");
    for (sint32 i = 0; i < 59; ++i)
    {
        if (gHelpShapes[i].clutID != 0)
        {
            Texture::MenuReleaseClutId(gHelpShapes[i].clutID);
            gHelpShapes[i].clutID = 0;
        }
    }
}

void tFEApplication::UpdateMusic(void)
{
    FUNCTION_MARKER(0x80013D60, "FRONT.BIN");
    static char menuPattern[] = "zmenu*";
    static char garagePattern[] = "garage*";
    static char showcasePattern[] = "showcase*";
    static char victoryPattern[] = "victory*";
    AudioMus::Volume(((uint8)frontEnd.musicVolume * 35) >> 6);
    uint32 music = fCurrentMusic;
    if ((music & 0x1000U) != 0)
        AudioMus::PlaySong(menuPattern);
    else if ((music & 0x2000U) != 0)
        AudioMus::PlaySong(garagePattern);
    else if ((music & 0x4000U) != 0)
        AudioMus::PlaySong(showcasePattern);
    else if ((music & 0x8000U) != 0)
        AudioMus::PlaySong(victoryPattern);
    else
        AudioMus::StopSong(1000);
}

void tFEApplication::SetMenu(sint16 i, tMenu *menu)
{
    FUNCTION_MARKER(0x80013E20, "FRONT.BIN");

    if (menu != fCurrentMenu[i])
    {
        if (i == 0)
        {
            uint32 music = menu->fFlags & 0xF000;
            if (music != fCurrentMusic)
            {
                fCurrentMusic = music;
                UpdateMusic();
            }
        }
        if (menu->IsSubMenu())
            fTransitionToMenu[i] = menu;
        else
        {
            tMenu *currentMenu = fCurrentMenu[i];
            fTransitionToMenu[i] = menu;
            if (currentMenu != 0 && menu != 0)
                currentMenu->TransitionOff();
        }
        SetScreen(i, menu->fScreen);
    }
}

void tFEApplication::SetScreen(sint16 i, tScreen *screen)
{
    FUNCTION_MARKER(0x80013F24, "FRONT.BIN");

    tScreen *currentScreen = fCurrentScreen[i];
    if (screen != currentScreen)
    {
        fTransitionToScreen[i] = screen;
        if (currentScreen != 0 && screen != 0)
            currentScreen->TransitionOff(tScreen::kScreen_TransitionTypeScreen, 0);
    }
}

void tFEApplication::DisplayHelp(sint16 variant)
{
    FUNCTION_MARKER(0x80013F74, "FRONT.BIN");

    helpPopup.variant = variant;
    helpPopup.Display();
}

void Front::ResetOverlayDemo(void)
{
    currentVideo = 0;
}

void tFEApplication::RunDemoVideo(void)
{
    FUNCTION_MARKER(0x80014014, "FRONT.BIN");
    if (fCurrentMenu[0] != &menuDefs->menuMain)
        return;
    AudioMus::StopSong(120);
    FeAudio::systemtask(0);
    fCurrentMenu[0]->TransitionOff();
    fCurrentScreen[0]->TransitionOff(tScreen::kScreen_TransitionTypeScreen, 0);
    while (!fCurrentMenu[0]->TransitionIsFinished() || !fCurrentScreen[0]->TransitionIsFinished())
    {
        Redraw();
        FeAudio::systemtask(0);
    }
    fCurrentScreen[0]->Cleanup();
    Audio::FECleanUp();
    Audio::DeInitDriver();
    PSXFront::FreeDrawMemory();
    FeTools::deinit();
    FreeHelpShapeCluts();
    gLargestUnused = largestunused();
    play_movie((char)(currentVideo + 1));
    gLargestUnused = largestunused();
    PSXFront::AllocateDrawMemory();
    FeTools::init();
    tScreen::DisplayLoadingText();
    Audio::InitDriver(0xD800, 0x18000);
    AudioCmn::LoadFESamples();
    LoadAllHelpShapes();
    UpdateMusic();
    AudioMus::Volume(((uint8)frontEnd.musicVolume * 35) >> 6);
    gLargestUnused = largestunused();
    fCurrentMenu[0]->Initialize();
    fCurrentScreen[0]->Initialize();
    fCurrentMenu[0]->TransitionOn();
    fCurrentScreen[0]->TransitionOn(tScreen::kScreen_TransitionTypeScreen, 0);
    currentVideo = (currentVideo + 1) % 3;
}

tAppCommand tFEApplication::MainLoop(tMenu *newMenu)
{
    FUNCTION_MARKER(0x800142A4, "FRONT.BIN");
    sint16 stackBackupPin = -1;
    bool needToSetChildMenu = false;
    uint32 ticksAtLastInput[2] = { 0, 0 };
    tMenuCommand command[2];
    tInputKeyType keyVal[2];
    tCarInfo carInfo;
    fInputPlayer = 0;
    PerformMenuInitialization();
    uint32 demoLoopLastInputTick = ticks;
    gFlip = 0;
    fPlayer = 0;
    do
    {
        backDepth[(uint8)fPlayer] = 0;
        fCurrentMenu[(uint8)fPlayer] = 0;
        fCurrentScreen[(uint8)fPlayer] = 0;
        fTransitionToMenu[(uint8)fPlayer] = 0;
        fTransitionToScreen[(uint8)fPlayer] = 0;
        waitingForOtherPlayer[(uint8)fPlayer] = 0;
        fLastKeyPressed[(uint8)fPlayer] = kInput_KeyType_NoKey;
        fParentMenu[(uint8)fPlayer] = 0;
        fPlayer = (char)((uint8)fPlayer + 1);
    } while ((uint8)fPlayer < 2);
    SetMenu(0, newMenu);
    for (;;)
    {
        uint32 tick = ticks;
        bool doRedraw = true;
        for (fPlayer = 0; (uint8)fPlayer < 2;
            fPlayer = (char)((uint8)fPlayer + 1))
        {
            if (fTransitionToMenu[(uint8)fPlayer] != 0)
            {
                bool wasSubMenu = false;
                tMenu *menu = fCurrentMenu[(uint8)fPlayer];
                if (menu != 0)
                {
                    bool finished = false;
                    if (menu->TransitionIsFinished())
                    {
                        tScreen *screen = fCurrentScreen[(uint8)fPlayer];
                        if (screen != 0)
                            finished = screen->TransitionIsFinished();
                    }
                    if (!finished)
                        goto checkSecondPlayer;
                    menu = fCurrentMenu[(uint8)fPlayer];
                    if (menu != 0)
                        wasSubMenu = menu->IsSubMenu();
                }
                if (fTransitionToMenu[(uint8)fPlayer]->IsSubMenu())
                    fParentMenu[(uint8)fPlayer] = fCurrentMenu[(uint8)fPlayer];
                else
                    fParentMenu[(uint8)fPlayer] = 0;
                fCurrentMenu[(uint8)fPlayer] = fTransitionToMenu[(uint8)fPlayer];
                fCurrentMenu[(uint8)fPlayer]->Initialize();
                fTransitionToMenu[(uint8)fPlayer] = 0;
                if (!wasSubMenu || fTransitionToScreen[0] != 0)
                {
                    fCurrentMenu[(uint8)fPlayer]->TransitionOn();
                    if (needToSetChildMenu)
                    {
                        needToSetChildMenu = false;
                        SetMenu(1, fCurrentMenu[(uint8)fPlayer]->fChildMenu);
                    }
                }
                else
                {
                    doRedraw = false;
                    backDepth[(uint8)fPlayer] = (backDepth[(uint8)fPlayer] - 1);
                }
                demoLoopLastInputTick = ticks;
            }
        checkSecondPlayer:
            if ((uint8)fPlayer == 1)
            {
                tMenu *menu = fCurrentMenu[(uint8)fPlayer];
                if (menu != 0 && menu->TransitionIsFinished() && stackBackupPin < 0)
                {
                    menu = fParentMenu[(uint8)fPlayer];
                    if (menu == 0 || menu->TransitionIsFinished())
                    {
                        fCurrentMenu[(uint8)fPlayer] = 0;
                        tScreen *screen = fCurrentScreen[(uint8)fPlayer];
                        if (screen != 0)
                            screen->Cleanup();
                        fCurrentScreen[(uint8)fPlayer] = 0;
                    }
                }
            }
            if (fTransitionToScreen[(uint8)fPlayer] != 0)
            {
                tScreen *screen = fCurrentScreen[(uint8)fPlayer];
                if (screen != 0)
                {
                    if (!screen->TransitionIsFinished() || fTransitionToMenu[(uint8)fPlayer] != 0)
                        goto processMenuInput;
                    screen = fCurrentScreen[(uint8)fPlayer];
                    if (screen != 0)
                        screen->Cleanup();
                }
                fCurrentScreen[(uint8)fPlayer] = fTransitionToScreen[(uint8)fPlayer];
                gLargestUnused = largestunused();
                fCurrentScreen[(uint8)fPlayer]->Initialize();
                fTransitionToScreen[(uint8)fPlayer] = 0;
                fCurrentScreen[(uint8)fPlayer]->TransitionOn(tScreen::kScreen_TransitionTypeScreen, 0);
            }
        processMenuInput:
            if (fCurrentMenu[(uint8)fPlayer] == 0)
                continue;
            sint32 inputStartPlayer = (uint8)fPlayer;
            sint32 inputEndPlayer = inputStartPlayer;
            uint32 flags = fCurrentMenu[(uint8)fPlayer]->fFlags;
            if ((flags & 0x10) != 0 || (frontEnd.gameMode == 1 && (flags & 8) == 0))
            {
                inputStartPlayer = 0;
                inputEndPlayer = 1;
            }
            flags = fCurrentMenu[(uint8)fPlayer]->fFlags;
            if ((flags & 0x20) != 0)
                inputStartPlayer = inputEndPlayer = (uint8)fInputPlayer;
            if ((flags & 0x40) != 0)
                inputStartPlayer = inputEndPlayer = 0;
            if ((flags & 0x80) != 0)
                inputStartPlayer = inputEndPlayer = 1;
            bool ready = false;
            tMenu *menu = fCurrentMenu[(uint8)fPlayer];
            if (menu != 0 && menu->TransitionIsFinished())
            {
                tScreen *screen = fCurrentScreen[(uint8)fPlayer];
                if (screen != 0)
                    ready = screen->TransitionIsFinished();
            }
            if (!ready)
                continue;
            sint32 debounce = fCurrentMenu[(uint8)fPlayer]->DebounceKeys();
            for (sint32 input = inputStartPlayer; (sint16)input <= inputEndPlayer;
                input = (input + 1))
            {
                sint16 player = (sint16)input;
                command[player].type = kMenu_Command_None;
                keyVal[player] = FEInput::GetKeyFromPlayer((tPlayer)player, debounce);
                if (keyVal[player] != kInput_KeyType_NoKey)
                    fInputPlayer = (char)input;
                if ((uint32)ticks - ticksAtLastInput[player] > 15 || (debounce & keyVal[player]) == 0)
                    fLastKeyPressed[player] = keyVal[player];
                if (keyVal[player] != kInput_KeyType_NoKey)
                {
                    tDialogBase *dialog = tDialogBase::GetTopMostDialog();
                    demoLoopLastInputTick = tick;
                    ticksAtLastInput[player] = tick;
                    if (keyVal[player] == kInput_KeyType_Circle && helpPopup.currentlyOn != 0)
                    {
                        keyVal[player] = kInput_KeyType_AlreadyProcessed;
                        helpPopup.Hide();
                    }
                    if (dialog != 0 && keyVal[player] != kInput_KeyType_Circle)
                        dialog->ProcessInput((tPlayer)player, keyVal[player], command[player]);
                    tScreen *screen = fCurrentScreen[(uint8)fPlayer];
                    if (screen != 0)
                        screen->ProcessInput((tPlayer)player, keyVal[player], command[player]);
                    if (keyVal[player] != kInput_KeyType_AlreadyProcessed)
                        fCurrentMenu[(uint8)fPlayer]->ProcessInput((tPlayer)player, keyVal[player], command[player]);
                }
                switch (command[player].type)
                {
                case kMenu_Command_GoToMenu:
                    AudioCmn::PlayFESFX(0);
                    backList[(uint8)fPlayer][backDepth[(uint8)fPlayer]] = fCurrentMenu[(uint8)fPlayer];
                    backDepth[(uint8)fPlayer] = (backDepth[(uint8)fPlayer] + 1);
                    SetMenu((uint8)fPlayer, command[player].nextMenu);
                    break;
                case kMenu_Command_GoToMenuOneWay:
                    AudioCmn::PlayFESFX(0);
                    backDepth[(uint8)fPlayer] = 0;
                    SetMenu((uint8)fPlayer, command[player].nextMenu);
                    break;
                case kMenu_Command_GoToMenuTwoPlayer:
                    AudioCmn::PlayFESFX(0);
                    backList[(uint8)fPlayer][backDepth[(uint8)fPlayer]] = fCurrentMenu[(uint8)fPlayer];
                    needToSetChildMenu = true;
                    backDepth[(uint8)fPlayer] = (backDepth[(uint8)fPlayer] + 1);
                    stackBackupPin = (sint16)backDepth[(uint8)fPlayer];
                    SetMenu((uint8)fPlayer, command[player].nextMenu);
                    backDepth[1] = 0;
                    break;
                case kMenu_Command_BackupMenu:
                    if (((uint8)fPlayer != 0 || backDepth[0] <= stackBackupPin) && ((uint8)fPlayer != 1 || backDepth[(uint8)fPlayer] <= 0))
                    {
                        if (waitingForOtherPlayer[(uint8)fPlayer] != 0)
                        {
                            waitingForOtherPlayer[(uint8)fPlayer] = 0;
                            break;
                        }
                        waitingForOtherPlayer[1 - (uint8)fPlayer] = 0;
                        waitingForOtherPlayer[(uint8)fPlayer] = 0;
                    }
                    if ((uint8)fPlayer == 0 && backDepth[0] == stackBackupPin && fCurrentMenu[1] != 0)
                    {
                        if (fParentMenu[0] != 0)
                            fParentMenu[0]->TransitionOff();
                        if (fParentMenu[1] != 0)
                            fParentMenu[1]->TransitionOff();
                        stackBackupPin = -1;
                        fCurrentMenu[1]->TransitionOff();
                        fCurrentScreen[1]->TransitionOff(tScreen::kScreen_TransitionTypeScreen, 0);
                        backDepth[1] = 0;
                    }
                    else if ((uint8)fPlayer == 1 && backDepth[1] <= 0 && fCurrentMenu[1] != 0)
                    {
                        backDepth[0] = stackBackupPin - 1;
                        fCurrentMenu[1]->TransitionOff();
                        if (fParentMenu[0] != 0)
                            fParentMenu[0]->TransitionOff();
                        if (fParentMenu[1] != 0)
                            fParentMenu[1]->TransitionOff();
                        stackBackupPin = -1;
                        fCurrentScreen[1]->TransitionOff(tScreen::kScreen_TransitionTypeScreen, 0);
                        SetMenu(0, backList[0][backDepth[0]]);
                    }
                    if (backDepth[(uint8)fPlayer] > 0)
                    {
                        AudioCmn::PlayFESFX(1);
                        backDepth[(uint8)fPlayer] = (backDepth[(uint8)fPlayer] - 1);
                        SetMenu((uint8)fPlayer, backList[(uint8)fPlayer][backDepth[(uint8)fPlayer]]);
                        if (fCurrentMenu[(uint8)fPlayer]->IsSubMenu())
                            backDepth[(uint8)fPlayer] = (backDepth[(uint8)fPlayer] + 1);
                    }
                    else if ((uint8)fPlayer == 1)
                        AudioCmn::PlayFESFX(1);
                    break;
                case kMenu_Command_StartRace:
                case kMenu_Command_ReStartRace:
                    goto selectRaceCar;
                case kMenu_Command_Start2PlayerRace:
                    if (waitingForOtherPlayer[1 - (uint8)fPlayer] == 0)
                    {
                        waitingForOtherPlayer[(uint8)fPlayer] = 1;
                        break;
                    }
                    waitingForOtherPlayer[1 - (uint8)fPlayer] = 0;
                    if (frontEnd.raceType != 6)
                        goto selectStockCar;
                    {
                        AudioMus::StopSong(400);
                        PinkSlipsErrorCode err = PinkSlipsNoError;
                        for (sint16 savePlayer = 0; savePlayer < 2 && err == PinkSlipsNoError; ++savePlayer)
                        {
                            err = SavePinkSlipsCars(savePlayer, (uint8)frontEnd.pinkSlipsCar[savePlayer]);
                            if (err != PinkSlipsNoError && savePlayer == 1)
                                SavePinkSlipsCars(0, -1);
                        }
                        if (err != PinkSlipsNoError)
                        {
#ifdef _DEBUG
                            fprintf(stderr, "pink slips save failed: %d\n", sint32(err));
#endif
                            UpdateMusic();
                            AudioMus::Volume(((uint8)frontEnd.musicVolume * 35) >> 6);
                            break;
                        }
                    }
                    goto selectRaceCar;
                case kMenu_Command_StartReplay:
                    GameSetup::gData.replayMode = 2;
                    PerformMenuDestruction();
                    return kApp_Command_StartReplay;
                default:
                    break;
                }
                continue;
            selectRaceCar:
                if (frontEnd.raceType == 6)
                {
                    carManager.GetPinkSlipsCar((uint8)frontEnd.pinkSlipsCar[(uint8)fPlayer], carInfo, (uint8)fPlayer);
                    goto raceCarSelected;
                }
            selectStockCar:
                if (frontEnd.carListType == 0)
                    carManager.GetStockCar((uint8)frontEnd.playerCar[(uint8)fPlayer], carInfo);
                else
                    carManager.GetGarageCar((uint8)frontEnd.garageCar[(uint8)fPlayer], carInfo, (uint8)fPlayer);
            raceCarSelected:
                if (carInfo.fEnginePatch != 0)
                    AudioCmn::PlayFESFX(carInfo.fEnginePatch);
                uint32 startTicks = ticks;
                while ((uint32)ticks - startTicks < 256)
                {
                    psx_root_counter_poll();
                    FeAudio::systemtask(0);
                }
                GameSetup::gData.replayMode = 0;
                PerformMenuDestruction();
                return kApp_Command_StartRace;
            }
        }
        if (doRedraw)
            Redraw();
        if ((sint32)(tick - demoLoopLastInputTick) > 0xf00)
        {
            RunDemoVideo();
            demoLoopLastInputTick = ticks;
        }
    }
}

tAppCommand tFEApplication::RunPostGame(void)
{
    FUNCTION_MARKER(0x80015430, "FRONT.BIN");
    tMenuCommand command;
    if (frontEnd.raceType != 6 && (frontEnd.raceType != 0 || frontEnd.carListType != 0))
    {
        StatChk::ClearNewRecords();
        for (sint32 i = 0; i < 2; ++i)
        {
            Front::ResetPSXController(i, (uint8)frontEnd.controlConfig[i]);
            gotName[i] = 0;
            needName[i] = 0;
            speechToPlay[i] = -1;
        }
        Car::tStats *dummyCars = Cars::gNewCarStatsList;
        sint16 nBestCarIndex;
        if (StatChk::IsRecordLapTime(dummyCars, Cars::gNumRaceCars, &nBestCarIndex))
        {
            if (frontEnd.gameMode == 1)
                speechToPlay[nBestCarIndex] = nBestCarIndex == 0 ? 7 : 8;
            else
                speechToPlay[nBestCarIndex] = 6;
            needName[nBestCarIndex] = 1;
        }
        sint32 ret = StatChk::IsTopTime(dummyCars, Cars::gNumRaceCars);
        if (ret != 0)
        {
            if (frontEnd.gameMode == 1)
            {
                if ((ret & 3) != 0)
                {
                    needName[0] = 1;
                    if (speechToPlay[0] != -1 && nBestCarIndex == 0)
                        speechToPlay[0] = (ret & 2) != 0 ? 10 : 13;
                    else
                        speechToPlay[0] = (ret & 2) != 0 ? 1 : 4;
                }
                if ((ret & 0xC) != 0)
                {
                    needName[1] = 1;
                    if (speechToPlay[1] != -1 && nBestCarIndex == 1)
                        speechToPlay[1] = (ret & 8) != 0 ? 11 : 14;
                    else
                        speechToPlay[1] = (ret & 8) != 0 ? 2 : 5;
                }
            }
            else
            {
                needName[0] = 1;
                if (speechToPlay[0] != -1)
                    speechToPlay[0] = (ret & 2) != 0 ? 9 : 12;
                else
                    speechToPlay[0] = (ret & 2) != 0 ? 0 : 3;
            }
        }
        if (needName[0] != 0)
        {
            command.nextMenu = &menuDefs->menuPostGamePlayer1Name;
            menuDefs->menuItemUserName1.fData = frontEnd.playerNameList[0];
            menuDefs->menuItemUserName1.fPlayer = 0;
            menuDefs->menuItemUserName1.fMaxStringLength = 7;
            menuDefs->menuItemUserName1.fCurrentRow = 0;
            menuDefs->menuItemUserName1.fCurrentColumn = 0;
            screenUserName->callingMenu = &menuDefs->menuPostGamePlayer1Name;
            return MainLoop(command.nextMenu);
        }
        if (needName[1] != 0)
        {
            command.nextMenu = &menuDefs->menuPostGamePlayer2Name;
            menuDefs->menuItemUserName2.fPlayer = 1;
            menuDefs->menuItemUserName2.fData = frontEnd.playerNameList[1];
            menuDefs->menuItemUserName2.fMaxStringLength = 7;
            menuDefs->menuItemUserName2.fCurrentRow = 0;
            menuDefs->menuItemUserName2.fCurrentColumn = 0;
            screenUserName->callingMenu = &menuDefs->menuPostGamePlayer2Name;
            return MainLoop(command.nextMenu);
        }
    }
    MenuExtended::PostGameMenu(command);
    return MainLoop(command.nextMenu);
}

tAppCommand tFEApplication::RunFrontEnd(void)
{
    FUNCTION_MARKER(0x80015738, "FRONT.BIN");
    return MainLoop(&menuDefs->menuMain);
}
