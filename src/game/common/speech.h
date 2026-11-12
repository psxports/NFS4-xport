#ifndef SPEECH_H
#define SPEECH_H

#include <stddef.h>
#include "xport.h"
#include "eaclib/psx/nfile.h"
#include "game/common/car.h"

class SPCHNFSType
{
public:
    struct POSITION
    {
        uint32 flags;
    };

    struct DISTANCE
    {
        uint32 flags;
    };

    struct COLOUR
    {
        uint32 flags;
    };

    struct ACCIDENT
    {
        uint32 flags;
    };

    struct AMBULANCE
    {
        uint32 flags;
    };

    struct vs_RDBLK_SSTRP
    {
        uint32 flags;
    };

    struct REVINTRO
    {
        uint32 flags;
    };

    struct CONFIRM
    {
        uint32 flags;
    };

    struct PERP_NAME
    {
        uint32 flags;
    };

    struct SPIKE_BELT_SIDE
    {
        uint32 flags;
    };

    struct PURS_UPDT
    {
        uint32 flags;
    };

    struct ARREST
    {
        uint32 flags;
    };

    struct VOICE
    {
        uint32 flags;
    };

    struct vs_KMH_MPH
    {
        uint32 flags;
    };

};

class Speech
{
public:
    static void *operator new(size_t size) throw();
    static void operator delete(void *memory) throw();

    static Speech *fgSpeech;

    struct tMobileVoiceAttr
    {
        sint32 voice;
        sint32 pitch;
    };

    struct tLocationDescription
    {
        char *name;
        sint16 start;
        sint16 end;
    };

    struct tCarDescription
    {
        char *game;
        char *full;
        char *make;
        char *model;
    };

    struct tCallSignDescription
    {
        char *AllUnits;
        char *Dispatch[5];
        char *Mobile[15];
    };

    static tMobileVoiceAttr gCopAttr[9];
    static tLocationDescription gLocationDescription00[13];
    static tLocationDescription gLocationDescription02[14];
    static tLocationDescription gLocationDescription03[13];
    static tLocationDescription gLocationDescription04[9];
    static tLocationDescription gLocationDescription05[14];
    static tLocationDescription gLocationDescription06[11];
    static tLocationDescription gLocationDescription07[11];
    static tLocationDescription *gLocationDescription[11];
    static tCarDescription gCarDescription[15];
    static tCallSignDescription gCallSignDescription[1];

    struct CarBankName
    {
        char *fFull;
        char *fMake;
        char *fModel;
        void SetCar(sint32 carIndex);
    };

    struct CarBank
    {
        sint32 fFull;
        sint32 fMake;
        sint32 fModel;
        bool Check(char *name, sint32 id, CarBankName *bankname);
    };

    struct LocationBank
    {
        sint32 fStartSlice;
        sint32 fEndSlice;
        sint32 fBankId;
        char *fName;
        sint32 Distance(sint32 slice);
    };

    struct CallSignBank
    {
        sint32 fAllUnits;
        sint32 fDispatch;
        sint32 fMobile[15];
    };

    struct Speaker
    {
        static void *operator new(size_t size) throw();
        static void operator delete(void *memory) throw();
        SPCHNFSType::POSITION fPosition;
        SPCHNFSType::DISTANCE fDistance;
        SPCHNFSType::COLOUR fColour;
        SPCHNFSType::ACCIDENT fAccident;
        SPCHNFSType::AMBULANCE fAmbulance;
        SPCHNFSType::vs_RDBLK_SSTRP fBlockade;
        SPCHNFSType::REVINTRO fReverse;
        SPCHNFSType::CONFIRM fConfirm;
        SPCHNFSType::PERP_NAME fPerpName;
        SPCHNFSType::SPIKE_BELT_SIDE fSpikeSide;
        SPCHNFSType::PURS_UPDT fUpdate;
        SPCHNFSType::ARREST fArrest;
        sint32 fCar;
        sint32 fLocation;
        sint32 fFrom;
        sint32 fTo;
        sint32 fWing;
        bool fHavePerp;
        Speaker *fSub;
        virtual void Report(Car::tObj *cop);

        virtual void Status(void)
        {
            FUNCTION_MARKER(0x800993D4, "NFS4.EXE");
            return;
        }

        virtual void Deny(void);
        virtual void Grant(void);
        virtual void Ready(Car::tObj *wing);
        virtual void Engage(Car::tObj *perp);
        virtual void Lose(void);
        virtual void Accident(sint32 slice);
        virtual void Catch(sint32 ticket);
        virtual void RoadBlock(void);
        virtual void SpikeBelt(void);
        virtual void Backup(void);
        virtual void ReportBlockade(void);
        virtual void Roger(void);
        virtual void Bullhorn(void);
        virtual void Purge(void);

        virtual sint32 Unit(void)
        {
            FUNCTION_MARKER(0x800993CC, "NFS4.EXE");
            return 0;
        }

        virtual bool KnownPerp(Car::tObj *car)
        {
            FUNCTION_MARKER(0x800993C4, "NFS4.EXE");
            (void)car;
            return false;
        }

        virtual void ClearPerp(Car::tObj *car)
        {
            FUNCTION_MARKER(0x800993BC, "NFS4.EXE");
            (void)car;
            return;
        }

        virtual bool IsSuper(void)
        {
            FUNCTION_MARKER(0x800993B4, "NFS4.EXE");
            return false;
        }

        virtual sint32 StatusCount(void)
        {
            FUNCTION_MARKER(0x800993AC, "NFS4.EXE");
            return 0;
        }

        virtual Speaker *StatusSub(void)
        {
            FUNCTION_MARKER(0x800993A4, "NFS4.EXE");
            return 0;
        }

        virtual void PurgeStatusSub(void)
        {
            FUNCTION_MARKER(0x8009939C, "NFS4.EXE");
            return;
        }

        virtual sint32 DistToPerp(void)
        {
            FUNCTION_MARKER(0x80099394, "NFS4.EXE");
            return 0x03e80000;
        }

        virtual Car::tObj *CarObj(void)
        {
            FUNCTION_MARKER(0x8009938C, "NFS4.EXE");
            return 0;
        }

        virtual void ReActivate(void)
        {
            FUNCTION_MARKER(0x80099384, "NFS4.EXE");
            return;
        }

        virtual Car::tObj *Perp(void)
        {
            FUNCTION_MARKER(0x8009937C, "NFS4.EXE");
            return 0;
        }

        virtual CarBank *GetCarBank(sint32 carIndex)
        {
            FUNCTION_MARKER(0x80099374, "NFS4.EXE");
            (void)carIndex;
            return 0;
        }

        virtual LocationBank *FindClosestLocationTo(sint32 slice)
        {
            FUNCTION_MARKER(0x8009936C, "NFS4.EXE");
            (void)slice;
            return 0;
        }

        virtual CallSignBank *CallSign(void)
        {
            FUNCTION_MARKER(0x80099364, "NFS4.EXE");
            return 0;
        }

        void SetCar(Car::tObj *car);
        void FindLocation(Car::tObj *car);
        void Promote(void);
        sint32 CalcMph(Car::tObj *perp);
    };

    struct MobileSpeaker : public Speaker
    {
        SPCHNFSType::VOICE fVoice;
        SPCHNFSType::vs_KMH_MPH fSpeedType;
        sint32 fSpeed;
        sint32 fUnit;
        Car::tObj *fCarObj;
        Car::tObj *fPerp;
        virtual void Report(Car::tObj *perp);
        virtual void Status(void);
        virtual void Engage(Car::tObj *perp);
        virtual void Lose(void);
        virtual void Accident(sint32 slice);
        virtual void Catch(sint32 ticket);
        virtual void RoadBlock(void);
        virtual void SpikeBelt(void);
        virtual void Backup(void);
        virtual void ReportBlockade(void);
        virtual void Roger(void);
        virtual void Bullhorn(void);
        virtual void Purge(void);
        virtual sint32 Unit(void);
        virtual bool IsSuper(void);
        virtual sint32 DistToPerp(void);
        virtual Car::tObj *CarObj(void);
        virtual void ReActivate(void);
        virtual Car::tObj *Perp(void);
        virtual CarBank *GetCarBank(sint32 carIndex);
        virtual LocationBank *FindClosestLocationTo(sint32 slice);
        virtual CallSignBank *CallSign(void);
        void Activate(Car::tObj *carObj);
        void SetSpeed(Car::tObj *perp);
    };

    struct DispatchSpeaker : public Speaker
    {
        sint32 fStatusCount;
        Speaker *fStatusSub;
        sint32 fUpdateCount;
        Car::tObj *fPerp[2];
        virtual void Report(Car::tObj *perp);
        virtual void Status(void);
        virtual void Deny(void);
        virtual void Grant(void);
        virtual void Ready(Car::tObj *carObj);
        virtual void Accident(sint32 slice);
        virtual void Roger(void);
        virtual bool KnownPerp(Car::tObj *car);
        virtual void ClearPerp(Car::tObj *car);
        virtual sint32 StatusCount(void);
        virtual Speaker *StatusSub(void);
        virtual void PurgeStatusSub(void);
        virtual CarBank *GetCarBank(sint32 carIndex);
        virtual LocationBank *FindClosestLocationTo(sint32 slice);
        virtual CallSignBank *CallSign(void);
        void Activate(sint32 seedupdatecount);
        void StatusReply(void);
        void AddPerp(Car::tObj *car);
    };

    struct
    {
        CarBank Mobile[9];
        CarBank Dispatch[9];
    } fCarBank;

    struct
    {
        LocationBank Mobile[16];
        LocationBank Dispatch[16];
    } fLocationBank;

    struct
    {
        CallSignBank Mobile;
        CallSignBank Dispatch;
    } fCallSignBank;

    sint32 fLocationCount;
    bool fFileOpen;
    NativeFileSlot *fFileHandle;
    sint32 *fBankOffset;
    sint32 fBankCount;
    sint32 fBlpClpBank;
    sint32 fStaticBank;
    sint32 fCarCount;
    sint32 fCopCount;
    sint32 fSuperCount;
    sint32 fMultiplePerps;
    Car::tObj *fSpeakerCar;
    MobileSpeaker *fMobile[4];
    DispatchSpeaker *fDispatch;
    static Speaker *fgUndefined;
    Speech(void);

    static char *AllocateRAM(sint32 numBytes, char *message);
    static void PurgeRAM(char *memPtr);
    static sint32 HandleRequest(sint32 bank, sint32 localoffset, sint32 size, sint32 event);
    static void Server(void);
    static void Reset(void);
    ~Speech(void);
    sint32 BankPatch(sint32 bank, Car::tObj *car);
    static sint32 SubmitRequest(sint32 bank, sint32 localoffset, sint32 size);
    static Speaker *Dispatch(void);
    sint32 PickVoice(Car::tObj *carObj);
    Speaker *FindMobile(Car::tObj *carObj);
    static Speaker *Mobile(Car::tObj *carObj);
    void SetDelayedStatus(Speaker *sub, sint32 delay);
    bool CheckCarBank(CarBank *carbank, char *name, sint32 id, CarBankName *bankname);
    void CountLocations(void);
    bool CheckLocationBank(LocationBank *locationbank, char *name, sint32 id);
    LocationBank *FindClosestLocationTo(LocationBank *bank, sint32 slice);
    bool CheckCallSignBank(CallSignBank *bank, char *name, sint32 id);
    bool CheckMultiBank(char *name, sint32 id, CarBankName *bn);
    sint32 CalculateBankSize(char *header, CarBankName *bn, sint32 *hoffset, sint32 *hsize);
    void LoadBankHeaders(char *header, CarBankName *bn, sint32 hoffset, sint32 hsize);
};

#endif
