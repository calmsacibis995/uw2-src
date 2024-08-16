/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/localias.h	1.3"
/****************************************************************************
 *
 * (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.
 *
 * No part of this file may be duplicated, revised, translated, localized
 * or modified in any manner or compiled, linked or uploaded or downloaded
 * to or from any computer system without the prior written permission of
 * Novell, Inc.
 *
 ****************************************************************************/
#ifndef   _UNIALIAS_HEADER_
   #define   _UNIALIAS_HEADER_

   #ifdef NWWIN
      #ifdef __BORLANDC__
         void N_FAR * N_FAR _cdecl dllmalloc(unsigned areaSize);
         void N_FAR _cdecl dllfree(void N_FAR *areaPtr);
      #else
         void N_FAR * N_FAR _cdecl dllmalloc(size_t areaSize);
         void N_FAR  _cdecl dllfree(void N_FAR *areaPtr);
      #endif
   #endif

   #ifdef NWOS2
      void N_FAR * N_FAR _cdecl dllmalloc(unsigned short areaSize);
      void N_FAR _cdecl dllfree(void N_FAR *areaPtr);
   #endif

   /*   Storage class definitions   */
   #define   fast      register      /* faster access qualifier (use with care)   */

   /* C Library alias names   */
   #define   nwabort          abort         
   #define   nwabs            abs         
   #define   nwaccess         access      
   #define   nwasctime        asctime      
   #define   nwassert         assert
   #define   nwatexit         atexit      
   #define   nwatof           atof         
   #define   nwatoi           atoi         
   #define   nwatol           atol         
   #define   nwbsearch        bsearch      
   #define   nwcalloc         calloc
   #define   nwcgets          cgets
   #define   nwchdir          chdir         
   #define   nwchmod          chmod         
   #define   nwclearerr       clearerr      
   #define   nwclock          clock         
   #define   nwclose          close         
   #define   nwclosedir       closedir      
   #define   nwcprintf        cprintf
   #define   nwcreat          creat         
   #define   nwctime          ctime         
   #define   nwdelay          delay
   #define   nwdifftime       difftime      
   #define   nwdiv            div         
   #define   nwdup            dup         
   #define   nwdup2           dup2         
   #define   nweof            eof
   #define   nwexit           exit
   #define   nwfclose         fclose
   #define   nwfcloseall      fcloseall
   #define   nwfdopen         fdopen
   #define   nwfeof           feof
   #define   nwferror         ferror
   #define   nwfflush         fflush
   #define   nwfgetc          fgetc
   #define   nwfgetpos        fgetpos
   #define   nwfgets          fgets
   #define   nwfilelength     filelength
   #define   nwfileno         fileno
   #define   nwflushall       flushall
   #define   nwfopen          fopen
   #define   nwfprintf        fprintf
   #define   nwfputc          fputc
   #define   nwfputs          fputs
   #define   nwfread          fread
   #define   nwfreopen        freopen
   #define   nwfscanf         fscanf
   #define   nwfseek          fseek
   #define   nwfsetpos        fsetpos
   #define   nwfstat          fstat
   #define   nwftell          ftell
   #define   nwfwrite         fwrite
   #define   nwgetc           getc
   #define   nwgetch          getch
   #define   nwgetchar        getchar
   #define   nwgetche         getche
   #define   nwgetcmd         getcmd
   #define   nwgetcwd         getcwd
   #define   nwgetenv         getenv
   #define   nwgets           gets
   #define   nwgmtime         gmtime
   #define   nwisalnum        isalnum
   #define   nwisalpha        isalpha
   #define   nwisascii        isascii
   #define   nwisatty         isatty
   #define   nwiscntrl        iscntrl
   #define   nwisdigit        isdigit
   #define   nwisgraph        isgraph
   #define   nwislower        islower
   #define   nwisprint        isprint
   #define   nwispunct        ispunct
   #define   nwisspace        isspace
   #define   nwisupper        isupper
   #define   nwisxdigit       isxdigit
   #define   nwitoa           itoa
   #define   nwkbhit          kbhit
   #define   nwlabs           labs
   #define   nwldiv           ldiv
   #define   nwlocaltime      localtime
   #define   nwlongjmp        longjmp
   #define   nwlseek          lseek
   #define   nwltoa           ltoa

   /* WATCOM does not do max and min with macros but   */
   /* with functions that take integer parameters!      */
   /* Define macros that conform to the expected.      */

   #define   nwmax(a,b)        (((a) > (b)) ? (a) : (b))
   #define   nwmin(a,b)        (((a) < (b)) ? (a) : (b))

   #define   nwmkdir          mkdir
   #define   nwmktime         mktime
   #define   nwopen           open
   #define   nwopendir        opendir
   #define   nwperror         perror
   #define   nwprintf         printf
   #define   nwputc           putc
   #define   nwputch          putch
   #define   nwputchar        putchar
   #define   nwputs           puts
   #define   nwqsort          qsort
   #define   nwraise          raise
   #define   nwrand           rand
   #define   nwread           read
   #define   nwreaddir        readdir
   #define   nwrealloc        realloc
   #define   nwremove         remove
   #define   nwrename         rename
   #define   nwrewind         rewind
   #define   nwrmdir          rmdir
   #define   nwscanf          scanf
   #define   nwsetbuf         setbuf
   #define   nwsetjmp         setjmp
   #define   nwsetlocale      setlocale
   #define   nwsetmode        setmode
   #define   nwsetvbuf        setvbuf
   #define   nwsignal         signal
   #define   nwsopen          sopen
   #define   nwspawnlp        spawnlp
   #define   nwspawnvp        spawnvp
   #define   nwsprintf        sprintf
   #define   nwsrand          srand
   #define   nwsscanf         sscanf
   #define   nwstat           stat
   #define   nwstrcoll        strcoll
   #define   nwstrerror       strerror
   #define   nwstrftime       strftime
   #define   nwstrtol         strtol
   #define   nwstrtoul        strtoul
   #define   nwstrxfrm        strxfrm
   #define   nwsystem         system
   #define   nwtell           tell
   #define   nwtime           time
   #define   nwtmpfile        tmpfile
   #define   nwtmpnam         tmpnam
   #define   nwtolower        tolower
   #define   nwtoupper        toupper
   #define   nwtzset          tzset
   #define   nwultoa          ultoa
   #define   nwungetc         ungetc
   #define   nwungetch        ungetch
   #define   nwunlink         unlink
   #define   nwutime          utime
   #define   nwutoa           utoa
   #define   nwvfprintf       vfprintf
   #define   nwvfscanf        vfscanf
   #define   nwvprintf        vprintf
   #define   nwvscanf         vscanf
   #define   nwvsprintf       vsprintf
   #define   nwvsscanf        vsscanf
   #define   nwwrite          write

   /* Aliases for certain functions in the NWWIN or "model independent"   */
   /* FAR PASCAL environments                                                */

   #if      defined(__BORLANDC__) || defined(MSC)

      #ifdef    __BORLANDC__
         #define   nwfarfree       farfree
         #define   nwfarmalloc(x)  farmalloc((unsigned long)(x))
      #else
         #define   nwfarfree       _ffree
         #define   nwfarmalloc     _fmalloc
      #endif

      #define   nwfarmemcmp        _fmemcmp
      #define   nwfarmemcpy        _fmemcpy
      #define   nwfarmemset        _fmemset

   #endif   /* Borland or Microsoft   compilers   */

   #ifdef   NWNLM

      #define   nwfarfree       free
      #define   nwfarmalloc     malloc
      #define   nwfarmemcmp     memcmp
      #define   nwfarmemcpy     memcpy
      #define   nwfarmemset     memset

   #endif   /* WCC386 compiler   */

   #if      !(defined(NWWIN) || defined(NWFARPASCAL) || defined(NWOS2))

      #ifndef MACINTOSH
         # define nwfree        free
         # define nwmalloc      malloc
         # define nwmemcpy      memcpy
         # define nwmemmove     memmove
         # define nwmemset      memset
         # define nwmemchr      memchr
         # define nwmemcmp      memcmp
         # define nwmemicmp     memicmp
         # define nwstrcmp      strcmp
         # define nwstrncmp     strncmp
         # define nwstrcpy      strcpy
         # define nwstricmp     stricmp
         # define nwstrcat      strcat
         # define nwstrchr      strchr
         # define nwstrcspn     strcspn
         # define nwstrlen      strlen
         # define nwstrncat     strncat
         # define nwstrstr      strstr
         # define nwstrncpy     strncpy
         # define nwstrupr      strupr
         # define nwstrdup      strdup
         # define nwstrlwr      strlwr
         # define nwstrnicmp    strnicmp
         # define nwstrnset     strnset
         # define nwstrpbrk     strpbrk
         # define nwstrrchr     strrchr
         # define nwstrrev      strrev
         # define nwstrset      strset
         # define nwstrspn      strspn
         # define nwstrtok      strtok
      #else      /* this is for the Macintosh...*/
         # include "MacStrin.h"
         # undef  nwitoa
         # define nwitoa          INWitoa
         # define nwfree          DisposPtr
         # define nwmalloc        NewPtr
         # define nwmemcpy(t,s,n) (BlockMove(s, t, n), t)
         # define nwmemmove       INWmemmove
         # define nwmemset        INWmemset
         # define nwmemchr        INWmemchr
         # define nwmemcmp        INWmemcmp
         # define nwmemicmp       INWmemicmp
         # define nwstrcmp        INWstrcmp
         # define nwstrncmp       INWstrncmp
         # define nwstrcpy        INWstrcpy
         # define nwstricmp       INWstricmp
         # define nwstrcat        INWstrcat
         # define nwstrchr        INWstrchr
         # define nwstrcspn       INWstrcspn
         # define nwstrlen        INWstrlen
         # define nwstrncat       INWstrncat
         # define nwstrstr        INWstrstr
         # define nwstrncpy       INWstrncpy
         # define nwstrupr        INWstrupr
         # define nwstrdup        INWstrdup
         # define nwstrlwr        INWstrlwr
         # define nwstrnicmp      INWstrnicmp
         # define nwstrnset       INWstrnset
         # define nwstrpbrk       INWstrpbrk
         # define nwstrrchr       INWstrrchr
         # define nwstrrev        INWstrrev
         # define nwstrset        INWstrset
         # define nwstrspn        INWstrspn
         # define nwstrtok        INWstrtok
      #endif
   #else      /* NWWIN, FAR PASCAL, or NWOS2   */

      #if defined(NWWIN) || defined(NWOS2)
         #define   nwfree         dllfree
         #define   nwmalloc       dllmalloc
      #else  /* NWDOS */

         #if defined(__BORLANDC__)
            #define   nwfree         farfree
            #define   nwmalloc(x)    farmalloc((unsigned long)(x))

         #elif defined(MSC) && defined(NWFARPASCAL)

            #define   nwfree         _ffree
            #define   nwmalloc       _fmalloc

         #endif
      #endif

      #define   nwmemcpy         _fmemcpy
      #define   nwmemmove        _fmemmove
      #define   nwmemset         _fmemset
      #define   nwmemchr         _fmemchr
      #define   nwmemcmp         _fmemcmp
      #define   nwmemicmp        _fmemicmp
      #define   nwstrcmp         _fstrcmp
      #define   nwstrncmp        _fstrncmp
      #define   nwstrcpy         _fstrcpy
      #define   nwstricmp        _fstricmp
      #define   nwstrcat         _fstrcat
      #define   nwstrchr         _fstrchr
      #define   nwstrcspn        _fstrcspn
      #define   nwstrlen         _fstrlen
      #define   nwstrncat        _fstrncat
      #define   nwstrstr         _fstrstr
      #define   nwstrncpy        _fstrncpy
      #define   nwstrupr         _fstrupr
      #define   nwstrdup         _fstrdup
      #define   nwstrlwr         _fstrlwr
      #define   nwstrnicmp       _fstrnicmp
      #define   nwstrnset        _fstrnset
      #define   nwstrpbrk        _fstrpbrk
      #define   nwstrrchr        _fstrrchr
      #define   nwstrrev         _fstrrev
      #define   nwstrset         _fstrset
      #define   nwstrspn         _fstrspn
      #define   nwstrtok         _fstrtok

      #define   memcpy           _fmemcpy
      #define   memmove          _fmemmove
      #define   memset           _fmemset
      #define   memchr           _fmemchr
      #define   memcmp           _fmemcmp
      #define   memicmp          _fmemicmp
      #define   strcmp           _fstrcmp
      #define   strncmp          _fstrncmp
      #define   strcpy           _fstrcpy
      #define   stricmp          _fstricmp
      #define   strcat           _fstrcat
      #define   strchr           _fstrchr
      #define   strcspn          _fstrcspn
      #define   strlen           _fstrlen
      #define   strncat          _fstrncat
      #define   strstr           _fstrstr
      #define   strncpy          _fstrncpy
      #define   strupr           _fstrupr
      #define   strdup           _fstrdup
      #define   strlwr           _fstrlwr
      #define   strnicmp         _fstrnicmp
      #define   strnset          _fstrnset
      #define   strpbrk          _fstrpbrk
      #define   strrchr          _fstrrchr
      #define   strrev           _fstrrev
      #define   strset           _fstrset
      #define   strspn           _fstrspn
      #define   strtok           _fstrtok

   #endif   /* else NWWIN, FAR PASCAL, or NWOS2   */

   /* Other miscellaneous function macros or aliases   */

   #define   nwsleep(x)         delay((x) * 1000)

#endif   /* Header not already included   */
/* ######################################################################## */
