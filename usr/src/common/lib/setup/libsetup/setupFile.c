/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libsetup:libsetup/setupFile.c	1.2"
#include	<stdio.h>
#include	<malloc.h>
#include	<string.h>
#include	<dlfcn.h>
#include	<mail/table.h>

#define	SETUPFILE_OBJ

#include	<mail/setupVar.h>

extern char
    *basename(char *);

static table_t
    *FileTable = NULL;

extern int
    DebugLevel;

#define	CONFIG_FILE	"/usr/lib/setup/filetypes/config"

typedef struct fileHandle_s
    {
    char
	*fh_filePath,
	*fh_objectPath;
    
    int
	(*fh_init)(int debugLevel, char *objectName),
	(*fh_perm)(setupFileHandle_t *handle),
	(*fh_apply)(setupFileHandle_t *handle);

    void
	(*fh_close)(setupFileHandle_t *handle),
	*fh_objectHandle;

    setupFileHandle_t
	*(*fh_open)(char *pathname, char *flags),
	*fh_fileHandle;

    setupVarOps_t
	*(*fh_getVarOps)(setupFileHandle_t *handle, setupVar_t *variable_p);
    }	fileHandle_t;

static void
    fileHandleFree(fileHandle_t *handle_p)
	{
	if(handle_p != NULL)
	    {
	    (void) tableDeleteEntryByValue(FileTable, handle_p);
	    if(handle_p->fh_filePath != NULL) free(handle_p->fh_filePath);
	    if(handle_p->fh_fileHandle != NULL)
		{
		handle_p->fh_close(handle_p->fh_fileHandle);
		}

	    if(handle_p->fh_objectHandle != NULL)
		{
		dlclose(handle_p->fh_objectHandle);
		}

	    free(handle_p);
	    }
	}

static void
    fileHandleNew(char *filePath, char *fileObjectPath)
	{
	fileHandle_t
	    *handle_p;
	
	if((handle_p = (fileHandle_t *)calloc(sizeof(*handle_p), 1)) == NULL)
	    {
	    /* ERROR, No Memory */
	    }
	else if((handle_p->fh_filePath = strdup(filePath)) == NULL)
	    {
	    /* ERROR, No Memory */
	    fileHandleFree(handle_p);
	    }
	else if((handle_p->fh_objectPath = strdup(fileObjectPath)) == NULL)
	    {
	    /* ERROR, No Memory */
	    fileHandleFree(handle_p);
	    }
	else
	    {
	    tableAddEntry
		(
		FileTable,
		handle_p->fh_filePath,
		handle_p,
		fileHandleFree
		);
	    }
	}

void
    setupFileInit()
	{
	FILE
	    *fp_config;

	char
	    *fileName,
	    *fileTypeObject,
	    buffer[256];

	if(FileTable != NULL)
	    {
	    }
	else if((fp_config = fopen(CONFIG_FILE, "r")) == NULL)
	    {
	    perror(CONFIG_FILE);
	    }
	else if((FileTable = tableNew()) == NULL)
	    {
	    }
	else while(fgets(buffer, sizeof(buffer), fp_config) != NULL)
	    {
	    if(*buffer == '#')
		{
		/* Comment line, ignore. */
		}
	    else if((fileName = strtok(buffer, " \t\r\n")) == NULL)
		{
		/* Blank line, ignore. */
		}
	    else if((fileTypeObject = strtok(NULL, " \t\r\n")) == NULL)
		{
		/* ERROR, Bad Syntax */
		}
	    else if
		(
		    (
		    tableGetValueByString
			(
			FileTable,
			fileName
			)
		    ) != NULL
		)
		{
		/* This file has already been initiailized. */
		}
	    else
		{
		fileHandleNew(fileName, fileTypeObject);
		}
	    }
	}

fileHandle_t
    *setupFileOpen(char *pathname, char *flags)
	{
	fileHandle_t
	    *result;
	
	if(DebugLevel > 2)
	    {
	    (void) fprintf
		(
		stderr,
		"setupFileOpen(%s, %s) Entered.\n",
		pathname,
		flags
		);
	    }

	if
	    (
		(
		result = (fileHandle_t *)tableGetValueByString(FileTable, pathname)
		) == NULL
	    )
	    {
	    /* ERROR, No Such Configuration File. */
	    }
	else if(result->fh_fileHandle != NULL)
	    {
	    /* Already Open */
	    }
	else if
	    (
		(
		result->fh_objectHandle = dlopen
		    (
		    result->fh_objectPath,
		    RTLD_LAZY
		    )
		) == NULL
	    )
	    {
	    /* ERROR, No Shared Object */
	    (void) fprintf(stderr, "%s: %s\n", result->fh_objectPath, dlerror());
	    fileHandleFree(result);
	    result = NULL;
	    }
	else if
	    (
		(
		result->fh_init = (int (*)(int, char *))dlsym
		    (
		    result->fh_objectHandle,
		    "fileInit"
		    )
		) == NULL
	    )
	    {
	    /* ERROR Bad object. */
	    fileHandleFree(result);
	    result = NULL;
	    }
        else if(result->fh_init(DebugLevel, basename(result->fh_objectPath)))
	    {
	    /* ERROR Bad object. */
	    if(DebugLevel > 0) (void) fprintf(stderr, "ERROR In initialization of %s.\n", result->fh_objectPath);
	    fileHandleFree(result);
	    result = NULL;
	    }
	else if
	    (
		(
		result->fh_open = (setupFileHandle_t *(*)(char *, char *))dlsym
		    (
		    result->fh_objectHandle,
		    "fileOpen"
		    )
		) == NULL
	    )
	    {
	    /* ERROR Bad object. */
	    fileHandleFree(result);
	    result = NULL;
	    }
	else if
	    (
		(
		result->fh_getVarOps =
		    (setupVarOps_t *(*)(setupFileHandle_t *, setupVar_t *))dlsym
			(
			result->fh_objectHandle,
			"fileGetVarOps"
			)
		) == NULL
	    )
	    {
	    /* ERROR Bad object. */
	    fileHandleFree(result);
	    result = NULL;
	    }
	else if
	    (
		(
		result->fh_close = (void (*)(setupFileHandle_t *))dlsym
		    (
		    result->fh_objectHandle,
		    "fileClose"
		    )
		) == NULL
	    )
	    {
	    /* ERROR Bad object. */
	    fileHandleFree(result);
	    result = NULL;
	    }
	else if
	    (
		(
		result->fh_perm = (int (*)(setupFileHandle_t *))dlsym
		    (
		    result->fh_objectHandle,
		    "filePermission"
		    )
		) == NULL
	    )
	    {
	    /* ERROR Bad object. */
	    fileHandleFree(result);
	    result = NULL;
	    }
	else if
	    (
		(
		result->fh_apply = (int (*)(setupFileHandle_t *))dlsym
		    (
		    result->fh_objectHandle,
		    "fileApply"
		    )
		) == NULL
	    )
	    {
	    /* ERROR Bad object. */
	    fileHandleFree(result);
	    result = NULL;
	    }
	else if
	    (
		(
		result->fh_fileHandle = result->fh_open
		    (
		    result->fh_filePath,
		    flags
		    )
		) == NULL
	    )
	    {
	    /* ERROR in opening file. */
	    result = NULL;
	    }

	if(DebugLevel > 2)
	    {
	    (void) fprintf
		(
		stderr,
		"setupFileOpen() = 0x%x Exited.\n",
		(int) result
		);
	    }

	return(result);
	}

void
    setupFileClose(fileHandle_t *handle_p)
	{
	if(handle_p != NULL)
	    {
	    if(handle_p->fh_fileHandle != NULL)
		{
		handle_p->fh_close(handle_p->fh_fileHandle);
		handle_p->fh_fileHandle = NULL;
		}

	    if(handle_p->fh_objectHandle != NULL)
		{
		dlclose(handle_p->fh_objectHandle);
		handle_p->fh_objectHandle = NULL;
		}
	    }
	}

setupVarOps_t
    *setupFileGetVarOps(char *pathname, setupVar_t *variable_p)
	{
	setupVarOps_t
	    *result;
	
	fileHandle_t
	    *handle_p;

	if(DebugLevel > 2)
	    {
	    (void) fprintf
		(
		stderr,
		"setupFileGetVarOps(%s, 0x%x) Entered.\n",
		pathname,
		(int) variable_p
		);
	    }

	if((handle_p = setupFileOpen(pathname, "r")) == NULL)
	    {
	    result = NULL;
	    }
	else if(handle_p->fh_fileHandle == NULL)
	    {
	    /* The file is not open */
	    result = NULL;
	    }
	else
	    {
	    if(DebugLevel > 5)
		{
		(void) fprintf
		    (
		    stderr,
		    "\thandle_p->fh_getVarOps = 0x%x.\n",
		    (int) handle_p->fh_getVarOps
		    );
		}

	    result = handle_p->fh_getVarOps
		(
		handle_p->fh_fileHandle,
		variable_p
		);
	    }

	if(DebugLevel > 2)
	    {
	    (void) fprintf
		(
		stderr,
		"setupFileGetVarOps() = 0x%x Exited.\n",
		(int) result
		);
	    }

	return(result);
	}

void
    setupFilePerm(fileHandle_t *handle_p, int *ok)
	{
	if(!ok)
	    {
	    }
	else if(handle_p == NULL)
	    {
	    *ok = 0;
	    }
	else if(handle_p->fh_fileHandle == NULL)
	    {
	    /* The file is not open */
	    *ok = 0;
	    }
	else
	    {
	    *ok = handle_p->fh_perm(handle_p->fh_fileHandle);
	    }
	}

int
    setupFileApply(fileHandle_t *handle_p)
	{
	int
	    result;
	
	if(handle_p == NULL)
	    {
	    result = -1;
	    }
	else if(handle_p->fh_fileHandle == NULL)
	    {
	    /* The file is not open */
	    result = -1;
	    }
	else
	    {
	    result = handle_p->fh_apply(handle_p->fh_fileHandle);
	    }

	return(result);
	}
