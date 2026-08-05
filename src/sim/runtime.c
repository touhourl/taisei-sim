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

TaiseiSimResult taisei_sim_runtime_init(const TaiseiSimGlobalConfig *config, char *error, size_t error_size) {
    if(runtime_initialized) {
        snprintf(error, error_size, "Taisei simulation runtime is already initialized");
        return TAISEI_SIM_ERROR_ALREADY_INITIALIZED;
    }
    if(config->flags != 0 || config->reserved != 0) {
        snprintf(error, error_size, "Unknown global simulation flags or nonzero field");
        return TAISEI_SIM_ERROR_INVALID_ARGUMENT;
    }

    setlocale(LC_ALL, "C");

    /*
     * This must be the first call into SDL. env_set() below go through
     * SDL_GetEnvironment(), which allocate SDL's process env.
     * Else, signsegv.
     */
    mem_install_sdl_callbacks();

    set_path_env("TAISEI_RES_PATH", config->resource_path);
    set_path_env("TAISEI_STORAGE_PATH", config->storage_path);
    set_path_env("TAISEI_CACHE_PATH", config->cache_path);
    env_set("SDL_AUDIODRIVER", "dummy", true);
    env_set("SDL_VIDEODRIVER", "dummy", true);
    env_set("TAISEI_AUDIO_BACKEND", "null", true);
    env_set("TAISEI_RENDERER", "null", true);
    env_set("TAISEI_FRAMELIMITER_LOGIC_ONLY", "1", true);

    thread_init();
    coroutines_init();
    log_init(LOG_DEFAULT_LEVELS);
    stageinfo_init();

    vfs_ready = false;
    vfs_setup(CALLCHAIN(sim_vfs_ready, NULL));
    if(!vfs_ready) {
        snprintf(error, error_size, "VFS setup is not supported");
        return TAISEI_SIM_ERROR_INTERNAL;
    }

    config_load();

    if(!SDL_Init(SDL_INIT_EVENTS)) {
        snprintf(error, error_size, "SDL_Init failed: %s", SDL_GetError());
        return TAISEI_SIM_ERROR_INTERNAL;
    }

    log_sdl_init(SDL_LOG_PRIORITY_WARN);
    taskmgr_global_init();
    gamemode_init();
    time_init();

    CLIAction cli = { .frameskip = 1 };
    init_global(&cli);
    global.is_headless = true;
    global.is_simulation = true;
    global.frameskip = 1;

    events_init();
    video_init(&(VideoInitParams) {
        .width = RESX,
        .height = RESY,
    });
    filewatch_init();
    res_init();
    r_models_init();
    r_sprite_batch_init();
    r_post_init();
    i18n_init();
    dynstage_init_monitoring();
    audio_init();
    bgm_init();
    res_post_init();
    gamepad_init();
    progress_load();
    video_post_init();

    runtime_initialized = true;
    return TAISEI_SIM_OK;
}

