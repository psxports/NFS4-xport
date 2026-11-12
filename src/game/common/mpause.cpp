#include <new>
#include <stddef.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "eaclib/psx/globals.h"
#include "game/common/audiocmn.h"
#include "game/common/audiomus.h"
#include "game/common/gmesetup.h"
#include "game/common/input.h"
#include "game/common/mpause.h"
#include "game/common/pausemenu.h"
#include "game/common/replay.h"
#include "game/common/textsys.h"
#include "game/psx/audio.h"
#include "game/psx/device.h"
#include "game/psx/hud.h"
#include "game/psx/psxcontroller.h"

class tPauseMenuDefs

{
public:
    static void *operator new(size_t size) throw();
    static void operator delete(void *memory) throw();
    tPauseMenuDefs(void);

    ~tPauseMenuDefs(void);

    tPMenuItemNonInteractiveText itemGamePaused;

    tPMenuItemCommandButton itemContinue;

    tPMenuItemCommandButton itemRestart;

    tPMenuItemGoToMenuButton itemOptions;

    tPMenuItemCommandButton itemQuitRace;

    tPMenuItemCommandButton itemForfeitRace;

    tPMenu menuPause;

    tPMenuItemNonInteractiveText itemOptionsTitle;

    tPMenuItemGoToMenuButton itemAudioSettings;

    tPMenuItemGoToMenuButton itemControllerSettings;

    tPMenu menuOptions;

    tPMenuItemNonInteractiveText itemAudioSettingsTitle;

    tPListIterator iteratorAudioMode;

    tPMenuItemLeftRightChoice itemAudioSettingsAudioMode;

    tPMenuItemLeftRightSlider itemAudioSettingsMusicVolume;

    tPMenuItemLeftRightSlider itemAudioSettingsFXVolume;

    tPMenuItemLeftRightSlider itemAudioSettingsSpeechVolume;

    tPMenuItemLeftRightSlider itemAudioSettingsEngineVolume;

    tPMenuItemLeftRightSlider itemAudioSettingsAmbientVolume;

    tPMenu menuAudioSettings;

    tPListIteratorIndexed iteratorConfig;

    tPMenuItemNonInteractiveText itemControllerSettingsTitle;

    tPMenuItemLeftRightChoice itemControllerConfig;

    tPMenuItemLeftRightSliderIndexed itemControllerShockMode;

    tPMenuItemLeftRightSliderIndexed itemControllerShockImpact;

    tPMenu menuControllerConfig;

    tPMenuItemNonInteractiveText itemConfirmTitle;

    tPMenuItemNonInteractiveText itemConfirmAreYouSure;

    tPMenuItemCommandButton itemConfirmNo;

    tPMenuItemCommandButton itemConfirmYes;

    tPMenu menuConfirmYesNo;
};

static sint16 kMovingHighlight;
static sint16 kMovingHighlightDir;
static tPMenu *gPauseCurrentMenu;
static sint32 gBackDepth;
static tPMenu *gBackList[6];
sint16 InGameSelectListAudioMode[4] = { 18, 17, 19, 0 };
sint16 SelectListConfig[4] = { 23, 24, 25, 0 };
tPauseMenuDefs *gPauseMenuDefs;
sint32 gMPauseUpdate;
sint32 gMPauseUpdateNextTime;

void *tPauseMenuDefs::operator new(size_t size) throw()
{
    return reservememadr(0, (sint32)size, 0);
}

void tPauseMenuDefs::operator delete(void *memory) throw()
{
    purgememadr(memory);
}

tPauseMenuDefs::tPauseMenuDefs(void) : itemGamePaused(0),
      itemContinue(1, kMPause_Continue),
      itemRestart(2, kMPause_Restart),
      itemOptions(3, &menuOptions, 0),
      itemQuitRace(4, kMPause_QuitToRaceSummary),
      itemForfeitRace(5, kMPause_ForfeitToRaceSummary),
      menuPause(&itemGamePaused, static_cast<tPMenuItem *>(&itemContinue), static_cast<tPMenuItem *>(&itemRestart), static_cast<tPMenuItem *>(&itemOptions), static_cast<tPMenuItem *>(&itemQuitRace), static_cast<tPMenuItem *>(&itemForfeitRace), static_cast<tPMenuItem *>(0)),
      itemOptionsTitle(6),
      itemAudioSettings(7, &menuAudioSettings, 0),
      itemControllerSettings(8, &menuControllerConfig, 0),
      menuOptions(&itemOptionsTitle, static_cast<tPMenuItem *>(&itemAudioSettings), static_cast<tPMenuItem *>(&itemControllerSettings), static_cast<tPMenuItem *>(0)),
      itemAudioSettingsTitle(10),
      iteratorAudioMode(&InGameSelectListAudioMode[0], &GameSetup::gData.userSetting.audioMode),
      itemAudioSettingsAudioMode(11, &iteratorAudioMode),
      itemAudioSettingsMusicVolume(12, &gMasterMusicLevel, 127),
      itemAudioSettingsFXVolume(13, &gMasterSFXLevel, 127),
      itemAudioSettingsSpeechVolume(14, &gMasterFENarrationLevel, 127),
      itemAudioSettingsEngineVolume(15, &gMasterEngineLevel, 127),
      itemAudioSettingsAmbientVolume(16, &gMasterAmbientLevel, 127),
      menuAudioSettings(&itemAudioSettingsTitle, static_cast<tPMenuItem *>(&itemAudioSettingsAudioMode), static_cast<tPMenuItem *>(&itemAudioSettingsMusicVolume), static_cast<tPMenuItem *>(&itemAudioSettingsFXVolume), static_cast<tPMenuItem *>(&itemAudioSettingsSpeechVolume), static_cast<tPMenuItem *>(&itemAudioSettingsEngineVolume), static_cast<tPMenuItem *>(&itemAudioSettingsAmbientVolume), static_cast<tPMenuItem *>(0)),
      iteratorConfig(&SelectListConfig[0], &GameSetup::gData.controllerData.controllerConfig[0], &Device::gPausePortIndex),
      itemControllerSettingsTitle(20),
      itemControllerConfig(21, &iteratorConfig),
      itemControllerShockMode(27, &GameSetup::gData.controllerData.shockMode[0], 127, &Device::gPausePortIndex),
      itemControllerShockImpact(28, &GameSetup::gData.controllerData.shockImpact[0], 127, &Device::gPausePortIndex),
      menuControllerConfig(&itemControllerSettingsTitle, static_cast<tPMenuItem *>(&itemControllerConfig), static_cast<tPMenuItem *>(&itemControllerShockMode), static_cast<tPMenuItem *>(&itemControllerShockImpact), static_cast<tPMenuItem *>(0)),
      itemConfirmTitle(4),
      itemConfirmAreYouSure(36),
      itemConfirmNo(38, kMPause_BackupMenu),
      itemConfirmYes(37, kMPause_BackupMenu),
      menuConfirmYesNo(&itemConfirmTitle, static_cast<tPMenuItem *>(&itemConfirmAreYouSure), static_cast<tPMenuItem *>(&itemConfirmNo), static_cast<tPMenuItem *>(&itemConfirmYes), static_cast<tPMenuItem *>(0))
{
    FUNCTION_MARKER(0x8009DED4, "NFS4.EXE");
}

tPauseMenuDefs::~tPauseMenuDefs(void)
{
    FUNCTION_MARKER(0x8009E234, "NFS4.EXE");
}

void MPause::MusicLogic(char active)
{
    FUNCTION_MARKER(0x8009E3E8, "NFS4.EXE");
    static char wasActive;
    static char testSFX;
    static char playingSFX;
    static sint32 vol = 127;
    static sint32 SFXHandle;
    static sint32 lastplaytick;
    sint32 sndover = 1;
    sint32 samp = 16;
    sint32 audioMode = GameSetup::gData.userSetting.audioMode;
    if (audioMode == 0)
    {
        gStereoMode = 1;
        Audio::direct3davail = 0;
    }
    else if (audioMode == 1)
    {
        gStereoMode = 0;
        Audio::direct3davail = 0;
    }
    else if (audioMode == 2)
    {
        gStereoMode = 1;
        Audio::direct3davail = 1;
    }
    if (active != 0)
    {
        if (wasActive == 0)
        {
            sint32 level = AudioCmn::MusicLevel(gMasterMusicLevel);
            AudioMus::AutoVolume(500, level);
        }
        sint32 current = gPauseCurrentMenu->fCurrentItem;
        if (current == 1 || current == 2)
        {
            sint32 level = AudioCmn::MusicLevel(gMasterMusicLevel);
            AudioMus::Volume(level);
        }
        tPMenu *menu = gPauseCurrentMenu;
        current = menu->fCurrentItem;
        testSFX = 0;
        if (current == 3)
        {
            sint32 volume = gMasterSFXLevel;
            testSFX = 1;
            vol = volume;
        }
        current = menu->fCurrentItem;
        if (current == 4)
        {
            sint32 volume = gMasterFENarrationLevel;
            testSFX = 1;
            vol = volume;
            current = menu->fCurrentItem;
        }
        if (current == 5)
        {
            sint32 volume = gMasterEngineLevel;
            testSFX = 1;
            vol = volume;
        }
        current = menu->fCurrentItem;
        if (current == 6)
        {
            sint32 volume = gMasterAmbientLevel;
            testSFX = 1;
            vol = volume;
        }
        if (testSFX != 0)
        {
            if (playingSFX == 0)
                playingSFX = 1;
            else
                sndover = SNDover(SFXHandle);
            if (sndover != 0)
            {
                sint32 elapsed = (sint32)((uint32)ticks - lastplaytick);
                if (elapsed >= 193)
                {
                    sint32 bank = gSndBnk[3].bnkID;
                    sint32 now = ticks;
                    sint32 volume = vol;
                    lastplaytick = now;
                    SFXHandle = AudioCmn::PlaySound(bank, samp, 0, volume, 64);
                }
            }
        }
        else if (playingSFX == 1)
        {
            SNDstop(SFXHandle);
            playingSFX = 0;
        }
    }
    else if (wasActive != 0)
    {
        SNDstop(SFXHandle);
        AudioMus::AutoVolume(500, 0);
    }
    wasActive = active;
}

void MPause::ControllerLogic(void)
{
    FUNCTION_MARKER(0x8009E6A0, "NFS4.EXE");
    sint32 port = (uint8)Device::gPausePortIndex == 0 ? 0 : 16;
    sint32 state = PadGetState(port);
    tPauseMenuDefs *defs = gPauseMenuDefs;
    if (state == 2)
    {
        if (defs->itemControllerShockMode.IsEnabled())
        {
            uint32 flags = defs->itemControllerShockMode.fFlags;
            ChangedEnabling = 1;
            defs->itemControllerShockMode.fFlags = flags | 1;
        }
        defs = gPauseMenuDefs;
        if (!defs->itemControllerShockImpact.IsEnabled())
            return;
        uint32 flags = defs->itemControllerShockImpact.fFlags;
        ChangedEnabling = 1;
        defs->itemControllerShockImpact.fFlags = flags | 1;
    }
    else
    {
        if (defs->itemControllerShockMode.IsDisabled())
        {
            ChangedEnabling = 1;
            defs->itemControllerShockMode.fFlags &= ~1U;
        }
        defs = gPauseMenuDefs;
        if (!defs->itemControllerShockImpact.IsDisabled())
            return;
        ChangedEnabling = 1;
        defs->itemControllerShockImpact.fFlags &= ~1U;
    }
}

sint32 MPause::Logic(void)
{
    FUNCTION_MARKER(0x8009E784, "NFS4.EXE");
    tInputKeyType keyVal = kInput_KeyType_NoKey;
    bool debounce = gPauseCurrentMenu->Debounce();
    if (Input::Interface(5, 1) != 0)
        keyVal = kInput_KeyType_Cross;
    else if (Input::Interface(1, 1) != 0)
        keyVal = kInput_KeyType_Up;
    else if (Input::Interface(2, 1) != 0)
        keyVal = kInput_KeyType_Down;
    else if (Input::Interface(3, debounce) != 0)
        keyVal = kInput_KeyType_Left;
    else if (Input::Interface(4, debounce) != 0)
        keyVal = kInput_KeyType_Right;
    else if (Input::Interface(6, 1) != 0)
        keyVal = kInput_KeyType_Start;
    else if (Input::Interface(24, 1) != 0)
        keyVal = kInput_KeyType_Triangle;
    tPMenuCommand command;
    command.type = kMPause_NoEvent;
    if (kMovingHighlight == 0)
    {
        if (keyVal != kInput_KeyType_NoKey)
        {
            gMPauseUpdate = 1;
            tPMenu *menu = gPauseCurrentMenu;
            sint16 previous = (sint16)menu->fCurrentItem;
            menu->ProcessInput(keyVal, command);
            sint16 current = (sint16)gPauseCurrentMenu->fCurrentItem;
            if (previous != current)
            {
                sint32 start = gPauseCurrentMenu->ItemEnabledNum(previous) * 13 + 106;
                sint32 finish = gPauseCurrentMenu->ItemEnabledNum(current) * 13 + 106;
                kMovingHighlight = (sint16)(start - finish);
                kMovingHighlightDir = (sint16)((finish - start) / 10);
            }
        }
        else if (ChangedEnabling != 0)
            gMPauseUpdate = 1;
        else if (gMPauseUpdateNextTime != 0)
        {
            gMPauseUpdate = 1;
            gMPauseUpdateNextTime = 0;
        }
        else
            gMPauseUpdate = 0;
    }
    else
        gMPauseUpdate = 1;
    if (command.type != kMPause_NoEvent)
    {
        if (((uint32)command.type & 0x100U) != 0)
        {
            tPMenu *menu = gPauseCurrentMenu;
            tPMenuItem *item = menu->fItemList[menu->fCurrentItem];
            tPauseMenuDefs *defs = gPauseMenuDefs;
            defs->itemConfirmTitle.fTextDescription = item->fTextDescription;
            uint8 confirmed = (uint8)command.type;
            defs->menuConfirmYesNo.fCurrentItem = 0;
            defs->itemConfirmYes.fCommand = (tPMenuCommandType)confirmed;
            command.type = kMPause_GoToMenu;
            command.nextMenu = &defs->menuConfirmYesNo;
        }
        if (command.type == kMPause_GoToMenu)
        {
            sint32 depth = gBackDepth;
            gBackList[depth] = gPauseCurrentMenu;
            tPMenu *next = command.nextMenu;
            gBackDepth = depth + 1;
            gPauseCurrentMenu = next;
            next->Initialize();
        }
        else if (command.type == kMPause_BackupMenu)
        {
            if (gBackDepth <= 0)
                return kMPause_Continue;
            sint32 depth = gBackDepth - 1;
            tPMenu *next = gBackList[depth];
            gBackDepth = depth;
            gPauseCurrentMenu = next;
            next->Initialize();
        }
        else
            return command.type;
    }
    MusicLogic(gPauseCurrentMenu == &gPauseMenuDefs->menuAudioSettings);
    ControllerLogic();
    return 0;
}

void MPause::Render(void)
{
    FUNCTION_MARKER(0x8009EAA0, "NFS4.EXE");
    sint16 previous = kMovingHighlight;
    ChangedEnabling = 0;
    if (previous > 0)
    {
        kMovingHighlight = (sint16)(previous + kMovingHighlightDir);
        if (kMovingHighlight < 0)
            kMovingHighlight = 0;
    }
    else if (previous < 0)
    {
        kMovingHighlight = (sint16)(previous + kMovingHighlightDir);
        if (kMovingHighlight > 0)
            kMovingHighlight = 0;
    }
    gPauseCurrentMenu->fHighlight = kMovingHighlight == 0;
    gPauseCurrentMenu->Draw();
    sint32 numItems = gPauseCurrentMenu->NumEnabledItems();
    tPMenu *menu = gPauseCurrentMenu;
    sint32 currentItem = menu->fCurrentItem;
    bool moving = kMovingHighlight != 0;
    sint32 enabled = menu->ItemEnabledNum(currentItem);
    uint32 y = enabled * 13U + 106U;
    if (moving)
        y += (uint32)(sint32)kMovingHighlight;
    Hud::FBuildF4(1, 80, y, 160, 13, 0, 0, 0);
    sint32 height = (((numItems + 1) * 13) + 20);
    Hud::RenderPauseBox(80, 100, 160, height);
}

void MPause::InitMPause(void)
{
    FUNCTION_MARKER(0x8009EC48, "NFS4.EXE");
    TextSys::LoadInGame(GameSetup::gData.userSetting.language);
    void *memory = tPauseMenuDefs::operator new(sizeof(tPauseMenuDefs));
    gPauseMenuDefs = ::new (memory) tPauseMenuDefs;
}

void MPause::StartPauseMenu(void)
{
    FUNCTION_MARKER(0x8009EC80, "NFS4.EXE");
    gPauseCurrentMenu = &gPauseMenuDefs->menuPause;
    gPauseCurrentMenu->Initialize();
    sint32 raceType = GameSetup::gData.raceType;
    gBackDepth = 0;
    tPauseMenuDefs *defs = gPauseMenuDefs;
    if (raceType == 6 || raceType == 2)
    {
        if (defs->itemRestart.IsEnabled())
        {
            uint32 flags = defs->itemRestart.fFlags;
            ChangedEnabling = 1;
            defs->itemRestart.fFlags = flags | 1;
        }
    }
    else if (defs->itemRestart.IsDisabled())
    {
        ChangedEnabling = 1;
        defs->itemRestart.fFlags &= ~1U;
    }
    raceType = GameSetup::gData.raceType;
    defs = gPauseMenuDefs;
    if (raceType == 6)
    {
        if (defs->itemForfeitRace.IsDisabled())
        {
            ChangedEnabling = 1;
            defs->itemForfeitRace.fFlags &= ~1U;
        }
        defs = gPauseMenuDefs;
        if (defs->itemQuitRace.IsEnabled())
        {
            uint32 flags = defs->itemQuitRace.fFlags;
            ChangedEnabling = 1;
            defs->itemQuitRace.fFlags = flags | 1;
        }
    }
    else
    {
        if (defs->itemForfeitRace.IsEnabled())
        {
            uint32 flags = defs->itemForfeitRace.fFlags;
            ChangedEnabling = 1;
            defs->itemForfeitRace.fFlags = flags | 1;
        }
        defs = gPauseMenuDefs;
        if (defs->itemQuitRace.IsDisabled())
        {
            ChangedEnabling = 1;
            defs->itemQuitRace.fFlags &= ~1U;
        }
    }
    defs = gPauseMenuDefs;
    if (defs->itemAudioSettingsMusicVolume.IsDisabled())
    {
        ChangedEnabling = 1;
        defs->itemAudioSettingsMusicVolume.fFlags &= ~1U;
    }
    sint32 replayMode = Replay::ReplayMode;
    defs = gPauseMenuDefs;
    if (replayMode >= 2)
    {
        defs->itemQuitRace.fCommand = (tPMenuCommandType)((uint32)defs->itemQuitRace.fCommand & ~0x100U);
        defs->itemRestart.fCommand = (tPMenuCommandType)((uint32)defs->itemRestart.fCommand & ~0x100U);
        defs->itemForfeitRace.fCommand = (tPMenuCommandType)((uint32)defs->itemForfeitRace.fCommand & ~0x100U);
    }
    else
    {
        defs->itemQuitRace.fCommand = (tPMenuCommandType)((uint32)defs->itemQuitRace.fCommand | 0x100U);
        defs->itemRestart.fCommand = (tPMenuCommandType)((uint32)defs->itemRestart.fCommand | 0x100U);
        defs->itemForfeitRace.fCommand = (tPMenuCommandType)((uint32)defs->itemForfeitRace.fCommand | 0x100U);
    }
}

void MPause::EndPauseMenu(void)
{
    FUNCTION_MARKER(0x8009EEB0, "NFS4.EXE");
    sint32 player = (uint8)Device::gPausePortIndex;
    InGame::ResetPSXController(player, GameSetup::gData.controllerData.controllerConfig[player]);
}

void MPause::KillMPause(void)
{
    FUNCTION_MARKER(0x8009EEEC, "NFS4.EXE");
    DrawSync(0);
    delete gPauseMenuDefs;
    TextSys::UnloadWords();
}
