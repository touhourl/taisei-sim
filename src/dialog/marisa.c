/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "dialog_macros.h"

/*
 * Stage 1
 */

DIALOG_TASK(marisa, Stage1PreBoss) {
	DIALOG_BEGIN(Stage1PreBoss);
	EVENT(boss_appears);
	EVENT(music_changes);
	DIALOG_END();
}

DIALOG_TASK(marisa, Stage1PostBoss) {
	DIALOG_BEGIN(Stage1PostBoss);
	DIALOG_END();
}

/*
 * Stage 2
 */

DIALOG_TASK(marisa, Stage2PreBoss) {
	DIALOG_BEGIN(Stage2PreBoss);
	EVENT(boss_appears);
	EVENT(music_changes);
	DIALOG_END();
}

DIALOG_TASK(marisa, Stage2PostBoss) {
	DIALOG_BEGIN(Stage2PostBoss);
	DIALOG_END();
}

/*
 * Stage 3
 */

DIALOG_TASK(marisa, Stage3PreBoss) {
	DIALOG_BEGIN(Stage3PreBoss);
	EVENT(boss_appears);
	EVENT(music_changes);
	DIALOG_END();
}

DIALOG_TASK(marisa, Stage3PostBoss) {
	DIALOG_BEGIN(Stage3PostBoss);
	DIALOG_END();
}

/*
 * Stage 4
 */

DIALOG_TASK(marisa, Stage4PreBoss) {
	DIALOG_BEGIN(Stage4PreBoss);
	EVENT(boss_appears);
	EVENT(music_changes);
	DIALOG_END();
}

DIALOG_TASK(marisa, Stage4PostBoss) {
	DIALOG_BEGIN(Stage4PostBoss);
	DIALOG_END();
}

/*
 * Stage 5
 */

DIALOG_TASK(marisa, Stage5PreBoss) {
	DIALOG_BEGIN(Stage5PreBoss);
	EVENT(boss_appears);
	EVENT(music_changes);
	DIALOG_END();
}

DIALOG_TASK(marisa, Stage5PostMidBoss) {
	DIALOG_BEGIN(Stage5PostMidBoss);
	// should be only one message with a fixed 180-frame (3 second) timeout
	DIALOG_END();
}

DIALOG_TASK(marisa, Stage5PostBoss) {
	DIALOG_BEGIN(Stage5PostBoss);
	DIALOG_END();
}

/*
 * Stage 6
 */

DIALOG_TASK(marisa, Stage6PreBoss) {
	DIALOG_BEGIN(Stage6PreBoss);
	EVENT(boss_appears);
	EVENT(music_changes);
	DIALOG_END();
}

DIALOG_TASK(marisa, Stage6PreFinal) {
	DIALOG_BEGIN(Stage6PreFinal);
	DIALOG_END();
}

/*
 * Register the tasks
 */

#define EXPORT_DIALOG_TASKS_CHARACTER marisa
#include "export_dialog_tasks.inc.h"
