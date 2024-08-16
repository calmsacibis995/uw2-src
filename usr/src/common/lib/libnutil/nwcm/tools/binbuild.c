/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnwutil:common/lib/libnutil/nwcm/tools/binbuild.c	1.6"
#ident	"$Id: binbuild.c,v 1.5.2.1 1994/10/19 18:24:18 vtag Exp $"

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
#include <sys/nwtdr.h>
#include "../../binhead.h"
		
#ifndef NULL
#define NULL	0
#endif

#define		binFileMode		0644

extern char *libName;
extern char *domainFile;
extern char *domainRevStr;
extern int  domain;

main()
{
	binHeader_t		header;
	binRecord_t		record;
	char			schemaName[20];
	int 			bfd, i, j, k, m, id;
	char			*tmpPtr;


	strcpy(schemaName, "schema.bin");

#ifdef HARD_DEBUG
	printf("Building %s file.\n", schemaName);
	printf("Header size = '%d', record size is '%d'\n", sizeof(header),
								sizeof(record));
#endif
/*
**	Create the bin file.
*/
	if((bfd = creat(schemaName, 0644)) == -1) {
		printf("DEBUG: Unable to create bin file '%s'\n", schemaName);
		exit(-1);
	}

/*
**	Read sizes of the various arrays.  Write them to .bin file.
*/
	header.numIntParams = GETINT32(IntegerParameterCount);
	header.numBoolParams = GETINT32(BooleanParameterCount);
	header.numStringParams = GETINT32(StringParameterCount);
	strcpy(header.libName, libName);
	strcpy(header.domainFile, domainFile);
	strcpy(header.domainRevStr, domainRevStr);
	header.messDomain = GETINT32(domain);

	(void) write(bfd, (char *)&header, sizeof(header));

/*
**	Fill in parameter records and write them to the .bin file.
**	Order is integer, boolean, string
**
**  1. - INTEGERS
*/
	for(i=0, j=0, k=0, m=0, id = 0; i<ConfigurationParameterCount; i++) {
#ifdef HARD_DEBUG
		printf("RECORD %d\n--------------\n", i);
#endif
		memset((char *)&record, 0, sizeof(binRecord_t));
		strcpy(record.paramName, ConfigParameters[i].name);
		record.folder = GETINT32(ConfigParameters[i].folder);
		record.description = GETINT32(ConfigParameters[i].description);
		record.helpString = GETINT32(ConfigParameters[i].helpString);
		record.dataType = GETINT32(ConfigParameters[i].type);
		record.displayFormat = GETINT32(ConfigParameters[i].format);

		if(ConfigParameters[i].type == NWCP_INTEGER ) {
			if(ConfigParameters[i].validation.opaque != NULL) {
				if(ConfigParameters[i].validation.data->func) {
					strcpy(record.validName, &intValFuncs[j]);
					j++;
				} else {
					record.lowLimit =
						GETINT32(ConfigParameters[i].validation.data->min);
					record.highLimit =
						GETINT32(ConfigParameters[i].validation.data->max);
				}
			}
			record.intDefault = GETINT32(IntParameterDefaults[id]);
			id++;

		} else if(ConfigParameters[i].type == NWCP_BOOLEAN ) {
			if(ConfigParameters[i].validation.func != NULL) {
				strcpy(record.validName, &boolValFuncs[k]);
				k++;
			}
			record.boolDefault =
					GETINT32(*((int *)ConfigParameters[i].def_val));
		} else if(ConfigParameters[i].type == NWCP_STRING ) {
			if(ConfigParameters[i].validation.func != NULL) {
				strcpy(record.validName, &stringValFuncs[m]);
				m++;
			}
			strcpy(record.stringDefault, (char *)ConfigParameters[i].def_val);
		} 

#ifdef HARD_DEBUG
		printf("Name: %s\n", record.paramName);
		printf("folder: %d\n", GETINT32(record.folder));
		printf("description: %d\n", GETINT32(record.description));
		printf("helpString: %d\n", GETINT32(record.helpString));
		printf("dataType: %d\n", GETINT32(record.dataType));
		printf("validName: %s\n", record.validName);
		printf("lowLimit: %d\n", GETINT32(record.lowLimit));
		printf("highLimit: %d\n", GETINT32(record.highLimit));
		printf("displayFormat: %d\n", GETINT32(record.displayFormat));
		printf("intDefault: %d\n", GETINT32(record.intDefault));
		printf("boolDefault: %d\n", GETINT32(record.boolDefault));
		printf("stringDefault: %s\n", record.stringDefault);
		getchar();
#endif

		(void) write(bfd, (char *)&record, sizeof(record));
	}

#ifdef HARD_DEBUG
	printf("End of binary file build.  %d records written. Close and exit.\n",
				ConfigurationParameterCount);
#endif
	close(bfd);
	exit(0);
}
