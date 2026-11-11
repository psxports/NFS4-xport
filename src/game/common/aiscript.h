#ifndef AISCRIPT_H
#define AISCRIPT_H

#include "xport.h"

class AIScript
{
public:
    enum tPlayAction
    {
        AISCRIPT_PLAYACTION_COLLISION = 0,
        AISCRIPT_PLAYACTION_OPP_WIZZED_BY = 1,
        AISCRIPT_PLAYACTION_HUMAN_BLOCK_OPP = 2,
        AISCRIPT_PLAYACTION_OPP_BLOCK_HUMAN = 3,
        AISCRIPT_PLAYACTION_OPP_PASS_HUMAN = 4,
        AISCRIPT_PLAYACTION_HUMAN_PASS_OPP = 5,
        AISCRIPT_PLAYACTION_HUMAN_HONK = 6,
        AISCRIPT_PLAYACTION_SENTINAL = 7
    };

    enum tAIReaction
    {
        AISCRIPT_AIREACTION_INIT = 1,
        AISCRIPT_AIREACTION_END = 2,
        AISCRIPT_AIREACTION_SPEED_BURST = 4,
        AISCRIPT_AIREACTION_HONK_HORN = 8,
        AISCRIPT_AIREACTION_HARD_BRAKE = 16,
        AISCRIPT_AIREACTION_SWIPE = 32,
        AISCRIPT_AIREACTION_ATTACK = 64,
        AISCRIPT_AIREACTION_PAUSE = 128,
        AISCRIPT_AIREACTION_REAR_END = 256,
        AISCRIPT_AIREACTION_SWERVE = 512,
        AISCRIPT_AIREACTION_MOVE_AWAY = 1024,
        AISCRIPT_AIREACTION_WIPE_OUT = 2048,
        AISCRIPT_AIREACTION_CRAZY_HORN = 4096,
        AISCRIPT_AIREACTION_TEST1 = 8192,
        AISCRIPT_AIREACTION_TEST2 = 16384,
        AISCRIPT_AIREACTION_TEST3 = 32768,
        AISCRIPT_AIREACTION_TEST4 = 65536
    };

    struct tReactionDetails
    {
        char reaction[4];
        char halfSeconds[4];
    };

    struct AIScript_t
    {
        tPlayAction detectAction;
        sint32 detectHumCarIndex;
        tPlayAction actionIndex;
        sint32 actionHumCarIndex;
        sint32 reactionIndex;
        tAIReaction reaction;
        sint32 reactionTicksLeft;
        tReactionDetails (*data)[7];
        sint32 lastReactionIndex[7];
        sint32 lastActionTime;
    };

    typedef tReactionDetails tScriptData[7];

    static void Assign(AIScript_t *script, tReactionDetails (*data)[7]);
    static void ClearLastReactionIndex(AIScript_t *script);
    static void Startup(AIScript_t *script);
    static void Cleanup(void);
    static void SubmitPlayerAction(AIScript_t *script, sint32 humCarIndex, tPlayAction playerAction, sint32 currentTime64);
    static void ProcessActionsAndReactions(AIScript_t *script, sint32 elapsedTicks);
    static sint32 DoReAction(AIScript_t *script, tAIReaction testReaction);
    static sint32 GetReactionTicksLeft(AIScript_t *script);
};

#endif
