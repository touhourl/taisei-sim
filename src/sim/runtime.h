#pragma once

#include "sim/taisei_sim.h"

TaiseiSimResult taisei_sim_runtime_init(const TaiseiSimGlobalConfig *config, char *error, size_t error_size);
void taisei_sim_runtime_shutdown(void);
