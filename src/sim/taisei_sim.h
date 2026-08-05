/*
 * C API for headless Taisei simulations.
 *
 * It is self contained.
 * For coordinates use Taisei's 480x560 playfield with (0, 0) at
 * the top-left, +x to the right, and +y downward.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32)
# if defined(TAISEI_SIM_BUILD)
#  define TAISEI_SIM_API __declspec(dllexport)
# else
#  define TAISEI_SIM_API __declspec(dllimport)
# endif
#elif defined(__GNUC__)
# define TAISEI_SIM_API __attribute__((visibility("default")))
#else
# define TAISEI_SIM_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define TAISEI_SIM_API_VERSION UINT32_C(1)
#define TAISEI_SIM_USE_DEFAULT_I32 INT32_C(-2147483647)
#define TAISEI_SIM_USE_DEFAULT_U64 UINT64_MAX

#define TAISEI_SIM_VIEWPORT_WIDTH 480.0
#define TAISEI_SIM_VIEWPORT_HEIGHT 560.0

#define TAISEI_SIM_ACTION_UP      (UINT32_C(1) << 0)
#define TAISEI_SIM_ACTION_DOWN    (UINT32_C(1) << 1)
#define TAISEI_SIM_ACTION_LEFT    (UINT32_C(1) << 2)
#define TAISEI_SIM_ACTION_RIGHT   (UINT32_C(1) << 3)
#define TAISEI_SIM_ACTION_FOCUS   (UINT32_C(1) << 4)
#define TAISEI_SIM_ACTION_SHOT    (UINT32_C(1) << 5)
#define TAISEI_SIM_ACTION_BOMB    (UINT32_C(1) << 6)
#define TAISEI_SIM_ACTION_SPECIAL (UINT32_C(1) << 7)
#define TAISEI_SIM_ACTION_ALL     UINT32_C(0xff)

#define TAISEI_SIM_INPUT_UP    (UINT32_C(1) << 0)
#define TAISEI_SIM_INPUT_DOWN  (UINT32_C(1) << 1)
#define TAISEI_SIM_INPUT_LEFT  (UINT32_C(1) << 2)
#define TAISEI_SIM_INPUT_RIGHT (UINT32_C(1) << 3)
#define TAISEI_SIM_INPUT_FOCUS (UINT32_C(1) << 4)
#define TAISEI_SIM_INPUT_SHOT  (UINT32_C(1) << 5)
#define TAISEI_SIM_INPUT_SKIP  (UINT32_C(1) << 6)

typedef uint32_t TaiseiSimDifficulty;
enum {
    TAISEI_SIM_DIFFICULTY_DEFAULT = 0,
    TAISEI_SIM_DIFFICULTY_EASY = 1,
    TAISEI_SIM_DIFFICULTY_NORMAL = 2,
    TAISEI_SIM_DIFFICULTY_HARD = 3,
    TAISEI_SIM_DIFFICULTY_LUNATIC = 4,
};

typedef uint32_t TaiseiSimProjectileCategory;
enum {
    TAISEI_SIM_PROJECTILE_INVALID = 0,
    TAISEI_SIM_PROJECTILE_ENEMY = 1,
    TAISEI_SIM_PROJECTILE_CLEARING = 2,
    TAISEI_SIM_PROJECTILE_PLAYER = 3,
};

typedef uint32_t TaiseiSimDamageType;
enum {
    TAISEI_SIM_DAMAGE_UNDEFINED = 0,
    TAISEI_SIM_DAMAGE_ENEMY_SHOT = 1,
    TAISEI_SIM_DAMAGE_ENEMY_COLLISION = 2,
    TAISEI_SIM_DAMAGE_PLAYER_SHOT = 3,
    TAISEI_SIM_DAMAGE_PLAYER_BOMB = 4,
    TAISEI_SIM_DAMAGE_PLAYER_DISCHARGE = 5,
};

#define TAISEI_SIM_PROJECTILE_FLAG_GRAZEABLE         (UINT32_C(1) << 0)
#define TAISEI_SIM_PROJECTILE_FLAG_CLEARABLE         (UINT32_C(1) << 1)
#define TAISEI_SIM_PROJECTILE_FLAG_COLLISION_ENABLED (UINT32_C(1) << 2)
#define TAISEI_SIM_PROJECTILE_FLAG_INDESTRUCTIBLE     (UINT32_C(1) << 3)
#define TAISEI_SIM_PROJECTILE_FLAG_ACTIVE_HAZARD      (UINT32_C(1) << 4)

#define TAISEI_SIM_ENEMY_FLAG_KILLED       (UINT32_C(1) << 0)
#define TAISEI_SIM_ENEMY_FLAG_TARGETABLE   (UINT32_C(1) << 1)
#define TAISEI_SIM_ENEMY_FLAG_DAMAGEABLE   (UINT32_C(1) << 2)
#define TAISEI_SIM_ENEMY_FLAG_HARMFUL      (UINT32_C(1) << 3)
#define TAISEI_SIM_ENEMY_FLAG_INVULNERABLE (UINT32_C(1) << 4)
#define TAISEI_SIM_ENEMY_FLAG_IMPENETRABLE (UINT32_C(1) << 5)
#define TAISEI_SIM_ENEMY_FLAG_NO_AUTOKILL  (UINT32_C(1) << 6)

#define TAISEI_SIM_CLEAR_FLAG_BULLETS        (UINT32_C(1) << 0)
#define TAISEI_SIM_CLEAR_FLAG_LASERS         (UINT32_C(1) << 1)
#define TAISEI_SIM_CLEAR_FLAG_FORCED         (UINT32_C(1) << 2)
#define TAISEI_SIM_CLEAR_FLAG_IMMEDIATE      (UINT32_C(1) << 3)
#define TAISEI_SIM_CLEAR_FLAG_SPAWNS_VOLTAGE (UINT32_C(1) << 4)

#define TAISEI_SIM_LASER_POINT_FLAG_DISCONTINUITY (UINT32_C(1) << 0)

typedef uint32_t TaiseiSimItemType;
enum {
    TAISEI_SIM_ITEM_PIV = 1,
    TAISEI_SIM_ITEM_POINTS = 2,
    TAISEI_SIM_ITEM_POWER_MINI = 3,
    TAISEI_SIM_ITEM_POWER = 4,
    TAISEI_SIM_ITEM_SURGE = 5,
    TAISEI_SIM_ITEM_VOLTAGE = 6,
    TAISEI_SIM_ITEM_BOMB_FRAGMENT = 7,
    TAISEI_SIM_ITEM_LIFE_FRAGMENT = 8,
    TAISEI_SIM_ITEM_BOMB = 9,
    TAISEI_SIM_ITEM_LIFE = 10,
};

typedef struct TaiseiSim TaiseiSim;

typedef int32_t TaiseiSimResult;
enum {
    TAISEI_SIM_OK = 0,
    TAISEI_SIM_ERROR_INVALID_ARGUMENT = -1,
    TAISEI_SIM_ERROR_INVALID_HANDLE = -2,
    TAISEI_SIM_ERROR_NOT_INITIALIZED = -3,
    TAISEI_SIM_ERROR_ALREADY_INITIALIZED = -4,
    TAISEI_SIM_ERROR_SIMULATION_ACTIVE = -5,
    TAISEI_SIM_ERROR_NO_ACTIVE_EPISODE = -6,
    TAISEI_SIM_ERROR_EPISODE_RUNNING = -7,
    TAISEI_SIM_ERROR_EPISODE_TERMINAL = -8,
    TAISEI_SIM_ERROR_STAGE_NOT_FOUND = -9,
    TAISEI_SIM_ERROR_PLAYER_MODE_NOT_FOUND = -10,
    TAISEI_SIM_ERROR_BUFFER_TOO_SMALL = -11,
    TAISEI_SIM_ERROR_IO = -12,
    TAISEI_SIM_ERROR_ABI_MISMATCH = -13,
    TAISEI_SIM_ERROR_INTERNAL = -14,
};

typedef uint32_t TaiseiSimEpisodeStatus;
enum {
    TAISEI_SIM_STATUS_INVALID = 0,
    TAISEI_SIM_STATUS_RUNNING = 1,
    TAISEI_SIM_STATUS_WON = 2,
    TAISEI_SIM_STATUS_LOST = 3,
    TAISEI_SIM_STATUS_ABORTED = 4,
    TAISEI_SIM_STATUS_ERROR = 5,
};

typedef uint32_t TaiseiSimStageType;
enum {
    TAISEI_SIM_STAGE_UNKNOWN = 0,
    TAISEI_SIM_STAGE_STORY = 1,
    TAISEI_SIM_STAGE_EXTRA = 2,
    TAISEI_SIM_STAGE_SPELL = 3,
    TAISEI_SIM_STAGE_SPECIAL = 4,
};

typedef uint32_t TaiseiSimBossPhaseType;
enum {
    TAISEI_SIM_BOSS_PHASE_NONE = 0,
    TAISEI_SIM_BOSS_PHASE_NONSPELL = 1,
    TAISEI_SIM_BOSS_PHASE_MOVE = 2,
    TAISEI_SIM_BOSS_PHASE_SPELL = 3,
    TAISEI_SIM_BOSS_PHASE_SURVIVAL = 4,
    TAISEI_SIM_BOSS_PHASE_EXTRA = 5,
};

typedef struct TaiseiSimVec2 {
    double x;
    double y;
} TaiseiSimVec2;

typedef struct TaiseiSimGlobalConfig {
    //sizeof(TaiseiSimGlobalConfig)
    uint32_t struct_size;
    //TAISEI_SIM_API_VERSION
    uint32_t api_version;
    // Optional, NULL can be.
    const char *resource_path;
    const char *storage_path;
    const char *cache_path;
    // == 0
    uint32_t flags;
    uint32_t reserved;
} TaiseiSimGlobalConfig;

typedef struct TaiseiSimConfig {
    //sizeof(TaiseiSimConfig)
    uint32_t struct_size;
    // == 0
    uint32_t flags;
    double laser_sample_step;
    // ==0
    uint32_t reserved;
} TaiseiSimConfig;

typedef struct TaiseiSimEpisodeConfig {
    //sizeof(TaiseiSimEpisodeConfig)
    uint32_t struct_size;
    uint16_t stage_id;
    uint8_t difficulty;
    uint8_t player_character;
    uint8_t shot_mode;
    uint8_t practice_mode;
    uint16_t reserved0;
    uint64_t rng_seed;
    // TAISEI_SIM_USE_DEFAULT_U64
    uint64_t start_time;
    //TAISEI_SIM_USE_DEFAULT_I32
    int32_t initial_lives;
    int32_t initial_bombs;
    int32_t initial_life_fragments;
    int32_t initial_bomb_fragments;
    int32_t initial_power;
    int32_t initial_point_item_value;
    uint64_t initial_score;
    uint32_t initial_graze;
    uint32_t reserved1;
} TaiseiSimEpisodeConfig;

typedef struct TaiseiSimAction {
    //sizeof(TaiseiSimAction)
    uint32_t struct_size;
    uint32_t buttons;
} TaiseiSimAction;

typedef struct TaiseiSimPlayerState {
    TaiseiSimVec2 position;
    TaiseiSimVec2 previous_position;
    TaiseiSimVec2 velocity;
    uint32_t input_flags;
    uint32_t focused;
    uint32_t shooting;
    int32_t lives;
    int32_t bombs;
    int32_t life_fragments;
    int32_t bomb_fragments;
    int32_t stored_power;
    int32_t effective_power;
    uint32_t point_item_value;
    uint64_t score;
    uint32_t graze;
    uint32_t voltage;
    uint32_t invulnerable;
    uint32_t recovering;
    uint32_t alive;
    int32_t death_timer;
    int32_t respawn_timer;
    int32_t recovery_timer;
    uint32_t bomb_active;
    double bomb_progress;
    int32_t bomb_trigger_frame;
    int32_t bomb_end_frame;
    uint32_t power_surge_active;
    float power_surge_positive;
    float power_surge_negative;
    uint8_t player_character;
    uint8_t shot_mode;
    uint16_t reserved;
} TaiseiSimPlayerState;

typedef struct TaiseiSimBossState {
    uint32_t active;
    TaiseiSimVec2 position;
    TaiseiSimVec2 velocity;
    float hp;
    float max_hp;
    uint32_t invulnerable;
    int32_t phase_index;
    uint32_t phase_type;
    uint16_t attack_id;
    uint16_t spell_id;
    uint32_t spell_active;
    int32_t phase_start_frame;
    int32_t phase_end_frame;
    int32_t phase_timeout_frames;
    int32_t remaining_timeout_frames;
    int32_t spell_failed_frame;
    uint32_t spell_failed;
    uint32_t spell_captured;
    float recent_damage;
} TaiseiSimBossState;

