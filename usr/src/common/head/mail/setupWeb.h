/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:mail/setupWeb.h	1.2"
#if	!defined(SETUPWEB_H)
#define	SETUPWEB_H

#include	<mail/setupTypes.h>

#if	!defined(__cplusplus)
#include	<mail/table.h>
setupVar_t
    *setupWebMoveExtended(setupWeb_t *setupWeb_p, int flag),
    *setupWebMoveTop(setupWeb_t *setupWeb_p),
    *setupWebMoveLeft(setupWeb_t *setupWeb_p),
    *setupWebMoveRight(setupWeb_t *setupWeb_p),
    *setupWebMoveUp(setupWeb_t *setupWeb_p),
    *setupWebMoveDown(setupWeb_t *setupWeb_p),
    *setupWebMoveTab(setupWeb_t *setupWeb_p),
    *setupWebMoveNext(setupWeb_t *setupWeb_p),
    *setupWebCurVar(setupWeb_t *setupWeb);

setupWeb_t
    *setupWebNew(char *filename);

int
    setupWebPerm(setupWeb_t *web_p);

char
    *setupWebErrorString(setupWeb_t *web_p),
    *setupWebHelpTitle(setupWeb_t *web_p),
    *setupWebHelpFile(setupWeb_t *web_p),
    *setupWebHelpSection(setupWeb_t *web_p),
    *setupWebIconTitle(setupWeb_t *web_p),
    *setupWebIconFilename(setupWeb_t *web_p),
    *setupWebTitle(setupWeb_t *web_p),
    *setupWebFilename(setupWeb_t *web_p);

void
    setupWebFree(setupWeb_t *setupWeb_p),
    setupWebApply(setupWeb_t *setupWeb_p),
    setupWebReset(setupWeb_t *setupWeb_p),
    setupWebDefault(setupWeb_t *setupWeb_p),
    setupWebDoFunc(setupWeb_t *setupWeb_p, void (*func)(), void *localData_p),
    setupWebInit(int debugLevel),
    setupWebStartSet(setupWeb_t *web_p, setupMove_t *move_p, int extendedFlag),
    setupWebTfadminSet(setupWeb_t *web_p, int flag),
    setupWebIconFilenameSet(setupWeb_t *web_p, char *str),
    setupWebHelpTitleSet(setupWeb_t *web_p, char *str),
    setupWebHelpFileSet(setupWeb_t *web_p, char *str),
    setupWebHelpSectionSet(setupWeb_t *web_p, char *str),
    setupWebIconTitleSet(setupWeb_t *web_p, char *str),
    setupWebTitleSet(setupWeb_t *web_p, char *str),
    setupWebDeleteNode(setupWeb_t *setupWeb_p, setupVar_t *node_p);

table_t
    *setupWebTable(setupWeb_t *setupWeb_p);
#else
extern "C" setupVar_t *setupWebMoveExtended(setupWeb_t *setupWeb_p, int flag);
extern "C" setupVar_t *setupWebMoveTop(setupWeb_t *setupWeb_p);
extern "C" setupVar_t *setupWebMoveLeft(setupWeb_t *setupWeb_p);
extern "C" setupVar_t *setupWebMoveRight(setupWeb_t *setupWeb_p);
extern "C" setupVar_t *setupWebMoveUp(setupWeb_t *setupWeb_p);
extern "C" setupVar_t *setupWebMoveDown(setupWeb_t *setupWeb_p);
extern "C" setupVar_t *setupWebMoveTab(setupWeb_t *setupWeb_p);
extern "C" setupVar_t *setupWebMoveNext(setupWeb_t *setupWeb_p);
extern "C" setupVar_t *setupWebCurVar(setupWeb_t *setupWeb);
extern "C" setupWeb_t *setupWebFree(setupWeb_t *setupWeb_p);
extern "C" setupWeb_t *setupWebNew(char *filename);
extern "C" int setupWebPerm(setupWeb_t *web_p);
extern "C" char *setupWebHelpTitle(setupWeb_t *web_p);
extern "C" char *setupWebHelpFile(setupWeb_t *web_p);
extern "C" char *setupWebHelpSection(setupWeb_t *web_p);
extern "C" char *setupWebIconTitle(setupWeb_t *web_p);
extern "C" char *setupWebIconFilename(setupWeb_t *web_p);
extern "C" char *setupWebTitle(setupWeb_t *web_p);
extern "C" char *setupWebFilename(setupWeb_t *web_p);
extern "C" void setupWebApply(setupWeb_t *setupWeb_p);
extern "C" void setupWebReset(setupWeb_t *setupWeb_p);
extern "C" void setupWebDefault(setupWeb_t *setupWeb_p);
extern "C" void setupWebDoFunc(setupWeb_t *setupWeb_p, void (*func)(), void *localData_p);
extern "C" void setupWebInit(int debugLevel);
#endif

#endif
