/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libsetup:libsetup/setupVar.h	1.1"
#if	!defined(SETUPVAR_H)
#define	SETUPVAR_H

#include	<mail/setupTypes.h>

#if	!defined(__cplusplus)
void
    setupVarSetCallback(setupVar_t *setupVar_p, void (*callback)()),
    setupVarNotifyChange(setupVar_t *setupVar_p),
    *setupVarClientData(setupVar_t *setupVar_p),
    setupVarClientDataSet(setupVar_t *setupVar_p, void *clientData_p);

char
    *getSetupFileStruct(char *name, setupWeb_t *setupWeb_p),
    **setupVarChoices(setupVar_t *setupVar_p),
    *setupVarSetValue(setupVar_t *setupVar_p, void *value_p),
    *setupVarDefault(setupVar_t *setupVar_p),
    *setupVarFileName(setupVar_t *setupVar_p),
    *setupVarLabel(setupVar_t *setupVar_p),
    *setupVarName(setupVar_t *setupVar_p),
    *setupVarDescription(setupVar_t *setupVar_p);

int
    setupVarGetValue(setupVar_t *setupVar_p, void *value_p),
    setupVarDefaultValue(setupVar_t *setupVar_p),
    setupVarApplyValue(setupVar_t *setupVar_p),
    setupVarResetValue(setupVar_t *setupVar_p);

setupVariableType_t
    setupVarType(setupVar_t *setupVar_p);
#else
extern "C" void setupVarSetCallback(setupVar_t *setupVar_p, void (*callback)());
extern "C" void *setupVarClientData(setupVar_t *setupVar_p);
extern "C" void setupVarClientDataSet(setupVar_t *setupVar_p, void *clientData_p);
extern "C" char **setupVarChoices(setupVar_t *setupVar_p);
extern "C" char *setupVarSetValue(setupVar_t *setupVar_p, void *value_p);
extern "C" char *setupVarLabel(setupVar_t *setupVar_p);
extern "C" char *setupVarDescription(setupVar_t *setupVar_p);
extern "C" int setupVarGetValue(setupVar_t *setupVar_p, void *value_p);
extern "C" setupVariableType_t setupVarType(setupVar_t *setupVar_p);

#endif
#endif
