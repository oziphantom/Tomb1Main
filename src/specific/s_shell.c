#include "specific/s_shell.h"

#include "args.h"
#include "game/demo.h"
#include "game/game.h"
#include "game/gameflow.h"
#include "game/inv.h"
#include "game/music.h"
#include "game/savegame.h"
#include "game/settings.h"
#include "game/setup.h"
#include "game/text.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "log.h"
#include "memory.h"
#include "specific/s_display.h"
#include "specific/s_frontend.h"
#include "specific/s_hwr.h"
#include "specific/s_init.h"
#include "specific/s_input.h"
#include "specific/s_output.h"
#include "specific/s_main.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const char *T1MGameflowPath = "cfg/Tomb1Main_gameflow.json5";
static const char *T1MGameflowGoldPath = "cfg/Tomb1Main_gameflow_ub.json5";

void GameMain()
{
    SoundIsActive = true;

    const char *gameflow_path = T1MGameflowPath;

    char **args;
    int arg_count;
    get_command_line(&args, &arg_count);
    for (int i = 0; i < arg_count; i++) {
        if (!strcmp(args[i], "-gold")) {
            gameflow_path = T1MGameflowGoldPath;
        }
    }

    for (int i = 0; i < arg_count; i++) {
        Memory_Free(args[i]);
    }
    Memory_Free(args);

    S_InitialiseSystem();

    if (!GF_LoadScriptFile(gameflow_path)) {
        ShowFatalError("MAIN: unable to load script file");
        return;
    }

    InitialiseStartInfo();
    S_FrontEndCheck();
    S_ReadUserSettings();

    TempVideoAdjust(2);
    S_DisplayPicture("data\\eidospc");
    S_InitialisePolyList();
    S_CopyBufferToScreen();
    S_OutputPolyList();
    S_DumpScreen();
    S_Wait(TICKS_PER_SECOND);

    HWR_PrepareFMV();
    WinPlayFMV(FMV_CORE, 1);
    WinPlayFMV(FMV_ESCAPE, 1);
    WinPlayFMV(FMV_INTRO, 1);
    HWR_FMVDone();

    int32_t gf_option = GF_EXIT_TO_TITLE;

    int8_t loop_continue = 1;
    while (loop_continue) {
        TempVideoRemove();
        int32_t gf_direction = gf_option & ~((1 << 6) - 1);
        int32_t gf_param = gf_option & ((1 << 6) - 1);
        LOG_INFO("%d %d", gf_direction, gf_param);

        switch (gf_direction) {
        case GF_START_GAME:
            gf_option = GF_InterpretSequence(gf_param, GFL_NORMAL);
            break;

        case GF_START_SAVED_GAME:
            S_LoadGame(&SaveGame, gf_param);
            gf_option = GF_InterpretSequence(SaveGame.current_level, GFL_SAVED);
            break;

        case GF_START_CINE:
            gf_option = GF_InterpretSequence(gf_param, GFL_CUTSCENE);
            break;

        case GF_START_DEMO:
            gf_option = StartDemo();
            break;

        case GF_LEVEL_COMPLETE:
            gf_option = LevelCompleteSequence(gf_param);
            break;

        case GF_EXIT_TO_TITLE:
            Text_RemoveAll();
            TempVideoAdjust(2);
            S_DisplayPicture("data\\titleh");
            NoInputCount = 0;
            if (!InitialiseLevel(GF.title_level_num, GFL_TITLE)) {
                gf_option = GF_EXIT_GAME;
                break;
            }

            gf_option = Display_Inventory(INV_TITLE_MODE);

            S_FadeToBlack();
            Music_Stop();
            break;

        case GF_EXIT_GAME:
            loop_continue = 0;
            break;

        default:
            S_ExitSystemFmt(
                "MAIN: Unknown request %x %d", gf_direction, gf_param);
            return;
        }
    }

    S_WriteUserSettings();
}