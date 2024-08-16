/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libsetup:libsetup/setupWeb.c	1.3"
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<mail/table.h>

#define	SETUPWEB_OBJ

typedef struct setupWeb_s
    {
    char
	*sw_filename,
	*sw_title,
	*sw_helpTitle,
	*sw_helpFile,
	*sw_helpSection,
	*sw_iconTitle,
	*sw_iconFilename,
	*sw_errorString,
	*sw_icon;

    int
	sw_tfadmin,
	sw_extended;

    table_t
	*sw_fileTable,
	*sw_varTable;

    struct setupMove_s
	*sw_startExtended,
	*sw_startShort,
	*sw_current;
    }	setupWeb_t;

#include	<mail/setupWeb.h>
#include	<mail/setupVar.h>
#include	"setupMove.h"
#include	"setupFile.h"

int
    DebugLevel = 0;

static char
    *ErrorString = NULL;

static int
    Initialized = 0;

char
    *setupWebErrorString(setupWeb_t *setupWeb_p)
	{
	return((setupWeb_p == NULL)? ErrorString: setupWeb_p->sw_errorString);
	}

setupVar_t
    *setupWebMoveExtended(setupWeb_t *setupWeb_p, int flag)
	{
	setupVar_t
	    *result;

	if(DebugLevel > 4)
	    {
	    (void) fprintf
		(
		stderr,
		"setupWebMoveExtended(0x%x, %d) Entered.\n",
		(int) setupWeb_p,
		flag
		);
	    }

	if(setupWeb_p == NULL)
	    {
	    result = NULL;
	    }
	else if
	    (
	    setupWeb_p->sw_extended == flag
		&& (result = setupMoveVar(setupWeb_p->sw_current)) != NULL
	    )
	    {
	    /* No change */
	    }
	else
	    {
	    setupWeb_p->sw_extended = flag;
	    setupWeb_p->sw_current = (flag)?
		setupWeb_p->sw_startExtended:
		setupWeb_p->sw_startShort;

	    result = setupMoveVar(setupWeb_p->sw_current);
	    }
	
	if(DebugLevel > 4)
	    {
	    (void) fprintf(stderr, "setupWebMoveExtended() = 0x%x Exited.\n", (int) result);
	    }

	return(result);
	}

setupVar_t
    *setupWebMoveLeft(setupWeb_t *setupWeb_p)
	{
	setupVar_t
	    *result;

	if(setupWeb_p == NULL)
	    {
	    result = NULL;
	    }
	else if
	    (
		(
		setupWeb_p->sw_current = setupMove
		    (
		    setupWeb_p->sw_current,
		    SM_LEFT
		    )
		) == NULL
	    )
	    {
	    setupWeb_p->sw_current = (setupWeb_p->sw_extended)?
		setupWeb_p->sw_startExtended:
		setupWeb_p->sw_startShort;

	    result = setupMoveVar(setupWeb_p->sw_current);
	    }
	else
	    {
	    result = setupMoveVar(setupWeb_p->sw_current);
	    }

	return(result);
	}

setupVar_t
    *setupWebMoveRight(setupWeb_t *setupWeb_p)
	{
	setupVar_t
	    *result;

	if(setupWeb_p == NULL)
	    {
	    result = NULL;
	    }
	else if
	    (
		(
		setupWeb_p->sw_current = setupMove
		    (
		    setupWeb_p->sw_current,
		    SM_RIGHT
		    )
		) == NULL
	    )
	    {
	    setupWeb_p->sw_current = (setupWeb_p->sw_extended)?
		setupWeb_p->sw_startExtended:
		setupWeb_p->sw_startShort;

	    result = setupMoveVar(setupWeb_p->sw_current);
	    }
	else
	    {
	    result = setupMoveVar(setupWeb_p->sw_current);
	    }

	return(result);
	}

setupVar_t
    *setupWebMoveUp(setupWeb_t *setupWeb_p)
	{
	setupVar_t
	    *result;

	if(setupWeb_p == NULL)
	    {
	    result = NULL;
	    }
	else if
	    (
		(
		setupWeb_p->sw_current = setupMove
		    (
		    setupWeb_p->sw_current,
		    SM_UP
		    )
		) == NULL
	    )
	    {
	    setupWeb_p->sw_current = (setupWeb_p->sw_extended)?
		setupWeb_p->sw_startExtended:
		setupWeb_p->sw_startShort;

	    result = setupMoveVar(setupWeb_p->sw_current);
	    }
	else
	    {
	    result = setupMoveVar(setupWeb_p->sw_current);
	    }

	return(result);
	}


setupVar_t
    *setupWebMoveDown(setupWeb_t *setupWeb_p)
	{
	setupVar_t
	    *result;

	if(setupWeb_p == NULL)
	    {
	    result = NULL;
	    }
	else if
	    (
		(
		setupWeb_p->sw_current = setupMove
		    (
		    setupWeb_p->sw_current,
		    SM_DOWN
		    )
		) == NULL
	    )
	    {
	    setupWeb_p->sw_current = (setupWeb_p->sw_extended)?
		setupWeb_p->sw_startExtended:
		setupWeb_p->sw_startShort;

	    result = setupMoveVar(setupWeb_p->sw_current);
	    }
	else
	    {
	    result = setupMoveVar(setupWeb_p->sw_current);
	    }

	return(result);
	}


setupVar_t
    *setupWebMoveTab(setupWeb_t *setupWeb_p)
	{
	setupVar_t
	    *result;

	if(setupWeb_p == NULL)
	    {
	    result = NULL;
	    }
	else if
	    (
		(
		setupWeb_p->sw_current = setupMove
		    (
		    setupWeb_p->sw_current,
		    SM_TAB
		    )
		) == NULL
	    )
	    {
	    setupWeb_p->sw_current = (setupWeb_p->sw_extended)?
		setupWeb_p->sw_startExtended:
		setupWeb_p->sw_startShort;

	    result = setupMoveVar(setupWeb_p->sw_current);
	    }
	else
	    {
	    result = setupMoveVar(setupWeb_p->sw_current);
	    }

	return(result);
	}

setupVar_t
    *setupWebMoveNext(setupWeb_t *setupWeb_p)
	{
	setupVar_t
	    *result;

	if(setupWeb_p == NULL)
	    {
	    result = NULL;
	    }
	else if
	    (
		(
		setupWeb_p->sw_current = setupMove
		    (
		    setupWeb_p->sw_current,
		    SM_NEXT
		    )
		) == NULL
	    )
	    {
	    result = NULL;
	    }
	else
	    {
	    result = setupMoveVar(setupWeb_p->sw_current);
	    }
	
	return(result);
	}

void
    setupWebFree(setupWeb_t *setupWeb_p)
	{
	if(DebugLevel > 4)
	    {
	    (void) fprintf
		(
		stderr,
		"setupWebFree(0x%x) Entered.\n",
		(int) setupWeb_p
		);
	    }

	if(setupWeb_p != NULL)
	    {
	    if(setupWeb_p->sw_varTable != NULL)
		{
		tableFree(setupWeb_p->sw_varTable);
		}

	    if(setupWeb_p->sw_fileTable != NULL)
		{
		tableFree(setupWeb_p->sw_fileTable);
		}

	    if(setupWeb_p->sw_filename != NULL) free(setupWeb_p->sw_filename);
	    if(setupWeb_p->sw_title != NULL) free(setupWeb_p->sw_title);
	    if(setupWeb_p->sw_iconTitle != NULL) free(setupWeb_p->sw_iconTitle);
	    if(setupWeb_p->sw_iconFilename != NULL) free(setupWeb_p->sw_iconFilename);
	    if(setupWeb_p->sw_icon != NULL) free(setupWeb_p->sw_icon);

	    free(setupWeb_p);
	    }

	if(DebugLevel > 4) (void) fprintf(stderr, "setupWebFree() Exited.\n");
	}

setupWeb_t
    *setupWebNew(char *filename)
	{
	setupWeb_t
	    *result;
	
	if(DebugLevel > 4)
	    {
	    (void) fprintf
		(
		stderr,
		"setupWebNew(%s) Entered.\n",
		filename
		);
	    }

	if(!Initialized) setupWebInit(0);

	if((result = (setupWeb_t *)calloc(sizeof(*result), 1)) == NULL)
	    {
	    }
	else if((result->sw_varTable = tableNew()) == NULL)
	    {
	    setupWebFree(result);
	    result = NULL;
	    }
	else if((result->sw_filename = strdup(filename)) == NULL)
	    {
	    setupWebFree(result);
	    result = NULL;
	    }
	else if
	    (
		(
		ErrorString = getSetupFileStruct
		    (
		    filename,
		    result
		    )
		) != NULL
	    )
	    {
	    setupWebFree(result);
	    result = NULL;
	    }
	else
	    {
	    if(result->sw_startShort != NULL)
		{
		}
	    else if(result->sw_startExtended != NULL)
		{
		result->sw_startShort = result->sw_startExtended;
		result->sw_startExtended = NULL;
		}

	    result->sw_current = result->sw_startShort;
	    result->sw_extended = 0;
	    }

	if(DebugLevel > 4)
	    {
	    (void) fprintf
		(
		stderr,
		"setupWebNew() = 0x%x Exited.\n",
		(int) result
		);
	    }

	return(result);
	}

char
    *setupWebFilename(setupWeb_t *web_p)
	{
	return((web_p == NULL)? NULL: web_p->sw_filename);
	}

char
    *setupWebTitle(setupWeb_t *web_p)
	{
	return((web_p == NULL)? NULL: web_p->sw_title);
	}

char
    *setupWebHelpTitle(setupWeb_t *web_p)
	{
	return((web_p == NULL)? NULL: web_p->sw_helpTitle);
	}

char
    *setupWebHelpFile(setupWeb_t *web_p)
	{
	return((web_p == NULL)? NULL: web_p->sw_helpFile);
	}

char
    *setupWebHelpSection(setupWeb_t *web_p)
	{
	return((web_p == NULL)? NULL: web_p->sw_helpSection);
	}

char
    *setupWebIconTitle(setupWeb_t *web_p)
	{
	return((web_p == NULL)? NULL: web_p->sw_iconTitle);
	}

char
    *setupWebIconFilename(setupWeb_t *web_p)
	{
	return((web_p == NULL)? NULL: web_p->sw_iconFilename);
	}

void
    setupWebTfadminSet(setupWeb_t *web_p, int flag)
	{
	if(!web_p == NULL)
	    {
	    web_p->sw_tfadmin = flag;
	    }
	}

void
    setupWebTitleSet(setupWeb_t *web_p, char *str)
	{
	if(web_p == NULL || str == NULL)
	    {
	    }
	else if((str = strdup(str)) == NULL)
	    {
	    }
	else
	    {
	    if(web_p->sw_title != NULL) free(web_p->sw_title);
	    web_p->sw_title = str;
	    }
	}

void
    setupWebHelpTitleSet(setupWeb_t *web_p, char *str)
	{
	if(web_p == NULL || str == NULL)
	    {
	    }
	else if((str = strdup(str)) == NULL)
	    {
	    }
	else
	    {
	    if(web_p->sw_helpTitle != NULL) free(web_p->sw_helpTitle);
	    web_p->sw_helpTitle = str;
	    }
	}

void
    setupWebHelpFileSet(setupWeb_t *web_p, char *str)
	{
	if(web_p == NULL || str == NULL)
	    {
	    }
	else if((str = strdup(str)) == NULL)
	    {
	    }
	else
	    {
	    if(web_p->sw_helpFile != NULL) free(web_p->sw_helpFile);
	    web_p->sw_helpFile = str;
	    }
	}

void
    setupWebHelpSectionSet(setupWeb_t *web_p, char *str)
	{
	if(web_p == NULL || str == NULL)
	    {
	    }
	else if((str = strdup(str)) == NULL)
	    {
	    }
	else
	    {
	    if(web_p->sw_helpSection != NULL) free(web_p->sw_helpSection);
	    web_p->sw_helpSection = str;
	    }
	}

void
    setupWebIconTitleSet(setupWeb_t *web_p, char *str)
	{
	if(web_p == NULL || str == NULL)
	    {
	    }
	else if((str = strdup(str)) == NULL)
	    {
	    }
	else
	    {
	    if(web_p->sw_iconTitle != NULL) free(web_p->sw_iconTitle);
	    web_p->sw_iconTitle = str;
	    }
	}

void
    setupWebIconFilenameSet(setupWeb_t *web_p, char *str)
	{
	if(web_p == NULL || str == NULL)
	    {
	    }
	else if((str = strdup(str)) == NULL)
	    {
	    }
	else
	    {
	    if(web_p->sw_iconFilename != NULL) free(web_p->sw_iconFilename);
	    web_p->sw_iconFilename = str;
	    }
	}

void
    setupWebStartSet(setupWeb_t *web_p, setupMove_t *move_p, int extendedFlag)
	{
	if(DebugLevel > 4)
	    {
	    (void) fprintf
		(
		stderr,
		"setupWebStartSet(0x%x, 0x%x, %d) Entered.\n",
		(int) web_p,
		(int) move_p,
		extendedFlag
		);
	    }

	if(extendedFlag)
	    {
	    web_p->sw_startExtended = move_p;
	    }
	else
	    {
	    web_p->sw_startShort = move_p;
	    }

	if(DebugLevel > 4) (void) fprintf(stderr, "setupWebStartSet() Exited.\n");
	}

void
    setupWebDeleteNode(setupWeb_t *setupWeb_p, setupVar_t *node_p)
	{
	if(setupWeb_p == NULL)
	    {
	    }
	else if(setupWeb_p->sw_varTable != NULL)
	    {
	    (void) tableDeleteEntryByValue(setupWeb_p->sw_varTable, node_p);
	    }
	}

table_t
    *setupWebTable(setupWeb_t *setupWeb_p)
	{
	return((setupWeb_p == NULL)? NULL: setupWeb_p->sw_varTable);
	}

setupVar_t
    *setupWebCurVar(setupWeb_t *setupWeb_p)
	{
	return((setupWeb_p == NULL)? NULL: setupMoveVar(setupWeb_p->sw_current));
	}

setupVar_t
    *setupWebMoveTop(setupWeb_t *setupWeb_p)
	{
	setupVar_t
	    *result;

	if(setupWeb_p == NULL)
	    {
	    result = NULL;
	    }
	else
	    {
	    setupWeb_p->sw_current = (setupWeb_p->sw_extended)?
		setupWeb_p->sw_startExtended:
		setupWeb_p->sw_startShort;

	    result = setupMoveVar(setupWeb_p->sw_current);
	    }
	
	return(result);
	}

static void
    getFileFunc(setupVariableHandle_t *varHandle_p, table_t *table_p)
	{
	char
	    *fileName;
	
	fileName = setupVarFileName(varHandle_p);
	if(tableGetValueByString(table_p, fileName) == NULL)
	    {
	    tableAddEntry
		(
		table_p,
		fileName,
		(void *)setupFileOpen(fileName, "w"),
		NULL
		);
	    }
	}

void
    setupWebDoFunc(setupWeb_t *setupWeb_p, void (*func)(), void *localData_p)
	{
	if(setupWeb_p == NULL)
	    {
	    }
	else if(setupWeb_p->sw_varTable == NULL)
	    {
	    }
	else
	    {
	    tableDoForEachEntry
		(
		setupWeb_p->sw_varTable,
		func,
		localData_p
		);
	    }
	}

void
    setupWebApply(setupWeb_t *setupWeb_p)
	{
	if(setupWeb_p == NULL)
	    {
	    }
	else if(setupWeb_p->sw_varTable == NULL)
	    {
	    }
	else if(setupWeb_p->sw_fileTable != NULL)
	    {
	    setupWebDoFunc(setupWeb_p, (void (*)())setupVarApplyValue, NULL);

	    tableDoForEachEntry
		(
		setupWeb_p->sw_fileTable,
		(void (*)())setupFileApply,
		NULL
		);
	    }
	else if((setupWeb_p->sw_fileTable = tableNew()) != NULL)
	    {
	    setupWebDoFunc(setupWeb_p, (void (*)())setupVarApplyValue, NULL);
	    setupWebDoFunc(setupWeb_p, getFileFunc, setupWeb_p->sw_fileTable);

	    tableDoForEachEntry
		(
		setupWeb_p->sw_fileTable,
		(void(*)())setupFileApply,
		NULL
		);
	    }
	}


static int
    tfadminOk(char *execName)
	{
	char    buf[256];

	sprintf(buf, "/sbin/tfadmin -t %s 2>/dev/null", execName);
	return (system (buf) == 0);
	}

int
    setupWebPerm(setupWeb_t *setupWeb_p)
	{
	int
	    result = 1;

	if(DebugLevel > 4)
	    {
	    (void) fprintf
		(
		stderr,
		"setupWebPerm(0x%x) Entered.\n",
		(int) setupWeb_p
		);
	    }
	if(setupWeb_p == NULL)
	    {
	    }
	else if(setupWeb_p->sw_tfadmin && !tfadminOk(setupWeb_p->sw_filename))
	    {
	    result = 0;
	    }
	else if(setupWeb_p->sw_fileTable != NULL)
	    {
	    tableDoForEachEntry
		(
		setupWeb_p->sw_fileTable,
		(void (*)())setupFilePerm,
		NULL
		);
	    }
	else if((setupWeb_p->sw_fileTable = tableNew()) != NULL)
	    {
	    /*setupWebDoFunc(setupWeb_p, (void (*)())setupVarApplyValue, NULL);*/
	    setupWebDoFunc(setupWeb_p, getFileFunc, setupWeb_p->sw_fileTable);

	    tableDoForEachEntry
		(
		setupWeb_p->sw_fileTable,
		(void(*)())setupFilePerm,
		&result
		);
	    }

	if(DebugLevel > 4)
	    {
	    (void) fprintf
		(
		stderr,
		"setupWebPerm() = %d Exited.\n",
		result
		);
	    }

	return(result);
	}

void
    setupWebDefault(setupWeb_t *setupWeb_p)
	{
	setupWebDoFunc(setupWeb_p, (void (*)())setupVarDefaultValue, NULL);
	}

void
    setupWebReset(setupWeb_t *setupWeb_p)
	{
	setupWebDoFunc(setupWeb_p, (void (*)())setupVarResetValue, NULL);
	}

void
    setupWebInit(int debugLevel)
	{
	char
	    *debugString;

	Initialized = 1;
	DebugLevel = debugLevel;
	if(DebugLevel != 0)
	    {
	    }
	else if((debugString = getenv("DEBUG_LEVEL")) != NULL)
	    {
	    DebugLevel = atoi(debugString);
	    }

	setupFileInit();
	}

