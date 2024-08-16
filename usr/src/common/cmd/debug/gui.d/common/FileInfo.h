/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef FILEINFO_H
#define FILEINFO_H
#ident	"@(#)debugger:gui.d/common/FileInfo.h	1.3"

enum FileType
{
	FT_UNKNOWN,	// unknown type
	FT_EXEC,	// executable
	FT_CORE,	// core file
	FT_TEXT		// ascii text file
};

class ELFcoreInfo;

class FileInfo 
{
	FileType 	ftype;		// file type
	int		fd;		// open file descriptor
	char 		*fname;		// file name
	ELFcoreInfo	*core_info;	// core file info
public:
	FileInfo(char *name);
	~FileInfo();

	FileType type() { return ftype; }
	char	*get_obj_name();
};

#endif
