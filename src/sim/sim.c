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

static uint32_t projectile_category(const Projectile *p) {
    switch(p->type) {
        case PROJ_ENEMY: return TAISEI_SIM_PROJECTILE_ENEMY;
        case PROJ_DEAD: return TAISEI_SIM_PROJECTILE_CLEARING;
        case PROJ_PLAYER: return TAISEI_SIM_PROJECTILE_PLAYER;
        default: return TAISEI_SIM_PROJECTILE_INVALID;
    }
}

static uint32_t projectile_flags(const Projectile *p) {
    uint32_t flags = 0;
    if(!(p->flags & PFLAG_NOGRAZE)) flags |= TAISEI_SIM_PROJECTILE_FLAG_GRAZEABLE;
    if(!(p->flags & PFLAG_NOCLEAR)) flags |= TAISEI_SIM_PROJECTILE_FLAG_CLEARABLE;
    if(!(p->flags & PFLAG_NOCOLLISION)) flags |= TAISEI_SIM_PROJECTILE_FLAG_COLLISION_ENABLED;
    if(p->flags & PFLAG_INDESTRUCTIBLE) flags |= TAISEI_SIM_PROJECTILE_FLAG_INDESTRUCTIBLE;
    if(p->type == PROJ_ENEMY && !(p->flags & PFLAG_NOCOLLISION)) flags |= TAISEI_SIM_PROJECTILE_FLAG_ACTIVE_HAZARD;
    return flags;
}

static uint32_t input_flags(PlrInputFlag flags) {
    uint32_t out = 0;
    if(flags & INFLAG_UP) out |= TAISEI_SIM_INPUT_UP;
    if(flags & INFLAG_DOWN) out |= TAISEI_SIM_INPUT_DOWN;
    if(flags & INFLAG_LEFT) out |= TAISEI_SIM_INPUT_LEFT;
    if(flags & INFLAG_RIGHT) out |= TAISEI_SIM_INPUT_RIGHT;
    if(flags & INFLAG_FOCUS) out |= TAISEI_SIM_INPUT_FOCUS;
    if(flags & INFLAG_SHOT) out |= TAISEI_SIM_INPUT_SHOT;
    if(flags & INFLAG_SKIP) out |= TAISEI_SIM_INPUT_SKIP;
    return out;
}

static uint32_t clear_flags(uint internal_flags) {
    uint32_t out = 0;
    if(internal_flags & CLEAR_HAZARDS_BULLETS) out |= TAISEI_SIM_CLEAR_FLAG_BULLETS;
    if(internal_flags & CLEAR_HAZARDS_LASERS) out |= TAISEI_SIM_CLEAR_FLAG_LASERS;
    if(internal_flags & CLEAR_HAZARDS_FORCE) out |= TAISEI_SIM_CLEAR_FLAG_FORCED;
    if(internal_flags & CLEAR_HAZARDS_NOW) out |= TAISEI_SIM_CLEAR_FLAG_IMMEDIATE;
    if(internal_flags & CLEAR_HAZARDS_SPAWN_VOLTAGE) out |= TAISEI_SIM_CLEAR_FLAG_SPAWNS_VOLTAGE;
    return out;
}

static uint32_t item_type(ItemType type) {
    switch(type) {
        case ITEM_PIV: return TAISEI_SIM_ITEM_PIV;
        case ITEM_POINTS: return TAISEI_SIM_ITEM_POINTS;
        case ITEM_POWER_MINI: return TAISEI_SIM_ITEM_POWER_MINI;
        case ITEM_POWER: return TAISEI_SIM_ITEM_POWER;
        case ITEM_SURGE: return TAISEI_SIM_ITEM_SURGE;
        case ITEM_VOLTAGE: return TAISEI_SIM_ITEM_VOLTAGE;
        case ITEM_BOMB_FRAGMENT: return TAISEI_SIM_ITEM_BOMB_FRAGMENT;
        case ITEM_LIFE_FRAGMENT: return TAISEI_SIM_ITEM_LIFE_FRAGMENT;
        case ITEM_BOMB: return TAISEI_SIM_ITEM_BOMB;
        case ITEM_LIFE: return TAISEI_SIM_ITEM_LIFE;
    }
    return 0;
}

static uint32_t damage_type(DamageType type) {
    switch(type) {
        case DMG_ENEMY_SHOT: return TAISEI_SIM_DAMAGE_ENEMY_SHOT;
        case DMG_ENEMY_COLLISION: return TAISEI_SIM_DAMAGE_ENEMY_COLLISION;
        case DMG_PLAYER_SHOT: return TAISEI_SIM_DAMAGE_PLAYER_SHOT;
        case DMG_PLAYER_BOMB: return TAISEI_SIM_DAMAGE_PLAYER_BOMB;
        case DMG_PLAYER_DISCHARGE: return TAISEI_SIM_DAMAGE_PLAYER_DISCHARGE;
        default: return TAISEI_SIM_DAMAGE_UNDEFINED;
    }
}

static uint32_t enemy_flags(Enemy *e) {
    uint32_t flags = 0;
    if(e->flags & EFLAG_KILLED) flags |= TAISEI_SIM_ENEMY_FLAG_KILLED;
    if(enemy_is_targetable(e)) flags |= TAISEI_SIM_ENEMY_FLAG_TARGETABLE;
    if(enemy_is_vulnerable(e)) flags |= TAISEI_SIM_ENEMY_FLAG_DAMAGEABLE;
    if(!(e->flags & EFLAG_NO_HURT)) flags |= TAISEI_SIM_ENEMY_FLAG_HARMFUL;
    if(e->flags & EFLAG_INVULNERABLE) flags |= TAISEI_SIM_ENEMY_FLAG_INVULNERABLE;
    if(e->flags & EFLAG_IMPENETRABLE) flags |= TAISEI_SIM_ENEMY_FLAG_IMPENETRABLE;
    if(e->flags & EFLAG_NO_AUTOKILL) flags |= TAISEI_SIM_ENEMY_FLAG_NO_AUTOKILL;
    return flags;
}

static uint32_t count_projectiles(void) {
    uint32_t count = 0;
    for(Projectile *p = global.projs.first; p; p = p->next) {
        if(p->type != PROJ_PARTICLE) {
            ++count;
        }
    }
    return count;
}

static uint32_t count_enemies(void) {
    uint32_t count = 0;
    for(Enemy *e = global.enemies.first; e; e = e->next) {
        ++count;
    }
    return count;
}

static uint32_t count_items(void) {
    uint32_t count = 0;
    for(Item *item = global.items.first; item; item = item->next) {
        ++count;
    }
    return count;
}

static uint32_t count_lasers(void) {
    uint32_t count = 0;
    for(Laser *laser = global.lasers.first; laser; laser = laser->next) {
        ++count;
    }
    return count;
}

typedef struct LaserTraceContext {
    TaiseiSim *sim;
    uint32_t count;
} LaserTraceContext;

static void *collect_laser_point(Laser *laser, const LaserTraceSample *sample, void *userdata) {
    (void)laser;
    LaserTraceContext *ctx = userdata;
    TaiseiSim *sim = ctx->sim;
    uint32_t index = ctx->count++;

    sim->laser_points = grow_array(
        sim->laser_points,
        &sim->laser_point_capacity,
        ctx->count,
        sizeof(*sim->laser_points)
    );

    float t = sample->segment_param;
    float width = sample->segment->width.a + (sample->segment->width.b - sample->segment->width.a) * t;
    sim->laser_points[index] = (TaiseiSimLaserPoint) {
        .position = vec2_from_complex(sample->pos),
        .half_width = width * 0.5f,
        .time = sample->segment->time.a + (sample->segment->time.b - sample->segment->time.a) * t,
        .flags = sample->discontinuous ? TAISEI_SIM_LASER_POINT_FLAG_DISCONTINUITY : 0u,
    };

    return userdata;
}

static void build_snapshot(TaiseiSim *sim) {
    TaiseiSimState *state = &sim->snapshot;
    *state = (TaiseiSimState) {
        .struct_size = sizeof(*state),
        .api_version = TAISEI_SIM_API_VERSION,
        .episode_status = sim->status,
        .initial_rng_seed = sim->initial_seed,
        .start_time = sim->start_time,
        .gameover_frame = global.gameover_time,
    };

    if(global.stage) {
        state->stage_id = global.stage->id;
        state->stage_type = stage_type(global.stage->type);
    }

    state->difficulty = global.diff;
    state->practice_mode = global.is_practice_mode;
    state->logical_frame = global.frames;
    state->stage_clear = stage_is_cleared() || sim->status == TAISEI_SIM_STATUS_WON;
    state->game_over = sim->status == TAISEI_SIM_STATUS_LOST;
    state->score = global.plr.points;
    state->graze = global.plr.graze;
    state->voltage = global.plr.voltage;
    state->deaths = global.plr.stats.stage.lives_used;
    state->bombs_used = global.plr.stats.stage.bombs_used;
    state->continues_used = global.plr.stats.stage.continues_used;

    Player *player = &global.plr;
    state->player = (TaiseiSimPlayerState) {
        .position = vec2_from_complex(player->pos),
        .previous_position = sim->previous_player_position,
        .velocity = vec2_from_complex(player->velocity),
        .input_flags = input_flags(player->inputflags),
        .focused = !!(player->inputflags & INFLAG_FOCUS),
        .shooting = !!(player->inputflags & INFLAG_SHOT),
        .lives = player->lives,
        .bombs = player->bombs,
        .life_fragments = player->life_fragments,
        .bomb_fragments = player->bomb_fragments,
        .stored_power = player->power_stored,
        .effective_power = player_get_effective_power(player),
        .point_item_value = player->point_item_value,
        .score = player->points,
        .graze = player->graze,
        .voltage = player->voltage,
        .invulnerable = !player_is_vulnerable(player),
        .recovering = player_is_recovering(player),
        .alive = player_is_alive(player),
        .death_timer = player->deathtime >= 0 ? player->deathtime - global.frames : -1,
        .respawn_timer = player->respawntime > global.frames ? player->respawntime - global.frames : 0,
        .recovery_timer = player->recoverytime > global.frames ? player->recoverytime - global.frames : 0,
        .bomb_active = player_is_bomb_active(player),
        .bomb_progress = player_get_bomb_progress(player),
        .bomb_trigger_frame = player->bomb_triggertime,
        .bomb_end_frame = player->bomb_endtime,
        .power_surge_active = player_is_powersurge_active(player),
        .power_surge_positive = player->powersurge.positive,
        .power_surge_negative = player->powersurge.negative,
    };

    if(player->mode) {
        state->player.player_character = player->mode->character->id;
        state->player.shot_mode = player->mode->shot_mode;
        state->player_character = player->mode->character->id;
        state->shot_mode = player->mode->shot_mode;
    }

    fill_boss_state(sim, &state->boss);

    state->projectile_count = count_projectiles();
    sim->projectiles = grow_array(sim->projectiles, &sim->projectile_capacity, state->projectile_count, sizeof(*sim->projectiles));
    uint32_t pi = 0;
    for(Projectile *p = global.projs.first; p; p = p->next) {
        if(p->type == PROJ_PARTICLE) {
            continue;
        }
        sim->projectiles[pi++] = (TaiseiSimProjectileState) {
            .spawn_id = p->ent.spawn_id,
            .category = projectile_category(p),
            .position = vec2_from_complex(p->pos),
            .previous_position = vec2_from_complex(p->prevpos),
            .velocity = vec2_from_complex(p->move.velocity),
            .collision_size = vec2_from_complex(p->collision_size),
            .damage = p->damage,
            .angle = p->angle,
            .age_frames = global.frames - p->birthtime,
            .flags = projectile_flags(p),
            .damage_type = damage_type(p->damage_type),
            .clear_flags = clear_flags(p->clear_flags),
        };
    }
    if(state->projectile_count > 1) {
        qsort(sim->projectiles, state->projectile_count, sizeof(*sim->projectiles), compare_projectiles);
    }

