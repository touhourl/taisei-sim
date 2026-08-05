/*
 * Internal runtime for thrl of taisei-sim.
 */
#include "runtime.h"

#include "audio/audio.h"
#include "resource/bgm.h"
#include "cli.h"
#include "config.h"
#include "coroutine/coroutine.h"
#include "dynstage.h"
#include "events.h"
#include "filewatch/filewatch.h"
#include "gamepad.h"
#include "global.h"
#include "hirestime.h"
#include "i18n/i18n.h"
#include "log.h"
#include "log_sdl.h"
#include "memory/memory.h"
#include "progress.h"
#include "renderer/api.h"
#include "renderer/common/models.h"
#include "renderer/common/sprite_batch.h"
#include "resource/resource.h"
#include "stageinfo.h"
#include "stageobjects.h"
#include "taskmanager.h"
#include "thread.h"
#include "util/env.h"
#include "util/gamemode.h"
#include "vfs/setup.h"
#include "video.h"
#include "watchdog.h"

#include <locale.h>
#include <stdio.h>
#include <string.h>

static bool runtime_initialized;
static bool vfs_ready;

static void sim_vfs_ready(CallChainResult ccr) {
    (void)ccr;
    vfs_ready = true;
}

static void set_path_env(const char *name, const char *value) {
    if(value && *value) {
        env_set(name, value, true);
    }
}

