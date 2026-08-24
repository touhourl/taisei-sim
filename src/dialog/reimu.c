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

DIALOG_TASK(reimu, Stage1PreBoss) {
	// Initialization, must be at the very top (after PRELOAD).
	DIALOG_BEGIN(Stage1PreBoss);
	// EVENT()s are handled by stage code.
	// You can find the list of events per dialogue in dialog_interface.h
	// All of them should be signaled eventually.
	EVENT(boss_appears);
	EVENT(music_changes);
	DIALOG_END();
}

DIALOG_TASK(reimu, Stage1PostBoss) {
	DIALOG_BEGIN(Stage1PostBoss);
	DIALOG_END();
}

/*
 * Stage 2
 */

DIALOG_TASK(reimu, Stage2PreBoss) {
	DIALOG_BEGIN(Stage2PreBoss);
	EVENT(boss_appears);
	EVENT(music_changes);
	DIALOG_END();
}

DIALOG_TASK(reimu, Stage2PostBoss) {
	DIALOG_BEGIN(Stage2PostBoss);
	DIALOG_END();
}

/*
 * Stage 3
 */

DIALOG_TASK(reimu, Stage3PreBoss) {
	DIALOG_BEGIN(Stage3PreBoss);
	EVENT(boss_appears);
	EVENT(music_changes);
	DIALOG_END();
}

DIALOG_TASK(reimu, Stage3PostBoss) {
	DIALOG_BEGIN(Stage3PostBoss);
	DIALOG_END();
}

/*
 * Stage 4
 */

DIALOG_TASK(reimu, Stage4PreBoss) {
	DIALOG_BEGIN(Stage4PreBoss);
	EVENT(boss_appears);
	EVENT(music_changes);
	DIALOG_END();
}

DIALOG_TASK(reimu, Stage4PostBoss) {
	DIALOG_BEGIN(Stage4PostBoss);
	DIALOG_END();
}

/*
 * Stage 5
 */

DIALOG_TASK(reimu, Stage5PreBoss) {
	DIALOG_BEGIN(Stage5PreBoss);
	EVENT(boss_appears);
	EVENT(music_changes);
	DIALOG_END();
}

DIALOG_TASK(reimu, Stage5PostMidBoss) {
	DIALOG_BEGIN(Stage5PostMidBoss);
	// should be only one message with a fixed 120-frames timeout
	// UNSKIPPABLE is required here to keep music in sync.
	// but since the music are changed anyway just delete you lol
	DIALOG_END();
}

DIALOG_TASK(reimu, Stage5PostBoss) {
	DIALOG_BEGIN(Stage5PostBoss);
	DIALOG_END();
}

/*
 * Stage 6
 */

DIALOG_TASK(reimu, Stage6PreBoss) {
	DIALOG_BEGIN(Stage6PreBoss);
	EVENT(boss_appears);
	EVENT(music_changes);
	DIALOG_END();
}

DIALOG_TASK(reimu, Stage6PreFinal) {
	DIALOG_BEGIN(Stage6PreFinal);
	DIALOG_END();
}

/*
 * Register the tasks
 */

#define EXPORT_DIALOG_TASKS_CHARACTER reimu
#include "export_dialog_tasks.inc.h"
