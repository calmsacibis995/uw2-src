/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnwutil:common/lib/libnutil/initcfg.c	1.8"
#ident	"$Id: initcfg.c,v 1.7.2.2 1994/10/27 17:26:37 vtag Exp $"
/*
 * Copyright 1994 Unpublished Work of Novell, Inc.
 * All Rights Reserved.
 *
 * This work is an unpublished work and contains confidential,
 * proprietary and trade secret information of Novell, Inc. Access
 * to this work is restricted to (I) Novell employees who have a
 * need to know to perform tasks within the scope of their
 * assignments and (II) entities other than Novell who have
 * entered into appropriate agreements.
 *
 * No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 */
#include <schemadef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/nwtdr.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include "./binhead.h"
#ifdef SHARED_LIBS
#include <dlfcn.h>
#else
#include "nwcm/symtab.h"
#endif

extern int errno;

/* Static function definitions */
static int		AllocTables( void );
static int		cpcmp( const void *, const void *);
#ifndef SHARED_LIBS
static void		*GetSymbol(char *);
#endif

/* Global data definitions (externs declared in schemadef.h) */
unsigned long	*IntegerParameterValues;
unsigned long	*IntegerParameterDefaults;
int 			IntegerParameterCount = 0;
int 			*BooleanParameterValues;
int 			*BooleanParameterDefaults;
int 			BooleanParameterCount = 0;
char			*StringParameterValues;
char			*StringParameterDefaults;
int 			StringParameterCount = 0;
struct cp_s		*ConfigurationParameters;
int 			ConfigurationParameterCount = 0;

/* locally accessed structures */
struct ipvd_s	*IntegerValidationData;
cFTab			handleTable[100];

/* locally accessed variables */ 
int				num_packages = 0;
binHeader_t		header[100];

/*
**	Initialize NetWare Configuration data tables.  Called from _InitConfigManager
**	function in nwcm.c
*/
int
InitNWCM( char *dirPtr )
{
	binRecord_t		record;
	int 			i, j, err;
	int 			pathLen;
	char			pathBuf[NWCM_MAX_STRING_SIZE];

	int 			bfd[100];
	int 			PkgCount[100];
	int				parmIdx = 0;
	int				intIdx = 0;
	int				boolIdx = 0;
	int				strIdx = 0;
	struct cp_s		*tmpPtr;
	DIR				*dirFd;
	struct dirent	*direntPtr;
	char			*cmp;


/*
**	Find the binary file directory
*/	
	strcpy(pathBuf, dirPtr);
	pathLen = strlen(pathBuf);

/*
**	Open the passed in directory, if it fails, return
*/
	while((dirFd = opendir(dirPtr)) == NULL) {
		if(errno == EINTR)
			continue;
		else {
#ifdef HARD_DEBUG
			printf("DEBUG: Unable to open bin file '%s'\n", pathBuf);
#endif
			return(-1);
		}
	}

/*
**	Determine the packages that are present by opening the binary files
**	containing their configuration parameters
*/
	i = 0;
top:
	errno = 0;
	while((direntPtr = readdir(dirFd)) != NULL) {

		if((cmp = strstr(direntPtr->d_name, ".bin")) == NULL) {
#ifdef HARD_DEBUG
			printf("found file that is not a .bin, '%s'\n", direntPtr->d_name);
#endif
			continue;
		}
		strcpy(&pathBuf[pathLen], direntPtr->d_name);
			
/*
**	Open the binary file.
*/
		if((bfd[i] = open(pathBuf, O_RDONLY)) == -1) {
#ifdef HARD_DEBUG
			printf("DEBUG: Unable to open bin file '%s'\n", pathBuf);
#endif
			continue;
		}

/*
**	Read the file header to get sizes of the various arrays and library name.
*/
		if((err = read(bfd[i], (char *)&header[i], sizeof(binHeader_t)))
					!= sizeof(binHeader_t))
		{
#ifdef DEBUG
			printf("DEBUG: Unable to read bin file header, file '%s'\n",
						pathBuf);
#endif
			return(-1);
		}

/*
**	If a .so file is associated with the package, dlopen it.
*/
#ifdef SHARED_LIBS
#ifdef HARD_DEBUG
		printf("library name for file %d is '%s'\n", i, header[i].libName);
#endif
		if(header[i].libName[0] != 0) {
			if((handleTable[i].dlHandle =
					dlopen(header[i].libName, RTLD_LAZY)) == NULL) {
#ifdef HARD_DEBUG
				printf("DEBUG: Unable to dlopen library file '%s'\n",
								header[i].libName);
				printf("ERROR: %s\n", dlerror());
#endif
				continue;
			}
		}
#endif


	
		IntegerParameterCount += GETINT32(header[i].numIntParams);
		PkgCount[i] = GETINT32(header[i].numIntParams);

		BooleanParameterCount += GETINT32(header[i].numBoolParams);
		PkgCount[i] += GETINT32(header[i].numBoolParams);

		StringParameterCount += GETINT32(header[i].numStringParams);
		PkgCount[i] += GETINT32(header[i].numStringParams);

		i++;
	} 

/*
**	If we got here due to an interrupt, try continuing.
*/
	if(errno == EINTR)
		goto top;

	closedir(dirFd);

	num_packages = i;

/*
**	Get the total number of configuration parameters
*/
	ConfigurationParameterCount = StringParameterCount +
							IntegerParameterCount + BooleanParameterCount;

#ifdef HARD_DEBUG
	printf("ConfigurationParameterCount = '%d'\n", ConfigurationParameterCount);
#endif

/*
**	Allocate space for parameters and defaults, initialize defaults
*/
	if(err = AllocTables())
		return( -1 );

/*
**	Now read the files and build the configuration database
*/
	for(i=0; i<num_packages; i++) {
		if(bfd[i] < 0)
			continue;

/*
**	Read records from binary file and fill in schema configuration
*/
		for(j=0; j<PkgCount[i]; j++) {
			if((err = read(bfd[i], (char *)&record, sizeof(record))) !=
							sizeof(record))
			{
#ifdef DEBUG
				printf("DEBUG: Unable to read bin file record from file %d\n", i);
#endif
				return(-1);
			}

/*
**	Read in values of common parameters
*/
			ConfigurationParameters[parmIdx].type = GETINT32(record.dataType);
			ConfigurationParameters[parmIdx].rel_order = parmIdx;
			ConfigurationParameters[parmIdx].name = (char *)strdup(record.paramName);
			ConfigurationParameters[parmIdx].folder = GETINT32(record.folder);
			ConfigurationParameters[parmIdx].description = GETINT32(record.description);
			ConfigurationParameters[parmIdx].helpString = GETINT32(record.helpString);
			ConfigurationParameters[parmIdx].format = GETINT32(record.displayFormat);

/*
**	Read in INTEGER parameters
*/
			if(ConfigurationParameters[parmIdx].type == NWCP_INTEGER ) {
#ifdef HARD_DEBUG
				printf("record %d is type INTEGER\n", parmIdx);
#endif
				if(record.validName[0] == 0) {
					if(record.lowLimit == 0 && record.highLimit == 0)
						ConfigurationParameters[parmIdx].validation.opaque = NULL;
					else {
						IntegerValidationData[intIdx].func = NULL;
						IntegerValidationData[intIdx].min = 
									GETINT32(record.lowLimit);
						IntegerValidationData[intIdx].max = 
									GETINT32(record.highLimit);
						ConfigurationParameters[parmIdx].validation.data =
								&IntegerValidationData[intIdx];
					}
				} else {
#ifdef SHARED_LIBS
					IntegerValidationData[intIdx].func = 
						(ivf_t)dlsym(handleTable[i].dlHandle, record.validName);
#else
					IntegerValidationData[intIdx].func = 
						(ivf_t)GetSymbol(record.validName);
#endif
					ConfigurationParameters[parmIdx].validation.data =
								&IntegerValidationData[intIdx];
				}
				IntegerParameterDefaults[intIdx] = GETINT32(record.intDefault);
				IntegerParameterValues[intIdx] = GETINT32(record.intDefault);
				ConfigurationParameters[parmIdx].def_val =
					(cv_t)&IntegerParameterDefaults[intIdx];
				ConfigurationParameters[parmIdx].cur_val =
					(cv_t)&IntegerParameterValues[intIdx];

				intIdx++;
			} else if(ConfigurationParameters[parmIdx].type == NWCP_BOOLEAN ) {
/*
**	Read in BOOLEAN parameters
*/
#ifdef HARD_DEBUG
				printf("record %d is type BOOLEAN\n", parmIdx);
#endif
				if(record.validName[0] == 0) {
					ConfigurationParameters[parmIdx].validation.opaque = NULL;
				} else {
#ifdef SHARED_LIBS
					ConfigurationParameters[parmIdx].validation.action = 
						(bvf_t)dlsym(handleTable[i].dlHandle, record.validName);
#else
					ConfigurationParameters[parmIdx].validation.action = 
						(bvf_t)GetSymbol(record.validName);
#endif
				}
				BooleanParameterDefaults[boolIdx] = GETINT32(record.boolDefault);
				BooleanParameterValues[boolIdx] = GETINT32(record.boolDefault);
				ConfigurationParameters[parmIdx].def_val =
						(cv_t)&BooleanParameterDefaults[boolIdx];
				ConfigurationParameters[parmIdx].cur_val =
						(cv_t)&BooleanParameterValues[boolIdx];

				boolIdx++;
			} else if(ConfigurationParameters[parmIdx].type == NWCP_STRING ) {
/*
**	Read in STRING parameters
*/
#ifdef HARD_DEBUG
				printf("record %d is type STRING\n", parmIdx);
#endif
				if(record.validName[0] == 0) {
					ConfigurationParameters[parmIdx].validation.opaque = NULL;
				} else {
#ifdef SHARED_LIBS
					ConfigurationParameters[parmIdx].validation.func = 
						(svf_t)dlsym(handleTable[i].dlHandle, record.validName);
#else
					ConfigurationParameters[parmIdx].validation.func = 
						(svf_t)GetSymbol(record.validName);
#endif
				}
				memcpy(&StringParameterDefaults[strIdx*NWCM_MAX_STRING_SIZE],
								record.stringDefault, NWCM_MAX_STRING_SIZE);
				memcpy(&StringParameterValues[strIdx*NWCM_MAX_STRING_SIZE],
								record.stringDefault, NWCM_MAX_STRING_SIZE);
				ConfigurationParameters[parmIdx].def_val =
					(cv_t)&StringParameterDefaults[strIdx*NWCM_MAX_STRING_SIZE];
				ConfigurationParameters[parmIdx].cur_val =
					(cv_t)&StringParameterValues[strIdx*NWCM_MAX_STRING_SIZE];

				strIdx++;
			} else {
#ifdef HARD_DEBUG
				printf("INVALID Parameter type.  Type = %d\n",
								ConfigurationParameters[parmIdx].type);
#endif
				return(-1);
			}

			parmIdx++;
		}
		close(bfd[i]);
	}

#ifdef HARD_DEBUG
	for(parmIdx=0; parmIdx<ConfigurationParameterCount; parmIdx++) {
		printf("\nRECORD #%d\n", parmIdx);
		printf("name: %s\n", ConfigurationParameters[parmIdx].name);
		printf("rel_order: %d\n", ConfigurationParameters[parmIdx].rel_order);
		printf("folder: %d\n", ConfigurationParameters[parmIdx].folder);
		printf("description: %d\n", ConfigurationParameters[parmIdx].description);
		printf("helpString: %d\n", ConfigurationParameters[parmIdx].helpString);
		printf("dataType: %d\n", ConfigurationParameters[parmIdx].type);
		printf("validName: %s\n", record.validName);
		printf("validNamePtr: %x\n", ConfigurationParameters[parmIdx].validation.data->func);
		if(ConfigurationParameters[parmIdx].type == NWCP_INTEGER ) {
			printf("lowLimit: 0x%lx\n", ConfigurationParameters[parmIdx].validation.data->min);
			printf("highLimit: 0x%lx\n", ConfigurationParameters[parmIdx].validation.data->max);
		}
		printf("displayFormat: %d\n", ConfigurationParameters[parmIdx].format);
		if(ConfigurationParameters[parmIdx].type == NWCP_STRING ) {
			printf("str.def_val: %s\n", (char *)ConfigurationParameters[parmIdx].def_val);
			printf("str.cur_val: %s\n", (char *)ConfigurationParameters[parmIdx].cur_val);
		} else if(ConfigurationParameters[parmIdx].type == NWCP_INTEGER ) {
			printf("int.def_val: %d (%d)\n", ConfigurationParameters[parmIdx].def_val,
						*((unsigned long *)ConfigurationParameters[parmIdx].def_val));
			printf("int.cur_val: %d (%d)\n", ConfigurationParameters[parmIdx].cur_val,
						*((unsigned long *)ConfigurationParameters[parmIdx].cur_val));
		} else {
			printf("bool.def_val: %d (%d)\n", ConfigurationParameters[parmIdx].def_val,
						*((int *)ConfigurationParameters[parmIdx].def_val));
			printf("bool.cur_val: %d (%d)\n", ConfigurationParameters[parmIdx].cur_val,
						*((int *)ConfigurationParameters[parmIdx].cur_val));
		}
		getchar();
	}
	printf("End of binary file read.  Close and return.\n");
#endif

/*
**	Sort the configuration table parameters
*/
	(void) qsort(ConfigurationParameters, ConfigurationParameterCount,
				sizeof(struct cp_s), cpcmp);

	return(0);
}


static int
cpcmp( const void *s1, const void *s2)
{
	return strcmpi(((struct cp_s *)s1)->name, ((struct cp_s *)s2)->name);
}


static int
AllocTables()
{
	if((ConfigurationParameters = (struct cp_s *)calloc(ConfigurationParameterCount,
				sizeof(struct cp_s))) == NULL)
	{
		printf("Unable to allocate memory segment #1\n");
		return(-1);
	}
		
	if((IntegerParameterValues =
				(unsigned long *)calloc(IntegerParameterCount,
				sizeof(unsigned long))) == NULL)
	{
		printf("Unable to allocate memory segment #2\n");
		return(-1);
	}
	if((IntegerParameterDefaults =
				(unsigned long *)calloc(IntegerParameterCount,
				sizeof(unsigned long))) == NULL)
	{
		printf("Unable to allocate memory segment #3\n");
		return(-1);
	}
	if((IntegerValidationData =
				(struct ipvd_s *)calloc(IntegerParameterCount,
				sizeof(struct ipvd_s))) == NULL)
	{
		printf("Unable to allocate memory segment #4\n");
		return(-1);
	}
	if((BooleanParameterValues = (int *)calloc(BooleanParameterCount,
				sizeof(int))) == NULL)
	{
		printf("Unable to allocate memory segment #5\n");
		return(-1);
	}
	if((BooleanParameterDefaults = (int *)calloc(BooleanParameterCount,
				sizeof(int))) == NULL)
	{
		printf("Unable to allocate memory segment #6\n");
		return(-1);
	}
	if((StringParameterValues = (char *)calloc(StringParameterCount,
				NWCM_MAX_STRING_SIZE)) == NULL)
	{
		printf("Unable to allocate memory segment #7\n");
		return(-1);
	}
	if((StringParameterDefaults = (char *)calloc(StringParameterCount,
				NWCM_MAX_STRING_SIZE)) == NULL)
	{
		printf("Unable to allocate memory segment #8\n");
		return(-1);
	}

	return( 0 );
}

/*
**  BindInstalledDomains
**  Bind the message domain containing the description and help messages
**  for the packages.
*/
int
BindInstalledDomains( void )
{
	int i, domain;

	for(i = 0; i<num_packages; i++) {
		domain = GETINT32(header[i].messDomain);
		if(MsgBindDomainFunc(domain, header[i].domainFile,
					header[i].domainRevStr, FALSE))
		{
			if(i == 0)
				return(-1);
		}
	}
	return( 0 );
}

#ifndef SHARED_LIBS

static void *
GetSymbol(char *symbolName)
{
	int 	i;

/*
**	Variable references are found in symtab.h
*/
	for(i=0; i<SYMBOL_COUNT; i++) {
		if(!strcmp(symbolName, symTab[i].name)) {
			return(symTab[i].funcPtr);
		}
	}
	printf("GetSymbol: Could not resolve '%s'\n", symbolName);
	return(NULL);
}
#endif
