/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libsetup:libsetup/setupFile.h	1.2"
#if	!defined(SETUPFILE_H)
#define	SETUPFILE_H

#include	<mail/setupTypes.h>

void
    setupFileInit(),
    setupFilePerm(fileHandle_t *handle_p, int *ok),
    setupFileClose(fileHandle_t *handle_p);

setupVarOps_t
    *setupFileGetVarOps(fileHandle_t *handle_p, setupVar_t *variable_p);

fileHandle_t
    *setupFileOpen(char *pathname, char *flags);

int
    setupFileApply(fileHandle_t *handle_p);

#endif
