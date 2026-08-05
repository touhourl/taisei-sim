/* 
 * So, it is again a project like ReC98. But it is pretty simple then: 
 * I don't need to look at ancient C++ code anymore and it is like just
 * the `github.com/touhourl/thrl/tree/main/src/games/th05_c/reader.rs`
 * but in C. 
 */
#include "sim/taisei_sim.h"
#include "sim/runtime.h"

#include "boss.h"
#include "difficulty.h"
#include "eventloop/eventloop.h"
#include "global.h"
#include "item.h"
#include "lasers/laser.h"
#include "memory/memory.h"
#include "player.h"
#include "plrmodes.h"
#include "projectile.h"
#include "replay/replay.h"
#include "replay/state.h"
#include "replay/struct.h"
#include "resource/resource.h"
#include "stage.h"
#include "stageinfo.h"
#include "stats.h"

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIM_ERROR_SIZE 512
#define SIM_ABORT_FRAME_LIMIT (FPS * 10)

struct TaiseiSim {
    ResourceGroup resources;
    Replay replay;
    TaiseiSimEpisodeStatus status;
    bool episode_active;
    bool replay_complete;
    bool replay_initialized;
    uint32_t desired_buttons;
    uint32_t applied_buttons;
    uint64_t initial_seed;
    uint64_t start_time;
    double laser_sample_step;
    TaiseiSimVec2 previous_player_position;
    float recent_boss_damage;
    char error[SIM_ERROR_SIZE];

    TaiseiSimState snapshot;
    TaiseiSimProjectileState *projectiles;
    size_t projectile_capacity;
    TaiseiSimEnemyState *enemies;
    size_t enemy_capacity;
    TaiseiSimItemState *items;
    size_t item_capacity;
    TaiseiSimLaserState *lasers;
    size_t laser_capacity;
    TaiseiSimLaserPoint *laser_points;
    size_t laser_point_capacity;
};

static bool library_initialized;
static TaiseiSim *active_sim;
static char global_error[SIM_ERROR_SIZE];

static TaiseiSimResult set_error(TaiseiSim *sim, TaiseiSimResult result, const char *message) {
    char *dst = sim ? sim->error : global_error;
    snprintf(dst, SIM_ERROR_SIZE, "%s", message ? message : "Unknown simulation error");
    return result;
}

static TaiseiSimResult validate_sim(TaiseiSim *sim) {
    if(sim == NULL || sim != active_sim) {
        return set_error(NULL, TAISEI_SIM_ERROR_INVALID_HANDLE, "Invalid or inactive simulation handle");
    }
    return TAISEI_SIM_OK;
}

static bool struct_is_compatible(uint32_t provided, size_t required) {
    return provided >= required;
}

static TaiseiSimVec2 vec2_from_complex(cmplx value) {
    return (TaiseiSimVec2) { .x = re(value), .y = im(value) };
}

static void *grow_array(void *ptr, size_t *capacity, size_t required, size_t elem_size) {
    if(required <= *capacity) {
        return ptr;
    }

    size_t cap = *capacity ? *capacity : 64;
    while(cap < required) {
        cap *= 2;
    }

    ptr = mem_realloc(ptr, cap * elem_size);
    *capacity = cap;
    return ptr;
}

