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

static int compare_projectiles(const void *a, const void *b) {
    const TaiseiSimProjectileState *pa = a;
    const TaiseiSimProjectileState *pb = b;
    return (pa->spawn_id > pb->spawn_id) - (pa->spawn_id < pb->spawn_id);
}

static int compare_enemies(const void *a, const void *b) {
    const TaiseiSimEnemyState *ea = a;
    const TaiseiSimEnemyState *eb = b;
    return (ea->spawn_id > eb->spawn_id) - (ea->spawn_id < eb->spawn_id);
}

static int compare_items(const void *a, const void *b) {
    const TaiseiSimItemState *ia = a;
    const TaiseiSimItemState *ib = b;
    return (ia->spawn_id > ib->spawn_id) - (ia->spawn_id < ib->spawn_id);
}

static int compare_laser_ptrs(const void *a, const void *b) {
    const Laser *la = *(Laser *const *)a;
    const Laser *lb = *(Laser *const *)b;
    return (la->ent.spawn_id > lb->ent.spawn_id) - (la->ent.spawn_id < lb->ent.spawn_id);
}

static uint64_t digest_bytes(uint64_t digest, const void *data, size_t size) {
    const uint8_t *bytes = data;
    for(size_t i = 0; i < size; ++i) {
        digest ^= bytes[i];
        digest *= UINT64_C(1099511628211);
    }
    return digest;
}

static uint32_t boss_phase_type(const Attack *attack) {
    if(!attack) {
        return TAISEI_SIM_BOSS_PHASE_NONE;
    }

    switch(attack->type) {
        case AT_Normal: return TAISEI_SIM_BOSS_PHASE_NONSPELL;
        case AT_Move: return TAISEI_SIM_BOSS_PHASE_MOVE;
        case AT_Spellcard: return TAISEI_SIM_BOSS_PHASE_SPELL;
        case AT_SurvivalSpell: return TAISEI_SIM_BOSS_PHASE_SURVIVAL;
        case AT_ExtraSpell: return TAISEI_SIM_BOSS_PHASE_EXTRA;
    }

    return TAISEI_SIM_BOSS_PHASE_NONE;
}

static uint16_t stable_spell_id(const Attack *attack) {
    if(!attack || !attack->info || !ATTACK_IS_SPELL(attack->type)) {
        return 0;
    }

    StageInfo *spell_stage = stageinfo_get_by_spellcard(attack->info, global.diff);
    return spell_stage ? spell_stage->id : 0;
}

static uint16_t stage_type(StageType type) {
    switch(type) {
        case STAGE_STORY: return TAISEI_SIM_STAGE_STORY;
        case STAGE_EXTRA: return TAISEI_SIM_STAGE_EXTRA;
        case STAGE_SPELL: return TAISEI_SIM_STAGE_SPELL;
        case STAGE_SPECIAL: return TAISEI_SIM_STAGE_SPECIAL;
    }
    return TAISEI_SIM_STAGE_UNKNOWN;
}

static void fill_boss_state(TaiseiSim *sim, TaiseiSimBossState *out) {
    *out = (TaiseiSimBossState) {};

    Boss *boss = global.boss;
    if(!boss) {
        out->phase_index = -1;
        return;
    }

    out->active = true;
    out->position = vec2_from_complex(boss->pos);
    out->velocity = vec2_from_complex(boss->move.velocity);
    out->invulnerable = !boss_is_vulnerable(boss);
    out->recent_damage = sim->recent_boss_damage;

    Attack *attack = boss->current;
    if(!attack) {
        out->phase_index = -1;
        return;
    }

    ptrdiff_t phase_index = attack - boss->attacks;
    out->phase_index = (phase_index >= 0 && phase_index < BOSS_MAX_ATTACKS) ? (int32_t)phase_index : -1;
    out->phase_type = boss_phase_type(attack);
    out->attack_id = out->phase_index >= 0 ? (uint16_t)(out->phase_index + 1) : 0;
    out->spell_id = stable_spell_id(attack);
    out->spell_active = ATTACK_IS_SPELL(attack->type) && attack_is_active(attack);
    out->hp = attack->hp;
    out->max_hp = attack->maxhp;
    out->phase_start_frame = attack->starttime;
    out->phase_end_frame = attack->endtime;
    out->phase_timeout_frames = attack->timeout;
    out->remaining_timeout_frames = max(0, attack->starttime + attack->timeout - global.frames);
    out->spell_failed_frame = attack->failtime;
    out->spell_failed = attack_was_failed(attack);
    out->spell_captured = ATTACK_IS_SPELL(attack->type) && attack_has_finished(attack) && !attack_was_failed(attack);
}

