/*
 * $Id: rr.h,v 1.4 2003/04/26 03:24:16 kenta Exp $
 *
 * Copyright 2003 Kenta Cho. All rights reserved.
 */

/**
 * rRootage header file.
 *
 * @version $Revision: 1.4 $
 */
#define CAPTION "rRootage"
#define VERSION_NUM 22
#define SAVE_VERSION_NUM 1
#define INTERVAL_BASE 16

#ifdef __cplusplus
extern "C" {
#endif

	extern int status;
	extern int interval;
	extern int tick;

#define TITLE 0
#define IN_GAME 1
#define GAMEOVER 2
#define STAGE_CLEAR 3
#define PAUSE 4
#define IN_CONFIG 5

	void quitLast();
	void initTitleStage(int stg);
	void initConfigStage();
	void initTitle();
	void initGame(int stg);
	void initGameover();

#ifdef __cplusplus
}
#endif
