#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/audiomus.h"
#include "game/common/bworldsm.h"
#include "game/common/car.h"
#include "game/common/carobj.h"
#include "game/common/copspeak.h"
#include "game/common/fastrand.h"
#include "game/common/paths.h"
#include "game/common/spchevnt.h"
#include "game/common/speech.h"
#include "game/psx/draww.h"
#include "lib/psx/fixdmult.h"

Speech *Speech::fgSpeech;
Speech::Speaker *Speech::fgUndefined;

Speech::tMobileVoiceAttr Speech::gCopAttr[9] =
{
    {1, 0},
    {2, 0},
    {4, 0},
    {1, 5},
    {2, 5},
    {4, 5},
    {1, -5},
    {2, -5},
    {4, -5},
};

Speech::tLocationDescription Speech::gLocationDescription00[13] =
{
    { "cab.", 45, 56 },
    { "cab.", 70, 76 },
    { "baov.", 188, 191 },
    { "tun.", 250, 260 },
    { "scrv.", 304, 340 },
    { "twr.", 480, 483 },
    { "brg.", 588, 600 },
    { "frm.", 660, 666 },
    { "tun.", 690, 699 },
    { "pike.", 794, 847 },
    { "ltun.", 855, 910 },
    { "icec.", 911, 950 },
    { 0, 0, 0 },
};

Speech::tLocationDescription Speech::gLocationDescription02[14] =
{
    { "mar.", 1061, 20 },
    { "bay.", 110, 130 },
    { "rarc.", 185, 188 },
    { "scrv.", 236, 260 },
    { "lth.", 305, 318 },
    { "ruin.", 456, 470 },
    { "spfl.", 616, 618 },
    { "brg.", 632, 638 },
    { "tbrg.", 639, 656 },
    { "brg.", 657, 662 },
    { "ltun.", 806, 839 },
    { "tun.", 959, 975 },
    { "bgdr.", 990, 1025 },
    { 0, 0, 0 },
};

Speech::tLocationDescription Speech::gLocationDescription03[13] =
{
    { "over.", 57, 59 },
    { "over.", 96, 99 },
    { "over.", 139, 142 },
    { "tun.", 156, 165 },
    { "fld.", 222, 260 },
    { "brg.", 264, 274 },
    { "stun.", 365, 416 },
    { "htou.", 545, 551 },
    { "rarc.", 661, 662 },
    { "ruin.", 679, 705 },
    { "tun.", 869, 877 },
    { "ogt.", 914, 929 },
    { 0, 0, 0 },
};

Speech::tLocationDescription Speech::gLocationDescription04[9] =
{
    { "trnx.", 1, 2 },
    { "lsp.", 94, 96 },
    { "cab.", 162, 163 },
    { "hsv.", 170, 300 },
    { "pent.", 305, 309 },
    { "tun.", 465, 474 },
    { "brg.", 575, 593 },
    { "ltun.", 693, 709 },
    { 0, 0, 0 },
};

Speech::tLocationDescription Speech::gLocationDescription05[14] =
{
    { "ogt.", 4, 8 },
    { "wfrm.", 35, 109 },
    { "tun.", 240, 250 },
    { "ruin.", 251, 325 },
    { "tun.", 326, 338 },
    { "fld.", 366, 452 },
    { "owel.", 569, 572 },
    { "over.", 681, 682 },
    { "toll.", 711, 713 },
    { "toll.", 741, 743 },
    { "obwm.", 875, 889 },
    { "vill.", 890, 925 },
    { "over.", 1066, 1069 },
    { 0, 0, 0 },
};

Speech::tLocationDescription Speech::gLocationDescription06[11] =
{
    { "fld.", 17, 45 },
    { "omus.", 118, 120 },
    { "kngm.", 265, 275 },
    { "vill.", 330, 398 },
    { "scrv.", 570, 627 },
    { "stun.", 637, 657 },
    { "hpin.", 705, 742 },
    { "over.", 760, 765 },
    { "arch.", 818, 821 },
    { "ltun.", 940, 1017 },
    { 0, 0, 0 },
};

Speech::tLocationDescription Speech::gLocationDescription07[11] =
{
    { "scrv.", 32, 74 },
    { "over.", 116, 158 },
    { "ltun.", 249, 301 },
    { "over.", 330, 334 },
    { "medi.", 348, 426 },
    { "frm.", 515, 536 },
    { "farm.", 537, 538 },
    { "frm.", 540, 577 },
    { "vill.", 627, 720 },
    { "tun.", 767, 785 },
    { 0, 0, 0 },
};

Speech::tLocationDescription *Speech::gLocationDescription[11] =
{
    Speech::gLocationDescription00,
    Speech::gLocationDescription02,
    Speech::gLocationDescription02,
    Speech::gLocationDescription03,
    Speech::gLocationDescription04,
    Speech::gLocationDescription05,
    Speech::gLocationDescription06,
    Speech::gLocationDescription07,
    Speech::gLocationDescription02,
    Speech::gLocationDescription02,
    Speech::gLocationDescription02,
};

Speech::tCarDescription Speech::gCarDescription[15] =
{
    { "BMWZ", "bmwz3.", "bmw.", "z3." },
    { "HSVT", "sedan.", "sedan.", "sedan." },
    { "FORD", "sedan.", "sedan.", "sedan." },
    { "CMRO", "chvcam.", "chevro.", "cam." },
    { "TRAM", "pont.", "pont.", "pont." },
    { "ADB7", "aston.", "aston.", "aston." },
    { "JXKR", "jag.", "jag.", "jag." },
    { "BMWM", "bmwm5.", "bmw.", "m5." },
    { "CORV", "chvcor.", "chevro.", "corv." },
    { "P993", "por911.", "porsc.", "porsc." },
    { "DIAB", "lamdia.", "lamb.", "dia." },
    { "MCF1", "mclar.", "mclar.", "mclar." },
    { "X993", "por911.", "porsc.", "porsc." },
    { "XCF1", "chvcor.", "chevro.", "corv." },
    { 0, "sport.", "sport.", "sport." },
};

Speech::tCallSignDescription Speech::gCallSignDescription[1] =
{
    { "all.",
      { "dis.", "cen.", "cont.", "hq.", "cou." },
      { "20.", "25.", "31.", "37.", "38.", "43.", "45.", "46.", "52.", "217.", "223.", "316.", "332.", "355.", "371." } },
};

void *Speech::operator new(size_t size) throw()
{
    return reservememadr(0, (sint32)size, 0);
}

void Speech::operator delete(void *memory) throw()
{
    purgememadr(memory);
}

void *Speech::Speaker::operator new(size_t size) throw()
{
    return reservememadr(0, (sint32)size, 0);
}

void Speech::Speaker::operator delete(void *memory) throw()
{
    purgememadr(memory);
}

sint32 Speech::DispatchSpeaker::StatusCount(void)
{
    FUNCTION_MARKER(0x80099358, "NFS4.EXE");
    return fStatusCount;
}

Speech::Speaker *Speech::DispatchSpeaker::StatusSub(void)
{
    FUNCTION_MARKER(0x8009934C, "NFS4.EXE");
    return fStatusSub;
}

void Speech::DispatchSpeaker::PurgeStatusSub(void)
{
    FUNCTION_MARKER(0x80099344, "NFS4.EXE");
    fStatusSub = 0;
}

Speech::CarBank *Speech::DispatchSpeaker::GetCarBank(sint32 carIndex)
{
    FUNCTION_MARKER(0x80099328, "NFS4.EXE");
    return &fgSpeech->fCarBank.Dispatch[carIndex];
}

Speech::LocationBank *Speech::DispatchSpeaker::FindClosestLocationTo(sint32 slice)
{
    FUNCTION_MARKER(0x80099300, "NFS4.EXE");
    return fgSpeech->FindClosestLocationTo(fgSpeech->fLocationBank.Dispatch, slice);
}

Speech::CallSignBank *Speech::DispatchSpeaker::CallSign(void)
{
    FUNCTION_MARKER(0x800992F4, "NFS4.EXE");
    return &fgSpeech->fCallSignBank.Dispatch;
}

bool Speech::MobileSpeaker::IsSuper(void)
{
    FUNCTION_MARKER(0x800992D8, "NFS4.EXE");
    return (fCarObj->carFlags & 0x40) != 0;
}

Car::tObj *Speech::MobileSpeaker::CarObj(void)
{
    FUNCTION_MARKER(0x800992CC, "NFS4.EXE");
    return fCarObj;
}

Speech::CarBank *Speech::MobileSpeaker::GetCarBank(sint32 carIndex)
{
    FUNCTION_MARKER(0x800992B4, "NFS4.EXE");
    return &fgSpeech->fCarBank.Mobile[carIndex];
}

Speech::LocationBank *Speech::MobileSpeaker::FindClosestLocationTo(sint32 slice)
{
    FUNCTION_MARKER(0x8009928C, "NFS4.EXE");
    return fgSpeech->FindClosestLocationTo(fgSpeech->fLocationBank.Mobile, slice);
}

Speech::CallSignBank *Speech::MobileSpeaker::CallSign(void)
{
    FUNCTION_MARKER(0x80099280, "NFS4.EXE");
    return &fgSpeech->fCallSignBank.Mobile;
}

sint32 Speech::MobileSpeaker::Unit(void)
{
    FUNCTION_MARKER(0x80099274, "NFS4.EXE");
    return fUnit;
}

Car::tObj *Speech::MobileSpeaker::Perp(void)
{
    FUNCTION_MARKER(0x80099268, "NFS4.EXE");
    return fPerp;
}

char *Speech::AllocateRAM(sint32 numBytes, char *message)
{
    FUNCTION_MARKER(0x800942E8, "NFS4.EXE");
    return (char *)reservememadr(message, numBytes, 0);
}

void Speech::PurgeRAM(char *memPtr)
{
    FUNCTION_MARKER(0x80094314, "NFS4.EXE");
    purgememadr(memPtr);
}

sint32 Speech::HandleRequest(sint32 bank, sint32 localoffset, sint32 size, sint32 event)
{
    FUNCTION_MARKER(0x80094334, "NFS4.EXE");
    return SubmitRequest(bank, localoffset, size);
}

void Speech::CarBankName::SetCar(sint32 carIndex)
{
    FUNCTION_MARKER(0x80094354, "NFS4.EXE");
    tCarDescription *d = gCarDescription;
    char *game = GameSetup::gCarNames[GameSetup::gData.carInfo[carIndex].carType];
    while (d->game != 0 && strncmp(game, d->game, 4) != 0)
        ++d;
    fFull = d->full;
    fMake = d->make;
    fModel = d->model;
}

bool Speech::CarBank::Check(char *name, sint32 id, Speech::CarBankName *bankname)
{
    FUNCTION_MARKER(0x8009441C, "NFS4.EXE");
    bool match = false;
    if (bankname->fFull != 0 && strncmp(name, bankname->fFull, strlen(bankname->fFull)) == 0)
    {
        fFull = id;
        match = true;
    }
    if (bankname->fMake != 0 && strncmp(name, bankname->fMake, strlen(bankname->fMake)) == 0)
    {
        fMake = id;
        match = true;
    }
    if (bankname->fModel != 0 && strncmp(name, bankname->fModel, strlen(bankname->fModel)) == 0)
    {
        fModel = id;
        match = true;
    }
    return match;
}

bool Speech::CheckCarBank(Speech::CarBank *carbank, char *name, sint32 id, Speech::CarBankName *bankname)
{
    FUNCTION_MARKER(0x80094524, "NFS4.EXE");
    bool match = false;
    for (sint32 i = 0; i < fCarCount; ++i)
    {
        if (carbank->Check(name, id, bankname))
            match = true;
        ++bankname;
        ++carbank;
    }
    return match;
}

void Speech::Speaker::SetCar(Car::tObj *car)
{
    FUNCTION_MARKER(0x800945D8, "NFS4.EXE");
    uint32 carcolour = 1u << (car->carInfo->SpeechColour & 31);
    if (GetCarBank(car->carIndex)->fFull == -1)
    {
        fCar = 0;
        fColour.flags = 0;
    }
    else
    {
        fColour.flags = fgSpeech->fMultiplePerps != 0 ? carcolour : carcolour | 0x78020;
        Speaker *dispatcher = Dispatch();
        if (dispatcher->KnownPerp(car))
            fCar = GetCarBank(car->carIndex)->fModel;
        else
            fCar = GetCarBank(car->carIndex)->fFull;
    }
}

void Speech::CountLocations(void)
{
    FUNCTION_MARKER(0x800946F8, "NFS4.EXE");
    fLocationCount = 0;
    tLocationDescription *d = gLocationDescription[GameSetup::gData.track];
    if (d != 0)
    {
        while (d->name != 0)
        {
            if (fLocationCount >= 16)
                return;
            ++d;
            ++fLocationCount;
        }
    }
}

bool Speech::CheckLocationBank(Speech::LocationBank *locationbank, char *name, sint32 id)
{
    FUNCTION_MARKER(0x80094768, "NFS4.EXE");
    tLocationDescription *d = gLocationDescription[GameSetup::gData.track];
    bool match = false;
    if (d == 0)
        return false;
    for (sint32 i = 0; i < fLocationCount; ++i)
    {
        if (strncmp(name, d->name, strlen(d->name)) == 0)
        {
            sint32 start = d->start;
            sint32 end = d->end;
            char *bankname = d->name;
            match = true;
            locationbank->fBankId = id;
            locationbank->fStartSlice = start;
            locationbank->fEndSlice = end;
            locationbank->fName = bankname;
        }
        ++d;
        ++locationbank;
    }
    return match;
}

sint32 Speech::LocationBank::Distance(sint32 slice)
{
    FUNCTION_MARKER(0x8009486C, "NFS4.EXE");
    sint32 start = fStartSlice;
    sint32 end = fEndSlice;
    sint32 first;
    sint32 second;
    if (end < start)
    {
        if (slice >= start || slice <= end)
            return 0;
        first = (start - slice);
        second = (slice - end);
    }
    else if (slice < start)
    {
        first = (start - slice);
        second = ((slice + gNumSlices) - end);
    }
    else if (slice > end)
    {
        first = (slice - end);
        second = ((start + gNumSlices) - slice);
    }
    else
        return 0;
    return first < second ? first : second;
}

Speech::LocationBank *Speech::FindClosestLocationTo(Speech::LocationBank *bank, sint32 slice)
{
    FUNCTION_MARKER(0x80094900, "NFS4.EXE");
    LocationBank *closestbank = 0;
    sint32 closestdistance = 10000;
    for (sint32 i = 0; i < fLocationCount; ++i)
    {
        LocationBank *locationbank = &bank[i];
        if (locationbank->fBankId != -1)
        {
            sint32 distance = locationbank->Distance(slice);
            if (distance < closestdistance)
            {
                closestdistance = distance;
                closestbank = locationbank;
            }
        }
    }
    return closestbank;
}

void Speech::Speaker::FindLocation(Car::tObj *car)
{
    FUNCTION_MARKER(0x800949D8, "NFS4.EXE");
    sint32 delta = fixedmult(car->currentSpeed, 0x50000) / 0x60000;
    sint32 slice = (car->N.simRoadInfo.slice) + delta;
    if (delta < 0)
    {
        if (slice < 0)
            slice += gNumSlices;
    }
    else if (slice >= gNumSlices)
        slice -= gNumSlices;
    LocationBank *location = FindClosestLocationTo(slice);

    XASSERT(location != 0);
    sint32 actual = location->Distance((sint32)car->N.simRoadInfo.slice);
    sint32 distance = location->Distance(slice);
    if (distance == 0)
    {
        fDistance.flags = 0;
        fPosition.flags = 4;
    }
    else if (distance < 34)
    {
        fDistance.flags = 0;
        if (distance + 2 < actual)
            fPosition.flags = 9;
        fPosition.flags = distance - 2 < actual ? 8 : 2;
    }
    else if (GameSetup::gData.measurement == 1)
    {
        fDistance.flags = distance < 166 ? 8 : 16;
        fPosition.flags = distance - 2 < actual ? 16 : 2;
    }
    else
    {
        fDistance.flags = distance < 134 ? 1 : distance < 268 ? 2 : 4;
        fPosition.flags = distance - 2 < actual ? 16 : 2;
    }
    fLocation = location->fBankId;
}

bool Speech::CheckCallSignBank(Speech::CallSignBank *bank, char *name, sint32 id)
{
    FUNCTION_MARKER(0x80094CD0, "NFS4.EXE");
    tCallSignDescription *d = gCallSignDescription;
    bool match = false;
    if (strncmp(name, d->AllUnits, strlen(d->AllUnits)) == 0)
    {
        bank->fAllUnits = id;
        match = true;
    }
    sint32 dispatchName = GameSetup::gData.track % 5;
    if (strncmp(name, d->Dispatch[dispatchName], strlen(d->Dispatch[dispatchName])) == 0)
    {
        bank->fDispatch = id;
        match = true;
    }
    for (sint32 i = 0; i < 15; ++i)
    {
        if (strncmp(name, d->Mobile[i], strlen(d->Mobile[i])) == 0)
        {
            bank->fMobile[i] = id;
            match = true;
        }
    }
    return match;
}

bool Speech::CheckMultiBank(char *name, sint32 id, Speech::CarBankName *bn)
{
    FUNCTION_MARKER(0x80094E08, "NFS4.EXE");
    if (strncmp(name, "j:id\\", 5) == 0)
        return CheckCallSignBank(&fCallSignBank.Dispatch, name + 5, id);
    if (strncmp(name, "j:cars\\", 7) == 0)
        return CheckCarBank(fCarBank.Dispatch, name + 7, id, bn);
    if (strncmp(name, "j:location\\", 11) == 0)
        return CheckLocationBank(fLocationBank.Dispatch, name + 11, id);
    if (strncmp(name, "j:cid\\", 6) == 0)
        return CheckCallSignBank(&fCallSignBank.Mobile, name + 6, id);
    if (strncmp(name, "j:ccars\\", 8) == 0)
        return CheckCarBank(fCarBank.Mobile, name + 8, id, bn);
    if (strncmp(name, "j:clocaton\\", 11) == 0)
        return CheckLocationBank(fLocationBank.Mobile, name + 11, id);
    if (strncmp(name, "j:clip.", 7) == 0)
        fBlpClpBank = id;
    else if (strncmp(name, "j:silenc.", 9) == 0)
        fStaticBank = id;
    return true;
}

sint32 Speech::CalculateBankSize(char *header, Speech::CarBankName *bn, sint32 *hoffset, sint32 *hsize)
{
    FUNCTION_MARKER(0x80094FA8, "NFS4.EXE");
    sint32 bcount = 0;
    sint32 bsize = 0;
    sint32 offset;
    sint32 size;
    if (locatebigentry(header, "j:eventdat\\event.dat", 0, &offset, &size) != 0)
    {
        bsize = size;
        const uint8 *p = (const uint8 *)header + 8;
        sint32 filecount = ((uint32)p[0] << 24 | (uint32)p[1] << 16 | (uint32)p[2] << 8 | p[3]);
        char *c = header + 16;
        for (sint32 i = 0; i < filecount; ++i)
        {
            p = (const uint8 *)c;
            offset = ((uint32)p[0] << 24 | (uint32)p[1] << 16 | (uint32)p[2] << 8 | p[3]);
            size = ((uint32)p[4] << 24 | (uint32)p[5] << 16 | (uint32)p[6] << 8 | p[7]);
            c += 8;
            char *name = c;
            while (*c != 0)
                ++c;
            if (c[-4] == '.' && c[-3] == 'h' && c[-2] == 'd' && c[-1] == 'r')
            {
                if (*hoffset == 0) *hoffset = offset;
                *hsize = ((offset + size) - (*hoffset));
                if (CheckMultiBank(name, bcount, bn))
                {
                    ++bcount;
                    bsize = (bsize + size);
                }
            }
            ++c;
        }
    }
    fBankCount = bcount;
    return bsize;
}

void Speech::LoadBankHeaders(char *header, Speech::CarBankName *bn, sint32 hoffset, sint32 hsize)
{
    FUNCTION_MARKER(0x800951BC, "NFS4.EXE");
    sint32 offset;
    sint32 size;
    sint32 id = 0;
    sint32 dt = 0;
    char *data = (char *)(fBankOffset + fBankCount);
    locatebigentry(header, "j:eventdat\\event.dat", 0, &offset, &size);
    XPORT_UNPLACED::FILE::readsync(fFileHandle, offset, data, size, 100);
    iSPCH::BindData(reinterpret_cast<uint8 *>(data));
    const uint8 *p = (const uint8 *)header + 8;
    sint32 filecount = ((uint32)p[0] << 24 | (uint32)p[1] << 16 | (uint32)p[2] << 8 | p[3]);
    data += size;
    char *hdata = (char *)reservememadr("spch temp", hsize, 16);
    XPORT_UNPLACED::FILE::readsync(fFileHandle, hoffset, hdata, hsize, 100);
    char **banknames = (char **)reservememadr("spch temp", (sint32)((uint32)fBankCount * sizeof(char *)), 16);
    for (sint32 j = 0; j < fBankCount; ++j)
    {
        banknames[j] = 0;
        fBankOffset[j] = 0;
    }
    char *c = header + 16;
    for (sint32 i = 0; i < filecount; ++i)
    {
        systemtask(0);
        p = (const uint8 *)c;
        offset = ((uint32)p[0] << 24 | (uint32)p[1] << 16 | (uint32)p[2] << 8 | p[3]);
        size = ((uint32)p[4] << 24 | (uint32)p[5] << 16 | (uint32)p[6] << 8 | p[7]);
        c += 8;
        char *name = c;
        sint32 namelen = 0;
        while (*c != 0)
        {
            ++namelen;
            ++c;
        }
        if (namelen > 4)
        {
            if (c[-4] == '.' && c[-3] == 'h' && c[-2] == 'd' && c[-1] == 'r' && CheckMultiBank(name, id, bn))
            {
                ++id;
                memcpy(data, hdata + (offset - hoffset), size);
                sint32 bank = SPCH::AddBank(data);
                data += size;
                banknames[bank] = name;
            }
            else if (c[-4] == '.' && c[-3] == 'd' && c[-2] == 'a' && c[-1] == 't')
            {
                if (dt < fBankCount && banknames[dt] != 0 && strncmp(name, banknames[dt], namelen - 3) == 0)
                {
                    sint32 j = dt;
                    ++dt;
                    fBankOffset[j] = offset;
                }
                else
                {
                    for (sint32 j = 0; j < fBankCount; ++j)
                    {
                        if (banknames[j] != 0 && strncmp(name, banknames[j], namelen - 3) == 0)
                        {
                            dt = j + 1;
                            fBankOffset[j] = offset;
                        }
                    }
                }
            }
        }
        ++c;
    }
    purgememadr(hdata);
    purgememadr(banknames);
}

Speech::Speech(void)
{
    FUNCTION_MARKER(0x800955F4, "NFS4.EXE");
    for (sint32 i = 0; i < 9; ++i)
    {
        fCarBank.Mobile[i].fFull = -1;
        fCarBank.Mobile[i].fMake = -1;
        fCarBank.Mobile[i].fModel = -1;
    }
    for (sint32 i = 0; i < 9; ++i)
    {
        fCarBank.Dispatch[i].fFull = -1;
        fCarBank.Dispatch[i].fMake = -1;
        fCarBank.Dispatch[i].fModel = -1;
    }
    for (sint32 i = 0; i < 16; ++i)
        fLocationBank.Mobile[i].fBankId = -1;
    for (sint32 i = 0; i < 16; ++i)
        fLocationBank.Dispatch[i].fBankId = -1;
    fDispatch = new DispatchSpeaker;
    fDispatch->fSub = 0;
    for (sint32 i = 0; i < 4; ++i)
    {
        MobileSpeaker *speaker = new MobileSpeaker;
        speaker->fSub = 0;
        speaker->fCarObj = 0;
        fMobile[i] = speaker;
    }
    fFileOpen = false;
    fBankOffset = 0;
    sint32 numracers = 0;
    sint32 numhumancops = 0;
    sint32 numCars = GameSetup::gData.numCars;
    for (sint32 i = 0; i < numCars; ++i)
    {
        sint32 carClass = GameSetup::gData.carInfo[i].carClass;
        if (carClass == 1 || carClass == 2)
            ++numracers;
        else if (carClass == 0x41)
            ++numhumancops;
    }
    fMultiplePerps = numhumancops == 0 && numracers > 1;
    gettick();
    fCarCount = GameSetup::gData.numCars;
    CarBankName bn[9];
    for (sint32 i = 0; i < fCarCount; ++i)
        bn[i].SetCar(i);
    CountLocations();
    fBankCount = 0;
    sint32 banksize = 0;
    const char *SpeechLanguage;
    if (GameSetup::gData.languageSpeech == 2)
        SpeechLanguage = "fre";
    else if (GameSetup::gData.languageSpeech == 1)
        SpeechLanguage = "ger";
    else if (GameSetup::gData.languageSpeech == 6)
        SpeechLanguage = "brt";
    else
        SpeechLanguage = "eng";
    char filename[100];
    sprintf(filename, "%szzzz%s.viv", Paths_Paths[29], SpeechLanguage);
    char *header = loadbigfileheader(filename, 16);
    sint32 hoffset = 0;
    sint32 hsize = 0;
    if (header != 0)
        banksize = CalculateBankSize(header, bn, &hoffset, &hsize);
    if (banksize > 0)
        fBankOffset = (sint32 *)reservememadr("spch index", (((fBankCount * 4) + banksize)), 0);
    if (fBankOffset != 0)
    {
        sint32 dataRate = SPCH::GetSampleDataRate(0x2B11, 16, 2);
        SPCH::Init(HandleRequest, 0x12345678, dataRate);
        SPCH::InitBankMem(AllocateRAM, PurgeRAM, fBankCount);
        fFileOpen = XPORT_UNPLACED::FILE::opensync(filename, 1, 100, &fFileHandle) != 0;
        LoadBankHeaders(header, bn, hoffset, hsize);
    }
    if (header != 0)
        purgememadr(header);
    gettick();
}

void Speech::Reset(void)
{
    FUNCTION_MARKER(0x80095994, "NFS4.EXE");
    Speech *speech = fgSpeech;
    if (speech == 0)
        return;
    for (sint32 i = 0; i < 4; ++i)
        speech->fMobile[i]->fCarObj = 0;
    SPCH::ClearEventQueue();
    randtemp = fastRandom * randSeed;
    fastRandom = randtemp & 0xFFFF;
    iSPCH::EACseedrandom((randtemp & 0xFFFF00) >> 8);
    randtemp = fastRandom * randSeed;
    fastRandom = randtemp & 0xFFFF;
    uint32 copVoice = ((randtemp & 0xFFFF00) >> 8) % 9;
    randtemp = fastRandom * randSeed;
    fastRandom = randtemp & 0xFFFF;
    uint32 superVoice = ((randtemp & 0xFFFF00) >> 8) % 6;
    randtemp = fastRandom * randSeed;
    fastRandom = randtemp & 0xFFFF;
    uint32 update = ((randtemp & 0xFFFF00) >> 8) % 7;
    speech = fgSpeech;
    speech->fCopCount = copVoice;
    speech->fSuperCount = superVoice;
    speech->fDispatch->Activate(update);
}

Speech::~Speech(void)
{
    FUNCTION_MARKER(0x80095B30, "NFS4.EXE");
    if (fBankOffset != 0)
    {
        SPCH::Deinit();
        purgememadr(fBankOffset);
    }
    if (fFileOpen)
        XPORT_UNPLACED::FILE::closesync(fFileHandle, 100);
    delete fDispatch;
    for (sint32 i = 0; i < 4; ++i)
        delete fMobile[i];
}

sint32 Speech::BankPatch(sint32 bank, Car::tObj *car)
{
    FUNCTION_MARKER(0x80095BE4, "NFS4.EXE");
    if (bank == fStaticBank)
        return 0x15;
    if (bank == fBlpClpBank)
        return car != 0 ? 0x14 : 0x13;
    return -1;
}

sint32 Speech::SubmitRequest(sint32 bank, sint32 localoffset, sint32 size)
{
    FUNCTION_MARKER(0x80095C20, "NFS4.EXE");
    Speech *speech = fgSpeech;
    if (speech == 0)
        return 0;
    speech->fDispatch->fStatusSub = 0;
    speech->fDispatch->fStatusCount = 0x200;
    Car::tObj *car = speech->fSpeakerCar;
    sint32 patch = speech->BankPatch(bank, car);
    speech = fgSpeech;
    sint32 offset = 0;
    if (bank >= 0 && bank < speech->fBankCount)
        offset = speech->fBankOffset[bank];
    sint32 requestoffset = (offset + localoffset);
    if (patch >= 0)
        CopSpeak::GenericBankRequest(patch, car);
    else if (offset != 0)
        CopSpeak::DirectRequest(fgSpeech->fFileHandle, requestoffset, size, car, 0);
    return requestoffset;
}

void Speech::Speaker::Report(Car::tObj *cop)
{
    FUNCTION_MARKER(0x80095D14, "NFS4.EXE");
}

void Speech::Speaker::Deny(void)
{
    FUNCTION_MARKER(0x80095D1C, "NFS4.EXE");
}

void Speech::Speaker::Grant(void)
{
    FUNCTION_MARKER(0x80095D24, "NFS4.EXE");
}

void Speech::Speaker::Ready(Car::tObj *wing)
{
    FUNCTION_MARKER(0x80095D2C, "NFS4.EXE");
}

void Speech::Speaker::Engage(Car::tObj *perp)
{
    FUNCTION_MARKER(0x80095D34, "NFS4.EXE");
}

void Speech::Speaker::Lose(void)
{
    FUNCTION_MARKER(0x80095D3C, "NFS4.EXE");
}

void Speech::Speaker::Accident(sint32 slice)
{
    FUNCTION_MARKER(0x80095D44, "NFS4.EXE");
}

void Speech::Speaker::Catch(sint32 ticket)
{
    FUNCTION_MARKER(0x80095D4C, "NFS4.EXE");
}

void Speech::Speaker::RoadBlock(void)
{
    FUNCTION_MARKER(0x80095D54, "NFS4.EXE");
}

void Speech::Speaker::SpikeBelt(void)
{
    FUNCTION_MARKER(0x80095D5C, "NFS4.EXE");
}

void Speech::Speaker::Backup(void)
{
    FUNCTION_MARKER(0x80095D64, "NFS4.EXE");
}

void Speech::Speaker::ReportBlockade(void)
{
    FUNCTION_MARKER(0x80095D6C, "NFS4.EXE");
}

void Speech::Speaker::Roger(void)
{
    FUNCTION_MARKER(0x80095D74, "NFS4.EXE");
}

void Speech::Speaker::Bullhorn(void)
{
    FUNCTION_MARKER(0x80095D7C, "NFS4.EXE");
}

void Speech::Speaker::Purge(void)
{
    FUNCTION_MARKER(0x80095D84, "NFS4.EXE");
}

void Speech::Speaker::Promote(void)
{
    FUNCTION_MARKER(0x80095D8C, "NFS4.EXE");
    Speaker *Super = Dispatch();
    Speaker *Sub = Super->fSub;
    while (Sub != 0 && Sub != this)
    {
        Super = Sub;
        Sub = Super->fSub;
    }
    Super->fSub = fSub;
    fSub = Dispatch()->fSub;
    Dispatch()->fSub = this;
}

void Speech::Server(void)
{
    FUNCTION_MARKER(0x80095DF8, "NFS4.EXE");
    Dispatch()->Status();
}

void Speech::SetDelayedStatus(Speech::Speaker *sub, sint32 delay)
{
    FUNCTION_MARKER(0x80095E34, "NFS4.EXE");
    Speech *speech = fgSpeech;
    speech->fDispatch->fStatusSub = sub;
    speech->fDispatch->fStatusCount = delay;
}

void Speech::DispatchSpeaker::Activate(sint32 seedupdatecount)
{
    FUNCTION_MARKER(0x80095E54, "NFS4.EXE");
    fFrom = CallSign()->fDispatch;
    sint32 track = GameSetup::gData.track;
    fConfirm.flags = 0xFF;
    fPerpName.flags = 0xF;
    fSub = 0;
    fReverse.flags = track & 1;
    for (sint32 i = 1; i >= 0; --i)
        fPerp[i] = 0;
    fStatusCount = 0x200;
    fStatusSub = 0;
    fUpdateCount = seedupdatecount;
    fHavePerp = false;
}

Speech::Speaker *Speech::Dispatch(void)
{
    FUNCTION_MARKER(0x80095EF0, "NFS4.EXE");
    if (fgSpeech != 0 && fgSpeech->fBankOffset != 0)
        return fgSpeech->fDispatch;
    return fgUndefined;
}

void Speech::DispatchSpeaker::Roger(void)
{
    FUNCTION_MARKER(0x80095F28, "NFS4.EXE");
    fgSpeech->fSpeakerCar = 0;
    if (fSub == 0 || fSub->Perp() == 0 || fSub->fBlockade.flags != 0)
        SPCHNFS_D_A_CONFIRM(&fConfirm);
    else if (fSub->fArrest.flags != 0)
    {
        CallSignBank *bank = CallSign();
        sint32 unit = fSub->Unit();
        fTo = bank->fMobile[unit];
        SPCHNFS_D_C_PERP_APPREHENSION_REPLY(fTo, &fConfirm, &fPerpName);
    }
    else if (fSub->fUpdate.flags == 0)
    {
        SPCHNFS_D_A_CONFIRM(&fConfirm);
        SPCH::PlaySpeech();
        SetCar(fSub->Perp());
        SPCHNFS_D_C_PERP_LOST_CONFIRM(&fColour, fCar);
    }
    else
    {
        SetCar(fSub->Perp());
        CallSignBank *bank = CallSign();
        sint32 unit = fSub->Unit();
        fTo = bank->fMobile[unit];
        sint32 distance = fSub->DistToPerp();
        if (distance < 0x280000)
            SPCHNFS_D_C_IN_PURS_NEAR_PERP_CONFIRM(&fColour, fCar, fTo, &fConfirm, &fPerpName);
        else
            SPCHNFS_D_C_IN_PURS_AWAY_PERP_CONFIRM(&fColour, fCar, fTo, &fConfirm, &fPerpName);
    }
    SPCH::PlaySpeech();
}

void Speech::DispatchSpeaker::StatusReply(void)
{
    FUNCTION_MARKER(0x8009619C, "NFS4.EXE");
    if (fSub == 0 || fSub->Perp() == 0)
        return;
    fgSpeech->fSpeakerCar = 0;
    Speaker *Blocker = 0;
    if (fSub->fBlockade.flags != 0)
    {
        if (fgSpeech->fMultiplePerps != 0)
        {
            fSub->ReportBlockade();
            return;
        }
        Speaker *next = fSub->fSub;
        if (next != 0 && next->fBlockade.flags != 0)
        {
            Blocker = next;
            CallSignBank *bank = CallSign();
            sint32 unit = fSub->Unit();
            fTo = bank->fMobile[unit];
        }
        else
        {
            Blocker = fSub;
            CallSignBank *bank = CallSign();
            fTo = bank->fAllUnits;
        }
        Blocker->ReportBlockade();
        fgSpeech->fSpeakerCar = 0;
        SPCHNFS_D_A_CONFIRM(&fConfirm);
        SPCH::PlaySpeech();
    }
    else
    {
        CallSignBank *bank = CallSign();
        sint32 unit = fSub->Unit();
        fTo = bank->fMobile[unit];
    }
    SPCHNFS_D_C_INTRO_CALL(fTo, fFrom, &fReverse);
    SPCH::PlaySpeech();
    Car::tObj *car = fSub->CarObj();
    FindLocation(car);
    uint32 blockade = fSub->fBlockade.flags;
    if (blockade == 1)
    {
        CallSignBank *bank = CallSign();
        sint32 unit = Blocker->Unit();
        sint32 wing = bank->fMobile[unit];
        fSpikeSide.flags = 4;
        fWing = wing;
        SPCHNFS_D_C_SPBLT_CONFIRMED(&fPosition, fLocation, &fDistance, wing, &fSpikeSide);
        SPCH::PlaySpeech();
        if (Blocker != fSub)
            fSub->Roger();
    }
    else if (blockade == 2)
    {
        SPCHNFS_D_C_RDBLK_CONFIRMED(&fPosition, fLocation, &fDistance);
        SPCH::PlaySpeech();
        if (Blocker != fSub)
            fSub->Roger();
    }
    else if (fSub->fSub != 0)
    {
        CallSignBank *bank = CallSign();
        sint32 unit = fSub->fSub->Unit();
        sint32 wing = bank->fMobile[unit];
        fWing = wing;
        SPCHNFS_D_C_BKUP_REQUEST_GRANT_REPLY(&fDistance, &fPosition, fLocation, wing);
        SPCH::PlaySpeech();
        fSub->Roger();
        Speaker *next = fSub->fSub;
        sint32 update = fUpdateCount;
        fStatusCount = 0x140;
        fUpdateCount = (update + 1);
        fStatusSub = next;
    }
    Speaker *current = fSub;
    Speaker *next = current->fSub;
    if (next != 0 && next->fBlockade.flags != 0)
        current->fBlockade.flags = 0;
}

void Speech::DispatchSpeaker::Status(void)
{
    FUNCTION_MARKER(0x800965D0, "NFS4.EXE");
    if (fSub == 0 || fSub->Perp() == 0)
        return;
    if ((fSub->CarObj()->carFlags & 0x200) != 0)
    {
        sint32 dist = fSub->DistToPerp();
        Speaker *current = fSub;
        if (!current->fHavePerp)
        {
            if (dist < 0x640000)
            {
                Car::tObj *perp = current->Perp();
                current->Engage(perp);
            }
        }
        else if (dist > 0x15e0000)
        {
            current->fHavePerp = false;
            fSub->fUpdate.flags = 0;
            fSub->Status();
        }
    }
    if (fStatusSub != 0)
    {
        fStatusCount = (fStatusCount - 1);
        if (fStatusCount != 0)
            return;
        Speaker *status = fStatusSub;
        if (status == this)
        {
            StatusReply();
            return;
        }
        bool nextInChain = fSub != 0 && status == fSub->fSub;
        status->Status();
        if (nextInChain)
            return;
        fgSpeech->fSpeakerCar = 0;
        Roger();
        return;
    }
    if (fStatusCount > 0)
    {
        fStatusCount = (fStatusCount - 1);
        return;
    }
    if (fSub->fBlockade.flags != 0 || fSub->fArrest.flags != 0)
        return;
    Speaker *next = fSub->fSub;
    if (next != 0)
    {
        Car::tObj *nextPerp = next->Perp();
        Car::tObj *currentPerp = fSub->Perp();
        if (nextPerp != currentPerp)
            fSub->fSub->Promote();
    }
    switch (fUpdateCount & 3U)
    {
    case 0:
        fSub->fUpdate.flags = 0x26;
        break;
    case 1:
        fSub->fUpdate.flags = 1;
        break;
    case 2:
    {
        bool fast = false;
        if (fSub->DistToPerp() < 0x280000)
        {
            Car::tObj *car = fSub->CarObj();
            if (CalcMph(car) > 50)
            {
                Car::tObj *perp = fSub->Perp();
                fast = CalcMph(perp) > 50;
            }
        }
        fSub->fUpdate.flags = fast ? 8 : 0x26;
        break;
    }
    case 3:
        if (fSub->IsSuper() && fUpdateCount == 7)
        {
            fSub->fUpdate.flags = 0;
            fSub->Status();
            fUpdateCount = (fUpdateCount + 1);
            return;
        }
        if (fSub->DistToPerp() < 0x140000)
        {
            fSub->Bullhorn();
            fUpdateCount = (fUpdateCount + 1);
            return;
        }
        fSub->fUpdate.flags = 1;
        break;
    }
    fgSpeech->fSpeakerCar = 0;
    CallSignBank *bank = CallSign();
    sint32 unit = fSub->Unit();
    sint32 to = bank->fMobile[unit];
    fTo = to;
    SPCHNFS_D_C_INTRO_CALL(to, fFrom, &fReverse);
    SPCH::PlaySpeech();
    SPCHNFS_D_C_IN_PURS_NEAR_PERP(&fSub->fUpdate);
    SPCH::PlaySpeech();
    Speaker *current = fSub;
    sint32 update = fUpdateCount;
    fStatusCount = 0x60;
    fStatusSub = current;
    fUpdateCount = (update + 1);
}

void Speech::MobileSpeaker::Status(void)
{
    FUNCTION_MARKER(0x80096B88, "NFS4.EXE");
    if (Perp() == 0)
        return;
    fgSpeech->fSpeakerCar = fCarObj;
    if (fArrest.flags == 1)
    {
        fTo = CallSign()->fDispatch;
        SPCHNFS_C_A_INTRO(&fVoice, fTo, fFrom, &fReverse);
        SPCH::PlaySpeech();
        SPCHNFS_C_D_PERP_APPREHENSION(&fVoice, &fPerpName);
    }
    else
    {
        if ((CarObj()->carFlags & 0x200) != 0 && fUpdate.flags == 0)
        {
            fTo = CallSign()->fDispatch;
            SPCHNFS_C_A_INTRO(&fVoice, fTo, fFrom, &fReverse);
            SPCH::PlaySpeech();
            Car::tObj *perp = Perp();
            SetCar(perp);
            FindLocation(fCarObj);
            SPCHNFS_C_D_PERP_LOST(&fVoice, &fColour, fCar, &fPosition, fLocation, &fDistance, &fPerpName);
            SPCH::PlaySpeech();
            Speaker *Sub = Dispatch()->fSub;
            Dispatch()->fSub = this;
            Dispatch()->Roger();
            Dispatch()->fSub = Sub;
            return;
        }
        bool criticize = false;
        if (IsSuper() && fUpdate.flags == 0 && fSub != 0)
            criticize = (fSub->CarObj()->carFlags & 0x40) == 0;
        if (criticize)
        {
            fTo = CallSign()->fAllUnits;
            SPCHNFS_C_A_INTRO(&fVoice, fTo, fFrom, &fReverse);
            SPCH::PlaySpeech();
            SPCHNFS_S_C_SUPER_COP_CRITICISM(&fVoice);
        }
        else
        {
            bool joining = false;
            if (Dispatch()->fSub != 0)
                joining = Dispatch()->fSub->fSub == this;
            if (joining)
            {
                SPCHNFS_C_C_NEW_OFFICER_ENGAGING(&fVoice, fFrom);
                Dispatch()->fSub->Roger();
                return;
            }
            fTo = CallSign()->fDispatch;
            SPCHNFS_C_A_INTRO(&fVoice, fTo, fFrom, &fReverse);
            SPCH::PlaySpeech();
            Car::tObj *perp = Perp();
            SetCar(perp);
            Car::tObj *carObj = CarObj();
            FindLocation(carObj);
            SetSpeed(fPerp);
            if (DistToPerp() < 0x280000)
            {
                if (Perp()->N.flightTime > 64)
                    SPCHNFS_C_D_IN_PURS_PERP_AIRBORN(&fVoice);
                else if (fUpdate.flags == 1)
                    SPCHNFS_C_D_IN_PURS_NEAR_PERP_REP_LOC(&fVoice, &fPosition, fLocation, &fDistance);
                else if (fUpdate.flags == 8)
                    SPCHNFS_C_D_IN_PURS_NEAR_PERP_REP_SPD(&fVoice, fSpeed, &fSpeedType);
                else
                    SPCHNFS_C_D_IN_PURS_NEAR_PERP_REP_STS(&fVoice, &fColour, fCar, &fDistance, &fPosition, fLocation);
            }
            else if (!fHavePerp)
            {
                if (fUpdate.flags == 1)
                    SPCHNFS_C_D_IN_PURS_LOOK_PERP_REPLY_LOC(&fVoice, &fPosition, fLocation, &fDistance, &fColour, fCar);
                else if (fUpdate.flags == 8)
                    SPCHNFS_C_D_IN_PURS_NEAR_PERP_REP_SPD(&fVoice, fSpeed, &fSpeedType);
                else
                    SPCHNFS_C_D_IN_PURS_LOOK_PERP_REPLY_STS(&fVoice, &fColour, fCar, &fPerpName);
            }
            else
            {
                if (fUpdate.flags == 1)
                    SPCHNFS_C_D_IN_PURS_AWAY_PERP_REPLY_LOC(&fVoice, &fPosition, fLocation, &fDistance, &fColour, fCar, &fPerpName);
                else if (fUpdate.flags == 8)
                    SPCHNFS_C_D_IN_PURS_NEAR_PERP_REP_SPD(&fVoice, fSpeed, &fSpeedType);
                else
                    SPCHNFS_C_D_IN_PURS_AWAY_PERP_REPLY_STS(&fVoice, &fColour, fCar, &fPosition, fLocation, &fDistance);
            }
        }
    }
    SPCH::PlaySpeech();
}

void Speech::DispatchSpeaker::ClearPerp(Car::tObj *car)
{
    FUNCTION_MARKER(0x80097120, "NFS4.EXE");
    for (sint32 i = 0; i < 2; ++i)
    {
        if (fPerp[i] == car)
            fPerp[i] = 0;
    }
}

bool Speech::DispatchSpeaker::KnownPerp(Car::tObj *car)
{
    FUNCTION_MARKER(0x80097150, "NFS4.EXE");
    for (sint32 i = 0; i < 2; ++i)
    {
        if (fPerp[i] == car)
            return true;
    }
    return false;
}

void Speech::DispatchSpeaker::AddPerp(Car::tObj *car)
{
    FUNCTION_MARKER(0x80097180, "NFS4.EXE");
    for (sint32 i = 0; i < 2; ++i)
    {
        if (fPerp[i] == 0)
            fPerp[i] = car;
    }
}

void Speech::DispatchSpeaker::Report(Car::tObj *perp)
{
    FUNCTION_MARKER(0x800971B0, "NFS4.EXE");
    fgSpeech->fSpeakerCar = 0;
    if (KnownPerp(perp) && fSub != 0)
    {
        if (fgSpeech->fMultiplePerps == 0)
        {
            CallSignBank *bank = CallSign();
            sint32 unit = fSub->Unit();
            fTo = bank->fMobile[unit];
            SPCHNFS_D_C_PERP_SIGHTED_CONFIRM(&fConfirm, fTo);
            SPCH::PlaySpeech();
        }
    }
    else
    {
        if (fgSpeech->fMultiplePerps == 0)
        {
            fTo = CallSign()->fAllUnits;
            SPCHNFS_D_C_INTRO_CALL(fTo, fFrom, &fReverse);
            SPCH::PlaySpeech();
            SetCar(perp);
            FindLocation(perp);
            SPCHNFS_D_C_BEGIN_PURS_REP_SPDR(&fColour, fCar, &fPosition, fLocation, &fDistance);
            SPCH::PlaySpeech();
        }
        AddPerp(perp);
        fStatusCount = 0x2A0;
    }
}

void Speech::DispatchSpeaker::Accident(sint32 slice)
{
    FUNCTION_MARKER(0x80097350, "NFS4.EXE");
}

void Speech::DispatchSpeaker::Deny(void)
{
    FUNCTION_MARKER(0x80097358, "NFS4.EXE");
    if (fSub != 0)
    {
        fgSpeech->fSpeakerCar = 0;
        CallSignBank *bank = CallSign();
        sint32 unit = fSub->Unit();
        fTo = bank->fMobile[unit];
        SPCHNFS_D_C_INTRO_CALL(fTo, fFrom, &fReverse);
        SPCH::PlaySpeech();
        if (fSub->fBlockade.flags != 0)
            SPCHNFS_D_C_RDBLK_SPBLT_DENIED_REPLY(&fSub->fBlockade);
        else
            SPCHNFS_D_C_BKUP_REQUEST_DENIED_REPLY();
        SPCH::PlaySpeech();
        fSub->fBlockade.flags = 0;
    }
}

void Speech::DispatchSpeaker::Grant(void)
{
    FUNCTION_MARKER(0x8009744C, "NFS4.EXE");
    if (fSub != 0)
    {
        Speech *speech = fgSpeech;
        sint32 multiple = speech->fMultiplePerps;
        speech->fSpeakerCar = 0;
        if (multiple == 0)
        {
            SPCHNFS_D_C_RDBLK_SPBLT_GRANT_REPLY(&fSub->fBlockade, &fConfirm);
            SPCH::PlaySpeech();
        }
    }
}

void Speech::DispatchSpeaker::Ready(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800974B0, "NFS4.EXE");
    Speaker *Wing = Mobile(carObj);
    if (fSub != 0 && Wing != fSub)
    {
        Wing->fBlockade.flags = fSub->fBlockade.flags;
        Car::tObj *perp = fSub->Perp();
        Wing->Engage(perp);
    }
    fStatusSub = this;
    fStatusCount = 0x80;
}

sint32 Speech::PickVoice(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8009755C, "NFS4.EXE");
    if ((carObj->carFlags & 0x40) != 0)
    {
        sint32 count = fSuperCount;
        fSuperCount = (count + 1);
        return count % 6;
    }
    if ((carObj->carFlags & 0x80) == 0)
    {
        sint32 count = fCopCount;
        fCopCount = (count + 1);
        return count % 9;
    }
    return 0;
}

void Speech::MobileSpeaker::Activate(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x80097620, "NFS4.EXE");
    fCarObj = carObj;
    sint32 voice = fgSpeech->PickVoice(carObj);
    fUnit = voice;
    if ((carObj->carFlags & 0x40) != 0)
    {
        fUnit = voice + 9;
        fVoice.flags = 8;
    }
    else
    {
        tMobileVoiceAttr *a = &gCopAttr[voice];
        fVoice.flags = a->voice;
    }
    CallSignBank *bank = CallSign();
    fFrom = bank->fMobile[fUnit];
    sint32 track = GameSetup::gData.track;
    fConfirm.flags = 0xFF;
    fPerpName.flags = 0xF;
    fBlockade.flags = 0;
    fArrest.flags = 0;
    fUpdate.flags = 0;
    fPerp = 0;
    fSub = 0;
    fHavePerp = false;
    fReverse.flags = track & 1;
}

void Speech::MobileSpeaker::ReActivate(void)
{
    FUNCTION_MARKER(0x8009770C, "NFS4.EXE");
    sint32 voice = fgSpeech->PickVoice(fCarObj);
    fUnit = voice;
    if ((fCarObj->carFlags & 0x40) != 0)
    {
        fUnit = voice + 9;
        fVoice.flags = 8;
    }
    else
    {
        tMobileVoiceAttr *a = &gCopAttr[voice];
        fVoice.flags = a->voice;
    }
    CallSignBank *bank = CallSign();
    fFrom = bank->fMobile[fUnit];
}

Speech::Speaker *Speech::FindMobile(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x800977B8, "NFS4.EXE");
    for (sint32 i = 0; i < 4; ++i) {
        MobileSpeaker *speaker = fMobile[i];
        if (speaker->fCarObj == carObj)
            return speaker;
    }
    for (sint32 i = 0; i < 4; ++i) {
        if (fMobile[i]->fCarObj == 0) {
            fMobile[i]->Activate(carObj);
            return fMobile[i];
        }
    }
    return fgUndefined;
}

Speech::Speaker *Speech::Mobile(Car::tObj *carObj)
{
    FUNCTION_MARKER(0x8009785C, "NFS4.EXE");
    Speech *speech = fgSpeech;
    if (!speech || !speech->fBankOffset)
        return fgUndefined;
    return speech->FindMobile(carObj);
}

sint32 Speech::Speaker::CalcMph(Car::tObj *perp)
{
    FUNCTION_MARKER(0x800978A4, "NFS4.EXE");
    sint32 speed = perp->linearVel_ch.z;
    if (speed < 0)
        speed = (-speed);
    return fixedmult(0x23CA5, speed) / 65536;
}

void Speech::MobileSpeaker::SetSpeed(Car::tObj *perp)
{
    FUNCTION_MARKER(0x800978EC, "NFS4.EXE");
    sint32 factor;
    if (GameSetup::gData.measurement == 1)
    {
        fSpeedType.flags = 1;
        factor = 0x39999;
    }
    else
    {
        fSpeedType.flags = 2;
        factor = 0x23CA5;
    }
    sint32 speed = perp->linearVel_ch.z;
    if (speed < 0)
        speed = (-speed);
    sint32 step = fixedmult(factor, speed) / 0xA0000 - 3;
    fSpeed = step < 0 ? 0 : step;
}

sint32 Speech::MobileSpeaker::DistToPerp(void)
{
    FUNCTION_MARKER(0x80097A18, "NFS4.EXE");
    Car::tObj *first = CarObj();
    Car::tObj *second = Perp();
    sint32 difference = (first->N.position.x - second->N.position.x);
    if (difference > 0)
    {
        first = CarObj();
        second = Perp();
    }
    else
    {
        first = Perp();
        second = CarObj();
    }
    sint32 x = (first->N.position.x - second->N.position.x);
    first = CarObj();
    second = Perp();
    difference = (first->N.position.z - second->N.position.z);
    if (difference > 0)
    {
        first = CarObj();
        second = Perp();
    }
    else
    {
        first = Perp();
        second = CarObj();
    }
    sint32 z = (first->N.position.z - second->N.position.z);
    sint32 d;
    if (z < x)
        d = (x + (z >> 2));
    else
        d = (z + (x >> 2));
    return d;
}

void Speech::MobileSpeaker::Report(Car::tObj *perp)
{
    FUNCTION_MARKER(0x80097BFC, "NFS4.EXE");
    fgSpeech->fSpeakerCar = fCarObj;
    fTo = CallSign()->fDispatch;
    SPCHNFS_C_A_INTRO(&fVoice, fTo, fFrom, &fReverse);
    SPCH::PlaySpeech();
    fgSpeech->fSpeakerCar = fCarObj;
    SetCar(perp);
    FindLocation(perp);
    SetSpeed(perp);
    SPCHNFS_C_D_PERP_SIGHTED(&fVoice, &fColour, fCar, &fDistance, &fPosition, fLocation, &fPerpName);
    SPCH::PlaySpeech();
    Dispatch()->fSub = this;
}

void Speech::MobileSpeaker::Engage(Car::tObj *perp)
{
    FUNCTION_MARKER(0x80097CE8, "NFS4.EXE");
    fgSpeech->fSpeakerCar = fCarObj;
    fHavePerp = true;
    if (perp == Perp())
    {
        if (fgSpeech->fMultiplePerps != 0 || Dispatch()->StatusCount() >= 0x160)
            return;
        Car::tObj *car = Perp();
        SetCar(car);
        fTo = CallSign()->fDispatch;
        SPCHNFS_C_A_INTRO(&fVoice, fTo, fFrom, &fReverse);
        SPCH::PlaySpeech();
        car = Perp();
        FindLocation(car);
        SPCHNFS_C_C_PERP_REAQUIRED(&fVoice, &fColour, fCar, &fPosition, fLocation, &fDistance);
        SPCH::PlaySpeech();
        return;
    }
    fPerp = perp;
    Car::tObj *car = Perp();
    SetCar(car);
    if ((CarObj()->carFlags & 0x200) == 0)
    {
        Speaker *SubChain = Dispatch();
        for (;;)
        {
            Speaker *next = SubChain->fSub;
            if (next == 0)
                break;
            if (next == this)
            {
                SubChain->fSub = fSub;
                fSub = 0;
                break;
            }
            SubChain = next;
        }
    }
    if (Dispatch()->fSub == this)
    {
        if ((CarObj()->carFlags & 0x200) == 0)
            return;
        Dispatch()->Report(perp);
        fgSpeech->fSpeakerCar = fCarObj;
        fTo = CallSign()->fDispatch;
        car = Perp();
        FindLocation(car);
        SPCHNFS_C_D_ENGAGE_PURS_REP_SPDR_REPLY(&fVoice, fTo, &fColour, fCar, &fDistance, &fPosition, fLocation, &fConfirm);
        SPCH::PlaySpeech();
        return;
    }
    bool arriving = false;
    if (IsSuper() && (Perp()->carFlags & 4) != 0 && Dispatch()->fSub != 0)
        arriving = (Dispatch()->fSub->CarObj()->carFlags & 0x40) == 0;
    if (arriving)
    {
        fSub = Dispatch()->fSub;
        Dispatch()->fSub = this;
        if (fBlockade.flags != 0)
            return;
        fTo = CallSign()->fAllUnits;
        SPCHNFS_C_A_INTRO(&fVoice, fTo, fFrom, &fReverse);
        SPCH::PlaySpeech();
        SPCHNFS_S_C_SUPER_COP_ARRIVAL(&fVoice);
        SPCH::PlaySpeech();
        return;
    }
    Speaker *SubChain = Dispatch();
    bool promote = false;
    if (Dispatch()->fSub != 0 && Dispatch()->fSub->Perp() != 0 && (Dispatch()->fSub->Perp()->carFlags & 4) == 0)
        promote = (Perp()->carFlags & 4) != 0;
    if (promote)
    {
        fSub = Dispatch()->fSub;
        Dispatch()->fSub = this;
    }
    else
    {
        while (SubChain->fSub != 0)
            SubChain = SubChain->fSub;
        SubChain->fSub = this;
    }
    if (fBlockade.flags != 0)
        return;
    if (Dispatch()->KnownPerp(perp) && Dispatch()->StatusCount() >= 0x180)
        return;
    fTo = CallSign()->fDispatch;
    SPCHNFS_C_A_INTRO(&fVoice, fTo, fFrom, &fReverse);
    SPCH::PlaySpeech();
    car = Perp();
    FindLocation(car);
    car = Perp();
    SetSpeed(car);
    Speaker *dispatcher = Dispatch();
    car = Perp();
    if (dispatcher->KnownPerp(car))
        SPCHNFS_C_D_PERP_SIGHTED(&fVoice, &fColour, fCar, &fDistance, &fPosition, fLocation, &fPerpName);
    else
        SPCHNFS_C_D_ENGAGE_PURS_REP_SPDR(&fVoice, &fColour, fCar, &fPosition, fLocation, &fDistance, fSpeed, &fSpeedType, &fPerpName);
    SPCH::PlaySpeech();
    Speaker *Sub = Dispatch()->fSub;
    Dispatch()->fSub = this;
    dispatcher = Dispatch();
    car = Perp();
    dispatcher->Report(car);
    Dispatch()->fSub = Sub;
}

void Speech::MobileSpeaker::Lose(void)
{
    FUNCTION_MARKER(0x80098434, "NFS4.EXE");
    if (Perp() == 0)
        return;
    fgSpeech->fSpeakerCar = fCarObj;
    Speaker *Leader;
    if (Dispatch()->fSub != 0 && Dispatch()->fSub->fSub == this && fBlockade.flags == 0 && fArrest.flags == 0)
    {
        Leader = Dispatch()->fSub;
        CallSignBank *bank = CallSign();
        sint32 unit = Leader->Unit();
        fTo = bank->fMobile[unit];
    }
    else
    {
        fTo = CallSign()->fDispatch;
        Leader = 0;
    }
    if (fArrest.flags != 0)
    {
        SPCHNFS_C_P_FALSE_ARREST_BULLHORN(&fVoice);
        SPCH::PlaySpeech();
    }
    else if (fBlockade.flags == 0 && Leader == 0 && Dispatch()->StatusCount() > 0x160)
        return;
    SPCHNFS_C_A_INTRO(&fVoice, fTo, fFrom, &fReverse);
    SPCH::PlaySpeech();
    SetCar(Perp());
    FindLocation(Perp());
    if (fArrest.flags != 0)
        SPCHNFS_C_D_DURING_FALSE_ARREST(&fVoice, &fPerpName);
    else if ((CarObj()->carFlags & 0x200) != 0)
        SPCHNFS_C_D_ENDGAME(&fVoice);
    else if (fBlockade.flags == 1)
        SPCHNFS_C_D_SPBLT_FAILED(&fVoice, &fColour, fCar);
    else if (fBlockade.flags == 2)
        SPCHNFS_C_D_RDBLK_FAILED(&fVoice, &fColour, fCar);
    else if (Leader != 0)
        SPCHNFS_C_C_IDLE_WINGMAN_DISAPPEARS(&fVoice);
    else
        SPCHNFS_C_D_PERP_LOST(&fVoice, &fColour, fCar, &fPosition, fLocation, &fDistance, &fPerpName);
    SPCH::PlaySpeech();
    fBlockade.flags = 0;
    fArrest.flags = 0;
    fUpdate.flags = 0;
    if (Leader == 0)
    {
        Speaker *Sub = Dispatch()->fSub;
        Dispatch()->fSub = this;
        Dispatch()->Roger();
        Dispatch()->fSub = Sub;
    }
}

void Speech::MobileSpeaker::Accident(sint32 slice)
{
    FUNCTION_MARKER(0x80098788, "NFS4.EXE");
}

void Speech::MobileSpeaker::Catch(sint32 ticket)
{
    FUNCTION_MARKER(0x80098790, "NFS4.EXE");
    if (Perp() == 0)
        return;
    fgSpeech->fSpeakerCar = fCarObj;
    if (Perp()->N.orientationToGround.y < 0)
    {
        fTo = CallSign()->fDispatch;
        SPCHNFS_C_A_INTRO(&fVoice, fTo, fFrom, &fReverse);
        SPCH::PlaySpeech();
        FindLocation(Perp());
        SPCHNFS_C_D_PERP_CRASH_ROLL(&fVoice, &fPosition, fLocation, &fDistance, &fPerpName);
        SPCH::PlaySpeech();
        fAmbulance.flags = 4;
        SPCHNFS_C_D_REQUEST_EMS(&fVoice, &fAmbulance);
        SPCH::PlaySpeech();
    }
    else if (Perp()->collision.smoking != 0)
    {
        fTo = CallSign()->fDispatch;
        SPCHNFS_C_A_INTRO(&fVoice, fTo, fFrom, &fReverse);
        SPCH::PlaySpeech();
        SetCar(Perp());
        FindLocation(Perp());
        SPCHNFS_C_D_PERP_CRASH_GEN(&fVoice, &fPosition, fLocation, &fColour, fCar, &fDistance, &fPerpName);
        SPCH::PlaySpeech();
        fAmbulance.flags = 0x20;
        SPCHNFS_C_D_REQUEST_EMS(&fVoice, &fAmbulance);
        SPCH::PlaySpeech();
    }
    else
    {
        fArrest.flags = ticket;
        if (ticket == 1)
            SPCHNFS_C_P_ARRESTED(&fVoice, &fArrest);
        else if (ticket == 2)
            SPCHNFS_C_P_WARNING(&fVoice, &fArrest);
        else
            SPCHNFS_C_P_TICKET(&fVoice, &fArrest);
        SPCH::PlaySpeech();
        if (ticket == 1)
            fgSpeech->SetDelayedStatus(this, 0x60);
    }
    Speaker *dispatcher = Dispatch();
    dispatcher->ClearPerp(fPerp);
}

void Speech::MobileSpeaker::RoadBlock(void)
{
    FUNCTION_MARKER(0x80098A4C, "NFS4.EXE");
    if (Dispatch()->fSub != 0 && Dispatch()->fSub != this)
    {
        Dispatch()->fSub->RoadBlock();
        return;
    }
    Promote();
    if (fgSpeech->fMultiplePerps == 0)
    {
        fgSpeech->fSpeakerCar = fCarObj;
        fTo = CallSign()->fDispatch;
        SPCHNFS_C_A_INTRO(&fVoice, fTo, fFrom, &fReverse);
        SPCH::PlaySpeech();
        SPCHNFS_C_D_REQ_RDBLK(&fVoice);
        SPCH::PlaySpeech();
    }
    fBlockade.flags = 2;
}

void Speech::MobileSpeaker::SpikeBelt(void)
{
    FUNCTION_MARKER(0x80098B60, "NFS4.EXE");
    if (Dispatch()->fSub != 0 && Dispatch()->fSub != this)
    {
        Dispatch()->fSub->SpikeBelt();
        return;
    }
    Promote();
    if (fgSpeech->fMultiplePerps == 0)
    {
        fgSpeech->fSpeakerCar = fCarObj;
        fTo = CallSign()->fDispatch;
        SPCHNFS_C_A_INTRO(&fVoice, fTo, fFrom, &fReverse);
        SPCH::PlaySpeech();
        SPCHNFS_C_D_REQ_SPBLT(&fVoice);
        SPCH::PlaySpeech();
    }
    fBlockade.flags = 1;
}

void Speech::MobileSpeaker::Backup(void)
{
    FUNCTION_MARKER(0x80098C74, "NFS4.EXE");
    fgSpeech->fSpeakerCar = fCarObj;
    fTo = CallSign()->fDispatch;
    SPCHNFS_C_A_INTRO(&fVoice, fTo, fFrom, &fReverse);
    SPCH::PlaySpeech();
    SetCar(Perp());
    FindLocation(CarObj());
    SPCHNFS_C_D_REQUEST_BKUP(&fVoice, &fColour, fCar, &fPosition, fLocation, &fDistance);
    SPCH::PlaySpeech();
    fBlockade.flags = 0;
}

void Speech::MobileSpeaker::Roger(void)
{
    FUNCTION_MARKER(0x80098D6C, "NFS4.EXE");
    fgSpeech->fSpeakerCar = fCarObj;
    bool responding = false;
    if (fSub != 0)
    {
        Speaker *status = Dispatch()->StatusSub();
        responding = status == fSub;
    }
    if (responding)
    {
        SPCH::PlaySpeech();
        CallSignBank *bank = CallSign();
        sint32 unit = fSub->Unit();
        fTo = bank->fMobile[unit];
        SPCHNFS_C_A_CONFIRM(&fVoice, fTo, &fConfirm);
        SPCH::PlaySpeech();
        SetCar(Perp());
        SPCHNFS_C_C_IN_PURS_NEAR_PERP(&fVoice, &fColour, fCar);
    }
    else
    {
        fTo = CallSign()->fDispatch;
        SPCHNFS_C_A_CONFIRM(&fVoice, fTo, &fConfirm);
    }
    SPCH::PlaySpeech();
}

void Speech::MobileSpeaker::Bullhorn(void)
{
    FUNCTION_MARKER(0x80098EE4, "NFS4.EXE");
    fgSpeech->fSpeakerCar = fCarObj;
    SPCHNFS_C_P_BULLHORN_SPEECH(&fVoice);
    SPCH::PlaySpeech();
}

void Speech::MobileSpeaker::Purge(void)
{
    FUNCTION_MARKER(0x80098F18, "NFS4.EXE");
    if (fCarObj == 0)
        return;
    if ((CarObj()->carFlags & 0x200) != 0)
    {
        CopSpeak::Flush();
        if (CopSpeak::gSpchHandle != -1)
        {
            if (stackSpeedUpEnbabledFlag != 0)
            {
                stackSpeedUpEnbabledFlag = 0;
                SNDstop(CopSpeak::gSpchHandle);
                stackSpeedUpEnbabledFlag = 1;
            }
            else
                SNDstop(CopSpeak::gSpchHandle);
        }
        if (Perp() != 0 && AudioMus::Threshold() != 0)
        {
            if (stackSpeedUpEnbabledFlag != 0)
            {
                stackSpeedUpEnbabledFlag = 0;
                AudioMus::StopSong(500);
                AudioMus::PlaySong(0);
                stackSpeedUpEnbabledFlag = 1;
            }
            else
            {
                AudioMus::StopSong(500);
                AudioMus::PlaySong(0);
            }
        }
        fBlockade.flags = 0;
        fArrest.flags = 0;
        fUpdate.flags = 0;
        fPerp = 0;
        fSub = 0;
    }
    else
    {
        fCarObj = 0;
        if (Dispatch()->StatusSub() == this)
            Dispatch()->PurgeStatusSub();
        Speaker *Chain = Dispatch();
        for (;;)
        {
            Speaker *next = Chain->fSub;
            if (next == this)
            {
                Chain->fSub = fSub;
                return;
            }
            if (next == 0)
                return;
            Chain = next;
        }
    }
}

void Speech::MobileSpeaker::ReportBlockade(void)
{
    FUNCTION_MARKER(0x8009916C, "NFS4.EXE");
    fgSpeech->fSpeakerCar = fCarObj;
    fTo = CallSign()->fDispatch;
    FindLocation(CarObj());
    fSpikeSide.flags = 4;
    if (fBlockade.flags == 2)
    {
        SPCHNFS_C_A_INTRO(&fVoice, fTo, fFrom, &fReverse);
        SPCH::PlaySpeech();
        SPCHNFS_W_D_RDBLK_PLC(&fVoice, &fPosition, fLocation, &fDistance);
    }
    else
        SPCHNFS_W_D_SPBLT_PLC(&fVoice, &fPosition, &fSpikeSide, fFrom, fLocation, &fDistance);
    SPCH::PlaySpeech();
}
