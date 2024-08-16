/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libsetup:sharedObjects/mailflgs/mailflgs.c	1.3"
#include	<stdio.h>
#include	<unistd.h>
#include	<ctype.h>
#include	<malloc.h>
#include	<string.h>
#include	<pwd.h>
#include	<mail/table.h>

#define	SETUPSPECIFICFILE_OBJ

typedef struct setupVariableHandle_s setupVariableHandle_t;
typedef struct setupFileHandle_s setupFileHandle_t;

#include	<mail/setupTypes.h>
#include	<mail/setupVar.h>

struct setupVariableHandle_s
    {
    char
	*svh_name;

    unsigned
	svh_defaultValue	:1,
	svh_originalValue	:1,
	svh_currentValue	:1;

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

static int
    DebugLevel = 0;

static int
    strcasecmp(char *str1, char *str2)
	{
	char
	    c1,
	    c2;
	
	for
	    (
	    c1 = toupper(*str1),
		c2 = toupper(*str2);
	    *str1 != '\0' && c1 == c2;
	    c1 = toupper(*++str1),
		c2 = toupper(*++str2)
	    );
	
	if(c1 == c2)
	    {
	    return(0);
	    }
	else
	    {
	    return((c1 > c2)? 1: -1);
	    }
	}

static int
    flagGetValue(char *value)
	{
	int
	    result;
	
	if(!strcasecmp(value, "TRUE"))
	    {
	    result = 1;
	    }
	else if(!strcasecmp(value, "YES"))
	    {
	    result = 1;
	    }
	else
	    {
	    result = 0;
	    }

	return(result);
	}

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
    getValue(setupVariableHandle_t *data_p, setupFlagType_t *flag)
	{
	int
	    result;

	if(flag == NULL)
	    {
	    result = -1;
	    }
	if(data_p == NULL)
	    {
	    result = -1;
	    }
	else
	    {
	    *flag = data_p->svh_currentValue;
	    result = 0;
	    }

	return(result);
	}

static char
    *setValue(setupVariableHandle_t *data_p, setupFlagType_t *flag)
	{
	char
	    *result;

	if(flag == NULL)
	    {
	    result = "BAD CALL TO SET VALUE";
	    }
	if(data_p == NULL)
	    {
	    result = "BAD CALL TO SET VALUE";
	    }
	else
	    {
	    data_p->svh_currentValue = *flag;
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
	else
	    {
	    data_p->svh_defaultValue = flagGetValue(value);
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
	    data_p->svh_currentValue = data_p->svh_defaultValue;
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
	    data_p->svh_currentValue = data_p->svh_originalValue;
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
	    data_p->svh_originalValue = data_p->svh_currentValue;
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
	char
	    **curChoice;

	if(data_p == NULL)
	    {
	    }
	else if(tableDeleteEntryByValue(data_p->svh_fileTable, data_p))
	    {
	    if(data_p->svh_variableHandleTable != NULL) tableFree(data_p->svh_variableHandleTable);
	    if(data_p->svh_name != NULL) free(data_p->svh_name);
	    if(data_p->svh_ops.svop_choices != NULL)
		{
		for
		    (
		    curChoice = data_p->svh_ops.svop_choices;
		    *curChoice != NULL;
		    curChoice++
		    )
		    {
		    free(*curChoice);
		    }
		
		free(data_p->svh_ops.svop_choices);
		}

	    free(data_p);
	    }
	}

static void
    localDataSet(setupVariableHandle_t *handle, char **localData)
	{
	char
	    **curChoice;

	if(handle != NULL)
	    {
	    if(handle->svh_ops.svop_choices != NULL)
		{
		for
		    (
		    curChoice = handle->svh_ops.svop_choices;
		    *curChoice != NULL;
		    curChoice++
		    )
		    {
		    free(*curChoice);
		    }
		
		free(handle->svh_ops.svop_choices);
		}

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
	else
	    {
	    result->svh_ops.svop_handle = result;
	    result->svh_ops.svop_localDataSet = localDataSet;
	    result->svh_ops.svop_free = variablePrivateDataFree;
	    result->svh_ops.svop_type = svt_flag;
	    result->svh_ops.svop_getValue =
		(int (*)(setupVariableHandle_t *, void *))getValue;

	    result->svh_ops.svop_setValue =
		(char *(*)(setupVariableHandle_t *, void *))setValue;

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
		variableHandle_p->svh_defaultValue = flagGetValue(value);
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
	    *flagName,
	    buffer[256];

/*fprintf(stderr, "fileOpen(%d, %d) Entered.\n", pathname, flags);*/
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
		if((flagName = strtok(buffer, "\t\r\n ")) == NULL)
		    {
		    /* Empty Line */
		    }
		else if(*flagName == '#')
		    {
		    /* Comment Line */
		    }
		else if
		    (
			(
			variable_p = variablePrivateDataNew(flagName, result)
			) == NULL
		    )
		    {
		    /* ERROR No Memory */
		    }
		else
		    {
		    variable_p->svh_defaultValue = 1;
		    variable_p->svh_originalValue = 1;
		    variable_p->svh_currentValue = 1;
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
	if(varHandle_p->svh_currentValue)
	    {
	    (void) fprintf(fp, "%s\n", varHandle_p->svh_name);
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


int
    fileInit(int debugLevel, char *objectName)
	{
	DebugLevel = debugLevel;
	return(0);
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
