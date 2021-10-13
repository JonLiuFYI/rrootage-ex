/*
 * $Id: background.h,v 1.1.1.1 2003/03/16 07:03:49 kenta Exp $
 *
 * Copyright 2003 Kenta Cho. All rights reserved.
 */

/**
 * Screen background.
 *
 * @version $Revision: 1.1.1.1 $
 */


#ifdef __cplusplus
extern "C" {
#endif

	typedef struct {
		float x, y, z, ox, oy;
		float mx, my;
		int d1;
		float width, height;
		int xn, yn;
		int r, g, b, a;
	} Plane;

	void initBackground(int s);
	void moveBackground();
	void drawBackground();

#ifdef __cplusplus
}
#endif
