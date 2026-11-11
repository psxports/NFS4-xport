#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "xport.h"
#include "game/common/carobj.h"
#include "game/common/cars.h"
#include "game/common/gmesetup.h"
#include "game/common/sim.h"
#include "game/common/textsys.h"
#include "game/psx/font.h"
#include "game/psx/hud.h"
#include "game/psx/textpix.h"

sint32 StatsTimer[2];
tBTCPerpInfo BTCPerpInfo[2][10];
sint16 Hud::NextPerp[2];

void OptionsBarThing(sint32 x, sint32 y, sint32 w, sint32 h)
{
    FUNCTION_MARKER(0x800D9968, "NFS4.EXE");
    Hud::GoTpage(1);
    char *text = TextSys::Word(0x40);
    char *measureText = TextSys::Word(0x40);
    sint32 width = Hud::BuildString(measureText, 0, 0, 0, 0, true);
    uint32 bottom = (uint32)y + (uint32)h;
    Hud::BuildString(text, (sint32)(160U - (uint32)(width >> 1)),
        (sint32)(bottom - 6U), 0xBEBE, 0, false);
    Hud::FBuildF4(0, x, (sint32)(bottom - 8U), w, 8, 0, 0, 0);
    Hud::GoTpage(0);
}

void RaceSummary(void)
{
    FUNCTION_MARKER(0x800D9A48, "NFS4.EXE");
    sint16 HUD_STATS_POS_X = GameSetup::gData.numLaps == 1 ? 40 : 8;
    sint16 HUD_STATS_SIZE_W = GameSetup::gData.numLaps == 1 ? 239 : 304;
    sint16 HUD_STATS_SIZE_H = (sint16)((uint32(Cars::gNumRaceCars) + 1U) * 12U + 30U);
    sint32 halfHeight = sint32(HUD_STATS_SIZE_H) >> 1;
    sint16 HUD_STATS_POS_Y = (sint16)(120 - halfHeight);
    sint32 titleWidth = textpixels(TextSys::Word(0x38));
    sint16 titleX = (sint16)(160U - uint32(titleWidth >> 1));
    Font::TextColor(6);
    Font::TextXY(TextSys::Word(0x38), titleX, 118 - halfHeight);
    Font::TextColor(3);
    sint16 colname = HUD_STATS_POS_X + 17;
    sint16 colcar = HUD_STATS_POS_X + 95;
    sint16 coltime = HUD_STATS_POS_X + 167;
    sint16 colbestlap = HUD_STATS_POS_X + 225;
    sint16 headerY = (sint16)(133 - halfHeight);
    Font::TextXY(TextSys::Word(0x2E), colname, headerY);
    Font::TextXY(TextSys::Word(0x3A), colcar, headerY);
    Font::TextXY(TextSys::Word(0x3B), coltime, headerY);
    if (GameSetup::gData.numLaps != 1)
        Font::TextXY(TextSys::Word(0x3C), colbestlap, headerY);
    sint16 rowBase = (sint16)(135 - halfHeight);
    Hud::FBuildF4(0, HUD_STATS_POS_X, rowBase + 12, HUD_STATS_SIZE_W, 1, 0, 0, 0);
    sint32 lineHeight = sint32(HUD_STATS_SIZE_H) - 8;
    Hud::FBuildF4(0, colname - 2, HUD_STATS_POS_Y, 1, lineHeight, 0, 0, 0);
    Hud::FBuildF4(0, colcar - 2, HUD_STATS_POS_Y, 1, lineHeight, 0, 0, 0);
    Hud::FBuildF4(0, coltime - 2, HUD_STATS_POS_Y, 1, lineHeight, 0, 0, 0);
    if (GameSetup::gData.numLaps != 1)
        Hud::FBuildF4(0, colbestlap - 2, HUD_STATS_POS_Y, 1, lineHeight, 0, 0, 0);
    char string[40];
    for (sint16 i = 0; i < Cars::gNumRaceCars; ++i)
    {
        sint32 pos = Cars::gRaceCarList[i]->stats.finalPosition;
        if ((sint32)(uint32(pos) * 2U + 4U) >= StatsTimer[0])
            continue;
        Font::TextColor((Cars::gRaceCarList[i]->carFlags & 4) != 0 ? 3 : 4);
        sprintf(string, "%d", pos);
        sint32 rowY = (sint32)(uint32(rowBase) + uint32(pos) * 12U);
        Font::TextXY(string, HUD_STATS_POS_X | 1, rowY);
        Font::TextColor(3);
        sprintf(string, "%s", Cars::gRaceCarList[i]->carInfo->driver);
        Font::TextXY(string, colname, rowY);
        Font::TextColor((Cars::gRaceCarList[i]->carFlags & 4) != 0 ? 3 : 4);
        sprintf(string, "%s", Cars::gRaceCarList[i]->carNameLocalized);
        Font::TextXY(string, colcar, rowY);
        if (GameSetup::gData.pinkSlipsForfeit == i)
            sprintf(string, TextSys::Word(0x36));
        else if (GameSetup::gData.raceType == 1 && Cars::gRaceCarList[i]->stats.finalNumArrests != 0)
            sprintf(string, TextSys::Word(0x3D));
        else if (Cars::gRaceCarList[i]->stats.finalFinishType != 2)
            sprintf(string, TextSys::Word(0x35));
        else
            Hud::ParseTime(Cars::gRaceCarList[i]->stats.finalTotalTime, string);
        Font::TextXY(string, coltime, rowY);
        if (GameSetup::gData.numLaps != 1)
        {
            Hud::ParseTime(Cars::gRaceCarList[i]->stats.finalBestLap, string);
            Font::TextXY(string, colbestlap, rowY);
        }
    }
    OptionsBarThing(HUD_STATS_POS_X, HUD_STATS_POS_Y, HUD_STATS_SIZE_W, HUD_STATS_SIZE_H);
    Hud::RenderPauseBox(HUD_STATS_POS_X, HUD_STATS_POS_Y, HUD_STATS_SIZE_W, HUD_STATS_SIZE_H);
}

void RaceStatistics(void)
{
    FUNCTION_MARKER(0x800D9FBC, "NFS4.EXE");
    sint16 HUD_STATS_POS_X = (sint16)(160U - uint32(Cars::gNumHumanRaceCars) * 75U);
    sint16 HUD_STATS_SIZE_W = (sint16)(uint32(Cars::gNumHumanRaceCars) * 150U);
    sint16 HUD_STATS_SIZE_H = (sint16)((uint32(GameSetup::gData.numLaps) + 1U) * 12U + 40U);
    if (GameSetup::gData.numLaps == 1)
        HUD_STATS_SIZE_H = 52;
    if (GameSetup::gData.raceType == 1)
        HUD_STATS_SIZE_H = (sint16)(HUD_STATS_SIZE_H + 27);
    sint32 halfHeight = sint32(HUD_STATS_SIZE_H) >> 1;
    sint16 HUD_STATS_POS_Y = (sint16)(120 - halfHeight);
    sint32 titleWidth = textpixels(TextSys::Word(0x39));
    sint16 col1 = (sint16)(HUD_STATS_POS_X + 10);
    sint16 col2 = Cars::gNumHumanRaceCars > 1 ? 85 : 160;
    sint32 titleY = 118 - halfHeight;
    sint16 HUD_STATS_HOTPURSUIT_Y = (sint16)(uint32(titleY) + (uint32(GameSetup::gData.numLaps) + 2U) * 12U + 19U);
    Font::TextColor(6);
    Font::TextXY(TextSys::Word(0x39), (sint16)(160U - uint32(titleWidth >> 1)), titleY);
    sint16 headerY = (sint16)(titleY + 17);
    Hud::FBuildF4(0, HUD_STATS_POS_X, headerY + 11, HUD_STATS_SIZE_W, 1, 0, 0, 0);
    if (GameSetup::gData.raceType == 1)
        Hud::FBuildF4(0, HUD_STATS_POS_X, HUD_STATS_HOTPURSUIT_Y, HUD_STATS_SIZE_W, 1, 0, 0, 0);
    char string[40];
    for (sint16 i = 0; i < Cars::gNumHumanRaceCars; ++i)
    {
        sint32 driverWidth = textpixels(Cars::gRaceCarList[i]->carInfo->driver);
        Hud::FBuildF4(0, col2 - 2, headerY + 11, 1,
            sint32(HUD_STATS_SIZE_H) - ((headerY - HUD_STATS_POS_Y) + 19), 0, 0, 0);
        if (i > 0)
            Hud::FBuildF4(0, col1 - 2, HUD_STATS_POS_Y, 1, sint32(HUD_STATS_SIZE_H) - 8, 0, 0, 0);
        if (StatsTimer[1] > 2)
        {
            Font::TextColor(3);
            sprintf(string, "%s", Cars::gRaceCarList[i]->carInfo->driver);
            Font::TextXY(string, col2 - ((driverWidth >> 1) + 2), headerY - 4);
        }
        if (GameSetup::gData.numLaps != 1 && GameSetup::gData.numLaps > 0)
        {
            for (sint16 j = 0; j < GameSetup::gData.numLaps; ++j)
            {
                if (j * 2 + 4 >= StatsTimer[1])
                    continue;
                uint32 offset = uint32(offsetof(Car::tObj, stats) + offsetof(Car::tStats, finalLapTime)) + uint32(j) * 4U;
                sint32 lapTime;
                memcpy(&lapTime, reinterpret_cast<uint8 *>(Cars::gHumanRaceCarList[i]) + offset, sizeof(lapTime));
                Font::TextColor(lapTime != 0 && lapTime == Cars::gHumanRaceCarList[i]->stats.finalBestLap ? 3 : 4);
                sprintf(string, TextSys::Word(0x34), sint32(j) + 1);
                sint32 lapY = headerY + sint32(j) * 12 + 12;
                Font::TextXY(string, col1, lapY);
                memcpy(&lapTime, reinterpret_cast<uint8 *>(Cars::gHumanRaceCarList[i]) + offset, sizeof(lapTime));
                Hud::ParseTime(lapTime, string);
                Font::TextXY(string, col2 + 5, lapY);
            }
        }
        if ((sint32)(uint32(GameSetup::gData.numLaps) * 2U + 4U) < StatsTimer[1])
        {
            sprintf(string, TextSys::Word(0x37));
            Font::TextColor(3);
            sint32 totalY = (sint32)(uint32(headerY) + uint32(GameSetup::gData.numLaps) * 12U);
            if (GameSetup::gData.numLaps != 1)
                totalY = (sint32)(uint32(totalY) + 12U);
            Font::TextXY(string, col1, totalY);
            if (GameSetup::gData.pinkSlipsForfeit == i)
                sprintf(string, TextSys::Word(0x36));
            else if (Cars::gHumanRaceCarList[i]->stats.finalFinishType == 2)
                Hud::ParseTime(Cars::gHumanRaceCarList[i]->stats.finalTotalTime, string);
            else if (GameSetup::gData.raceType == 1 && Cars::gHumanRaceCarList[i]->stats.finalNumArrests != 0)
                sprintf(string, TextSys::Word(0x3D));
            else
                sprintf(string, TextSys::Word(0x35));
            sint32 timeY = headerY;
            if (GameSetup::gData.numLaps != 1)
                timeY = (sint32)(uint32(timeY) + uint32(GameSetup::gData.numLaps) * 12U);
            Font::TextXY(string, col2 + 5, (sint32)(uint32(timeY) + 12U));
        }
        if (GameSetup::gData.raceType == 1)
        {
            if ((sint32)(uint32(GameSetup::gData.numLaps) * 2U + 6U) < StatsTimer[1])
            {
                sprintf(string, TextSys::Word(0x3E));
                Font::TextColor(3);
                Font::TextXY(string, col1, HUD_STATS_HOTPURSUIT_Y + 1);
                sprintf(string, "%d", Cars::gHumanRaceCarList[i]->stats.finalNumWarnings);
                Font::TextXY(string, col2 + 5, HUD_STATS_HOTPURSUIT_Y + 1);
            }
            if ((sint32)(uint32(GameSetup::gData.numLaps) * 2U + 8U) < StatsTimer[1])
            {
                sprintf(string, TextSys::Word(0x3F));
                Font::TextColor(3);
                Font::TextXY(string, col1, HUD_STATS_HOTPURSUIT_Y + 13);
                sprintf(string, "%d", Cars::gHumanRaceCarList[i]->stats.finalNumFines);
                Font::TextXY(string, col2 + 5, HUD_STATS_HOTPURSUIT_Y + 13);
            }
        }
        col1 = (sint16)(col1 + 150);
        col2 = (sint16)(col2 + 150);
    }
    OptionsBarThing(HUD_STATS_POS_X, HUD_STATS_POS_Y, HUD_STATS_SIZE_W, HUD_STATS_SIZE_H);
    Hud::RenderPauseBox(HUD_STATS_POS_X, HUD_STATS_POS_Y, HUD_STATS_SIZE_W, HUD_STATS_SIZE_H);
}

void Hud::BTCStats(sint16 player, bool postgame)
{
    FUNCTION_MARKER(0x800DA728, "NFS4.EXE");
    bool chasinghuman = false;
    bool showname = false;
    if (Cars::gNumHumanRaceCars > 1)
    {
        if ((Cars::gHumanRaceCarList[1 - player]->carFlags & 0x200) == 0)
            chasinghuman = true;
        else
            showname = true;
    }
    sint16 PLAYERWIDTH = chasinghuman ? 161 : 231;
    bool showtimeleft = !postgame;
    if (postgame)
    {
        sint32 caught;
        sint32 last = sint32(player) * 10 + NextPerp[player] - 1;
        if (player == 0 && NextPerp[player] == 0)
            memcpy(&caught, &gCopMarkerColor[11], sizeof(caught));
        else
            memcpy(&caught, reinterpret_cast<uint8 *>(&BTCPerpInfo) + last * sizeof(tBTCPerpInfo) + offsetof(tBTCPerpInfo, caught), sizeof(caught));
        showtimeleft = caught != 0;
    }
    uint16 HUD_STATS_SIZE_W = PLAYERWIDTH + 6;
    sint16 HUD_STATS_POS_X = 160 - (PLAYERWIDTH >> 1);
    sint16 HUD_STATS_SIZE_H = (sint16)((sint32(NextPerp[player]) + 1) * 12 + 22 +
        (showtimeleft ? 12 : 0) + (postgame ? 8 : 0) + (showname ? 12 : 0));
    sint32 halfHeight = sint32(HUD_STATS_SIZE_H) >> 1;
    sint16 HUD_STATS_POS_Y = (sint16)(120 - halfHeight);
    sint32 titleWidth = textpixels(TextSys::Word(postgame ? 0x48 : 0x47));
    sint16 col[4];
    col[0] = HUD_STATS_POS_X + 3;
    col[1] = HUD_STATS_POS_X + 7 + (chasinghuman ? 12 : 15);
    col[2] = HUD_STATS_POS_X + 7 + (chasinghuman ? 12 : 80);
    col[3] = HUD_STATS_POS_X + 7 + (chasinghuman ? 80 : 150);
    Font::TextColor(6);
    Font::TextXY(TextSys::Word(postgame ? 0x48 : 0x47),
        (sint16)(160U - uint32(titleWidth >> 1)), (sint16)(118 - halfHeight));
    sint16 startY = (sint16)(133 - halfHeight);
    if (showname)
    {
        Font::TextColor(4);
        Font::TextXY(Cars::gRaceCarList[player]->carInfo->driver, col[2], startY);
        startY = (sint16)(startY + 12);
    }
    Font::TextColor(3);
    if (!chasinghuman)
        Font::TextXY(TextSys::Word(0x4A), col[1], startY);
    Font::TextXY(TextSys::Word(0x4B), col[2], startY);
    Font::TextXY(TextSys::Word(0x4C), col[3], startY);
    sint32 ruleY = sint32(startY) + 15;
    FBuildF4(0, HUD_STATS_POS_X, ruleY, HUD_STATS_SIZE_W, 1, 0, 0, 0);
    sint32 lineHeight = sint32(HUD_STATS_SIZE_H) - (ruleY - HUD_STATS_POS_Y) -
        (postgame ? 8 : 0) - (showtimeleft ? 16 : 0);
    for (sint16 i = 1; i < 4; ++i)
        FBuildF4(0, col[i] - 2, ruleY, 1, lineHeight, 0, 0, 0);
    if (showtimeleft)
        FBuildF4(0, HUD_STATS_POS_X, sint32(HUD_STATS_POS_Y) + HUD_STATS_SIZE_H -
            (postgame ? 24 : 16), HUD_STATS_SIZE_W, 1, 0, 0, 0);
    startY = (sint16)(startY + 15);
    char string[40];
    sint16 i = 0;
    for (; i < NextPerp[player]; ++i)
    {
        if (sint32(i) * 2 + 4 >= StatsTimer[player])
            continue;
        Font::TextColor(4);
        sprintf(string, "%d", sint32(i) + 1);
        sint32 rowY = sint32(startY) + sint32(i) * 12;
        Font::TextXY(string, col[0], rowY);
        if (!chasinghuman)
        {
            sprintf(string, "%s", BTCPerpInfo[player][i].name);
            Font::TextXY(string, col[1], rowY);
        }
        ParseTime(BTCPerpInfo[player][i].time, string);
        Font::TextXY(string, col[2], rowY);
        Font::TextXY(TextSys::Word(BTCPerpInfo[player][i].caught != 0 ? 0x3D : 0x49), col[3], rowY);
    }
    if (showtimeleft && sint32(i) * 2 + 4 < StatsTimer[player])
    {
        Font::TextColor(3);
        ParseTime(FinalBTC_Countdown, string);
        char *label = TextSys::Word(0x4D);
        sint32 rowY = sint32(startY) + sint32(i) * 12 + 2;
        Font::TextXY(label, col[0], rowY);
        Font::TextXY(string, col[chasinghuman ? 3 : 2], rowY);
    }
    if (postgame)
        OptionsBarThing(HUD_STATS_POS_X, HUD_STATS_POS_Y, HUD_STATS_SIZE_W, HUD_STATS_SIZE_H);
    RenderPauseBox(HUD_STATS_POS_X, HUD_STATS_POS_Y, HUD_STATS_SIZE_W, HUD_STATS_SIZE_H);
}

void Hud::RenderStatsView(void)
{
    FUNCTION_MARKER(0x800DAE8C, "NFS4.EXE");
    sint32 screen = (uint32(simGlobal.gameTicks) >> 9) & 1;
    if ((Cars::gHumanRaceCarList[0]->carFlags & 0x200) != 0)
    {
        if (GameSetup::gData.commMode != 1)
            screen = 0;
        else if ((Cars::gHumanRaceCarList[1]->carFlags & 0x200) == 0 || NextPerp[0] != 0)
            screen = 0;
        else if (NextPerp[1] != 0)
            screen = 1;
    }
    else if (GameSetup::gData.commMode == 1)
    {
        if ((Cars::gHumanRaceCarList[1]->carFlags & 0x200) != 0)
            screen = 1;
    }
    else if (Cars::gNumRaceCars == 1)
        screen = 1;

    if (screen == 0)
    {
        StatsTimer[1] = 0;
        sint32 time = (sint32)(uint32(StatsTimer[0]) + 1U);
        StatsTimer[0] = time;
        if (time > 10000)
            StatsTimer[0] = 10000;
        if ((Cars::gHumanRaceCarList[0]->carFlags & 0x200) == 0)
            RaceSummary();
        else
            BTCStats(0, true);
    }
    else
    {
        StatsTimer[0] = 0;
        sint32 time = (sint32)(uint32(StatsTimer[1]) + 1U);
        StatsTimer[1] = time;
        if (time > 10000)
            StatsTimer[1] = 10000;

        if (GameSetup::gData.commMode != 1 || (Cars::gHumanRaceCarList[1]->carFlags & 0x200) == 0)
            RaceStatistics();
        else
            BTCStats(1, true);
    }
}
