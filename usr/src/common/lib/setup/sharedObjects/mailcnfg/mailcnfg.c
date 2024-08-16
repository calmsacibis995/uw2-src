/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libsetup:sharedObjects/mailcnfg/mailcnfg.c	1.9"
#include	<stdio.h>
#include	<unistd.h>
#include	<stdlib.h>
#include	<malloc.h>
#include	<string.h>
#include	<pwd.h>
#include	<mail/table.h>

#define	SETUPSPECIFICFILE_OBJ

typedef struct setupVariableHandle_s setupVariableHandle_t;
typedef struct setupFileHandle_s setupFileHandle_t;

#include	<mail/setupTypes.h>
#include	<mail/setupVar.h>

typedef struct menuEntry_s
    {
    char
	*me_name;
    
    int
	me_msgNum;
    }	menuEntry_t;

typedef struct fileVariableType_s
    {
    char
	*fvt_name,
	**fvt_translatedChoices,
	**fvt_fileChoices,
	*(*fvt_setValue)();
    
    int
	(*fvt_getValue)(),
	fvt_menuLength;

    table_t
	*fvt_menuTable;

    setupVariableType_t
	fvt_type;

    void
	(*fvt_localDataSet)();
    }	fileVariableType_t;

struct setupVariableHandle_s
    {
    char
	*svh_name,
	*svh_defaultValue,
	*svh_originalValue,
	*svh_currentValue,
	**svh_fileChoices;

    int
	svh_menuLength;

    table_t
	*svh_fileTable,
	*svh_menuTable,
	*svh_variableHandleTable;

    setupVarOps_t
	svh_ops;
    };

struct setupFileHandle_s
    {
    char
	*sfh_pathName;

    table_t
	*sfh_variableTable;
    };

static int
    DebugLevel = 0;

static table_t
    *FileVariableTable = NULL;

static menuEntry_t
    MailFileMenuEntries[] =
	{
	"Default", 0,
	"Yes", 1,
	"No", 2,
	NULL, -1
	};
    
static void
    setupFileHandleFree(setupFileHandle_t *handle_p)
	{
	if(handle_p != NULL)
	    {
	    if(handle_p->sfh_pathName != NULL) free(handle_p->sfh_pathName);
	    if(handle_p->sfh_variableTable != NULL)
		{
		tableFree(handle_p->sfh_variableTable);
		}
	    }
	}

static setupFileHandle_t
    *setupFileHandleNew(char *pathname)
	{
	setupFileHandle_t
	    *result;
	
	if((result = (setupFileHandle_t *)calloc(sizeof(*result), 1)) == NULL)
	    {
	    }
	else if((result->sfh_pathName = strdup(pathname)) == NULL)
	    {
	    setupFileHandleFree(result);
	    result = NULL;
	    }
	else if((result->sfh_variableTable = tableNew()) == NULL)
	    {
	    setupFileHandleFree(result);
	    result = NULL;
	    }
	else
	    {
	    }
	
	return(result);
	}

static int
    getMenuValue(setupVariableHandle_t *data_p, int *value)
	{
	menuEntry_t
	    *menuEntry_p;

	int
	    result = -1;

	char
	    **position;

	if(value == NULL)
	    {
	    menuEntry_p = NULL;
	    }
	else if(data_p == NULL)
	    {
	    menuEntry_p = NULL;
	    }
	else if(data_p->svh_currentValue == NULL)
	    {
	    menuEntry_p = NULL;
	    *value = 0;
	    result = 0;
	    }
	else
	    {
	    menuEntry_p = (menuEntry_t *)tableGetValueByString
		(
		data_p->svh_menuTable,
		data_p->svh_currentValue
		);
	    }

	if(menuEntry_p == NULL)
	    {
	    }
	else
	    {
	    *value = menuEntry_p->me_msgNum;
	    result = 0;
	    }

	return(result);
	}

static char
    *setMenuValue(setupVariableHandle_t *data_p, int *value)
	{
	char
	    *result;

	if(data_p == NULL)
	    {
	    result = "BAD CALL TO SET MENU VALUE";
	    }
	else if(value == NULL)
	    {
	    if(data_p->svh_currentValue != NULL)
		{
		free(data_p->svh_currentValue);
		data_p->svh_currentValue = NULL;
		tableDoForEachEntry(data_p->svh_variableHandleTable, setupVarNotifyChange, NULL);
		}

	    result = NULL;
	    }
	else if(*value >= data_p->svh_menuLength || *value < 0)
	    {
	    result = "No such menu entry.";
	    }
	else
	    {
	    if(data_p->svh_currentValue != NULL) free(data_p->svh_currentValue);
	    data_p->svh_currentValue = (*value == 0)?
		NULL:
		strdup(data_p->svh_fileChoices[*value]);

	    tableDoForEachEntry(data_p->svh_variableHandleTable, setupVarNotifyChange, NULL);
	    result = NULL;
	    }

	return(result);
	}

static int
    getIntegerValue(setupVariableHandle_t *data_p, int *value)
	{
	int
	    result;

	if(value == NULL)
	    {
	    result = -1;
	    }
	else if(data_p == NULL)
	    {
	    result = -1;
	    }
	else if(data_p->svh_currentValue != NULL)
	    {
	    *value = atoi(data_p->svh_currentValue);
	    result = 0;
	    }
	else
	    {
	    result = -1;
	    }

	return(result);
	}

static char
    *setIntegerValue(setupVariableHandle_t *data_p, int *value)
	{
	char
	    *result;

	char
	    buffer[32];

	if(data_p == NULL)
	    {
	    result = "BAD CALL TO SET INTEGER VALUE";
	    }
	else if(value == NULL)
	    {
	    if(data_p->svh_currentValue != NULL)
		{
		free(data_p->svh_currentValue);
		data_p->svh_currentValue = NULL;
		tableDoForEachEntry(data_p->svh_variableHandleTable, setupVarNotifyChange, NULL);
		}

	    result = NULL;
	    }
	else
	    {
	    if(data_p->svh_currentValue != NULL) free(data_p->svh_currentValue);
	    (void) sprintf(buffer, "%d", *value);
	    data_p->svh_currentValue = strdup(buffer);
	    tableDoForEachEntry(data_p->svh_variableHandleTable, setupVarNotifyChange, NULL);
	    result = NULL;
	    }

	return(result);
	}


static int
    getFlagValue(setupVariableHandle_t *data_p, int *value)
	{
	int
	    result;

	if(value == NULL)
	    {
	    result = -1;
	    }
	else if(data_p == NULL)
	    {
	    result = -1;
	    }
	else if(data_p->svh_currentValue == NULL)
	    {
	    *value = 0;
	    result = 0;
	    }
	else
	    {
	    *value = 1;
	    result = 0;
	    }

	return(result);
	}

static char
    *setFlagValue(setupVariableHandle_t *data_p, int *value)
	{
	char
	    *result;

	if(value == NULL)
	    {
	    result = "BAD CALL TO SET FLAG VALUE";
	    }
	if(data_p == NULL)
	    {
	    result = "BAD CALL TO SET FLAG VALUE";
	    }
	else
	    {
	    if(data_p->svh_currentValue != NULL) free(data_p->svh_currentValue);
	    data_p->svh_currentValue = (*value)? strdup("1"): NULL;
	    tableDoForEachEntry(data_p->svh_variableHandleTable, setupVarNotifyChange, NULL);
	    result = NULL;
	    }

	return(result);
	}

static int
    getInvFlagValue(setupVariableHandle_t *data_p, int *value)
	{
	int
	    result;

	if(value == NULL)
	    {
	    result = -1;
	    }
	else if(data_p == NULL)
	    {
	    result = -1;
	    }
	else if(data_p->svh_currentValue == NULL)
	    {
	    *value = 1;
	    result = 0;
	    }
	else
	    {
	    *value = 0;
	    result = 0;
	    }

	return(result);
	}

static char
    *setInvFlagValue(setupVariableHandle_t *data_p, int *value)
	{
	char
	    *result;

	if(value == NULL)
	    {
	    result = "BAD CALL TO SET FLAG VALUE";
	    }
	if(data_p == NULL)
	    {
	    result = "BAD CALL TO SET FLAG VALUE";
	    }
	else
	    {
	    if(data_p->svh_currentValue != NULL) free(data_p->svh_currentValue);
	    data_p->svh_currentValue = (*value)? NULL: strdup("1");
	    tableDoForEachEntry(data_p->svh_variableHandleTable, setupVarNotifyChange, NULL);
	    result = NULL;
	    }

	return(result);
	}

static int
    getStringValue(setupVariableHandle_t *data_p, char **string)
	{
	int
	    result;

	if(string == NULL)
	    {
	    result = -1;
	    }
	if(data_p == NULL)
	    {
	    result = -1;
	    }
	else
	    {
	    *string = data_p->svh_currentValue;
	    result = 0;
	    }

	return(result);
	}

static char
    *setStringValue(setupVariableHandle_t *data_p, char **string)
	{
	char
	    *result;

	if(string == NULL)
	    {
	    result = "BAD CALL TO SET STRING VALUE";
	    }
	if(data_p == NULL)
	    {
	    result = "BAD CALL TO SET STRING VALUE";
	    }
	else if(*string == NULL)
	    {
	    if(data_p->svh_currentValue != NULL)
		{
		free(data_p->svh_currentValue);
		tableDoForEachEntry(data_p->svh_variableHandleTable, setupVarNotifyChange, NULL);
		data_p->svh_currentValue = NULL;
		}

	    result = NULL;
	    }
	else
	    {
	    if(data_p->svh_currentValue != NULL) free(data_p->svh_currentValue);
	    data_p->svh_currentValue = strdup(*string);
	    tableDoForEachEntry(data_p->svh_variableHandleTable, setupVarNotifyChange, NULL);
	    result = NULL;
	    }

	return(result);
	}


static int
    getFailsafeStringValue(setupVariableHandle_t *data_p, char **string)
	{
	int
	    result;

	if(string == NULL)
	    {
	    result = -1;
	    }
	if(data_p == NULL)
	    {
	    result = -1;
	    }
	else if(data_p->svh_currentValue == NULL)
	    {
	    *string = NULL;
	    result = 0;
	    }
	else if((*string = strchr(data_p->svh_currentValue, '@')) == NULL)
	    {
	    *string = NULL;
	    result = 0;
	    }
	else
	    {
	    (*string)++;
	    result = 0;
	    }

	return(result);
	}

static char
    *setFailsafeStringValue(setupVariableHandle_t *data_p, char **string)
	{
	char
	    *result;

	if(string == NULL)
	    {
	    result = "BAD CALL TO SET STRING VALUE";
	    }
	if(data_p == NULL)
	    {
	    result = "BAD CALL TO SET STRING VALUE";
	    }
	else if(*string == NULL)
	    {
	    if(data_p->svh_currentValue != NULL)
		{
		free(data_p->svh_currentValue);
		tableDoForEachEntry(data_p->svh_variableHandleTable, setupVarNotifyChange, NULL);
		data_p->svh_currentValue = NULL;
		}

	    result = NULL;
	    }
	else
	    {
	    if(data_p->svh_currentValue != NULL) free(data_p->svh_currentValue);
	    data_p->svh_currentValue = malloc(strlen(*string) + strlen("%n@") + 1);
	    strcpy(data_p->svh_currentValue, "%n@");
	    strcat(data_p->svh_currentValue, *string);
	    tableDoForEachEntry(data_p->svh_variableHandleTable, setupVarNotifyChange, NULL);
	    result = NULL;
	    }

	return(result);
	}

static int
    setDefaultValue(setupVariableHandle_t *data_p, char *value)
	{
	int
	    result;

	if(value == NULL)
	    {
	    result = -1;
	    }
	if(data_p == NULL)
	    {
	    result = -1;
	    }
	else if(value == NULL)
	    {
	    if(data_p->svh_defaultValue != NULL)
		{
		free(data_p->svh_defaultValue);
		data_p->svh_defaultValue = NULL;
		}
	    }
	else
	    {
	    if(data_p->svh_defaultValue != NULL) free(data_p->svh_defaultValue);
	    data_p->svh_defaultValue = strdup(value);
	    result = 0;
	    }

	return(result);
	}

static int
    defaultValue(setupVariableHandle_t *data_p)
	{
	int
	    result;

	if(data_p != NULL)
	    {
	    if(data_p->svh_currentValue != NULL) free(data_p->svh_currentValue);
	    data_p->svh_currentValue = (data_p->svh_defaultValue != NULL)?
		strdup(data_p->svh_defaultValue):
		NULL;

	    tableDoForEachEntry(data_p->svh_variableHandleTable, setupVarNotifyChange, NULL);
	    result = 0;
	    }
	else
	    {
	    result = -1;
	    }

	return(result);
	}

static int
    resetValue(setupVariableHandle_t *data_p)
	{
	int
	    result;

	if(data_p != NULL)
	    {
	    if(data_p->svh_currentValue != NULL) free(data_p->svh_currentValue);
	    data_p->svh_currentValue = (data_p->svh_originalValue != NULL)?
		strdup(data_p->svh_originalValue):
		NULL;

	    tableDoForEachEntry(data_p->svh_variableHandleTable, setupVarNotifyChange, NULL);
	    result = 0;
	    }
	else
	    {
	    result = -1;
	    }

	return(result);
	}

static int
    applyValue(setupVariableHandle_t *data_p)
	{
	int
	    result;

	if(data_p != NULL)
	    {
	    if(data_p->svh_originalValue != NULL)
		{
		free(data_p->svh_originalValue);
		}

	    data_p->svh_originalValue = (data_p->svh_currentValue != NULL)?
		strdup(data_p->svh_currentValue):
		NULL;

	    result = 0;
	    }
	else
	    {
	    result = -1;
	    }

	return(result);
	}

static void
    variablePrivateDataFree(setupVariableHandle_t *data_p)
	{
	if(DebugLevel > 2)
	    {
	    (void) fprintf
		(
		stderr,
		"variablePrivateDataFree(0x%x) Entered.\n",
		(int) data_p
		);
	    }

	if(data_p == NULL)
	    {
	    }
	else if(tableDeleteEntryByValue(data_p->svh_fileTable, data_p))
	    {
	    if(data_p->svh_variableHandleTable != NULL) tableFree(data_p->svh_variableHandleTable);
	    if(DebugLevel > 6) (void) fprintf(stderr, "\tFreed variableHandleTable.\n");
	    if(data_p->svh_name != NULL) free(data_p->svh_name);
	    if(DebugLevel > 6) (void) fprintf(stderr, "\tFreed name.\n");
	    if(data_p->svh_currentValue != NULL) free(data_p->svh_currentValue);
	    if(DebugLevel > 6) (void) fprintf(stderr, "\tFreed currentValue.\n");
	    if(data_p->svh_defaultValue != NULL) free(data_p->svh_defaultValue);
	    if(DebugLevel > 6) (void) fprintf(stderr, "\tFreed defaultValue.\n");
	    if(data_p->svh_originalValue != NULL)
		{
		free(data_p->svh_originalValue);
		}
	    if(DebugLevel > 6) (void) fprintf(stderr, "\tFreed originalValue.\n");

	    free(data_p);
	    }

	if(DebugLevel > 2) (void) fprintf(stderr, "variablePrivateDataFree() Exited.\n");
	}

static void
    flagLocalDataSet(setupVariableHandle_t *handle, char **localData)
	{
	char
	    **curChoice;

	if(handle == NULL)
	    {
	    }
	else if(handle->svh_ops.svop_choices != NULL)
	    {
	    for
		(
		curChoice = localData;
		*curChoice != NULL;
		curChoice++
		)
		{
		free(*curChoice);
		}
	    
	    free(localData);
	    }
	else
	    {
	    handle->svh_ops.svop_choices = localData;
	    }
	}

static setupVariableHandle_t
    *variablePrivateDataNew
	(
	char *name,
	setupFileHandle_t *fileHandle_p
	)
	{
	fileVariableType_t
	    *varType_p;

	setupVariableHandle_t
	    *result;
	
	if(DebugLevel > 2)
	    {
	    (void) fprintf
		(
		stderr,
		"variablePrivateDataNew(%s, 0x%x) Entered.\n",
		name,
		(int) fileHandle_p
		);
	    }

	if
	    (
		(
		result = (setupVariableHandle_t *)tableGetValueByString
		    (
		    fileHandle_p->sfh_variableTable,
		    name
		    )
		) != NULL
	    )
	    {
	    }
	else if
	    (
		(
		result = (setupVariableHandle_t *)calloc(sizeof(*result), 1)
		) == NULL
	    )
	    {
	    /* ERROR, No Memory */
	    }
	else if((result->svh_name = strdup(name)) == NULL)
	    {
	    variablePrivateDataFree(result);
	    result = NULL;
	    }
	else if((result->svh_variableHandleTable = tableNew()) == NULL)
	    {
	    variablePrivateDataFree(result);
	    result = NULL;
	    }
	else if((varType_p = (fileVariableType_t *)tableGetValueByString(FileVariableTable, name)) != NULL)
	    {
	    if(DebugLevel > 5) (void) fprintf(stderr, "got entry for %s.\n", name);
	    result->svh_ops.svop_handle = result;
	    result->svh_ops.svop_free = variablePrivateDataFree;
	    result->svh_ops.svop_localDataSet = varType_p->fvt_localDataSet;
	    result->svh_ops.svop_type = varType_p->fvt_type;
	    result->svh_ops.svop_getValue =
		(int (*)(setupVariableHandle_t *, void *))varType_p->fvt_getValue;

	    result->svh_ops.svop_setValue =
		(char *(*)(setupVariableHandle_t *, void *))varType_p->fvt_setValue;

	    result->svh_ops.svop_choices = varType_p->fvt_translatedChoices;
	    result->svh_fileChoices = varType_p->fvt_fileChoices;
	    result->svh_menuTable = varType_p->fvt_menuTable;
	    result->svh_menuLength = varType_p->fvt_menuLength;
	    result->svh_ops.svop_defaultValue = defaultValue;
	    result->svh_ops.svop_setDefaultValue = setDefaultValue;
	    result->svh_ops.svop_applyValue = applyValue;
	    result->svh_ops.svop_resetValue = resetValue;
	    result->svh_fileTable = fileHandle_p->sfh_variableTable,
	    tableAddEntry
		(
		fileHandle_p->sfh_variableTable,
		result->svh_name,
		result,
		variablePrivateDataFree
		);
	    }
	else
	    {
	    if(DebugLevel > 5) (void) fprintf(stderr, "could not get entry for %s.\n", name);
	    result->svh_ops.svop_handle = result;
	    result->svh_ops.svop_free = variablePrivateDataFree;
	    result->svh_ops.svop_type = svt_string;
	    result->svh_ops.svop_getValue =
		(int (*)(setupVariableHandle_t *, void *))getStringValue;

	    result->svh_ops.svop_setValue =
		(char *(*)(setupVariableHandle_t *, void *))setStringValue;

	    result->svh_ops.svop_defaultValue = defaultValue;
	    result->svh_ops.svop_setDefaultValue = setDefaultValue;
	    result->svh_ops.svop_applyValue = applyValue;
	    result->svh_ops.svop_resetValue = resetValue;
	    result->svh_fileTable = fileHandle_p->sfh_variableTable,
	    tableAddEntry
		(
		fileHandle_p->sfh_variableTable,
		result->svh_name,
		result,
		variablePrivateDataFree
		);
	    }
	
	if(DebugLevel > 2)
	    {
	    (void) fprintf
		(
		stderr,
		"variablePrivateDataNew() = 0x%x Exiting.\n",
		(int) result
		);
	    }

	return(result);
	}

setupVarOps_t
    *fileGetVarOps(setupFileHandle_t *fileHandle, setupVar_t *variable_p)
	{
	setupVariableHandle_t
	    *variableHandle_p;
	
	setupVarOps_t
	    *result;
	
	char
	    *value;

	if
	    (
		(
		variableHandle_p = variablePrivateDataNew
		    (
		    setupVarName(variable_p),
		    fileHandle
		    )
		) == NULL
	    )
	    {
	    /* ERROR No Memory */
	    result = NULL;
	    }
	else
	    {
	    if((value = setupVarDefault(variable_p)) != NULL)
		{
		(void) setDefaultValue(variableHandle_p, value);
		}

	    tableAddEntry(variableHandle_p->svh_variableHandleTable, NULL, variable_p, NULL);
	    result = &variableHandle_p->svh_ops;
	    }
	
	return(result);
	}

setupFileHandle_t
    *fileOpen(char *pathname, char *flags)
	{
	FILE
	    *fp;

	setupFileHandle_t
	    *result;
	
	setupVariableHandle_t
	    *variable_p;

	char
	    *variableName,
	    *variableValue,
	    buffer[256];

	if(DebugLevel > 2)(void) fprintf(stderr, "fileOpen(%s, %s) Entered.\n", pathname, flags);
	if((result = setupFileHandleNew(pathname)) == NULL)
	    {
	    }
	else if((fp = fopen(pathname, "r")) == NULL)
	    {
	    /*
	    perror(pathname);
	    setupFileHandleFree(result);
	    result = NULL;
	    */
	    }
	else
	    {
	    while(fgets(buffer, sizeof(buffer), fp) != NULL)
		{
		if((variableName = strtok(buffer, "\t\r\n =")) == NULL)
		    {
		    /* Empty Line */
		    }
		else if(*variableName == '#')
		    {
		    /* Comment Line */
		    }
		else if
		    (
			(
			variable_p = variablePrivateDataNew(variableName, result)
			) == NULL
		    )
		    {
		    /* ERROR No Memory */
		    }
		else if((variableValue = strtok(NULL, "\r\n")) != NULL)
		    {
		    variable_p->svh_defaultValue = strdup(variableValue);
		    variable_p->svh_originalValue = strdup(variableValue);
		    variable_p->svh_currentValue = strdup(variableValue);
		    }
		}
	    
	    (void) fclose(fp);
	    }

	return(result);
	}

void
    fileClose(setupFileHandle_t *fileHandle)
	{
	setupFileHandleFree(fileHandle);
	}

static void
    writeFunc(setupVariableHandle_t *varHandle_p, FILE *fp)
	{
	if(varHandle_p->svh_currentValue != NULL && *(varHandle_p->svh_currentValue) != '\0')
	    {
	    (void) fprintf
		(
		fp,
		"%s=%s\n",
		varHandle_p->svh_name,
		varHandle_p->svh_currentValue
		);
	    }
	}

int
    fileApply(setupFileHandle_t *fileHandle)
	{
	struct passwd
	    *etcpasswd;

	FILE
	    *fp;
	
	int
	    result;

	uid_t
	    oldUid;
	
	gid_t
	    oldGid;

	mode_t
	    oldMode;

	oldUid = getuid();
	oldGid = getgid();
	oldMode = umask(0002);

	if((etcpasswd = getpwnam ((const char *)"mail")) == NULL)
	    {
	    result = -1;
	    }
	else if((setgid (etcpasswd->pw_gid)) == -1)
	    {
	    result = -1;
	    }
	else if((setuid (etcpasswd->pw_uid)) == -1)
	    {
	    result = -1;
	    }
	else if((fp = fopen(fileHandle->sfh_pathName, "w")) == NULL)
	    {
	    perror(fileHandle->sfh_pathName);
	    result = -1;
	    }
	else
	    {
	    tableDoForEachEntry
		(
		fileHandle->sfh_variableTable,
		writeFunc,
		(void *)fp
		);

	    (void) fclose(fp);
	    (void) system("/usr/lib/mail/surrcmd/createSurr");
	    result = 0;
	    }

	(void) umask(oldMode);
	(void) setuid(oldUid);
	(void) setgid(oldGid);

	return(result);
	}

static fileVariableType_t
    FileVariableTypeList[] =
	{
	"ADD_DATE", NULL, NULL, setFlagValue, getFlagValue, 0, NULL, svt_flag, flagLocalDataSet,
	"ADD_FROM", NULL, NULL, setFlagValue, getFlagValue, 0, NULL, svt_flag, flagLocalDataSet,
	"ADD_RECEIVED", NULL, NULL, setFlagValue, getFlagValue, 0, NULL, svt_flag, flagLocalDataSet,
	"DEBUG", NULL, NULL, setIntegerValue, getIntegerValue, 0, NULL, svt_integer, NULL,
	"CLUSTER", NULL, NULL, setStringValue, getStringValue, 0, NULL, svt_string, NULL,
	"REMOTEFROM", NULL, NULL, setStringValue, getStringValue, 0, NULL, svt_string, NULL,
	"FAILSAFE", NULL, NULL, setFailsafeStringValue, getFailsafeStringValue, 0, NULL, svt_string, NULL,
	"DEL_EMPTY_MFILE", NULL, NULL, setMenuValue, getMenuValue, 0, NULL, svt_menu, NULL,
	"DOMAIN", NULL, NULL, setStringValue, getStringValue, 0, NULL, svt_string, NULL,
	"SMARTERHOST", NULL, NULL, setStringValue, getStringValue, 0, NULL, svt_string, NULL,
	"ARG_MAX", NULL, NULL, setIntegerValue, getIntegerValue, 0, NULL, svt_integer, NULL,
	"SURR_EXPORT", NULL, NULL, setStringValue, getStringValue, 0, NULL, svt_string, NULL,
	"CNFG_EXPORT", NULL, NULL, setStringValue, getStringValue, 0, NULL, svt_string, NULL,
	"NOCOMPILEDSURRFILE", NULL, NULL, setInvFlagValue, getInvFlagValue, 0, NULL, svt_flag, flagLocalDataSet,
	"ADD_MESSAGE_ID", NULL, NULL, setFlagValue, getFlagValue, 0, NULL, svt_flag, flagLocalDataSet,
	"FORCE_7BIT_MIME", NULL, NULL, setFlagValue, getFlagValue, 0, NULL, svt_flag, flagLocalDataSet,
	"FORCE_7BIT_HEADERS", NULL, NULL, setFlagValue, getFlagValue, 0, NULL, svt_flag, flagLocalDataSet,
	"FORCE_MIME", NULL, NULL, setFlagValue, getFlagValue, 0, NULL, svt_flag, flagLocalDataSet,
	"ADD_TO", NULL, NULL, setFlagValue, getFlagValue, 0, NULL, svt_flag, flagLocalDataSet,
	NULL, NULL, NULL, NULL, NULL, 0, NULL, svt_none, NULL
	};

int
    fileInit(int debugLevel, char *objectName)
	{
	int
	    result = 0;

	char
	    **translatedChoice,
	    **fileChoice,
	    tmpBuff[256];

	fileVariableType_t
	    *curType_p;
	
	menuEntry_t
	    *curEntry_p;

	DebugLevel = debugLevel;

	if(DebugLevel > 2) (void) fprintf(stderr, "fileInit(%d) entered.\n", debugLevel);
	if(FileVariableTable != NULL)
	    {
	    }
	else if((FileVariableTable = tableNew()) == NULL)
	    {
	    result = 1;
	    }
	else
	    {
	    for
		(
		curType_p = FileVariableTypeList;
		curType_p->fvt_type != svt_none;
		curType_p++
		)
		{
		if(DebugLevel > 5)
		    {
		    (void) fprintf(stderr, "\tadding %s of type %d to table.\n", curType_p->fvt_name, curType_p->fvt_type);
		    }

		tableAddEntry(FileVariableTable, curType_p->fvt_name, curType_p, NULL);
		}

	    /*
		Add the menu choices for menu type variables.
	    */
	    if((curType_p = tableGetValueByString(FileVariableTable, "DEL_EMPTY_MFILE")) == NULL)
		{
		}
	    else if((curType_p->fvt_translatedChoices = (char **)calloc(sizeof(MailFileMenuEntries), 1)) == NULL)
		{
		}
	    else if((curType_p->fvt_fileChoices = (char **)calloc(sizeof(MailFileMenuEntries), 1)) == NULL)
		{
		}
	    else if((curType_p->fvt_menuTable = tableNew()) == NULL)
		{
		}
	    else for
		(
		curEntry_p = MailFileMenuEntries,
		    translatedChoice = curType_p->fvt_translatedChoices,
		    fileChoice = curType_p->fvt_fileChoices,
		    curType_p->fvt_menuLength = 0;
		curEntry_p->me_name != NULL;
		curEntry_p++,
		    translatedChoice++,
		    fileChoice++,
		    curType_p->fvt_menuLength++
		)
		{
		*fileChoice = curEntry_p->me_name;
		(void) sprintf(tmpBuff, "%s:%d", objectName, curEntry_p->me_msgNum);
		*translatedChoice = gettxt(tmpBuff, curEntry_p->me_name);
		tableAddEntry(curType_p->fvt_menuTable, curEntry_p->me_name, curEntry_p, NULL);
		}
	    }

	return(result);
	}

int
    filePermission(setupFileHandle_t *fileHandle)
	{
	struct passwd
	    *etcpasswd;

	int
	    result;

	uid_t
	    oldUid;
	
	gid_t
	    oldGid;

	if(DebugLevel > 2)
	    {
	    (void) fprintf
		(
		stderr,
		"filePermission(0x%x) Entered.\n",
		(int) fileHandle
		);
	    }

	oldUid = getuid();
	oldGid = getgid();

	if((etcpasswd = getpwnam ((const char *)"mail")) == NULL)
	    {
	    if(DebugLevel > 2)
		{
		(void) fprintf(stderr, "\tmail, no such user.\n");
		}

	    result = 0;
	    }
	else if((setgid (etcpasswd->pw_gid)) == -1)
	    {
	    if(DebugLevel > 2)
		{
		perror("setgid");
		}

	    result = 0;
	    }
	else if((setuid (etcpasswd->pw_uid)) == -1)
	    {
	    if(DebugLevel > 2)
		{
		perror("setuid");
		}

	    result = 0;
	    }
	else if(!access(fileHandle->sfh_pathName, F_OK))
	    {
	    char
		*lastSlash;

	    if((lastSlash = strrchr(fileHandle->sfh_pathName, '/')) == NULL)
		{
		result = 0;
		}
	    else
		{
		*lastSlash = '\0';
		if(access(fileHandle->sfh_pathName, W_OK))
		    {
		    if(DebugLevel > 2)
			{
			perror(fileHandle->sfh_pathName);
			}

		    result = 0;
		    }
		else
		    {
		    result = 1;
		    }

		*lastSlash = '/';
		}
	    }
	else
	    {
	    char
		*lastSlash;
		
	    if((lastSlash = strrchr(fileHandle->sfh_pathName, '/')) != NULL)
		{
		*lastSlash = '\0';
		}

	    if(DebugLevel > 8)
		{
		(void) fprintf
		    (
		    stderr,
		    "\tNo file checking directory %s.\n",
		    fileHandle->sfh_pathName
		    );
		}

	    if(access(fileHandle->sfh_pathName, W_OK))
		{
		if(DebugLevel > 2)
		    {
		    perror(fileHandle->sfh_pathName);
		    }

		result = 0;
		}
	    else
		{
		result = 1;
		}

	    if(lastSlash != NULL)
		{
		*lastSlash = '/';
		}
	    }
	
	(void) setuid(oldUid);
	(void) setgid(oldGid);

	if(DebugLevel > 2)
	    {
	    (void) fprintf
		(
		stderr,
		"filePermission() = %d Exited.\n",
		result
		);
	    }

	return(result);
	}
