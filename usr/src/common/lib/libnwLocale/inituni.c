/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:inituni.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/inituni.c,v 1.1 1994/09/26 17:20:37 rebekah Exp $"
/****************************************************************************
 *                                                                          *
 * Library name:  NWLOCALE.LIB                                              *
 *	                                                                         *
 * Filename:      INITUNI.C                                                 *
 *                                                                          *
 * Date Created:  August 1992                                               *
 *                                                                          *
 * Version:       1.00                                                      *
 *                                                                          *
 * (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.          *
 *                                                                          *
 * No part of this file may be duplicated, revised, translated, localized   *
 * or modified in any manner or compiled, linked or uploaded or downloaded	 *
 * to or from any computer system without the prior written consent of      *
 * Novell, Inc.                                                             *
 *                                                                          *
 ****************************************************************************/


#define NWL_EXCLUDE_TIME 1
#define NWL_EXCLUDE_FILE 1

#ifdef WIN32
# define  NOATOM                  // Atom Manager routines
# define  NOCLIPBOARD             // Clipboard routines
# define  NOCOMM                  // to eliminate lint warnings
# define  NODEFERWINDOWPOS        // DeferWindowPos routines
# define  NOGDICAPMASKS           // CC_*, LC_*, PC_*, CP_*, etc
# define  NOKANJI                 // Kanji support stuff
# define  NOMETAFILE              // typedef METAFILEPICT
# define  NOMINMAX                // NO min(a,b) or max(a,b)
# define  NOOPENFILE              // OpenFile(), OemToAnsi(), AnsiToOem()
# define  NOPROFILER              // Profiler interface
# define  NOSOUND                 // Sound Driver routines
# undef   OEMRESOURCE             // OEM Resource values
# undef   NOLSTRING               // using lstrlen()
# include <windows.h>
#endif

#ifdef N_PLAT_UNIX
# include	<sys/stat.h>
# include	<errno.h>
# include	<fcntl.h>
# include	<libgen.h>
# include	<stdlib.h>
#endif

#if defined(__BORLANDC__)
# include <alloc.h>
#else
# include <malloc.h>
#endif

#if (defined NWDOS || (defined N_PLAT_MSW && defined N_ARCH_16) || defined N_PLAT_OS2)
# include <dos.h>
# include <string.h>
#endif

#ifdef N_PLAT_OS2
# include <os2def.h>
# define INCL_DOS
# include	<bsedos.h>
#endif

#if defined N_PLAT_MSW && defined N_ARCH_16
# include <windows.h>
#endif

#include "ntypes.h"
#ifdef N_PLAT_UNIX
# include	"libnwlocale_mt.h"
#endif
#include "nwlocale.h"

#if !(defined N_PLAT_MSW && defined N_ARCH_32)
#include "localias.h"
#include "enable.h"
#include "locifunc.h"
#endif

#ifndef N_PLAT_OS2
#include "locdefs.h"
#endif

#include "uniintrn.h"
#include "unicode.h"

#ifndef N_PLAT_UNIX		/* UNIX doesn't use these internal functions */
int far _NWUniIntdos(union REGS far *rregs,
                     struct SREGS far *sregs);

#if !defined(N_PLAT_OS2)
static char far *uniGetenv(char N_FAR *envVariable);
#endif
static char far *GetEnvPtr(void);
static int CheckReadAccess(char N_FAR *filePath);
static int CheckCurrentDirectory(char N_FAR *fullPath,
                                 char N_FAR *fileName);
static int CheckFilePath(char N_FAR *fullPath,
                         char N_FAR *path,
                         char N_FAR *fileName);
static int CheckSearchPath(char N_FAR *fullPath,
                           char N_FAR *fileName);
#endif /* UNIX */

#define PATH_SIZE	256

void N_FAR *localToUniHandle = NULL;
void N_FAR *uniToLocalHandle = NULL;
void N_FAR *compareHandle = NULL;
void N_FAR *monoHandle = NULL;

/* routines and globals used by near model too */
int GetUniFileFullPath(char N_FAR *fullPath,
                              char N_FAR *loadPath,
                              char N_FAR *fileName);

int GetLoadPath(char N_FAR *loadPath);
int loaded = 0;

/*********************************************************/
#if defined( WIN32 ) || defined( N_PLAT_NLM )
int N_API NWFreeUnicodeTables(void)
{
	return 0;
}
#else

int N_API NWFreeUnicodeTables(void)
{
#if defined(N_PLAT_UNIX)
	MUTEX_LOCK(&_libnwlocale_loaded_lock);
#endif
    /* only clear out the tables if we are already loaded */
    if (loaded)
    {
        /* if this is the last thread, clear out our tables */
        if (--loaded == 0 && localToUniHandle != NULL)
        {
            /* unload the rule tables */
            NWUnloadRuleTable(localToUniHandle);
            NWUnloadRuleTable(uniToLocalHandle);
            NWUnloadRuleTable(compareHandle);
            NWUnloadRuleTable(monoHandle);

            /* zero the pointers to avoid problems */
            localToUniHandle = NULL;
            uniToLocalHandle = NULL;
            compareHandle = NULL;
            monoHandle = NULL;
        }
    }
#if defined(N_PLAT_UNIX)
	MUTEX_UNLOCK(&_libnwlocale_loaded_lock);
#endif

    return 0;
}
#endif /* End of everything but WIN32 and N_PLAT_NLM */

#ifdef WIN32
int N_API NWGetUnicodeToLocalHandle
(
   void N_FAR * N_FAR *handle
)
{
   *handle = (void N_FAR *) 0xFFFFFFFF;
   return(0);
}
#elif defined( N_PLAT_NLM )
   /*  This function is defined and exported by UNICODE.NLM  */

#else
int N_API NWGetUnicodeToLocalHandle
(
	void N_FAR * N_FAR *handle
)
{
   *handle = uniToLocalHandle;
	
#ifdef N_PLAT_OS2
	if (*handle)
		DosGetSeg(SELECTOROF(*handle));
#endif

	return (uniToLocalHandle == 0) ? UNI_HANDLE_BAD : 0;
		
}
#endif /* End of everything but WIN32 and N_PLAT_NLM */

#ifdef WIN32
int N_API NWGetLocalToUnicodeHandle
(
   void N_FAR * N_FAR *handle
)
{
   *handle = (void N_FAR *) 0xFFFFFFFF;
   return(0);
}

#elif defined( N_PLAT_NLM )
   /*  This function is defined and exported by UNICODE.NLM  */

#else

int N_API NWGetLocalToUnicodeHandle
(
	void N_FAR * N_FAR *handle
)
{
	*handle = localToUniHandle;
	
#ifdef N_PLAT_OS2
	if (*handle)
		DosGetSeg(SELECTOROF(*handle));
#endif

	return (localToUniHandle == 0) ? UNI_HANDLE_BAD : 0;
		
}
#endif /* End of everything but WIN32 and N_PLAT_NLM */

#ifdef WIN32
int N_API NWGetMonocaseHandle
(
	void N_FAR * N_FAR *handle
)
{
   *handle = (void N_FAR *)0xFFFFFFFF;
   return(0);
}
#elif defined( N_PLAT_NLM )
   /*  This function is defined and exported by UNICODE.NLM  */

#else

int N_API NWGetMonocaseHandle
(
	void N_FAR * N_FAR *handle
)
{
	*handle = monoHandle;
	
#ifdef N_PLAT_OS2
	if (*handle)
		DosGetSeg(SELECTOROF(*handle));
#endif

	return (monoHandle == 0) ? UNI_HANDLE_BAD : 0;
		
}
#endif /* End of everything but WIN32 and N_PLAT_NLM */

#ifdef WIN32
int N_API NWGetCollationHandle
(
   void N_FAR * N_FAR *handle
)
{
   *handle = (void N_FAR *) 0xFFFFFFFF;
   return(0);

}

#elif defined( N_PLAT_NLM )

   /*  This function is identical to NWGetMonocaseHandle to now  */
int N_API NWGetCollationHandle
(
   void N_FAR * N_FAR *handle
)
{
   return( NWGetMonocaseHandle((LONG *) handle ) );
}

#else

int N_API NWGetCollationHandle
(
	void N_FAR * N_FAR *handle
)
{
	*handle = compareHandle;
	
#ifdef N_PLAT_OS2
	if (*handle)
		DosGetSeg(SELECTOROF(*handle));
#endif

	return (compareHandle == 0) ? UNI_HANDLE_BAD : 0;
		
}

#endif /* End of everything but WIN32 and N_PLAT_NLM */

#ifdef WIN32
int N_API NWInitUnicodeTables
(
   int            countryCode, 
   int            codePage
)
{
	if(IsValidCodePage((unsigned int) codePage) == TRUE)
		return 0;
	else
		return UNI_LOAD_FAILED;
}

#elif defined( N_PLAT_NLM )
int N_API NWInitUnicodeTables
(
   int      countryCode,
   int      codePage
)
{
   /*  Clear up compiler warnings  */
   countryCode = countryCode;
   codePage = codePage;


   return( 0 );
   /*  This leaves only the code page loaded by UNICODE.NLM as accessable.  */
}

#else

int N_API NWInitUnicodeTables
(
	int countryCode,
	int codePage
)
{
   static int loadedCountry = 0,
              loadedCodePage = 0;
   char   codeP[5];
   char   fullPath[PATH_SIZE],
          loadPath[PATH_SIZE],
          tableName[13],
          countryID[4],

/* #ifdef NWDOS
			 shortMachineName[4],
#endif
5-25-93 */
          temp[4];
   int    ccode;
   size_t len;
	

#if defined(N_PLAT_UNIX)
	MUTEX_LOCK(&_libnwlocale_loaded_lock);
#endif
   if (loaded)
   {
      if (countryCode != loadedCountry ||
            codePage != loadedCodePage)
#if defined(N_PLAT_UNIX)
	{
		MUTEX_UNLOCK(&_libnwlocale_loaded_lock);
         	return UNI_ALREADY_LOADED;
	}
#else
         return UNI_ALREADY_LOADED;
#endif

      loaded++;
#if defined(N_PLAT_UNIX)
	MUTEX_UNLOCK(&_libnwlocale_loaded_lock);
#endif
      return 0;
   }
   else
	{
      loadedCountry = countryCode;
      loadedCodePage = codePage;


   	NWitoa(codePage, (char N_FAR *)codeP, 10);

#ifndef N_PLAT_UNIX
		if ((ccode = GetLoadPath(loadPath)) != 0)
      	return (UNI_NO_SUCH_FILE);
#endif

   	NWitoa(countryCode, temp, 10);
   	len = NWstrlen(temp);
   	countryID[0] = '\0';
   	nwstrncat(countryID, "000", (3 - len));
   	nwstrcat(countryID, temp);

   	/*
    	* Load the Unicode-to-local rule table
    	*/
   	nwstrcpy(tableName, "UNI_");

/* Deleted 5-25-93 This special case would require each Japanese machine to
have its own Unicode tables. This is not necessary as the translation rules
are not hardware dependent. Now Japanese follows the same rules as other
languages, ie. UNI_932.081.

#ifdef NWDOS
		if (countryCode == JAPAN)
		{
			ccode = NWGetShortMachineName(shortMachineName);
	   	nwstrcat(tableName, shortMachineName);
		}
		else
#endif End of deletions 5-25-93 */

   	nwstrcat(tableName, codeP);
   	nwstrcat(tableName, ".");
   	nwstrcat(tableName, countryID);

#if defined(N_PLAT_UNIX)
   	if ((ccode = GetUniFileFullPath(fullPath, loadPath, tableName)) != 0)
	{
		MUTEX_UNLOCK(&_libnwlocale_loaded_lock);
      		return ccode;
	}
   	if ((ccode = NWLoadRuleTable(fullPath, &uniToLocalHandle)) != 0)
	{
		MUTEX_UNLOCK(&_libnwlocale_loaded_lock);
      		return ccode;
	}
#else
   	if ((ccode = GetUniFileFullPath(fullPath, loadPath, tableName)) != 0)
      	return ccode;
   	if ((ccode = NWLoadRuleTable(fullPath, &uniToLocalHandle)) != 0)
      	return ccode;
#endif

		/*
    	* Load the Unicode-compare rule table
    	*/
   	nwstrcpy(tableName, "UNI_COL.");
   	nwstrcat(tableName, countryID);

#if defined(N_PLAT_UNIX)
   	if ((ccode = GetUniFileFullPath(fullPath, loadPath, tableName)) != 0)
	{
		MUTEX_UNLOCK(&_libnwlocale_loaded_lock);
      		return ccode;
	}
   	if ((ccode = NWLoadRuleTable(fullPath, &compareHandle)) != 0)
	{
		MUTEX_UNLOCK(&_libnwlocale_loaded_lock);
      		return ccode;
	}
#else
   	if ((ccode = GetUniFileFullPath(fullPath, loadPath, tableName)) != 0)
      	return ccode;
   	if ((ccode = NWLoadRuleTable(fullPath, &compareHandle)) != 0)
      	return ccode;
#endif

		/*
    	* Load the Unicode-monocase rule table
    	*/
   	nwstrcpy(tableName, "UNI_MON.");
   	nwstrcat(tableName, countryID);

#if defined(N_PLAT_UNIX)
   	if ((ccode = GetUniFileFullPath(fullPath, loadPath, tableName)) != 0)
	{
		MUTEX_UNLOCK(&_libnwlocale_loaded_lock);
      		return ccode;
	}
   	if ((ccode = NWLoadRuleTable(fullPath, &monoHandle)) != 0)
	{
		MUTEX_UNLOCK(&_libnwlocale_loaded_lock);
      		return ccode;
	}
#else
   	if ((ccode = GetUniFileFullPath(fullPath, loadPath, tableName)) != 0)
      	return ccode;
   	if ((ccode = NWLoadRuleTable(fullPath, &monoHandle)) != 0)
      	return ccode;
#endif

   	/*
    	* Load the local-to-Unicode rule table
    	*/
   	nwstrcpy(tableName, codeP);
   	nwstrcat(tableName, "_UNI.");
   	nwstrcat(tableName, countryID);

#if defined(N_PLAT_UNIX)
   	if ((ccode = GetUniFileFullPath(fullPath, loadPath, tableName)) != 0)
	{
		MUTEX_UNLOCK(&_libnwlocale_loaded_lock);
      		return ccode;
	}
   	if ((ccode = NWLoadRuleTable(fullPath, &localToUniHandle)) != 0)
	{
		MUTEX_UNLOCK(&_libnwlocale_loaded_lock);
      		return ccode;
	}

   	loaded++;
	MUTEX_UNLOCK(&_libnwlocale_loaded_lock);
	   return 0;   
#else
   	if ((ccode = GetUniFileFullPath(fullPath, loadPath, tableName)) != 0)
      	return ccode;
   	if ((ccode = NWLoadRuleTable(fullPath, &localToUniHandle)) != 0)
      	return ccode;

   	loaded++;
	   return 0;   
#endif
	}
}


int GetUniFileFullPath
(
   char N_FAR *fullPath,
   char N_FAR *loadPath,
   char N_FAR *fileName
)
{
   int ccode;
   char N_FAR *workPath;
   char path[PATH_SIZE];

#ifdef N_PLAT_UNIX

   struct stat statVar;

   /* first look in the current directory.  I used stat and then look 
      at ccode to see if the entry was there.
   */
   ccode = stat(fileName, &statVar);
   if(!ccode) /* the file is in the current directory */
   {
      strcpy(fullPath, fileName);
      ccode =0;
   }
	else
   {
	/* let UNIX search the path for us */
		workPath = pathfind(getenv("PATH"), fileName, "r");
		if(workPath == NULL)
				ccode = UNI_NO_SUCH_FILE;
		else
			{
				strcpy(fullPath, workPath);
				ccode = 0;
			}
   } 
   return(ccode);


#else /* Do the following if we're not UNIX */

	/*
    *  Look first in the current working directory
    */
   if ((ccode = CheckCurrentDirectory(fullPath, fileName)) != 0)
   {
      /*
       * Look in the load directory
       */
      if ((ccode = CheckFilePath(fullPath, loadPath, fileName)) != 0)
      {
         /*
         * Look in the NLS directory under the load directory
         */
         nwstrcpy(path, loadPath);
         nwstrcat(path, "\\NLS");
   
         if ((ccode = CheckFilePath(fullPath, path, fileName)) != 0)
         {
            /*
            * Look in the NLS directory a sibling to the load directory
            */
            nwstrcpy(path, loadPath);
            workPath = nwstrrchr(path, '\\');
            if (workPath != 0)
               *workPath = '\0';
            nwstrcat(path, "\\NLS");
   
            if ((ccode = CheckFilePath(fullPath, path, fileName)) != 0)
               /*
               * Look in the environment under PATH and for OS/2 look in DPATH
               */
               ccode = CheckSearchPath(fullPath, fileName);
         }
      }
   }

   return ccode;

#endif  /* Everything but UNIX */

}
/* GetMessageFileFullPath */


/*********************************************************/

#ifndef N_PLAT_UNIX	/* Don't compile this routine for UNIX */

int GetLoadPath
(
   char N_FAR *loadPath
)
{
   char N_FAR *lpp;

#if !(defined N_PLAT_MSW && defined N_ARCH_16)
   lpp = GetEnvPtr();

   do
   {
      while (*lpp++ != 0)
         ;
   } while (*lpp++ != 0);
#if defined(NWDOS)
   lpp += 2;
#endif

   nwstrcpy(loadPath, lpp);

#else /* defined N_PLAT_MSW && defined N_ARCH_16 */
	 /*
      This method works well, except that in Real Mode
      the TaskFindHandle call makes protected mode calls.
      This code was changed to avoid this problem.
			JBI 12/11/92

   * TASKENTRY FAR *lpte;
   * HANDLE		htask;
	 *
   * htask = GetCurrentTask();
   * lpte = (TASKENTRY FAR *) nwmalloc(sizeof(TASKENTRY));
   * lpte->dwSize = sizeof(TASKENTRY);
   * if (!TaskFindHandle(lpte, htask))
   * {
   *    nwfree(lpte);
   *    return UNI_NO_LOAD_DIR;
   * }
	 *
   * if ((ccode = GetModuleFileName(lpte->hInst, (LPSTR) loadPath,
   *    PATH_SIZE)) == 0)
   * {
   * 	ccode++;
   *    nwfree(lpte);
   *    return UNI_NO_LOAD_DIR;
   * }
   * nwfree(lpte);
	 * hInst = (((HINSTANCE N_FAR *) htask) << 16) + 0x1C;
	 */

   /*
	    This is really bad programing, but after reading
			"Undocumented Windows" it appears that it is safe.
			JBI 12/11/92
   */
   HANDLE		htask;
	 BYTE			N_FAR *hInst;

   htask = GetCurrentTask();

   *((unsigned N_FAR *)&(hInst) + 1) = htask;
   *((unsigned N_FAR *)&(hInst)) = 0;

	 /* make sure this is really the Task Database -JBI */
	 if (*(nuint16 N_FAR*)(hInst + 0xFA) != 0x4454) /* 4454 = "TD" */
      return UNI_NO_LOAD_DIR;

	 /* If this is the Task Database, go ahead and use it */
   if (GetModuleFileName(*(nuint16 N_FAR *)(hInst + 0x1C),
         (LPSTR) loadPath, PATH_SIZE) == 0)
      return UNI_NO_LOAD_DIR;

#endif  /* defined N_PLAT_MSW && defined N_ARCH_16 */

   lpp = nwstrrchr(loadPath, '\\');
   if (lpp != 0)
      *lpp = '\0';
   else
      loadPath[0] = '\0';

   return 0;
}
/* GetLoadPath */

#endif /* Don't compile the above routine for UNIX */

/****************************************************************************/

#ifndef N_PLAT_UNIX	/* Don't compile this routine for UNIX */


static int CheckCurrentDirectory
(
   char N_FAR *fullPath,
   char N_FAR *fileName
)
{
#if defined(N_PLAT_OS2)
   return DosSearchPath(SEARCH_CUR_DIRECTORY, ".", fileName, fullPath,
      PATH_SIZE);

#elif defined N_PLAT_MSW && defined N_ARCH_16
   if (GetWindowsDirectory(fullPath, PATH_SIZE) == 0)
      return UNI_OPEN_FAILED;

   nwstrcat(fullPath, "\\");
   nwstrcat(fullPath, fileName);

   return CheckReadAccess(fullPath);

#else  /* NWDOS */
# define DOS_GET_CURRENT_DISK          0x19
# define DOS_GET_CURRENT_DIRECTORY     0x47
   int ccode;
   char N_FAR *p;
   union REGS regs;
   struct SREGS sregs;

   regs.h.ah = DOS_GET_CURRENT_DISK;

   _NWUniIntdos(&regs, NULL);

   p = fullPath;
   *p++ = (char) ('A' + regs.h.al);
   *p++ = ':';
   *p++ = '\\';

   regs.h.ah = DOS_GET_CURRENT_DIRECTORY;
   regs.h.dl = (unsigned char) (regs.h.al + 1);
   sregs.ds = FP_SEG(p);
   regs.x.si = FP_OFF(p);

   ccode = _NWUniIntdos(&regs, &sregs);
   if (regs.x.cflag != 0)
      return ccode;
	
	/* if we're at the root, don't add a backslash */
   if (strlen(fullPath) > 3)
		nwstrcat(fullPath, "\\");
   nwstrcat(fullPath, fileName);

   return CheckReadAccess(fullPath);
#endif
}


#endif /* Don't compile the above routine for UNIX */

/****************************************************************************/

#ifndef N_PLAT_UNIX	/* Don't compile this routine for UNIX */


static int CheckFilePath
(
   char N_FAR *fullPath,
   char N_FAR *path,
   char N_FAR *fileName
)
{
   nwstrcpy(fullPath, path);
   nwstrcat(fullPath, "\\");
   nwstrcat(fullPath, fileName);

   return CheckReadAccess(fullPath);
}


/*
   Make sure that this function always returns a valid external code.
   It is the last call in the list and the code is returned directly
   to the caller of the API.
*/
static int CheckSearchPath
(
   char N_FAR *fullPath,
   char N_FAR *fileName
)
{
#if defined(N_PLAT_OS2)
   int ccode;

   if ((ccode = DosSearchPath(SEARCH_ENVIRONMENT, "DPATH", fileName,
         fullPath, PATH_SIZE)) != 0)
      if ((ccode = DosSearchPath(SEARCH_ENVIRONMENT, "PATH", fileName,
            fullPath, PATH_SIZE)) != 0)
         ccode = UNI_NO_SUCH_FILE;

   return ccode;

#else  /* (defined N_PLAT_MSW && defined N_ARCH_16) || NWDOS */
   int envLen;
   char far *envPtr;
   char N_FAR *p;
   char tmp[PATH_SIZE];
   char N_FAR *envCopy;

   if ((envPtr = uniGetenv("PATH")) == NULL)
      return UNI_NO_SUCH_FILE;
   
   envPtr += 5;              /* Go past 'PATH=' */

   envLen = NWstrlen(envPtr);
/*
   tmp = nwmalloc(envLen + 2);
   if (tmp == NULL)
      return UNI_NO_MEMORY;
*/

   envCopy = tmp;
   nwstrcpy(envCopy, envPtr);
   envCopy[envLen + 1] = '\0';       /* Put an extra null at the end */

   for (; *envCopy != 0; envCopy = p + 1)
   {
      p = nwstrchr(envCopy, ';');
      if (p != NULL)
         *p = '\0';
      else
         p = envCopy + NWstrlen(envCopy);

      nwstrcpy(fullPath, envCopy);
		if (fullPath[nwstrlen(fullPath) - 1] != '\\')
	      nwstrcat(fullPath, "\\");
      nwstrcat(fullPath, fileName);

      if (CheckReadAccess(fullPath) == 0)
      {
/*
         nwfree(tmp);
*/
         return 0;
      }
   }

/*
   nwfree(tmp);
*/
   return UNI_NO_SUCH_FILE;
#endif
}


#endif /* Don't compile the above routine for UNIX */

/****************************************************************************/

#ifndef N_PLAT_UNIX	/* Don't compile this routine for UNIX */


static char far *GetEnvPtr
(
   void
)
{
#if defined N_PLAT_MSW && defined N_ARCH_16
   return GetDOSEnvironment();

#elif defined(N_PLAT_OS2)
   char far *envPtr;
   SEL envSeg;
   USHORT argOff;

   DosGetEnv(&envSeg, &argOff);
   envPtr = (char far *) MAKEP(envSeg, 0);

   return envPtr;

#else  /* NWDOS */
# define DOS_GET_PSP_ADDRESS     0x62
# define DOS_ENVIRONMENT_OFFSET  0x2C
   char far *envPtr;
   unsigned int far *envSeg;
   union REGS regs;

   regs.h.ah = DOS_GET_PSP_ADDRESS;
   _NWUniIntdos(&regs, NULL);

#ifndef __WATCOMC__
   FP_SEG(envSeg) = regs.x.bx;
   FP_OFF(envSeg) = DOS_ENVIRONMENT_OFFSET;

   FP_SEG(envPtr) = *envSeg;
   FP_OFF(envPtr) = 0;

#else /* watcom fix up stuff */
   envSeg = MK_FP(regs.x.bx, DOS_ENVIRONMENT_OFFSET);
   envPtr = MK_FP(*envSeg, 0);

#endif

   return envPtr;

#endif
}


#endif /* Don't compile the above routine for UNIX */

/****************************************************************************/

#ifndef N_PLAT_UNIX	/* Don't compile this routine for UNIX */


static int CheckReadAccess
(
   char N_FAR *path
)
{
   int ccode;
   nuint16 handle;

   ccode = NWUniOpenReadOnly(&handle, path);
   if (ccode == 0)
      NWUniClose(handle);

   return ccode;
}


#if !defined(N_PLAT_OS2)
static char far *uniGetenv
(
   char N_FAR *envVariable
)
{
   int envLen;
   char far *envPtr;

   envPtr = GetEnvPtr();
   envLen = NWstrlen(envVariable);

	while (*envPtr != 0)
	{
  		if (nwstrnicmp(envVariable, envPtr, envLen) == 0
      && envPtr[envLen] == '=')
			break;
      while (*envPtr++ != 0)
         ;
	}

	if (*envPtr != 0)
		return envPtr;
	else
		return NULL;
}
#endif


#endif /* Don't compile the above routine for UNIX */

/****************************************************************************/

#endif /* End of everything but WIN32 and N_PLAT_NLM */

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/inituni.c,v 1.1 1994/09/26 17:20:37 rebekah Exp $
*/
