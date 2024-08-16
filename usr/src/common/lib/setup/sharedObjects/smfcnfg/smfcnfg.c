/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libsetup:sharedObjects/smfcnfg/smfcnfg.c	1.10"
#include	<stdio.h>
#include	<unistd.h>
#include	<stdlib.h>
#include	<string.h>
#include	<malloc.h>
#include	<pwd.h>
#include	<mail/table.h>

#define	SETUPSPECIFICFILE_OBJ

typedef struct setupVariableHandle_s setupVariableHandle_t;
typedef struct setupFileHandle_s setupFileHandle_t;

#include	<mail/setupTypes.h>
#include	<mail/setupVar.h>

typedef struct fileVariableType_s
    {
    char
	*fvt_name,
	*(*fvt_setValue)();
    
    int
	(*fvt_getValue)(),
	(*fvt_specialApply)(),
	fvt_min,
	fvt_max;

    setupVariableType_t
	fvt_type;
    }	fileVariableType_t;

struct setupVariableHandle_s
    {
    int
	(*svh_specialApply)(),
	svh_min,
	svh_max;

    char
	*svh_name,
	*svh_defaultValue,
	*svh_originalValue,
	*svh_currentValue;

    table_t
	*svh_fileTable,
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

extern unsigned char
    *encryptPasswd(char *);

extern char
    *decryptPasswd(char *);

static char
    ErrorBuffer[160];

static int
    DebugLevel = 0;

static table_t
    *FileVariableTable = NULL;

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
	    *result,
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
	else if
	    (
		(
		data_p->svh_max > data_p->svh_min
		) && (
		(*value > data_p->svh_max) || (*value < data_p->svh_min)
		)
	    )
	    {
	    sprintf
		(
		ErrorBuffer,
		"This field must be an integer from %d to %d.",
		data_p->svh_min,
		data_p->svh_max
		);

	    result = ErrorBuffer;
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
		data_p->svh_currentValue = NULL;
		tableDoForEachEntry(data_p->svh_variableHandleTable, setupVarNotifyChange, NULL);
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

static char
    *setNocaseStringValue(setupVariableHandle_t *data_p, char **string)
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
		data_p->svh_currentValue = NULL;
		tableDoForEachEntry(data_p->svh_variableHandleTable, setupVarNotifyChange, NULL);
		}

	    result = NULL;
	    }
	else
	    {
	    char
		*p;

	    if(data_p->svh_currentValue != NULL) free(data_p->svh_currentValue);
	    data_p->svh_currentValue = strdup(*string);
	    for
		(
		p = data_p->svh_currentValue;
		*p != '\0';
		p++
		)
		{
		*p = tolower(*p);
		}

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
	if(data_p == NULL)
	    {
	    }
	else if(tableDeleteEntryByValue(data_p->svh_fileTable, data_p))
	    {
	    if(data_p->svh_variableHandleTable != NULL) tableFree(data_p->svh_variableHandleTable);
	    if(data_p->svh_name != NULL) free(data_p->svh_name);
	    if(data_p->svh_currentValue != NULL) free(data_p->svh_currentValue);
	    if(data_p->svh_defaultValue != NULL) free(data_p->svh_defaultValue);
	    if(data_p->svh_originalValue != NULL)
		{
		free(data_p->svh_originalValue);
		}

	    free(data_p);
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
	    result->svh_ops.svop_handle = result;
	    result->svh_ops.svop_free = variablePrivateDataFree;
	    result->svh_ops.svop_type = varType_p->fvt_type;
	    result->svh_min = varType_p->fvt_min;
	    result->svh_max = varType_p->fvt_max;
	    result->svh_ops.svop_getValue =
		(int (*)(setupVariableHandle_t *, void *))varType_p->fvt_getValue;

	    result->svh_ops.svop_setValue =
		(char *(*)(setupVariableHandle_t *, void *))varType_p->fvt_setValue;

	    result->svh_ops.svop_defaultValue = defaultValue;
	    result->svh_ops.svop_setDefaultValue = setDefaultValue;
	    result->svh_ops.svop_applyValue = applyValue;
	    result->svh_ops.svop_resetValue = resetValue;
	    result->svh_specialApply = varType_p->fvt_specialApply;
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

/*fprintf(stderr, "fileOpen(%d, %d) Entered.\n", pathname, flags);*/
	if((result = setupFileHandleNew(pathname)) == NULL)
	    {
	    }
	else if((fp = fopen(pathname, "r")) == NULL)
	    {
	    /*
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
		else if((variableValue = strtok(NULL, "\"\r\n")) != NULL)
		    {
		    switch(variable_p->svh_ops.svop_type)
			{
			default:
			    {
			    variable_p->svh_defaultValue = strdup(variableValue);
			    variable_p->svh_originalValue = strdup(variableValue);
			    variable_p->svh_currentValue = strdup(variableValue);
			    break;
			    }

			case	svt_password:
			    {
			    variableValue = decryptPasswd(variableValue);
			    variable_p->svh_defaultValue = strdup(variableValue);
			    variable_p->svh_originalValue = strdup(variableValue);
			    variable_p->svh_currentValue = strdup(variableValue);
			    }
			}
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
	if(varHandle_p->svh_originalValue != NULL)
	    {
	    switch(varHandle_p->svh_ops.svop_type)
		{
		default:
		    {
		    (void) fprintf
			(
			fp,
			"%s=\"%s\"\n",
			varHandle_p->svh_name,
			varHandle_p->svh_originalValue
			);

		    break;
		    }

		case	svt_password:
		    {
		    (void) fprintf
			(
			fp,
			"%s=\"%s\"\n",
			varHandle_p->svh_name,
			(char *) encryptPasswd(varHandle_p->svh_originalValue)
			);

		    break;
		    }
		}
	    }

	if(varHandle_p->svh_specialApply != NULL) varHandle_p->svh_specialApply(varHandle_p);
	}

static int
    setCronEntry(setupVariableHandle_t *varHandle_p)
	{
	int
	    perHour,
	    result,
	    i,
	    interval;
	char
	    tmpBuff[16],
	    pollEntry[500],
	    cronEntry[500];

	struct passwd
	    *etcpasswd;

	if((etcpasswd = getpwnam ((const char *)"mhsmail")) == NULL)
	    {
	    result = -1;
	    }
	else if((setgid (etcpasswd->pw_gid)) == -1)
	    {
	    /*perror ("setgid failed: ");*/
	    result = -1;
	    }
	else if((setuid (etcpasswd->pw_uid)) == -1)
	    {
	    /*perror ("setuid failed: ");*/
	    result = -1;
	    }
	else if((perHour = (varHandle_p->svh_originalValue == NULL)? 0: atoi(varHandle_p->svh_originalValue)) != 0)
	    {
	    (void) strcpy(pollEntry, "0");
	    for
		(
		interval = 6000/perHour,
		    i = interval;
		i < 6000;
		i += interval
		)
		{
		(void) sprintf(tmpBuff, ",%d", i/100);
		(void) strcat(pollEntry, tmpBuff);
		}

	    /*  Add new entry in the root crontab, with the new interval */
	    (void) sprintf
		(
		cronEntry,
	       "{ { echo '%s * * * * %s'; echo '0,10 * * * * %s' 2>/dev/null; /usr/bin/crontab -l 2>/dev/null | /usr/bin/egrep -v 'smf-poll|smfsched' 2>/dev/null; } | crontab 2>/dev/null; } &",
	       pollEntry,
	       "/usr/lib/mail/surrcmd/smf-poll",
	       "/usr/lib/mail/surrcmd/smfsched"
	       );

	    if(system (cronEntry))
		{
		/*perror ("crontab update failed:");*/
		result = -1;
		}
	    else
		{
		result = 0;
		}
	    }
	else
	    {
	    (void) sprintf
		(
		cronEntry,
	       "{ /usr/bin/crontab -l 2>/dev/null | /usr/bin/egrep -v 'smf-poll|smfsched' 2>/dev/null | crontab 2>/dev/null; } &"
	       );

	    if(system (cronEntry))
		{
		/*perror ("crontab update failed:");*/
		result = -1;
		}
	    else
		{
		result = 0;
		}
	    }

	return(result);
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

	if((etcpasswd = getpwnam ((const char *)"mhsmail")) == NULL)
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
	"MHSINTERVAL", setIntegerValue, getIntegerValue, setCronEntry, 0, 60, svt_integer,
	"SMFVERSION", setIntegerValue, getIntegerValue, NULL, 70, 71, svt_integer,
	"MHSGATEWAY", setNocaseStringValue, getStringValue, NULL, 0, 0, svt_string,
	"DIRECTORY", setNocaseStringValue, getStringValue, NULL, 0, 0, svt_string,
	"MAILSERV", setNocaseStringValue, getStringValue, NULL, 0, 0, svt_string,
	"MV", setNocaseStringValue, getStringValue, NULL, 0, 0, svt_string,
	"MHSLOGIN", setStringValue, getStringValue, NULL, 0, 0, svt_string,
	"MHSPASSWD", setStringValue, getStringValue, NULL, 0, 0, svt_password,
	NULL, NULL, NULL, NULL, 0, 0, svt_none
	};

int
    fileInit(int debugLevel)
	{
	int
	    result = 0;

	fileVariableType_t
	    *curType_p;
	
	DebugLevel = debugLevel;

	if(DebugLevel > 2) (void) fprintf(stderr, "fileInit(%d) entered.\n", debugLevel);
	if(FileVariableTable != NULL)
	    {
	    }
	else if((FileVariableTable = tableNew()) == NULL)
	    {
	    result = 1;
	    }
	else for
	    (
	    curType_p = FileVariableTypeList;
	    curType_p->fvt_type != svt_none;
	    curType_p++
	    )
	    {
	    tableAddEntry(FileVariableTable, curType_p->fvt_name, curType_p, NULL);
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

	if((etcpasswd = getpwnam ((const char *)"mhsmail")) == NULL)
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
