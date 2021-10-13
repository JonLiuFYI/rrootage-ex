/*
 * $Id: rand.h,v 1.1.1.1 2003/03/16 07:03:49 kenta Exp $
 *
 * Copyright 2003 Kenta Cho. All rights reserved.
 */

/**
 * Make random number function.
 *
 * @version $Revision: 1.1.1.1 $
 */

#ifdef __cplusplus
extern "C" {
#endif

	void setSeed(unsigned long s);
	unsigned long nextRand();

#ifdef __cplusplus
}
#endif
