#include "xport.h"
#include "game/common/aiscript.h"

void AIScript::Assign(AIScript::AIScript_t *script, AIScript::tReactionDetails (*data)[7])
{
    FUNCTION_MARKER(0x8006F6F8, "NFS4.EXE");

    script->data = data;
}

void AIScript::ClearLastReactionIndex(AIScript::AIScript_t *script)
{
    FUNCTION_MARKER(0x8006F700, "NFS4.EXE");

    for (sint32 initLoop = 6; initLoop >= 0; --initLoop)
        script->lastReactionIndex[initLoop] = -1;
}

void AIScript::Startup(AIScript::AIScript_t *script)
{
    FUNCTION_MARKER(0x8006F724, "NFS4.EXE");

    script->actionIndex = AISCRIPT_PLAYACTION_SENTINAL;
    script->detectAction = AISCRIPT_PLAYACTION_SENTINAL;
    script->reaction = AISCRIPT_AIREACTION_INIT;
    ClearLastReactionIndex(script);
    script->lastActionTime = 0;
}

void AIScript::Cleanup(void)
{
    FUNCTION_MARKER(0x8006F760, "NFS4.EXE");
}

void AIScript::SubmitPlayerAction(AIScript::AIScript_t *script, sint32 humCarIndex, AIScript::tPlayAction playerAction, sint32 currentTime64)
{
    FUNCTION_MARKER(0x8006F768, "NFS4.EXE");

    if (script->lastActionTime < (sint32)((uint32)currentTime64 - 0x500))
        ClearLastReactionIndex(script);
    script->lastActionTime = currentTime64;
    if (script->actionIndex == AISCRIPT_PLAYACTION_SENTINAL && (sint32)playerAction < (sint32)script->detectAction)
    {
        script->detectAction = playerAction;
        script->detectHumCarIndex = humCarIndex;
    }
}

void AIScript::ProcessActionsAndReactions(AIScript::AIScript_t *script, sint32 elapsedTicks)
{
    FUNCTION_MARKER(0x8006F7F0, "NFS4.EXE");

    tReactionDetails (*scriptData)[7] = script->data;
    if (script->actionIndex == AISCRIPT_PLAYACTION_SENTINAL)
    {
        if (script->detectAction != AISCRIPT_PLAYACTION_SENTINAL)
        {
            sint32 go = 1;
            script->actionIndex = script->detectAction;
            script->detectAction = AISCRIPT_PLAYACTION_SENTINAL;
            script->reactionTicksLeft = 0;
            sint32 *lastReactionIndex = &script->lastReactionIndex[script->actionIndex];
            script->actionHumCarIndex = script->detectHumCarIndex;
            sint32 reactionIndex = *lastReactionIndex;
            script->reaction = AISCRIPT_AIREACTION_INIT;
            script->reactionIndex = reactionIndex;
            while (go != 0)
            {
                sint32 nextIndex = (sint32)((uint32)script->reactionIndex + 1);
                if (nextIndex < 4 && (1U << ((uint8)(*scriptData)[script->actionIndex].reaction[nextIndex] & 31)) != 2)
                {
                    script->reactionIndex = nextIndex;
                    *lastReactionIndex = (sint32)((uint32)*lastReactionIndex + 1);
                }
                tAIReaction newReaction = (tAIReaction)(1U << ((uint8)(*scriptData)[script->actionIndex].reaction[script->reactionIndex] & 31));
                script->reaction = (tAIReaction)((uint32)script->reaction | (uint32)newReaction);
                sint32 newTime = (uint8)(*scriptData)[script->actionIndex].halfSeconds[script->reactionIndex];
                if (newTime != 0)
                {
                    script->reactionTicksLeft = newTime << 4;
                    go = 0;
                }
                if (script->reaction == AISCRIPT_AIREACTION_INIT)
                {
                    script->actionIndex = AISCRIPT_PLAYACTION_SENTINAL;
                    script->detectAction = AISCRIPT_PLAYACTION_SENTINAL;
                }
            }
        }
        if (script->actionIndex == AISCRIPT_PLAYACTION_SENTINAL)
            return;
    }
    if (script->reactionTicksLeft > 0)
        script->reactionTicksLeft = (sint32)((uint32)script->reactionTicksLeft - (uint32)elapsedTicks);
    else
    {
        script->actionIndex = AISCRIPT_PLAYACTION_SENTINAL;
        script->detectAction = AISCRIPT_PLAYACTION_SENTINAL;
    }
}

sint32 AIScript::DoReAction(AIScript::AIScript_t *script, AIScript::tAIReaction testReaction)
{
    FUNCTION_MARKER(0x8006F958, "NFS4.EXE");

    sint32 humCarIndex = -1;
    if (script->actionIndex != AISCRIPT_PLAYACTION_SENTINAL && ((uint32)testReaction & (uint32)script->reaction) != 0)
        humCarIndex = script->actionHumCarIndex;
    return humCarIndex;
}

sint32 AIScript::GetReactionTicksLeft(AIScript::AIScript_t *script)
{
    FUNCTION_MARKER(0x8006F988, "NFS4.EXE");

    return script->actionIndex == AISCRIPT_PLAYACTION_SENTINAL ? 0 : script->reactionTicksLeft;
}
