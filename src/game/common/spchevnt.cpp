#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/spchevnt.h"

extern "C"
{
    sint32 SPCHNFS_C_A_CONFIRM(SPCHNFSType::VOICE *VOICE, sint32 ID_UNIT, SPCHNFSType::CONFIRM *CONFIRM)
    {
        FUNCTION_MARKER(0x80092F54, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x151;
        parms[1] = (sint32)VOICE->flags;
        parms[2] = ID_UNIT;
        parms[3] = (sint32)CONFIRM->flags;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_C_D_REQUEST_EMS(SPCHNFSType::VOICE *VOICE, SPCHNFSType::AMBULANCE *AMBULANCE)
    {
        FUNCTION_MARKER(0x80092FA8, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x152;
        parms[1] = (sint32)VOICE->flags;
        parms[2] = (sint32)AMBULANCE->flags;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_D_C_BEGIN_PURS_REP_SPDR(SPCHNFSType::COLOUR *COLOUR, sint32 ID_CAR, SPCHNFSType::POSITION *POSITION, sint32 ID_LOCATION, SPCHNFSType::DISTANCE *DISTANCE)
    {
        FUNCTION_MARKER(0x80092FFC, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x154;
        parms[1] = (sint32)COLOUR->flags;
        parms[2] = ID_CAR;
        parms[3] = (sint32)POSITION->flags;
        parms[4] = ID_LOCATION;
        parms[5] = (sint32)DISTANCE->flags;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_C_D_ENGAGE_PURS_REP_SPDR_REPLY(SPCHNFSType::VOICE *VOICE, sint32 ID_UNIT, SPCHNFSType::COLOUR *COLOUR, sint32 ID_CAR, SPCHNFSType::DISTANCE *DISTANCE, SPCHNFSType::POSITION *POSITION, sint32 ID_LOCATION, SPCHNFSType::CONFIRM *CONFIRM)
    {
        FUNCTION_MARKER(0x80093064, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x155;
        parms[1] = (sint32)VOICE->flags;
        parms[2] = ID_UNIT;
        parms[3] = (sint32)COLOUR->flags;
        parms[4] = ID_CAR;
        parms[5] = (sint32)DISTANCE->flags;
        parms[6] = (sint32)POSITION->flags;
        parms[7] = ID_LOCATION;
        parms[8] = (sint32)CONFIRM->flags;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_C_D_ENGAGE_PURS_REP_SPDR(SPCHNFSType::VOICE *VOICE, SPCHNFSType::COLOUR *COLOUR, sint32 ID_CAR, SPCHNFSType::POSITION *POSITION, sint32 ID_LOCATION, SPCHNFSType::DISTANCE *DISTANCE, sint32 ID_SPEED, SPCHNFSType::vs_KMH_MPH *vs_KMH_MPH, SPCHNFSType::PERP_NAME *PERP_NAME)
    {
        FUNCTION_MARKER(0x800930F8, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x156;
        parms[1] = (sint32)VOICE->flags;
        parms[2] = (sint32)COLOUR->flags;
        parms[3] = ID_CAR;
        parms[4] = (sint32)POSITION->flags;
        parms[5] = ID_LOCATION;
        parms[6] = (sint32)DISTANCE->flags;
        parms[7] = ID_SPEED;
        parms[8] = (sint32)vs_KMH_MPH->flags;
        parms[9] = (sint32)PERP_NAME->flags;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_C_C_IN_PURS_NEAR_PERP(SPCHNFSType::VOICE *VOICE, SPCHNFSType::COLOUR *COLOUR, sint32 ID_CAR)
    {
        FUNCTION_MARKER(0x80093198, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x158;
        parms[1] = (sint32)VOICE->flags;
        parms[2] = (sint32)COLOUR->flags;
        parms[3] = ID_CAR;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_D_C_IN_PURS_NEAR_PERP(SPCHNFSType::PURS_UPDT *PURS_UPDT)
    {
        FUNCTION_MARKER(0x800931F0, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x159;
        parms[1] = (sint32)PURS_UPDT->flags;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_C_D_IN_PURS_NEAR_PERP_REP_STS(SPCHNFSType::VOICE *VOICE, SPCHNFSType::COLOUR *COLOUR, sint32 ID_CAR, SPCHNFSType::DISTANCE *DISTANCE, SPCHNFSType::POSITION *POSITION, sint32 ID_LOCATION)
    {
        FUNCTION_MARKER(0x80093238, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x15A;
        parms[1] = (sint32)VOICE->flags;
        parms[2] = (sint32)COLOUR->flags;
        parms[3] = ID_CAR;
        parms[4] = (sint32)DISTANCE->flags;
        parms[5] = (sint32)POSITION->flags;
        parms[6] = ID_LOCATION;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_D_C_IN_PURS_NEAR_PERP_CONFIRM(SPCHNFSType::COLOUR *COLOUR, sint32 ID_CAR, sint32 ID_UNIT, SPCHNFSType::CONFIRM *CONFIRM, SPCHNFSType::PERP_NAME *PERP_NAME)
    {
        FUNCTION_MARKER(0x800932B4, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x15B;
        parms[1] = (sint32)COLOUR->flags;
        parms[2] = ID_CAR;
        parms[3] = ID_UNIT;
        parms[4] = (sint32)CONFIRM->flags;
        parms[5] = (sint32)PERP_NAME->flags;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_C_D_IN_PURS_AWAY_PERP_REPLY_STS(SPCHNFSType::VOICE *VOICE, SPCHNFSType::COLOUR *COLOUR, sint32 ID_CAR, SPCHNFSType::POSITION *POSITION, sint32 ID_LOCATION, SPCHNFSType::DISTANCE *DISTANCE)
    {
        FUNCTION_MARKER(0x80093320, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x15E;
        parms[1] = (sint32)VOICE->flags;
        parms[2] = (sint32)COLOUR->flags;
        parms[3] = ID_CAR;
        parms[4] = (sint32)POSITION->flags;
        parms[5] = ID_LOCATION;
        parms[6] = (sint32)DISTANCE->flags;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_D_C_IN_PURS_AWAY_PERP_CONFIRM(SPCHNFSType::COLOUR *COLOUR, sint32 ID_CAR, sint32 ID_UNIT, SPCHNFSType::CONFIRM *CONFIRM, SPCHNFSType::PERP_NAME *PERP_NAME)
    {
        FUNCTION_MARKER(0x80093394, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x15F;
        parms[1] = (sint32)COLOUR->flags;
        parms[2] = ID_CAR;
        parms[3] = ID_UNIT;
        parms[4] = (sint32)CONFIRM->flags;
        parms[5] = (sint32)PERP_NAME->flags;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_C_D_IN_PURS_LOOK_PERP_REPLY_STS(SPCHNFSType::VOICE *VOICE, SPCHNFSType::COLOUR *COLOUR, sint32 ID_CAR, SPCHNFSType::PERP_NAME *PERP_NAME)
    {
        FUNCTION_MARKER(0x80093400, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x162;
        parms[1] = (sint32)VOICE->flags;
        parms[2] = (sint32)COLOUR->flags;
        parms[3] = ID_CAR;
        parms[4] = (sint32)PERP_NAME->flags;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_C_C_IDLE_WINGMAN_DISAPPEARS(SPCHNFSType::VOICE *VOICE)
    {
        FUNCTION_MARKER(0x80093460, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x165;
        parms[1] = (sint32)VOICE->flags;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_C_D_REQ_RDBLK(SPCHNFSType::VOICE *VOICE)
    {
        FUNCTION_MARKER(0x800934A8, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x167;
        parms[1] = (sint32)VOICE->flags;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_D_C_RDBLK_SPBLT_DENIED_REPLY(SPCHNFSType::vs_RDBLK_SSTRP *vs_RDBLK_SSTRP)
    {
        FUNCTION_MARKER(0x800934F0, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x168;
        parms[1] = (sint32)vs_RDBLK_SSTRP->flags;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_D_C_RDBLK_SPBLT_GRANT_REPLY(SPCHNFSType::vs_RDBLK_SSTRP *vs_RDBLK_SSTRP, SPCHNFSType::CONFIRM *CONFIRM)
    {
        FUNCTION_MARKER(0x80093538, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x169;
        parms[1] = (sint32)vs_RDBLK_SSTRP->flags;
        parms[2] = (sint32)CONFIRM->flags;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_D_C_RDBLK_CONFIRMED(SPCHNFSType::POSITION *POSITION, sint32 ID_LOCATION, SPCHNFSType::DISTANCE *DISTANCE)
    {
        FUNCTION_MARKER(0x8009358C, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x16A;
        parms[1] = (sint32)POSITION->flags;
        parms[2] = ID_LOCATION;
        parms[3] = (sint32)DISTANCE->flags;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_C_D_RDBLK_FAILED(SPCHNFSType::VOICE *VOICE, SPCHNFSType::COLOUR *COLOUR, sint32 ID_CAR)
    {
        FUNCTION_MARKER(0x800935E0, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x16D;
        parms[1] = (sint32)VOICE->flags;
        parms[2] = (sint32)COLOUR->flags;
        parms[3] = ID_CAR;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_C_D_REQUEST_BKUP(SPCHNFSType::VOICE *VOICE, SPCHNFSType::COLOUR *COLOUR, sint32 ID_CAR, SPCHNFSType::POSITION *POSITION, sint32 ID_LOCATION, SPCHNFSType::DISTANCE *DISTANCE)
    {
        FUNCTION_MARKER(0x80093638, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x16F;
        parms[1] = (sint32)VOICE->flags;
        parms[2] = (sint32)COLOUR->flags;
        parms[3] = ID_CAR;
        parms[4] = (sint32)POSITION->flags;
        parms[5] = ID_LOCATION;
        parms[6] = (sint32)DISTANCE->flags;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_D_C_BKUP_REQUEST_GRANT_REPLY(SPCHNFSType::DISTANCE *DISTANCE, SPCHNFSType::POSITION *POSITION, sint32 ID_LOCATION, sint32 ID_UNIT)
    {
        FUNCTION_MARKER(0x800936AC, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x172;
        parms[1] = (sint32)DISTANCE->flags;
        parms[2] = (sint32)POSITION->flags;
        parms[3] = ID_LOCATION;
        parms[4] = ID_UNIT;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_D_C_BKUP_REQUEST_DENIED_REPLY(void)
    {
        FUNCTION_MARKER(0x80093708, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x174;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_C_C_NEW_OFFICER_ENGAGING(SPCHNFSType::VOICE *VOICE, sint32 ID_UNIT)
    {
        FUNCTION_MARKER(0x80093748, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x177;
        parms[1] = (sint32)VOICE->flags;
        parms[2] = ID_UNIT;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_C_D_PERP_SIGHTED(SPCHNFSType::VOICE *VOICE, SPCHNFSType::COLOUR *COLOUR, sint32 ID_CAR, SPCHNFSType::DISTANCE *DISTANCE, SPCHNFSType::POSITION *POSITION, sint32 ID_LOCATION, SPCHNFSType::PERP_NAME *PERP_NAME)
    {
        FUNCTION_MARKER(0x80093794, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x178;
        parms[1] = (sint32)VOICE->flags;
        parms[2] = (sint32)COLOUR->flags;
        parms[3] = ID_CAR;
        parms[4] = (sint32)DISTANCE->flags;
        parms[5] = (sint32)POSITION->flags;
        parms[6] = ID_LOCATION;
        parms[7] = (sint32)PERP_NAME->flags;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_D_C_PERP_SIGHTED_CONFIRM(SPCHNFSType::CONFIRM *CONFIRM, sint32 ID_UNIT)
    {
        FUNCTION_MARKER(0x80093820, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x179;
        parms[1] = (sint32)CONFIRM->flags;
        parms[2] = ID_UNIT;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_C_D_PERP_LOST(SPCHNFSType::VOICE *VOICE, SPCHNFSType::COLOUR *COLOUR, sint32 ID_CAR, SPCHNFSType::POSITION *POSITION, sint32 ID_LOCATION, SPCHNFSType::DISTANCE *DISTANCE, SPCHNFSType::PERP_NAME *PERP_NAME)
    {
        FUNCTION_MARKER(0x8009386C, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x17A;
        parms[1] = (sint32)VOICE->flags;
        parms[2] = (sint32)COLOUR->flags;
        parms[3] = ID_CAR;
        parms[4] = (sint32)POSITION->flags;
        parms[5] = ID_LOCATION;
        parms[6] = (sint32)DISTANCE->flags;
        parms[7] = (sint32)PERP_NAME->flags;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_D_C_PERP_LOST_CONFIRM(SPCHNFSType::COLOUR *COLOUR, sint32 ID_CAR)
    {
        FUNCTION_MARKER(0x800938F4, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x17B;
        parms[1] = (sint32)COLOUR->flags;
        parms[2] = ID_CAR;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_C_C_PERP_REAQUIRED(SPCHNFSType::VOICE *VOICE, SPCHNFSType::COLOUR *COLOUR, sint32 ID_CAR, SPCHNFSType::POSITION *POSITION, sint32 ID_LOCATION, SPCHNFSType::DISTANCE *DISTANCE)
    {
        FUNCTION_MARKER(0x80093940, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x17C;
        parms[1] = (sint32)VOICE->flags;
        parms[2] = (sint32)COLOUR->flags;
        parms[3] = ID_CAR;
        parms[4] = (sint32)POSITION->flags;
        parms[5] = ID_LOCATION;
        parms[6] = (sint32)DISTANCE->flags;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_C_D_PERP_APPREHENSION(SPCHNFSType::VOICE *VOICE, SPCHNFSType::PERP_NAME *PERP_NAME)
    {
        FUNCTION_MARKER(0x800939B4, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x17F;
        parms[1] = (sint32)VOICE->flags;
        parms[2] = (sint32)PERP_NAME->flags;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_D_C_PERP_APPREHENSION_REPLY(sint32 ID_UNIT, SPCHNFSType::CONFIRM *CONFIRM, SPCHNFSType::PERP_NAME *PERP_NAME)
    {
        FUNCTION_MARKER(0x80093A08, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x180;
        parms[1] = ID_UNIT;
        parms[2] = (sint32)CONFIRM->flags;
        parms[3] = (sint32)PERP_NAME->flags;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_C_D_PERP_CRASH_GEN(SPCHNFSType::VOICE *VOICE, SPCHNFSType::POSITION *POSITION, sint32 ID_LOCATION, SPCHNFSType::COLOUR *COLOUR, sint32 ID_CAR, SPCHNFSType::DISTANCE *DISTANCE, SPCHNFSType::PERP_NAME *PERP_NAME)
    {
        FUNCTION_MARKER(0x80093A60, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x181;
        parms[1] = (sint32)VOICE->flags;
        parms[2] = (sint32)POSITION->flags;
        parms[3] = ID_LOCATION;
        parms[4] = (sint32)COLOUR->flags;
        parms[5] = ID_CAR;
        parms[6] = (sint32)DISTANCE->flags;
        parms[7] = (sint32)PERP_NAME->flags;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_C_P_ARRESTED(SPCHNFSType::VOICE *VOICE, SPCHNFSType::ARREST *ARREST)
    {
        FUNCTION_MARKER(0x80093AE8, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x187;
        parms[1] = (sint32)VOICE->flags;
        parms[2] = (sint32)ARREST->flags;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_C_P_WARNING(SPCHNFSType::VOICE *VOICE, SPCHNFSType::ARREST *ARREST)
    {
        FUNCTION_MARKER(0x80093B3C, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x188;
        parms[1] = (sint32)VOICE->flags;
        parms[2] = (sint32)ARREST->flags;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_C_P_TICKET(SPCHNFSType::VOICE *VOICE, SPCHNFSType::ARREST *ARREST)
    {
        FUNCTION_MARKER(0x80093B90, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x189;
        parms[1] = (sint32)VOICE->flags;
        parms[2] = (sint32)ARREST->flags;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_C_P_FALSE_ARREST_BULLHORN(SPCHNFSType::VOICE *VOICE)
    {
        FUNCTION_MARKER(0x80093BE4, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x18A;
        parms[1] = (sint32)VOICE->flags;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_C_D_DURING_FALSE_ARREST(SPCHNFSType::VOICE *VOICE, SPCHNFSType::PERP_NAME *PERP_NAME)
    {
        FUNCTION_MARKER(0x80093C2C, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x18B;
        parms[1] = (sint32)VOICE->flags;
        parms[2] = (sint32)PERP_NAME->flags;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_C_P_BULLHORN_SPEECH(SPCHNFSType::VOICE *VOICE)
    {
        FUNCTION_MARKER(0x80093C80, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x18D;
        parms[1] = (sint32)VOICE->flags;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_S_C_SUPER_COP_ARRIVAL(SPCHNFSType::VOICE *VOICE)
    {
        FUNCTION_MARKER(0x80093CC8, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x18E;
        parms[1] = (sint32)VOICE->flags;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_S_C_SUPER_COP_CRITICISM(SPCHNFSType::VOICE *VOICE)
    {
        FUNCTION_MARKER(0x80093D10, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x18F;
        parms[1] = (sint32)VOICE->flags;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_C_D_IN_PURS_NEAR_PERP_REP_LOC(SPCHNFSType::VOICE *VOICE, SPCHNFSType::POSITION *POSITION, sint32 ID_LOCATION, SPCHNFSType::DISTANCE *DISTANCE)
    {
        FUNCTION_MARKER(0x80093D58, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x19E;
        parms[1] = (sint32)VOICE->flags;
        parms[2] = (sint32)POSITION->flags;
        parms[3] = ID_LOCATION;
        parms[4] = (sint32)DISTANCE->flags;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_C_D_IN_PURS_NEAR_PERP_REP_SPD(SPCHNFSType::VOICE *VOICE, sint32 ID_SPEED, SPCHNFSType::vs_KMH_MPH *vs_KMH_MPH)
    {
        FUNCTION_MARKER(0x80093DB8, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x19F;
        parms[1] = (sint32)VOICE->flags;
        parms[2] = ID_SPEED;
        parms[3] = (sint32)vs_KMH_MPH->flags;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_C_D_IN_PURS_AWAY_PERP_REPLY_LOC(SPCHNFSType::VOICE *VOICE, SPCHNFSType::POSITION *POSITION, sint32 ID_LOCATION, SPCHNFSType::DISTANCE *DISTANCE, SPCHNFSType::COLOUR *COLOUR, sint32 ID_CAR, SPCHNFSType::PERP_NAME *PERP_NAME)
    {
        FUNCTION_MARKER(0x80093E0C, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x1A0;
        parms[1] = (sint32)VOICE->flags;
        parms[2] = (sint32)POSITION->flags;
        parms[3] = ID_LOCATION;
        parms[4] = (sint32)DISTANCE->flags;
        parms[5] = (sint32)COLOUR->flags;
        parms[6] = ID_CAR;
        parms[7] = (sint32)PERP_NAME->flags;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_C_D_IN_PURS_LOOK_PERP_REPLY_LOC(SPCHNFSType::VOICE *VOICE, SPCHNFSType::POSITION *POSITION, sint32 ID_LOCATION, SPCHNFSType::DISTANCE *DISTANCE, SPCHNFSType::COLOUR *COLOUR, sint32 ID_CAR)
    {
        FUNCTION_MARKER(0x80093E98, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x1A2;
        parms[1] = (sint32)VOICE->flags;
        parms[2] = (sint32)POSITION->flags;
        parms[3] = ID_LOCATION;
        parms[4] = (sint32)DISTANCE->flags;
        parms[5] = (sint32)COLOUR->flags;
        parms[6] = ID_CAR;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_D_C_INTRO_CALL(sint32 ID_UNIT, sint32 ID_UNIT1, SPCHNFSType::REVINTRO *REVINTRO)
    {
        FUNCTION_MARKER(0x80093F14, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x1A3;
        parms[1] = ID_UNIT;
        parms[2] = ID_UNIT1;
        parms[3] = (sint32)REVINTRO->flags;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_C_A_INTRO(SPCHNFSType::VOICE *VOICE, sint32 ID_UNIT, sint32 ID_UNIT1, SPCHNFSType::REVINTRO *REVINTRO)
    {
        FUNCTION_MARKER(0x80093F64, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x1A5;
        parms[1] = (sint32)VOICE->flags;
        parms[2] = ID_UNIT;
        parms[3] = ID_UNIT1;
        parms[4] = (sint32)REVINTRO->flags;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_C_D_IN_PURS_PERP_AIRBORN(SPCHNFSType::VOICE *VOICE)
    {
        FUNCTION_MARKER(0x80093FBC, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x1A6;
        parms[1] = (sint32)VOICE->flags;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_D_C_SPBLT_CONFIRMED(SPCHNFSType::POSITION *POSITION, sint32 ID_LOCATION, SPCHNFSType::DISTANCE *DISTANCE, sint32 ID_UNIT, SPCHNFSType::SPIKE_BELT_SIDE *SPIKE_BELT_SIDE)
    {
        FUNCTION_MARKER(0x80094004, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x1A8;
        parms[1] = (sint32)POSITION->flags;
        parms[2] = ID_LOCATION;
        parms[3] = (sint32)DISTANCE->flags;
        parms[4] = ID_UNIT;
        parms[5] = (sint32)SPIKE_BELT_SIDE->flags;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_C_D_REQ_SPBLT(SPCHNFSType::VOICE *VOICE)
    {
        FUNCTION_MARKER(0x8009406C, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x1A9;
        parms[1] = (sint32)VOICE->flags;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_C_D_SPBLT_FAILED(SPCHNFSType::VOICE *VOICE, SPCHNFSType::COLOUR *COLOUR, sint32 ID_CAR)
    {
        FUNCTION_MARKER(0x800940B4, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x1AA;
        parms[1] = (sint32)VOICE->flags;
        parms[2] = (sint32)COLOUR->flags;
        parms[3] = ID_CAR;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_W_D_RDBLK_PLC(SPCHNFSType::VOICE *VOICE, SPCHNFSType::POSITION *POSITION, sint32 ID_LOCATION, SPCHNFSType::DISTANCE *DISTANCE)
    {
        FUNCTION_MARKER(0x8009410C, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x1AB;
        parms[1] = (sint32)VOICE->flags;
        parms[2] = (sint32)POSITION->flags;
        parms[3] = ID_LOCATION;
        parms[4] = (sint32)DISTANCE->flags;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_W_D_SPBLT_PLC(SPCHNFSType::VOICE *VOICE, SPCHNFSType::POSITION *POSITION, SPCHNFSType::SPIKE_BELT_SIDE *SPIKE_BELT_SIDE, sint32 ID_UNIT, sint32 ID_LOCATION, SPCHNFSType::DISTANCE *DISTANCE)
    {
        FUNCTION_MARKER(0x8009416C, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x1AC;
        parms[1] = (sint32)VOICE->flags;
        parms[2] = (sint32)POSITION->flags;
        parms[3] = (sint32)SPIKE_BELT_SIDE->flags;
        parms[4] = ID_UNIT;
        parms[5] = ID_LOCATION;
        parms[6] = (sint32)DISTANCE->flags;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_C_D_PERP_CRASH_ROLL(SPCHNFSType::VOICE *VOICE, SPCHNFSType::POSITION *POSITION, sint32 ID_LOCATION, SPCHNFSType::DISTANCE *DISTANCE, SPCHNFSType::PERP_NAME *PERP_NAME)
    {
        FUNCTION_MARKER(0x800941E4, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x1AE;
        parms[1] = (sint32)VOICE->flags;
        parms[2] = (sint32)POSITION->flags;
        parms[3] = ID_LOCATION;
        parms[4] = (sint32)DISTANCE->flags;
        parms[5] = (sint32)PERP_NAME->flags;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_D_A_CONFIRM(SPCHNFSType::CONFIRM *CONFIRM)
    {
        FUNCTION_MARKER(0x80094258, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x1B1;
        parms[1] = (sint32)CONFIRM->flags;
        return SPCH::AddEvent(parms);
    }

    sint32 SPCHNFS_C_D_ENDGAME(SPCHNFSType::VOICE *VOICE)
    {
        FUNCTION_MARKER(0x800942A0, "NFS4.EXE");
        sint32 parms[12] = { 0 };
        parms[0] = 0x1B3;
        parms[1] = (sint32)VOICE->flags;
        return SPCH::AddEvent(parms);
    }
}
