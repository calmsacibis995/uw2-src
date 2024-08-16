/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libsetup:libsetup/setupVar.c	1.3"
#include	<stdio.h>
#include	<unistd.h>
#include	<stdlib.h>
#include	<malloc.h>
#include	<string.h>
#include	<ctype.h>
#include	<fcntl.h>
#include	<sys/stat.h>
#include	<mail/table.h>

#define	SETUPVAR_OBJ

#include	<mail/setupTypes.h>

typedef struct setupVar_s
    {
    setupMove_t
	*sv_shortMove,
	*sv_extendedMove;

    setupWeb_t
	*sv_web;

    setupVarOps_t
	*sv_ops;

    void 
	(*sv_clientCallback)(),	/* change callback */
	*sv_clientData;	/*  for client's use */

    char
	*sv_default,
	*sv_fileName,	/*  which file is modified (this var pertains to)*/
	*sv_label,		/*  the label of this variable "line"*/
	*sv_translatedLabel,	/*  the localized label of this variable "line"*/
	*sv_name,		/*  variable name in file*/
	*sv_description;	/*  descriptive paragraph about variable*/
    }	setupVar_t;

#include	<mail/setupVar.h>
#include	"setupFile.h"
#include	<mail/setupWeb.h>
#include	"setupMove.h"

#define	SETUP_DIR	"/usr/lib/setup/"
#define	SETUP_FILE	"/setup.def"

typedef enum eolFlag_e
    {
    ef_atEnd,
    ef_atStart,
    ef_inMiddle
    }	eolFlag_t;
    
static char
    *StringsFile = NULL;

extern int
    DebugLevel;

static table_t
    *TagTable = NULL;

static void
    setupVarFree(setupVar_t *setupVar_p)
	{
	if(setupVar_p != NULL)
	    {
	    setupWebDeleteNode(setupVar_p->sv_web, setupVar_p);
	    if(setupVar_p->sv_shortMove != NULL)
		{
		setupMoveFree(setupVar_p->sv_shortMove);
		}

	    if(setupVar_p->sv_extendedMove != NULL)
		{
		setupMoveFree(setupVar_p->sv_extendedMove);
		}

	    if(setupVar_p->sv_label != NULL) free(setupVar_p->sv_label);
	    if(setupVar_p->sv_translatedLabel != NULL) free(setupVar_p->sv_translatedLabel);
	    if(setupVar_p->sv_default != NULL) free( setupVar_p->sv_default);
	    if(setupVar_p->sv_fileName != NULL) free(setupVar_p->sv_fileName);
	    if(setupVar_p->sv_name != NULL) free(setupVar_p->sv_name);
	    if(setupVar_p->sv_ops != NULL)
		{
		setupVar_p->sv_ops->svop_free(setupVar_p->sv_ops);
		}

	    if(setupVar_p->sv_description != NULL)
		{
		free(setupVar_p->sv_description);
		}

	    free(setupVar_p);
	    }
	}

static setupVar_t
    *setupVarGet(char *label, setupWeb_t *setupWeb_p)
	{
	setupVar_t
	    *result;
	
	if
	    (
		(
		result = (setupVar_t *)tableGetValueByNoCaseString
		    (
		    setupWebTable(setupWeb_p),
		    label
		    )
		) != NULL
	    )
	    {
	    /* The variable was already defined */
	    }
	else if((result = (setupVar_t *)calloc(sizeof(*result), 1)) == NULL)
	    {
	    /* ERROR No Memory */
	    }
	else if((result->sv_label = strdup(label)) == NULL)
	    {
	    /* ERROR No Memory */
	    setupVarFree(result);
	    result = NULL;
	    }
	else
	    {
	    result->sv_extendedMove = setupMoveNew(result);
	    result->sv_shortMove = setupMoveNew(result);
	    tableAddEntry
		(
		setupWebTable(setupWeb_p),
		label,
		result,
		setupVarFree
		);
	    }
	
	return(result);
	}

static eolFlag_t
    EolFlag = ef_atStart;

static char
    *In_p = NULL;

static void
    newLine()
	{
	switch(EolFlag)
	    {
	    case	ef_atStart:
	    case	ef_atEnd:
		{
		break;
		}

	    case	ef_inMiddle:
		{
		while(*In_p != '\n' && *In_p != '\0') In_p++;
		if(*In_p == '\n')
		    {
		    *In_p++ = '\0';
		    }
		else
		    {
		    In_p = NULL;
		    }

		break;
		}
	    }

	EolFlag = ef_atStart;
	}

static char
    *getTag(char *buffer)
	{
	char
	    *out_p,
	    *result;
	
	int
	    done,
	    escape;

	if(buffer != NULL)
	    {
	    In_p = buffer;
	    EolFlag = ef_atStart;
	    }

	if(In_p == NULL)
	    {
	    result = NULL;
	    }
	else for
	    (
	    done = 0;
	    !done;
	    )
	    {
	    switch(EolFlag)
		{
		case	ef_atEnd:
		    {
		    result = NULL;
		    done = 1;
		    break;
		    }
		
		case	ef_atStart:
		    {
		    while(isspace(*In_p)) In_p++;
		    EolFlag = ef_inMiddle;
		    if(*In_p == '#')
			{
			newLine();
			if(In_p == NULL)
			    {
			    result = NULL;
			    done = 1;
			    }

			break;
			}
		    /* Deliberate fall through */
		    }
		
		case	ef_inMiddle:
		    {
		    while(isspace(*In_p) || *In_p == '=') In_p++;
		    if(*In_p == '\0')
			{
			result = NULL;
			In_p = NULL;
			done = 1;
			}
		    else if(*In_p != '"')
			{
			result = In_p;
			while(!isspace(*In_p) && *In_p != '=' && *In_p != '\0' && *In_p != '\n') In_p++;
			if(*In_p == '\n') EolFlag = ef_atEnd;
			*In_p++ = '\0';
			done = 1;
			}
		    else for
			(
			done = 0,
			    escape = 0,
			    In_p++,
			    result = In_p,
			    out_p = In_p;
			!done && *In_p != '\0';
			In_p++
			)
			{
			if(escape)
			    {
			    if(*In_p == '\n')
				{
				*out_p++ = *In_p;
				while(isspace(*++In_p));
				In_p--;
				}
			    else
				{
				*out_p++ = *In_p;
				}

			    escape = 0;
			    }
			else switch(*In_p)
			    {
			    default:
				{
				*out_p++ = *In_p;
				break;
				}
			    
			    case	'\\':
				{
				escape = 1;
				break;
				}

			    case	'"':
				{
				*out_p = '\0';
				done = 1;
				break;
				}

			    case	'\n':
				{
				*out_p++ = (escape)? *In_p: ' ';
				while(isspace(*++In_p));
				In_p--;
				break;
				}
			    }
			}
		    
		    done = 1;
		    break;
		    }
		}
	    }
	
	/*
	(void) fprintf
	    (
	    stderr,
	    "getTag() = %s Exited.\n",
	    (result == NULL)? "NIL": result
	    );
	*/

	return(result);
	}

static void
    doPanelHelpTitle
	(
	setupVar_t **setupVar_p,
	int *extendedFlag,
	setupWeb_t *setupWeb_p
	)
	{
	int
	    stringNumber;

	char
	    tagBuffer[64],
	    *colon,
	    *varName;
	
	if(StringsFile == NULL)
	    {
	    StringsFile = strdup(setupWebFilename(setupWeb_p));
	    }

	if((varName = getTag(NULL)) == NULL)
	    {
	    /* Bad Syntax, ignore */
	    }
	else if((colon = strrchr(varName, ':')) == NULL)
	    {
	    setupWebHelpTitleSet(setupWeb_p, varName);
	    }
	else if((stringNumber = atoi(colon + 1)) < 0)
	    {
	    setupWebHelpTitleSet(setupWeb_p, varName);
	    }
	else
	    {
	    *colon = '\0';
	    (void) sprintf(tagBuffer, "%s:%d", StringsFile, stringNumber);
	    setupWebHelpTitleSet(setupWeb_p, gettxt(tagBuffer, varName));
	    }
	}


static void
    doPanelHelpFile
	(
	setupVar_t **setupVar_p,
	int *extendedFlag,
	setupWeb_t *setupWeb_p
	)
	{
	int
	    stringNumber;

	char
	    tagBuffer[64],
	    *colon,
	    *varName;
	
	if(StringsFile == NULL)
	    {
	    StringsFile = strdup(setupWebFilename(setupWeb_p));
	    }

	if((varName = getTag(NULL)) == NULL)
	    {
	    /* Bad Syntax, ignore */
	    }
	else if((colon = strrchr(varName, ':')) == NULL)
	    {
	    setupWebHelpFileSet(setupWeb_p, varName);
	    }
	else if((stringNumber = atoi(colon + 1)) < 0)
	    {
	    setupWebHelpFileSet(setupWeb_p, varName);
	    }
	else
	    {
	    *colon = '\0';
	    (void) sprintf(tagBuffer, "%s:%d", StringsFile, stringNumber);
	    setupWebHelpFileSet(setupWeb_p, gettxt(tagBuffer, varName));
	    }
	}


static void
    doPanelHelpSection
	(
	setupVar_t **setupVar_p,
	int *extendedFlag,
	setupWeb_t *setupWeb_p
	)
	{
	int
	    stringNumber;

	char
	    tagBuffer[64],
	    *colon,
	    *varName;
	
	if(StringsFile == NULL)
	    {
	    StringsFile = strdup(setupWebFilename(setupWeb_p));
	    }

	if((varName = getTag(NULL)) == NULL)
	    {
	    /* Bad Syntax, ignore */
	    }
	else if((colon = strrchr(varName, ':')) == NULL)
	    {
	    setupWebHelpSectionSet(setupWeb_p, varName);
	    }
	else if((stringNumber = atoi(colon + 1)) < 0)
	    {
	    setupWebHelpSectionSet(setupWeb_p, varName);
	    }
	else
	    {
	    *colon = '\0';
	    (void) sprintf(tagBuffer, "%s:%d", StringsFile, stringNumber);
	    setupWebHelpSectionSet(setupWeb_p, gettxt(tagBuffer, varName));
	    }
	}

static void
    doPanelTitle
	(
	setupVar_t **setupVar_p,
	int *extendedFlag,
	setupWeb_t *setupWeb_p
	)
	{
	int
	    stringNumber;

	char
	    tagBuffer[64],
	    *colon,
	    *varName;
	
	if(StringsFile == NULL)
	    {
	    StringsFile = strdup(setupWebFilename(setupWeb_p));
	    }

	if((varName = getTag(NULL)) == NULL)
	    {
	    /* Bad Syntax, ignore */
	    }
	else if((colon = strrchr(varName, ':')) == NULL)
	    {
	    setupWebTitleSet(setupWeb_p, varName);
	    }
	else if((stringNumber = atoi(colon + 1)) < 0)
	    {
	    setupWebTitleSet(setupWeb_p, varName);
	    }
	else
	    {
	    *colon = '\0';
	    (void) sprintf(tagBuffer, "%s:%d", StringsFile, stringNumber);
	    setupWebTitleSet(setupWeb_p, gettxt(tagBuffer, varName));
	    }

	}

static void
    doTfadmin
	(
	setupVar_t **setupVar_p,
	int *extendedFlag,
	setupWeb_t *setupWeb_p
	)
	{
	int
	    flag;

	char
	    *flagStr;
	
	if((flagStr = getTag(NULL)) == NULL)
	    {
	    /* Bad Syntax, ignore */
	    }
	else 
	    {
	    flag = (atoi(flagStr) != 0)? 1: 0;
	    setupWebTfadminSet(setupWeb_p, flag);
	    }
	}

static void
    doPanelIconTitle
	(
	setupVar_t **setupVar_p,
	int *extendedFlag,
	setupWeb_t *setupWeb_p
	)
	{
	int
	    stringNumber;

	char
	    tagBuffer[64],
	    *colon,
	    *varName;
	
	if(StringsFile == NULL)
	    {
	    StringsFile = strdup(setupWebFilename(setupWeb_p));
	    }

	if((varName = getTag(NULL)) == NULL)
	    {
	    /* Bad Syntax, ignore */
	    }
	else if((colon = strrchr(varName, ':')) == NULL)
	    {
	    setupWebIconTitleSet(setupWeb_p, varName);
	    }
	else if((stringNumber = atoi(colon + 1)) < 0)
	    {
	    setupWebIconTitleSet(setupWeb_p, varName);
	    }
	else
	    {
	    *colon = '\0';
	    (void) sprintf(tagBuffer, "%s:%d", StringsFile, stringNumber);
	    setupWebIconTitleSet(setupWeb_p, gettxt(tagBuffer, varName));
	    }
	}

static void
    doPanelIconFilename
	(
	setupVar_t **setupVar_p,
	int *extendedFlag,
	setupWeb_t *setupWeb_p
	)
	{
	int
	    stringNumber;

	char
	    tagBuffer[64],
	    *colon,
	    *varName;
	
	if(StringsFile == NULL)
	    {
	    StringsFile = strdup(setupWebFilename(setupWeb_p));
	    }

	if((varName = getTag(NULL)) == NULL)
	    {
	    /* Bad Syntax, ignore */
	    }
	else if((colon = strrchr(varName, ':')) == NULL)
	    {
	    setupWebIconFilenameSet(setupWeb_p, varName);
	    }
	else if((stringNumber = atoi(colon + 1)) < 0)
	    {
	    setupWebIconFilenameSet(setupWeb_p, varName);
	    }
	else
	    {
	    *colon = '\0';
	    (void) sprintf(tagBuffer, "%s:%d", StringsFile, stringNumber);
	    setupWebIconFilenameSet(setupWeb_p, gettxt(tagBuffer, varName));
	    }
	}

static void
    doStringsFile
	(
	setupVar_t **setupVar_p,
	int *extendedFlag,
	setupWeb_t *setupWeb_p
	)
	{
	if(DebugLevel > 2)
	    {
	    (void) fprintf
		(
		stderr,
		"doStringsFile(0x%x, 0x%x, 0x%x) Entered.\n",
		(int) setupVar_p,
		(int) extendedFlag,
		(int) setupWeb_p
		);
	    }

	if(StringsFile != NULL)
	    {
	    free(StringsFile);
	    StringsFile = NULL;
	    }
	
	if((StringsFile = getTag(NULL)) != NULL)
	    {
	    StringsFile = strdup(StringsFile);
	    }

	if(DebugLevel > 2) (void) fprintf(stderr, "doStringsFile() Exited.\n");
	}

static void
    doLabel
	(
	setupVar_t **setupVar_p,
	int *extendedFlag,
	setupWeb_t *setupWeb_p
	)
	{
	int
	    labelNumber;

	char
	    tagBuffer[64],
	    *translatedLabel,
	    *colon,
	    *label;
	
	
	if(StringsFile == NULL)
	    {
	    StringsFile = strdup(setupWebFilename(setupWeb_p));
	    }

	if((label = getTag(NULL)) == NULL)
	    {
	    /* Bad Syntax, ignore */
	    }
	else if((colon = strrchr(label, ':')) == NULL)
	    {
	    translatedLabel = label;
	    }
	else if((labelNumber = atoi(colon + 1)) < 0)
	    {
	    translatedLabel = label;
	    }
	else
	    {
	    *colon = '\0';
	    (void) sprintf(tagBuffer, "%s:%d", StringsFile, labelNumber);
	    translatedLabel = gettxt(tagBuffer, label);
	    }

	if((*setupVar_p = setupVarGet(label, setupWeb_p)) != NULL)
	    {
	    if((*setupVar_p)->sv_translatedLabel != NULL)
		{
		free((*setupVar_p)->sv_translatedLabel);
		}

	    (*setupVar_p)->sv_translatedLabel = strdup(translatedLabel);
	    }

	if(colon != NULL) *colon = ':';
	}

static void
    doName
	(
	setupVar_t **setupVar_p,
	int *extendedFlag,
	setupWeb_t *setupWeb_p
	)
	{
	(*setupVar_p)->sv_name = strdup(getTag(NULL));
	if((*setupVar_p)->sv_fileName != NULL)
	    {
	    (*setupVar_p)->sv_ops = setupFileGetVarOps
		(
		(*setupVar_p)->sv_fileName,
		(*setupVar_p)
		);
	    }
	}

static void
    doFile
	(
	setupVar_t **setupVar_p,
	int *extendedFlag,
	setupWeb_t *setupWeb_p
	)
	{
	(*setupVar_p)->sv_fileName = strdup(getTag(NULL));
	if((*setupVar_p)->sv_name != NULL)
	    {
	    (*setupVar_p)->sv_ops = setupFileGetVarOps
		(
		(*setupVar_p)->sv_fileName,
		(*setupVar_p)
		);
	    }
	}

static void
    doDescription
	(
	setupVar_t **setupVar_p,
	int *extendedFlag,
	setupWeb_t *setupWeb_p
	)
	{
	int
	    stringNumber;

	char
	    tagBuffer[64],
	    *colon,
	    *varName;
	
	if(StringsFile == NULL)
	    {
	    StringsFile = strdup(setupWebFilename(setupWeb_p));
	    }

	if((*setupVar_p)->sv_description != NULL)
	    {
	    free((*setupVar_p)->sv_description);
	    (*setupVar_p)->sv_description = NULL;
	    }

	if((varName = getTag(NULL)) == NULL)
	    {
	    /* Bad Syntax, ignore */
	    }
	else if((colon = strrchr(varName, ':')) == NULL)
	    {
	    (*setupVar_p)->sv_description = strdup(varName);
	    }
	else if((stringNumber = atoi(colon + 1)) < 0)
	    {
	    (*setupVar_p)->sv_description = strdup(varName);
	    }
	else
	    {
	    *colon = '\0';
	    (void) sprintf(tagBuffer, "%s:%d", StringsFile, stringNumber);
	    (*setupVar_p)->sv_description = strdup(gettxt(tagBuffer, varName));
	    }
	}

static void
    doDefault
	(
	setupVar_t **setupVar_p,
	int *extendedFlag,
	setupWeb_t *setupWeb_p
	)
	{
	(*setupVar_p)->sv_default = strdup(getTag(NULL));

	if((*setupVar_p)->sv_ops == NULL)
	    {
	    }
	else if((*setupVar_p)->sv_ops->svop_setDefaultValue == NULL)
	    {
	    }
	else
	    {
	    (*setupVar_p)->sv_ops->svop_setDefaultValue
		(
		(*setupVar_p)->sv_ops->svop_handle,
		(*setupVar_p)->sv_default
		);
	    }
	}

static void
    doExitData
	(
	setupVar_t **setupVar_p,
	int *extendedFlag,
	setupWeb_t *setupWeb_p
	)
	{
	char
	    *curTag,
	    *stringList[1024],
	    **newList;

	int
	    stringCount;

	if(DebugLevel > 2)
	    {
	    (void) fprintf
		(
		stderr,
		"doExitData(0x%x, 0x%x, 0x%x) Entered.\n",
		(int) setupVar_p,
		(int) extendedFlag,
		(int) setupWeb_p
		);
	    }

	if((*setupVar_p)->sv_ops->svop_localDataSet != NULL)
	    {
	    if(DebugLevel > 4) (void) fprintf(stderr, "\tGot localDataSet.\n");
	    if(StringsFile == NULL)
		{
		StringsFile = strdup(setupWebFilename(setupWeb_p));
		}

	    for
		(
		stringCount = 0;
		(curTag = getTag(NULL)) != NULL;
		stringCount++
		)
		{
		int
		    stringNumber;

		char
		    tagBuffer[64],
		    *translatedString,
		    *colon;
		
		if((colon = strrchr(curTag, ':')) == NULL)
		    {
		    translatedString = curTag;
		    }
		else if((stringNumber = atoi(colon + 1)) < 0)
		    {
		    translatedString = curTag;
		    }
		else
		    {
		    *colon = '\0';
		    (void) sprintf(tagBuffer, "%s:%d", StringsFile, stringNumber);
		    translatedString = gettxt(tagBuffer, curTag);
		    }

		stringList[stringCount] = strdup(translatedString);
		}
	    
	    stringList[stringCount] = NULL;
	    if
		(
		    (
		    newList = (char **)calloc(stringCount + 1, sizeof(*newList))
		    ) == NULL
		)
		{
		while(stringCount-- > 0) free(stringList[stringCount]);
		}
	    else
		{
		(void) memcpy(newList, stringList, stringCount * sizeof(*stringList));
		(*setupVar_p)->sv_ops->svop_localDataSet
		    (
		    (*setupVar_p)->sv_ops->svop_handle,
		    newList
		    );
		}
	    }

	if(DebugLevel > 2) (void) fprintf(stderr, "doExitData() Exited.\n");
	}

static void
    doShort
	(
	setupVar_t **setupVar_p,
	int *extendedFlag,
	setupWeb_t *setupWeb_p
	)
	{
	*extendedFlag = 0;
	}

static void
    doExtended
	(
	setupVar_t **setupVar_p,
	int *extendedFlag,
	setupWeb_t *setupWeb_p
	)
	{
	*extendedFlag = 1;
	}

static void
    doStart
	(
	setupVar_t **setupVar_p,
	int *extendedFlag,
	setupWeb_t *setupWeb_p
	)
	{
	setupWebStartSet
	   (
	   setupWeb_p,
	   (*extendedFlag)?
	       (*setupVar_p)->sv_extendedMove:
	       (*setupVar_p)->sv_shortMove,
	   *extendedFlag
	   );
	}

static void
    doRight
	(
	setupVar_t **setupVar_p,
	int *extendedFlag,
	setupWeb_t *setupWeb_p
	)
	{
	setupVar_t
	    *destVariable;

	char
	    *varName;
	
	if((varName = getTag(NULL)) == NULL)
	    {
	    /* Bad Syntax, ignore */
	    }
	else if((destVariable = setupVarGet(varName, setupWeb_p)) == NULL)
	    {
	    }
	else if(*extendedFlag)
	    {
	    setupMoveSet
		(
		(*setupVar_p)->sv_extendedMove,
		SM_RIGHT,
		destVariable->sv_extendedMove
		);
	    }
	else
	    {
	    setupMoveSet
		(
		(*setupVar_p)->sv_shortMove,
		SM_RIGHT,
		destVariable->sv_shortMove
		);
	    }
	}

static void
    doLeft
	(
	setupVar_t **setupVar_p,
	int *extendedFlag,
	setupWeb_t *setupWeb_p
	)
	{
	setupVar_t
	    *destVariable;

	char
	    *varName;
	
	if((varName = getTag(NULL)) == NULL)
	    {
	    /* Bad Syntax, ignore */
	    }
	else if((destVariable = setupVarGet(varName, setupWeb_p)) == NULL)
	    {
	    }
	else if(*extendedFlag)
	    {
	    setupMoveSet
		(
		(*setupVar_p)->sv_extendedMove,
		SM_LEFT,
		destVariable->sv_extendedMove
		);
	    }
	else
	    {
	    setupMoveSet
		(
		(*setupVar_p)->sv_shortMove,
		SM_LEFT,
		destVariable->sv_shortMove
		);
	    }
	}

static void
    doUp
	(
	setupVar_t **setupVar_p,
	int *extendedFlag,
	setupWeb_t *setupWeb_p
	)
	{
	setupVar_t
	    *destVariable;

	char
	    *varName;
	
	if((varName = getTag(NULL)) == NULL)
	    {
	    /* Bad Syntax, ignore */
	    }
	else if((destVariable = setupVarGet(varName, setupWeb_p)) == NULL)
	    {
	    }
	else if(*extendedFlag)
	    {
	    setupMoveSet
		(
		(*setupVar_p)->sv_extendedMove,
		SM_UP,
		destVariable->sv_extendedMove
		);
	    }
	else
	    {
	    setupMoveSet
		(
		(*setupVar_p)->sv_shortMove,
		SM_UP,
		destVariable->sv_shortMove
		);
	    }
	}

static void
    doDown
	(
	setupVar_t **setupVar_p,
	int *extendedFlag,
	setupWeb_t *setupWeb_p
	)
	{
	setupVar_t
	    *destVariable;

	char
	    *varName;
	
	if((varName = getTag(NULL)) == NULL)
	    {
	    /* Bad Syntax, ignore */
	    }
	else if((destVariable = setupVarGet(varName, setupWeb_p)) == NULL)
	    {
	    }
	else if(*extendedFlag)
	    {
	    setupMoveSet
		(
		(*setupVar_p)->sv_extendedMove,
		SM_DOWN,
		destVariable->sv_extendedMove
		);
	    }
	else
	    {
	    setupMoveSet
		(
		(*setupVar_p)->sv_shortMove,
		SM_DOWN,
		destVariable->sv_shortMove
		);
	    }
	}

static void
    doTab
	(
	setupVar_t **setupVar_p,
	int *extendedFlag,
	setupWeb_t *setupWeb_p
	)
	{
	setupVar_t
	    *destVariable;

	char
	    *varName;
	
	if((varName = getTag(NULL)) == NULL)
	    {
	    /* Bad Syntax, ignore */
	    }
	else if((destVariable = setupVarGet(varName, setupWeb_p)) == NULL)
	    {
	    }
	else if(*extendedFlag)
	    {
	    setupMoveSet
		(
		(*setupVar_p)->sv_extendedMove,
		SM_TAB,
		destVariable->sv_extendedMove
		);
	    }
	else
	    {
	    setupMoveSet
		(
		(*setupVar_p)->sv_shortMove,
		SM_TAB,
		destVariable->sv_shortMove
		);
	    }
	}

static void
    doNext
	(
	setupVar_t **setupVar_p,
	int *extendedFlag,
	setupWeb_t *setupWeb_p
	)
	{
	setupVar_t
	    *destVariable;

	char
	    *varName;
	
	if((varName = getTag(NULL)) == NULL)
	    {
	    /* Bad Syntax, ignore */
	    }
	else if((destVariable = setupVarGet(varName, setupWeb_p)) == NULL)
	    {
	    }
	else if(*extendedFlag)
	    {
	    setupMoveSet
		(
		(*setupVar_p)->sv_extendedMove,
		SM_NEXT,
		destVariable->sv_extendedMove
		);
	    }
	else
	    {
	    setupMoveSet
		(
		(*setupVar_p)->sv_shortMove,
		SM_NEXT,
		destVariable->sv_shortMove
		);
	    }
	}

char
    *getSetupFileStruct
	(
	char *name,
	setupWeb_t *setupWeb_p
	)
	{
	setupVar_t
	    *curVar_p;
	
	struct stat
	    statStruct;

	int
	    fd = -1,
	    extendedFlag = 0;

	char
	    *result = NULL,
	    *tag,
	    *buffer = NULL,
	    pathName[256];
	
	void
	    (*func_p)();

	if(TagTable == NULL)
	    {
	    TagTable = tableNew();
	    tableAddEntry(TagTable, "panel-title", (void *)doPanelTitle, NULL);
	    tableAddEntry(TagTable, "tfadmin", (void *)doTfadmin, NULL);
	    tableAddEntry(TagTable, "icon-title", (void *)doPanelIconTitle, NULL);
	    tableAddEntry(TagTable, "icon-filename", (void *)doPanelIconFilename, NULL);
	    tableAddEntry(TagTable, "help-title", (void *)doPanelHelpTitle, NULL);
	    tableAddEntry(TagTable, "help-file", (void *)doPanelHelpFile, NULL);
	    tableAddEntry(TagTable, "help-section", (void *)doPanelHelpSection, NULL);
	    tableAddEntry(TagTable, "label", (void *)doLabel, NULL);
	    tableAddEntry(TagTable, "name", (void *)doName, NULL);
	    tableAddEntry(TagTable, "default", (void *)doDefault, NULL);
	    tableAddEntry(TagTable, "file", (void *)doFile, NULL);
	    tableAddEntry(TagTable, "description", (void *)doDescription, NULL);
	    tableAddEntry(TagTable, "localData", (void *)doExitData, NULL);
	    tableAddEntry(TagTable, "short", (void *)doShort, NULL);
	    tableAddEntry(TagTable, "extended", (void *)doExtended, NULL);
	    tableAddEntry(TagTable, "right", (void *)doRight, NULL);
	    tableAddEntry(TagTable, "left", (void *)doLeft, NULL);
	    tableAddEntry(TagTable, "up", (void *)doUp, NULL);
	    tableAddEntry(TagTable, "down", (void *)doDown, NULL);
	    tableAddEntry(TagTable, "tab", (void *)doTab, NULL);
	    tableAddEntry(TagTable, "next", (void *)doNext, NULL);
	    tableAddEntry(TagTable, "start", (void *)doStart, NULL);
	    tableAddEntry(TagTable, "strings-file", (void *)doStringsFile, NULL);
	    }

	if(name == NULL)
	    {
	    result = "";
	    }
	else
	    {
	    (void) sprintf(pathName, "%s%s%s", SETUP_DIR, name, SETUP_FILE);
	    if((fd = open(pathName, O_RDONLY)) < 0)
		{
		perror(pathName);
		result = "";
		}
	    else if(fstat(fd, &statStruct))
		{
		perror(pathName);
		result = "";
		}
	    else if((buffer = calloc(statStruct.st_size + 1, 1)) == NULL)
		{
		result = "";
		}
	    else if(read(fd, buffer, statStruct.st_size) < statStruct.st_size)
		{
		result = "";
		}
	    else for
		(
		tag = getTag(buffer);
		tag != NULL;
		tag = getTag(NULL)
		)
		{
		if(*tag == '\0')
		    {
		    /* Blank Line. */
		    }
		else if
		    (
			(
			func_p = (void (*)())tableGetValueByNoCaseString
			    (
			    TagTable,
			    tag
			    )
			) == NULL
		    )
		    {
		    /* ERROR Bad Flag */
		    break;
		    }
		else
		    {
		    func_p(&curVar_p, &extendedFlag, setupWeb_p);
		    }

		newLine();
		}

	    if(buffer != NULL) free(buffer);
	    if(fd >= 0) (void) close(fd);
	    }

	return(result);
	}

void
*setupVarClientData(setupVar_t *setupVar_p)
    {
    return((setupVar_p == NULL)? NULL: setupVar_p->sv_clientData);
    }

void
setupVarClientDataSet(setupVar_t *setupVar_p, void *clientData_p)
    {
    if(setupVar_p != NULL)
	{
	setupVar_p->sv_clientData = clientData_p;
	}
    }

char
*setupVarDefault(setupVar_t *setupVar_p)
    {
    char
	*result;

    result = (setupVar_p == NULL)? NULL: setupVar_p->sv_default;
    return(result);
    }

char
*setupVarFileName(setupVar_t *setupVar_p)
    {
    return((setupVar_p == NULL)? NULL: setupVar_p->sv_fileName);
    }

char
*setupVarLabel(setupVar_t *setupVar_p)
    {
    return((setupVar_p == NULL)? NULL: setupVar_p->sv_translatedLabel);
    }

char
*setupVarName(setupVar_t *setupVar_p)
    {
    return((setupVar_p == NULL)? NULL: setupVar_p->sv_name);
    }

char
*setupVarDescription(setupVar_t *setupVar_p)
    {
    return((setupVar_p == NULL)? NULL: setupVar_p->sv_description);
    }

char
**setupVarChoices(setupVar_t *setupVar_p)
    {
    return((setupVar_p == NULL)? NULL: setupVar_p->sv_ops->svop_choices);
    }

int
    setupVarGetValue(setupVar_t *setupVar_p, void *value_p)
	{
	int
	    result;

	if(setupVar_p == NULL)
	    {
	    result = -1;
	    }
	else if(setupVar_p->sv_ops->svop_getValue == NULL)
	    {
	    result = -1;
	    }
	else
	    {
	    result = setupVar_p->sv_ops->svop_getValue
		(
		setupVar_p->sv_ops->svop_handle,
		value_p
		);
	    }

	return(result);
	}

char
    *setupVarSetValue(setupVar_t *setupVar_p, void *value_p)
	{
	char
	    *result;

	if(setupVar_p == NULL)
	    {
	    result = "BAD ARGUMENTS TO setupVarSetValue()";
	    }
	else if(setupVar_p->sv_ops->svop_setValue == NULL)
	    {
	    result = "BAD SHARED OBJECT TO setupVarSetValue()";
	    }
	else
	    {
	    result = setupVar_p->sv_ops->svop_setValue
		(
		setupVar_p->sv_ops->svop_handle,
		value_p
		);
	    }

	return(result);
	}

int
    setupVarDefaultValue(setupVar_t *setupVar_p)
	{
	int
	    result;

	if(setupVar_p == NULL)
	    {
	    result = -1;
	    }
	else if(setupVar_p->sv_ops->svop_defaultValue == NULL)
	    {
	    result = -1;
	    }
	else
	    {
	    result = setupVar_p->sv_ops->svop_defaultValue
		(
		setupVar_p->sv_ops->svop_handle
		);
	    }

	return(result);
	}

int
    setupVarResetValue(setupVar_t *setupVar_p)
	{
	int
	    result;

	if(DebugLevel > 2)
	    {
	    (void) fprintf
		(
		stderr,
		"setupVarResetValue(0x%x) Entered.\n",
		(int) setupVar_p
		);
	    }

	if(DebugLevel > 5)
	    {
	    (void) fprintf(stderr, "\tresetting %s.\n", setupVar_p->sv_label);
	    }

	if(setupVar_p == NULL)
	    {
	    result = -1;
	    }
	else if(setupVar_p->sv_ops->svop_resetValue == NULL)
	    {
	    result = -1;
	    }
	else
	    {
	    result = setupVar_p->sv_ops->svop_resetValue
		(
		setupVar_p->sv_ops->svop_handle
		);
	    }

	if(DebugLevel > 2)
	    {
	    (void) fprintf(stderr, "setupVarResetValue() = %d.\n", result);
	    }

	return(result);
	}


setupVariableType_t
    setupVarType(setupVar_t *setupVar_p)
	{
	setupVariableType_t
	    result;

	if(setupVar_p == NULL)
	    {
	    result = svt_none;
	    }
	else {
	    result = setupVar_p->sv_ops->svop_type;
	    }

	return(result);
	}

int
    setupVarApplyValue(setupVar_t *setupVar_p)
	{
	int
	    result;

	if(setupVar_p == NULL)
	    {
	    result = -1;
	    }
	else if(setupVar_p->sv_ops->svop_applyValue == NULL)
	    {
	    result = -1;
	    }
	else
	    {
	    result = setupVar_p->sv_ops->svop_applyValue
		(
		setupVar_p->sv_ops->svop_handle
		);
	    }

	return(result);
	}

void
    setupVarSetCallback(setupVar_t *setupVar_p, void (*callback)())
	{
	if(setupVar_p != NULL)
	    {
	    setupVar_p->sv_clientCallback = callback;
	    }
	}

void
    setupVarNotifyChange(setupVar_t *setupVar_p)
	{
	if(setupVar_p->sv_clientCallback != NULL)
	    {
	    setupVar_p->sv_clientCallback(setupVar_p);
	    }
	}
