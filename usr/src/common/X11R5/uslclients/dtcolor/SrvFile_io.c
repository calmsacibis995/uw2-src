/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtcolor:SrvFile_io.c	1.3"

/*****************************************************************************
 *****************************************************************************
 **
 **  File:        SrvFile_io.c
 **
 **  Description:
 **  -----------
 **       This file initializes the user specified ( or default) palette
 **       for this session.
 **
 *****************************************************************************
 *************************************<+>*************************************/
/* #includes */

#include <fcntl.h>
#include <unistd.h>
#include "Srv.h"
#include "SrvFile_io.h"
#include "msgstr.h"

/* #defines */

#define BUFSIZE              1024


/* Static Function Declarations */

#ifdef _NO_PROTO

static Boolean FindPalette( ) ;
static struct _palette* ReadPaletteFile( ) ; 
static void ParsePaletteInfo() ;

#else

static Boolean FindPalette(char *, char *) ;
static struct _palette* ReadPaletteFile(Display *, int, char *, char *); 
static void ParsePaletteInfo(Display *, int, char *, int, struct _palette *);
#endif /* _NO_PROTO */




 
/************************************************************************
**
** GetPaletteDefinition -
**  Query the database for the Dtstyle.paletteDirectories resource
**  Use Dtstyle default (CoralReef) if not specified.
**  Search the directories for the palette in reverse order
**
************************************************************************/
struct _palette * 
#ifdef _NO_PROTO
GetPaletteDefinition( dpy, screen_number, palette )
Display *dpy;
int     screen_number;
char    *palette;
#else
GetPaletteDefinition( 
Display *dpy,
int     screen_number,
char    *palette)

#endif /* _NO_PROTO */
{
	struct _palette *paletteDef = NULL;
	char *path;


	/* 
	 * Check if name has ".vp" suffix.  If it does than 
	 * check in the home directory only.  Otherwise, check in 
	 * the system directory only. 
	 */
	if (!strstr(palette, istrs.suffix))
		path = getsyspdir();
	else  
		path = gethomepdir();
	if  (FindPalette(palette, path) == False) { 
		char *tmpStr;
		/* palette file is bad */
		tmpStr = (char *)XtMalloc(strlen(getstr(MSG3)) + 
			strlen(palette) +1);
		sprintf(tmpStr, getstr(MSG3), palette); 
		XtWarning(tmpStr); 
		XtFree(tmpStr);
	} else {
		/* Parse the data from the palette file */
		paletteDef = (struct _palette *) ReadPaletteFile(dpy, 
				screen_number, path, palette);
	}

	XtFree(path);
	return (paletteDef);
}


/************************************************************************
**
**  FindPalette -
**  Open the directory and look for the palette file.
**  If found, read in data and return true.
**  If not found, return false
**
************************************************************************/
static Boolean
#ifdef _NO_PROTO
FindPalette( palette, directory )
char *palette;
char *directory;

#else
FindPalette( 
char *palette,
char *directory)

#endif /* _NO_PROTO */
{
    DIR  *dirp;
    struct dirent *file_descpt;

    /* Open the directory */
    if( (dirp = opendir(directory)) == NULL)
       return(False);
    file_descpt = readdir(dirp);

    /* cycle through the files in the directory until found a match */
    while( file_descpt != NULL) {
        /* check for a palette filename match */
        if (strcmp(palette, file_descpt->d_name) == 0) {
            closedir(dirp);
            return(True);
        } 
        /* read the next file */
        file_descpt = readdir(dirp);
    } /* while( file_descpt != NULL) */

    closedir(dirp);
    return (False);
}


/***************************************************************************
 *
 * ReadPaletteFile - this routines reads and parses the palette file.
 *    It fills in the pCurrentPalette structure for the screen_number
 *    that was passed in. 
 *
 *************************************************************************/
static struct _palette * 
#ifdef _NO_PROTO
ReadPaletteFile( dpy, screen_number, palettePath, palette)
        Display *dpy ;
        int     screen_number ;
        char *palettePath ;
        char *palette;
#else
ReadPaletteFile(
        Display *dpy,
        int     screen_number,
        char *palettePath,
        char *palette)
#endif /* _NO_PROTO */
{
   struct _palette *new_palette;
   int             fd, nbytes;
   char            buf[BUFSIZE];
   char            *tmppalette;
   char            *p;
   char            *tmpStr;
   char            *fullPath;

   /* 
    * create full path, don't forget to add 1 for the NULL byte
    * and 1 for the slash in the format string for sprintf.
    */
   fullPath = (char *) XtMalloc(strlen(palettePath) + strlen(palette) + 2);
   sprintf(fullPath,"%s/%s", palettePath, palette); 

   if( (fd = open( fullPath, O_RDONLY)) == -1) { /* open failed */
       tmpStr = (char *)XtMalloc(strlen(getstr(MSG1)) + strlen(fullPath) + 1);
       sprintf(tmpStr, getstr(MSG1), fullPath); 
       fprintf(stderr, tmpStr); 
       XtFree(tmpStr); 
       XtFree(fullPath);
       return((struct _palette *) NULL);
   }
      
   /*
   **  Read a buffer of data ... BUFSIZE == 1024, as long as 
   **  screen < MAX_NUM_SCREENS (5) then we should never need more. 
   */
   nbytes = read(fd, buf, BUFSIZE);
   if(nbytes == 0 || nbytes == BUFSIZE) {  /* A bogus number of bytes */
       /*
	* Don't forget to add 1 for the NULL byte and 2 for the 
	* period and the newline in the format string for sprintf
	*/
       tmpStr = (char *)XtMalloc(strlen(getstr(MSG2)) + strlen(fullPath) + 3);
       sprintf(tmpStr, getstr(MSG2), palettePath); 
       XtWarning(tmpStr); 
       XtFree(tmpStr);
       XtFree(fullPath);
       close(fd);
       return((struct _palette *) NULL);
   }

   tmppalette = strdup(palette);
   if (!(p = strstr(tmppalette, istrs.suffix))) {
   	char 	*msgcat;
   	int 	defptsz;
   	int 	num;
	/* Get I18N name */
	num = atoi(tmppalette);
	for (defptsz = 0; defpname[defptsz]; defptsz++)
		;
	if (num < defptsz) { 
		msgcat = XtMalloc(strlen(CATALOGFILE) + 
		 		strlen(tmppalette) + 2);
		strcpy(msgcat, CATALOGFILE);
		strcat(msgcat, ":");
		strcat(msgcat, palette);
		XtFree(tmppalette);
		tmppalette = gettxt(msgcat, defpname[num]);
	}
   }

   /* Allocate space for this new palette. */
   new_palette = (palettes *)XtMalloc( sizeof(struct _palette) + 1 );

   new_palette->name = (char *)XtMalloc(strlen(tmppalette) + 1);
   strcpy(new_palette->name, tmppalette);
   
   /* set the next pointer to NULL*/
   new_palette->next = NULL;
   
   ParsePaletteInfo(dpy, screen_number, buf, nbytes, new_palette);

   /* close the file */
   close(fd);
   XtFree(fullPath);
   return(new_palette);
}

/***********************************************************************
 *
 * ParsePaletteInfo - This routine reads from the buffer(buf) the 
 *      actual data into the new_palette.  It reads in the bg colors
 *      then uses the XmCalculateColorRGB to generate the ts, bs, and sc 
 *      colors.  This routine doesn't allocate any pixel numbers but
 *      does generate the RGB values for each color in a palette.
 *
 ***********************************************************************/
static void 
#ifdef _NO_PROTO
ParsePaletteInfo( dpy, screen_num, buf, nbytes, new_palette )
        Display *dpy ;
        int screen_num ;
        char *buf ;
        int nbytes ;
        struct _palette *new_palette ;
#else
ParsePaletteInfo(
        Display *dpy,
        int screen_num,
        char *buf,
        int nbytes,
        struct _palette *new_palette )
#endif /* _NO_PROTO */
{
   static XmColorProc   calcRGB = NULL;

   new_palette->num_of_colors = 0;

   _XmGetDefaultThresholdsForScreen( XScreenOfDisplay(dpy, screen_num));

   if (calcRGB == NULL) 
	/* 
	 * Get the procedure that calcuates the 
	 * default colors
	 */
	calcRGB = XmGetColorCalculation();

   ReadInInfo(buf, nbytes, new_palette, dpy, DefaultColormap(dpy, screen_num),
	      colorSrv.TypeOfMonitor[screen_num], 
	      colorSrv.FgColor[screen_num], 
	      colorSrv.UsePixmaps[screen_num], calcRGB); 
}


/***********************************************************************
 *
 * SaveDefaultPalette - used to save the palette.dt file to 
 *         either $HOME/.dt/$DISPLAY/current or $HOME/.dt/$DISPLAY/home.
 *         The parameter mode determines whether it is home or
 *         current.  
 *
 ***********************************************************************/
void 
#ifdef _NO_PROTO
SaveDefaultPalette( dpy, dtPath, mode )
        Display *dpy ;
        char *dtPath ;
        int mode ;
#else
SaveDefaultPalette(
        Display *dpy,
        char *dtPath,
        int mode )
#endif /* _NO_PROTO */
{
}

/* Stub function */
void 
savecolor(palettes *npalette)
{
	return;
}
