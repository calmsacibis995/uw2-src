/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/debug.c	1.1"

#include "stdenv.h"
#include "sidep.h"
#include "generic.h"
#include "g_gs.h"
#include "p9k_opt.h"
#include "p9k_gs.h"
#include "g_state.h"
#include "p9k_state.h"
#include "p9k.h"
#include "p9k_regs.h"

void
print_oor_status()
{
	unsigned char oor;
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();




	oor = P9000_READ_PARAMETER_CONTROL_REGISTER(P9000_PARAMETER_CONTROL_OOR);

	if (oor & P9000_OOR_X_0)
		printf("XO");

	if (oor & P9000_OOR_X_1)
		printf("X1");

	if (oor & P9000_OOR_X_2)
		printf("X2");

	if (oor & P9000_OOR_X_3)
		printf("X3");

	if (oor & P9000_OOR_Y_0)
		printf("YO");

	if (oor & P9000_OOR_Y_1)
		printf("Y1");

	if (oor & P9000_OOR_Y_2)
		printf("Y2");

	if (oor & P9000_OOR_Y_3)
		printf("Y3");

}
unsigned int
read_status()
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();

	return P9000_READ_STATUS_REGISTER();
}

