#include <stddef.h>
#include <stdio.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "eaclib/psx/ssysinit.h"
#include "frontend/common/feaudio.h"
#include "frontend/common/fefades.h"
#include "frontend/common/femenudefs.h"
#include "frontend/common/fetextrender.h"
#include "frontend/common/front.h"
#include "frontend/common/screenaudio.h"
#include "frontend/psx/psxfront.h"
#include "game/common/audiocmn.h"
#include "game/common/nfs3.h"
#include "game/common/textsys.h"
#include "game/psx/audio.h"
#include "game/psx/textpix.h"

static sint32 lastpercentage = -1;
static sint32 perfade = 128;
tScreenAudio *screenAudio;

void tScreenAudio::PlaySound(void)
{
    FUNCTION_MARKER(0x80045A28, "FRONT.BIN");
    if (menuDefs->itemSlidingPlayList.fActive != 0 && fPrevSelectedSong != fSelectedSong)
    {
        AudioMus::StopSong(10);
        AudioMus::PlaySong(songlist->song[fSelectedSong].filename);
        fPrevSelectedSong = fSelectedSong;
    }
    AudioMus::Volume(((uint8)frontEnd.musicVolume * 35) >> 6);
    gMasterFENarrationLevel = (uint8)frontEnd.narrationVolume;
    if ((uint8)frontEnd.audioMode != (uint8)prevAudioMode)
    {
        gStereoMode = 1;
        Audio::direct3davail = 0;
        audioTest = 1;
        SNDSYSOPTS opts;
        SNDSYS::getopts(&opts);
        if ((uint8)frontEnd.audioMode == 1)
        {
            opts.set.outputchannels = 1;
            gStereoMode = 0;
            audioTest = 2;
        }
        else if ((uint8)frontEnd.audioMode == 0)
            opts.set.outputchannels = 2;
        else
        {
            opts.set.outputchannels = 2;
            Audio::direct3davail = 1;
        }
        SNDSYS::setopts(&opts);
        prevAudioMode = frontEnd.audioMode;
    }
    sint16 item = (sint16)menuDefs->menuAudio.fCurrentItem;
    if (item < 1 || item > 5)
    {
        if (audioTest != 0)
        {
            gMasterSFXLevel = (uint8)frontEnd.sfxVolume;
            SNDstop(audioTestHandle);
            audioTest = 0;
        }
        return;
    }
    sint32 sndover = 1;
    sint32 vol;
    sint32 RepresentativeSound;
    switch (item)
    {
    case 1:
        vol = (uint8)frontEnd.sfxVolume;
        RepresentativeSound = 31;
        break;
    case 2:
        vol = (uint8)frontEnd.engineVolume;
        RepresentativeSound = randPSX() % 6 + 41;
        break;
    case 3:
        vol = (uint8)frontEnd.narrationVolume;
        RepresentativeSound = -1;
        break;
    case 4:
        vol = (uint8)frontEnd.ambientVolume;
        RepresentativeSound = 30;
        break;
    default:
        vol = (uint8)frontEnd.sfxVolume;
        RepresentativeSound = randPSX() % 6 + 41;
        break;
    }
    gMasterSFXLevel = vol;
    if (audioTest == 0)
        audioTest = (uint8)frontEnd.audioMode == 1 ? 2 : 1;
    else
        sndover = (sint32)SNDover(audioTestHandle);
    if (sndover != 0 && ginfo.areLoading == 0 && ginfo.soundIsPlaying == 0 && RepresentativeSound != 0)
    {
        sint32 azimuth = 0;
        switch (audioTest)
        {
        case 1:
            azimuth = 0xC000;
            audioTest = (uint8)frontEnd.audioMode == 2 ? 2 : 3;
            break;
        case 2:
            audioTest = (uint8)frontEnd.audioMode == 1 ? 2 : 3;
            break;
        case 3:
            azimuth = 0x3FFF;
            audioTest = (uint8)frontEnd.audioMode == 2 ? 4 : 1;
            break;
        case 4:
            azimuth = 0x8000;
            audioTest = 1;
            break;
        }
        if (RepresentativeSound == -1)
        {
            FeAudio::AsyncPlaySpeech(2, 3);
            audioTestHandle = 0;
        }
        else
            audioTestHandle = AudioCmn::PlaySound(gSndBnk[0].bnkID, RepresentativeSound, azimuth, vol, 64);
    }
    gMasterSFXLevel = (uint8)frontEnd.sfxVolume;
}

void tScreenAudio::DrawForeground(void)
{
    FUNCTION_MARKER(0x80045DC8, "FRONT.BIN");
    sint16 fade = (sint16)(menuDefs->menuAudio.fScreenFade >> 1);
    if (fade < 0)
        fade = 0;
    else if (fade > 128)
        fade = 128;
    for (sint32 i = 0; i < 4; ++i)
        DrawShapeExtended(i + 48, 1, 0, 0, fade, 0, 0);
    if (fade >= 100)
    {
        FETextRender::MenuTextPositionedJustify(0x27D, 480, 220, 1, textState_Selected, textType_ScreenInfo);
        char *text = TextSys::Word(0x27D);
        sint32 width = (sint32)(0 - (uint32)textpixels(text) - 5);
        PSXDrawSquare(0, 480, 220, width, 7);
    }
}

void Front::ResetOverlayAudioScreen(void)
{
    lastpercentage = -1;
    perfade = 128;
}

void tScreenAudio::DrawBackground(void)
{
    FUNCTION_MARKER(0x80045ED8, "FRONT.BIN");
    PlaySound();
    sint16 fade = (sint16)(menuDefs->menuAudio.fScreenFade >> 1);
    if (fade > 128)
        fade = 128;
    sint32 percent = -1;
    switch ((sint16)menuDefs->menuAudio.fCurrentItem)
    {
    case 0: percent = menuDefs->itemMusicVolume.Percentage(); break;
    case 1: percent = menuDefs->itemSoundEffectsVolume.Percentage(); break;
    case 2: percent = menuDefs->itemEngineVolume.Percentage(); break;
    case 3: percent = menuDefs->itemSpeechVolume.Percentage(); break;
    case 4: percent = menuDefs->itemAmbientVolume.Percentage(); break;
    }
    if (percent >= 0)
        lastpercentage = percent;
    if (percent >= 0 || lastpercentage >= 0)
    {
        sint32 change = percent == -1 || !menuDefs->menuAudio.tOptionsMenu::TransitionIsFinished() ? 4 : -4;
        perfade = (sint32)((uint32)perfade + (uint32)change);
        if (perfade > 128)
            perfade = 128;
        if (perfade < 0)
            perfade = 0;
        sint32 ColText = CalcFadeVal(kRGBVals[(uint8)textDefinitions[6][5]], 0, fade, perfade);
        char sBuildOutput[255];
        sprintf(sBuildOutput, "%d%%", percent < 0 ? lastpercentage : percent);
        if (perfade != 128)
        {
            sint16 x = (sint16)TextSys::WordX(0x1DC);
            sint16 y = (sint16)TextSys::WordY(0x1DC);
            FETextRender::FullTextRGB(sBuildOutput, x, y, ColText, 0, 1);
        }
    }
    for (sint32 i = 0; i < 32; ++i)
        DrawShapeExtended(i + 6, 1, 0, 0, fade, 0, 0);
}

void tScreenAudio::GetShapeInfo(sint16 &numPermShapes, sint16 &numSwapShapes, char **permFileName, char **swapFileName)
{
    FUNCTION_MARKER(0x80046140, "FRONT.BIN");
    numSwapShapes = 0;
    *swapFileName = 0;
    numPermShapes = 52;
    *permFileName = const_cast<char *>("zAudio");
}

tScreenAudio::tScreenAudio(void)
{
    FUNCTION_MARKER(0x80046164, "FRONT.BIN");
    fSelectedSong = 0;
    fCurrentAudioMode = 0;
    songlist = 0;
}

void tScreenAudio::Initialize(void)
{
    FUNCTION_MARKER(0x800461A4, "FRONT.BIN");
    fPrevSelectedSong = -1;
    menuDefs->itemSlidingPlayList.SetMenu(true, &menuDefs->menuPlayListMenu);
    tScreen::Initialize();
    char mode = frontEnd.audioMode;
    audioTest = 0;
    audioTestHandle = 0;
    songlist = 0;
    prevAudioMode = mode;
}

void tScreenAudio::Cleanup(void)
{
    FUNCTION_MARKER(0x80046204, "FRONT.BIN");
    if (audioTest != 0)
    {
        SNDstop(audioTestHandle);
        audioTest = 0;
    }
    AudioMus::Volume(((uint8)frontEnd.musicVolume * 35) >> 6);
    gMasterMusicLevel = (uint8)frontEnd.musicVolume;
    sint32 volume = (gMasterMusicLevel * 35) >> 6;
    gMasterSFXLevel = (uint8)frontEnd.sfxVolume;
    gMasterFENarrationLevel = (uint8)frontEnd.narrationVolume;
    gMasterEngineLevel = (uint8)frontEnd.engineVolume;
    gMasterAmbientLevel = (uint8)frontEnd.ambientVolume;
    AudioMus::Volume(volume);
    tScreen::Cleanup();
    while (ginfo.areLoading != 0 || ginfo.soundIsPlaying != 0)
        FeAudio::systemtask(0);
}
