/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dtcolor:colorfile.c	1.11"
/*
 *
 *
 *   File:        ColorFile.c
 *
 *   Project:     DT 3.0
 *
 *   Description:
 *
 *
 *  (c) Copyright Hewlett-Packard Company, 1990.  
 *
 *
 *
 */

/* include files                         */

#include <stdio.h>
#include <unistd.h>
#include <sys/fcntl.h>

#include <X11/Xlib.h>

#ifdef __hpux
#include <ndir.h>               /*   opendir(), directory(3C) */
#else
#if SVR4 || sco
#include <dirent.h>             /* opendir(), directory(3C) */
#else
#include <sys/dir.h>
#ifdef __apollo
#include <X11/apollosys.h>      /* needed for S_ISDIR macro */
#endif
#endif
#endif


#include <Xm/Xm.h>


#include "main.h"
#include "colormain.h"
#include "coloredit.h"
#include "msgstr.h"

/* include extern functions              */
#include "colorfile.h"

/* Local #defines                        */
#define BUFSIZE              1024

/* Local typedefs                        */



/* Internal Functions                    */

#ifdef _NO_PROTO

static palettes *RemovePaletteFromList();
static void SetColors() ;

#else

static palettes *RemovePaletteFromList( palettes *tmp_palette );

#endif /* _NO_PROTO */


/* Internal Variables                    */
int NumOfPalettes = 0;

static char *hpdir;

static DIR  *dirp;


savecolor_t saved_color; 

/*
 *
 * WriteOutPalette -
 * 	This routine writes out a file (palette) to the users .dt/palettes
 * 	directory. The palette name is passed into the routine. 
 *
 */
int 
#ifdef _NO_PROTO
WriteOutPalette( paletteName )
        char *paletteName ;
#else
WriteOutPalette(
        char *paletteName )
#endif /* _NO_PROTO */
{
	char *temp, *tmpStr;
	char *fname;
	int  i, fd;

	/* use the $HOME environment varible then constuct the full file name */
	fname = (char *)XtMalloc(strlen(style.home) + 
		       strlen(DT_PAL_DIR) +
		       strlen(paletteName) + 
		       strlen(istrs.suffix) + 2);

	/* create the full path name plus file name */
	strcpy(fname, style.home);
	strcat(fname, DT_PAL_DIR);
	strcat(fname, paletteName);
	strcat(fname, istrs.suffix);

	if (fname == NULL) {
		XtFree(fname);
		return(-1);
	}

	/* open the file for writing */
	if ((fd = open( fname, O_RDWR | O_CREAT)) == -1) {
		tmpStr = (char *)XtMalloc(strlen(getstr(ERR4)) + strlen(fname) + 1);
		sprintf (tmpStr, getstr(ERR4), fname);
		fprintf(stderr, tmpStr);
		XtFree(tmpStr);
		XtFree(fname);
		return(-1);
	}

	tmpStr =  temp = (char *)XtMalloc(80);
	for (i = 0; i < MAX_NUM_COLORS; i++) {
		int flags = pCurrentPalette->flags[i];
		/* 
		 * put the colors of the palette in the 
		 * form #RRRRGGGGBBBB 
		 */   
		if (TypeOfMonitor == LOW_COLOR && (i == 0 || i == 1))
			sprintf(temp,"#%04x%04x%04x", 
				saved_color.s_bgcolor[i].red, 
				saved_color.s_bgcolor[i].green, 
				saved_color.s_bgcolor[i].blue);
		else if (TypeOfMonitor == LOW_COLOR && i == 2)
			sprintf(temp,"#%04x%04x%04x", 
				pCurrentPalette->color[1].bg.red,
				pCurrentPalette->color[1].bg.green,
				pCurrentPalette->color[1].bg.blue);
		else if (TypeOfMonitor == LOW_COLOR && i == 3)
			sprintf(temp,"#%04x%04x%04x",
				pCurrentPalette->color[0].bg.red,
				pCurrentPalette->color[0].bg.green,
				pCurrentPalette->color[0].bg.blue);
		else 
			sprintf(temp,"#%04x%04x%04x",
				pCurrentPalette->color[i].bg.red,
				pCurrentPalette->color[i].bg.green,
				pCurrentPalette->color[i].bg.blue);
		
		temp += 13;
		if (flags & FOREGROUND) {
			if (TypeOfMonitor == LOW_COLOR && (i == 0 || i == 1))
				sprintf(temp,":#%04x%04x%04x", 
					saved_color.s_fgcolor[i].red, 
					saved_color.s_fgcolor[i].green, 
					saved_color.s_fgcolor[i].blue);
			else if (TypeOfMonitor == LOW_COLOR && i == 2)
				sprintf(temp,":#%04x%04x%04x", 
					pCurrentPalette->color[1].fg.red,
					pCurrentPalette->color[1].fg.green,
					pCurrentPalette->color[1].fg.blue);
			else if (TypeOfMonitor == LOW_COLOR && i == 3)
				sprintf(temp,":#%04x%04x%04x",
					pCurrentPalette->color[0].fg.red,
					pCurrentPalette->color[0].fg.green,
					pCurrentPalette->color[0].fg.blue);
			else 
				sprintf(temp,":#%04x%04x%04x",
					pCurrentPalette->color[i].fg.red,
					pCurrentPalette->color[i].fg.green,
					pCurrentPalette->color[i].fg.blue);
			temp += 14;
			flags &= ~FOREGROUND;
		} else if (flags)
			*temp++ = ':';
		if (flags & SELECT) {
			if (TypeOfMonitor == LOW_COLOR && (i == 0 || i == 1))
				sprintf(temp,":#%04x%04x%04x", 
					saved_color.s_sccolor[i].red, 
					saved_color.s_sccolor[i].green, 
					saved_color.s_sccolor[i].blue);
			else if (TypeOfMonitor == LOW_COLOR && i == 2)
				sprintf(temp,":#%04x%04x%04x", 
					pCurrentPalette->color[1].sc.red,
					pCurrentPalette->color[1].sc.green,
					pCurrentPalette->color[1].sc.blue);
			else if (TypeOfMonitor == LOW_COLOR && i == 3)
				sprintf(temp,":#%04x%04x%04x",
					pCurrentPalette->color[0].sc.red,
					pCurrentPalette->color[0].sc.green,
					pCurrentPalette->color[0].sc.blue);
			else 
				sprintf(temp,":#%04x%04x%04x",
					pCurrentPalette->color[i].sc.red,
					pCurrentPalette->color[i].sc.green,
					pCurrentPalette->color[i].sc.blue);
			temp += 14;
			flags &= ~SELECT;
		} else if (flags)
			*temp++ = ':';
		if (flags & TOPSHADOW) {
			if (TypeOfMonitor == LOW_COLOR && (i == 0 || i == 1))
				sprintf(temp,":#%04x%04x%04x", 
					saved_color.s_tscolor[i].red, 
					saved_color.s_tscolor[i].green, 
					saved_color.s_tscolor[i].blue);
			else if (TypeOfMonitor == LOW_COLOR && i == 2)
				sprintf(temp,":#%04x%04x%04x", 
					pCurrentPalette->color[1].ts.red,
					pCurrentPalette->color[1].ts.green,
					pCurrentPalette->color[1].ts.blue);
			else if (TypeOfMonitor == LOW_COLOR && i == 3)
				sprintf(temp,":#%04x%04x%04x",
					pCurrentPalette->color[0].ts.red,
					pCurrentPalette->color[0].ts.green,
					pCurrentPalette->color[0].ts.blue);
			else 
				sprintf(temp,":#%04x%04x%04x",
					pCurrentPalette->color[i].ts.red,
					pCurrentPalette->color[i].ts.green,
					pCurrentPalette->color[i].ts.blue);
			temp += 14;
		} else if (flags)
			*temp++ = ':';
		if (flags & BOTTOMSHADOW) {
			if (TypeOfMonitor == LOW_COLOR && (i == 0 || i == 1))
				sprintf(temp,":#%04x%04x%04x", 
					saved_color.s_bscolor[i].red, 
					saved_color.s_bscolor[i].green, 
					saved_color.s_bscolor[i].blue);
			else if (TypeOfMonitor == LOW_COLOR && i == 2)
				sprintf(temp,":#%04x%04x%04x", 
					pCurrentPalette->color[1].bs.red,
					pCurrentPalette->color[1].bs.green,
					pCurrentPalette->color[1].bs.blue);
			else if (TypeOfMonitor == LOW_COLOR && i == 3)
				sprintf(temp,":#%04x%04x%04x",
					pCurrentPalette->color[0].bs.red,
					pCurrentPalette->color[0].bs.green,
					pCurrentPalette->color[0].bs.blue);
			else 
				sprintf(temp,":#%04x%04x%04x",
					pCurrentPalette->color[i].bs.red,
					pCurrentPalette->color[i].bs.green,
					pCurrentPalette->color[i].bs.blue);
			temp += 14;
		}
		
		*temp++ = '\n';
		*temp++ = NULL;
		write(fd, tmpStr, strlen(tmpStr));
		temp = tmpStr;
	}
	XtFree(tmpStr);
	close(fd);
	/* make sure the file is read/writable */
	chmod(fname,438);
	XtFree(fname);
	return(0);
}


/*
 * This routine removes a palette.  
 * It actually creates a file in the users home palette directory with 
 * the current palette name preceeded by a '~'.
 * If the current palette is from the users home palette directory, that
 * file is removed.
 */
Boolean 
#ifdef _NO_PROTO
RemovePalette()
#else
RemovePalette( void )
#endif /* _NO_PROTO */
{
	int  result;
	char *fname1, *fname2;
	char *tmpStr;

	/* prepend the palette name with '~' */

	fname1 = (char *)XtMalloc(strlen(pCurrentPalette->name) +2); 
	strcpy(fname1, "~");
	strcat(fname1, pCurrentPalette->name);

	if (WriteOutPalette(fname1) != 0) {
		tmpStr = (char *)XtMalloc(strlen(getstr(ERR1)) + 
	       		  strlen(pCurrentPalette->name) + 1); 

		sprintf(tmpStr, getstr(ERR1), pCurrentPalette->name);
		ErrDialog (tmpStr, style.colorDialog);
		XtFree (tmpStr);
		XtFree (fname1);
		return(False);
	}

	if (strncmp(pCurrentPalette->directory, hpdir, strlen(hpdir)) == 0) {
		/* 
		 * get the HOME palette directory varible and constuct the 
		 * full file name 
		 */
		fname2 = (char *)XtMalloc(strlen(hpdir) + 
			 strlen(pCurrentPalette->name) + 
			 strlen( istrs.suffix) + 1);

		/* create the full path name plus file name */
		strcpy(fname2, hpdir);
		strcat(fname2,"/");
		strcat(fname2, pCurrentPalette->name);
		strcat(fname2, istrs.suffix);

		result = unlink(fname2);

		if (result != 0) {
			tmpStr = (char *)XtMalloc(strlen(getstr(ERR1)) + 
				 strlen(fname2) + 1); 
			sprintf (tmpStr, getstr(ERR1), fname2);
			fprintf(stderr, tmpStr);
			XtFree(tmpStr);

			tmpStr = (char *)XtMalloc(strlen(getstr(ERR1)) +
				strlen(pCurrentPalette->name) + 1); 
			sprintf(tmpStr, getstr(ERR1), pCurrentPalette->name);
			ErrDialog (tmpStr, style.colorDialog);
			XtFree (tmpStr);
			XtFree(fname1);
			XtFree(fname2);
			return(False);
		}

		XtFree(fname2);
	}

	XtFree(fname1);
	return(True);
}


/*
 * 
 * CheckForDeletedFile 
 *	- This routine looks through the linked list
 *     of palettes for the file name passed in.  If it finds it, that
 *     member of the linked list is deleted. 
 *
 */
void 
#ifdef _NO_PROTO
CheckForDeletedFile( dp )
#ifdef __hpux
        struct direct *dp ;
#else
        struct dirent *dp ;
#endif /* __hpux */
#else
CheckForDeletedFile(
#ifdef __hpux
        struct direct *dp )
#else
        struct dirent *dp )
#endif /* __hpux */
#endif /* _NO_PROTO */
{
	char 	*filename;
	int	i;
	int 	sz;
	palettes *tmp_palette;

	sz = strlen(dp->d_name);
	/* alloc enough space for the name of the ~ file  */
	filename = (char *)XtMalloc(sz + 1);

	/* copy the name into filename taking out the ~ */
	for (i = 0; i < (int)(sz - (strlen(istrs.suffix) + 1)); i++)
		filename[i] = dp->d_name[i+1];
	filename[i] = '\0';

	/* now go see if the filename is in the palette link list */
	tmp_palette = pHeadPalette;
	while (tmp_palette != NULL)
		if (strcmp(tmp_palette->name, filename)) 
			tmp_palette = tmp_palette->next;
		else
			tmp_palette = RemovePaletteFromList(tmp_palette);

	XtFree(filename);
}


/*
 *
 * SetColors - 
 *       It sets bs and ts if Pixmaps are to be used for topshadow 
 *       and bottomshadow.
 *
 */
static void 
#ifdef _NO_PROTO
SetColors( num_of_colors, new_palette )
        int num_of_colors ;
        palettes *new_palette ;
#else
SetColors(
        int num_of_colors,
        palettes *new_palette )
#endif /* _NO_PROTO */
{

	/*
	 *  Set the foreground pixel to either black or white 
	 * depending on brightness
	 */
	if (FgColor != DYNAMIC) {
		if (FgColor == BLACK) {
			new_palette->color[num_of_colors].fg.red = 0;
			new_palette->color[num_of_colors].fg.blue = 0;
			new_palette->color[num_of_colors].fg.green = 0;
		} else {	 /* WHITE */
			new_palette->color[num_of_colors].fg.red = 65535;
			new_palette->color[num_of_colors].fg.blue = 65535;
			new_palette->color[num_of_colors].fg.green = 65535;
		}
	}


	/*
	 * Now set ts and bs if the user is using pixmaps
	 */
	if (UsePixmaps != FALSE) {
		new_palette->color[num_of_colors].ts.red = 65535; 
		new_palette->color[num_of_colors].ts.blue = 65535;
		new_palette->color[num_of_colors].ts.green = 65535;

		new_palette->color[num_of_colors].bs.red = 0;
		new_palette->color[num_of_colors].bs.blue = 0;
		new_palette->color[num_of_colors].bs.green = 0;
	}

}


/*
 *
 * RemovePaletteFromList 
 *	- used to remove the palette pointed to by 
 *    	tmp_palette from the linked list of palettes.  The returned palette
 *    	is the palette immediately before the deleted palette 
 *
 */
static palettes * 
#ifdef _NO_PROTO
RemovePaletteFromList( tmp_palette )
        palettes *tmp_palette ;
#else
RemovePaletteFromList( palettes *tmp_palette )
#endif /* _NO_PROTO */
{ 
	palettes *tmp2_palette; 
	int i, count;

	/* it is in here ... get rid of this entry in the link list */
	if (tmp_palette->item_position == 1) {
		pHeadPalette = tmp_palette->next;
		tmp2_palette = pHeadPalette;
	} else {
		/* find palette just above the one being deleted */
		tmp2_palette = pHeadPalette;
		for (i = 1; i < tmp_palette->item_position-1; i++)
	   		tmp2_palette = tmp2_palette->next;

		/* 
		 * set the palette before's next to the one being 
		 * deleted next 
		 */
		tmp2_palette->next = tmp_palette->next;
	}

	/* deallocate the palette structure */
	XtFree(tmp_palette->name);
	XtFree(tmp_palette->directory);
	XtFree((char *)tmp_palette);

	/* make sure the item_positions are correct*/
	count = 1;
	tmp_palette = pHeadPalette;
	while (tmp_palette != NULL ) {
		tmp_palette->item_position = tmp_palette->p_num = count++;
		tmp_palette = tmp_palette->next;
	}

	/* decrease the number of palettes */
	NumOfPalettes--;

	return(tmp2_palette);
}



/*
 *  ReadPalette()
 *	 Reads the contents of the file in_filename, allocates space
 *  	for a new palette, and puts it in there.  The varible in_filename 
 *	passed in should be just the filename of the file.  The absolute 
 *	file name is constructed in this routine. i.e.  directory/file.
 *  	Finally this routine ups the count for number of palettes in the
 *  	customizer.
 *
 *  	The parameter length, which is passed in, is the length of the 
 *	in_fname * string. After the system palettes are read in, if there 
 *	are duplicate copies of palette names, the users home palettes 
 *	is kept.
 */
void 
#ifdef _NO_PROTO
ReadPalette(dir, infname, char *pname, len)
        char *dir;
        char *infname;
	char *pname)
#else
ReadPalette(
        char *dir,
        char *infname,
	char *pname)
#endif /* _NO_PROTO */
{
	char *fname;
	int  fd, nbytes;
	char buf[BUFSIZE];
	palettes *npalette, *tpalette;
	Boolean  found;
	char *tmpStr;


	/* Allocate space for this new palette.*/
	npalette = (palettes *)XtMalloc(sizeof(palettes)  + 1);

	/* create the filename to be read by adding directory in front of it */
	fname = (char *)XtMalloc( strlen(dir) + strlen(infname) + 2);
	strcpy(fname, dir); 

	if (strlen(strrchr(dir, '/')) != 1)
		strcat(fname, "/"); 
	strcat(fname, infname); 

	/* open the file for reading */
	if ((fd = open(fname, O_RDONLY)) == -1) {
		tmpStr = (char *)XtMalloc(strlen(getstr(ERR4)) + strlen(fname) + 1);
		sprintf(tmpStr, getstr(ERR4), fname);
		fprintf(stderr, tmpStr);
		XtFree(tmpStr);
		XtFree(fname);
		XtFree(pname);
		return;
	} 


	/* allocate enough space for the name */
	npalette->name = pname;

	/* 
	 * Now we have a name lets check to see if it is already in 
	 * the linked list.  If it is and it is from the system 
	 * paletters, replace it. If it is already in the list but is 
	 * not from the system palettes, don't overwrite it - discard 
	 * the one just read and go on to next palette.
	 */
	found = FALSE;
	tpalette = pHeadPalette;
	while (tpalette) {
		if (strcmp(tpalette->name, npalette->name)) 
			tpalette = tpalette->next;
		else {
			found = TRUE;
			XtFree(npalette->name);
			XtFree((char *)npalette);
			XtFree(tpalette->fname);
			XtFree(tpalette->directory);
			npalette = tpalette;
			break;
		}
	}

	/* allocate enough space for the directory */
	npalette->directory = (char *)XtMalloc(strlen(dir) + 1);

	/* load the directory name into the directory element */
	strcpy(npalette->directory, dir);
	npalette->fname = strdup(infname);

	/* set the num_of_colors  to 0 */
	npalette->num_of_colors = 0;

	nbytes = read(fd, buf, BUFSIZE);
	if (nbytes == BUFSIZE || nbytes == 0) {
		tmpStr = (char *)XtMalloc(strlen(ERR5) + 
				strlen(fname) + 1);
		sprintf(tmpStr, ERR5, fname);
		fprintf(stderr, tmpStr);
		XtFree(fname);
		XtFree(tmpStr);
		close(fd);
		return;
	} 

	if (edit.calcRGB == NULL) 
		/* 
		 * Get the procedure that calcuates the 
		 * default colors
		 */
		edit.calcRGB = XmGetColorCalculation();

	/* go parse the data from the palette file */
	ReadInInfo(buf, nbytes, npalette, style.display, style.colormap, 
	 	   TypeOfMonitor, FgColor, UsePixmaps, edit.calcRGB);


	/* close the file */
	close(fd); 


	if (!found) {
		/*
		 * Find the next available location within the linked list of 
		 * palettes to  store the palette just created.  If it is the 
		 * first one store it in pHeadPalettes, else store it in the 
		 * first place a NULL is encountered for the next member 
		 * of the palette structure.
		 */

		/* increment the total number of palettes in the customizer */
		NumOfPalettes++;

		/* set the item_position for the scrolled list */
		npalette->item_position = NumOfPalettes;
		npalette->p_num = NumOfPalettes;

		/* set the next pointer to NULL*/
		npalette->next = NULL;

		if (pHeadPalette == NULL )
			pHeadPalette = npalette;
		else {
			tpalette = pHeadPalette;
			while (tpalette->next != NULL)
				tpalette = tpalette->next;
			tpalette->next = npalette;
		}
	} else

	/* done with filename so XtFree it */
	XtFree(fname);
}


Boolean 
#ifdef _NO_PROTO
ReadPaletteDir(dirnmp, ishdir)
        char *dirnmp; 
	ushort ishdir;
#else
ReadPaletteDir( 
	char  *dirnmp,
	ushort ishdir)
#endif /* _NO_PROTO */
{
#ifdef __hpux
        struct direct *dp ;
#else
	struct dirent *dp;
#endif
	char *pname;		/* palette name */
	char *p;


	/*
	 *  Open the directory.
	 *  dirp is a pointer to the directory stream
	 */
	if (!(dirp = opendir(dirnmp))) {
		char *tmpstr;
		/* 
		 * note: if there is an error in opening a 
		 * directory, the directory is not deleted from 
		 * the pdirs structure.
		 */
		tmpstr = (char *)XtMalloc(strlen(getstr(ERR3)) + 
						strlen(dirnmp + 1));
		sprintf(tmpstr, getstr(ERR3), dirnmp);
		fprintf(stderr, tmpstr);
		XtFree(tmpstr);
		return(False);
	}
	/*
	 * get a pointer to the next file entry .. 
	 * dp the structure for direct looks
	 * like: 
	 *  struct direct {
	 *    long    d_fileno;  #file number of entry
	 *    short   d_reclen;  #length of this record
	 *    short   d_namlen;  #length of string 
	 *    char    d_name[256];  #name 
	 *  };
	 */
	dp = readdir(dirp);
	while (dp != NULL) {
		/* skip over . and .. files */
		if ((dp->d_name[0] == '.')  && ((dp->d_name[1] == '\0') ||
		    ((dp->d_name[1] == '.') && (dp->d_name[2] == '\0')))) {
			dp = readdir(dirp);
			continue;
		}
		/* Get the palette name from the catelog */
		if (!ishdir) {
			int num = atoi(dp->d_name);
			strcpy(style.msgcat, CATALOGFILE);
			strcat(style.msgcat, ":");
			strncat(style.msgcat, dp->d_name, 4);
			if (num >= style.defpsize)
				pname = NULL;
			else
				pname = strdup(gettxt(style.msgcat, defpname[num]));
		} else {
			
			/* check for a valid palette file */
			if (!(strncmp(istrs.suffix, strrchr(dp->d_name, '.'), 
			     strlen(istrs.suffix)))) {
				/*
				 * A file in the users home palette directory 
				 * which beging with a ~ is a palette that the 
				 *  user does not want.  Remove it from the 
				 *  palette link list.
				 */
				if (strncmp(dp->d_name, "~", 1) == 0) { 
					if (pHeadPalette != NULL) 
						CheckForDeletedFile(dp);
					dp = readdir(dirp);
					continue;
				}
			} else  {
				dp = readdir(dirp);
				continue;
			}
			pname = NULL;
		} 
		/* 
		 * make sure if Black and white monitor the 
		 * file is one that can be read for a 
		 * B_W monitor 
		 */
		if (pname == NULL)
			pname = strdup(dp->d_name);
		if (p = strstr(pname, istrs.suffix))
				*p = '\0';
		if (verifyname(style.depth, TypeOfMonitor, &pname, istrs))
			ReadPalette(dirnmp, dp->d_name, pname);

		/* read the next file */
		dp = readdir(dirp);

	} 	/* while (dp != NULL) */

	closedir(dirp);
	return(True);
}


/*
 * ReadInPalettes reads in the different palettes for the customizer.
 *
 * The directories read from include:
 *     1. the systme palettes (PALETTE_DIR)
 *     2. the users $HOME/.palettes
 */
void 
#ifdef _NO_PROTO
ReadInPalettes()
#else
ReadInPalettes(void)
#endif /* _NO_PROTO */
{

	char *sys_palette_dir;

	sys_palette_dir = getsyspdir(); 

	ReadPaletteDir(sys_palette_dir, False);

	hpdir = gethomepdir();

	/* check  $HOME/.palettes */
	if(access(hpdir, R_OK|W_OK) == -1) {
		/*  create .palettes if it does not exit */
		if (mkdir(hpdir, 511)) {
			char *tmpstr;
			tmpstr = (char *)XtMalloc(strlen(getstr(ERR4)) + 
			strlen(hpdir) +1);
			sprintf(tmpstr, getstr(ERR4), hpdir);
			fprintf(stderr, tmpstr);
			XtFree(tmpstr);
		} 
	} else
		ReadPaletteDir(hpdir, True);
}


/*
 *
 * savecolor 
 *	used in a LOW_COLOR system to switch the Acitive and
 *    Inactive colorsets to use the Primary and Secondary colorsets.  It
 *    was determined that this would look much better to the user.
 *
 */
void 
#ifdef _NO_PROTO
savecolor( new_palette )
        palettes *new_palette ;
#else
savecolor( palettes *new_palette )
#endif /* _NO_PROTO */
{
	/* Save backgroung color */
	saved_color.s_bgcolor[0].red = new_palette->color[0].bg.red;
	saved_color.s_bgcolor[0].green = new_palette->color[0].bg.green;
	saved_color.s_bgcolor[0].blue = new_palette->color[0].bg.blue;
	saved_color.s_bgcolor[1].red = new_palette->color[1].bg.red;
	saved_color.s_bgcolor[1].green = new_palette->color[1].bg.green;
	saved_color.s_bgcolor[1].blue = new_palette->color[1].bg.blue;

	if (new_palette->flags[0] & FOREGROUND) {
		saved_color.s_fgcolor[0].red = new_palette->color[0].fg.red;
		saved_color.s_fgcolor[0].green =new_palette->color[0].fg.green;
		saved_color.s_fgcolor[0].blue = new_palette->color[0].fg.blue;
		saved_color.s_flags[0] |= FOREGROUND;
	}
	if (new_palette->flags[1] & FOREGROUND) {
		saved_color.s_fgcolor[1].red = new_palette->color[1].fg.red;
		saved_color.s_fgcolor[1].green =new_palette->color[1].fg.green;
		saved_color.s_fgcolor[1].blue = new_palette->color[1].fg.blue;
		saved_color.s_flags[1] |= FOREGROUND;
	}
	if (new_palette->flags[0] & SELECT) {
		saved_color.s_sccolor[0].red = new_palette->color[0].sc.red;
		saved_color.s_sccolor[0].green =new_palette->color[0].sc.green;
		saved_color.s_sccolor[0].blue = new_palette->color[0].sc.blue;
		saved_color.s_flags[0] |= SELECT;
	}
	if (new_palette->flags[1] & SELECT) {
		saved_color.s_sccolor[1].red = new_palette->color[1].sc.red;
		saved_color.s_sccolor[1].green =new_palette->color[1].sc.green;
		saved_color.s_sccolor[1].blue = new_palette->color[1].sc.blue;
		saved_color.s_flags[1] |= SELECT;
	}
	if (new_palette->flags[0] & TOPSHADOW) {
		saved_color.s_tscolor[0].red = new_palette->color[0].ts.red;
		saved_color.s_tscolor[0].green =new_palette->color[0].ts.green;
		saved_color.s_tscolor[0].blue = new_palette->color[0].ts.blue;
		saved_color.s_flags[0] |= TOPSHADOW;
	}
	if (new_palette->flags[1] & TOPSHADOW) {
		saved_color.s_tscolor[1].red = new_palette->color[1].ts.red;
		saved_color.s_tscolor[1].green =new_palette->color[1].ts.green;
		saved_color.s_tscolor[1].blue = new_palette->color[1].ts.blue;
		saved_color.s_flags[1] |= TOPSHADOW;
	}
	if (new_palette->flags[0] & BOTTOMSHADOW) {
		saved_color.s_bscolor[0].red = new_palette->color[0].bs.red;
		saved_color.s_bscolor[0].green =new_palette->color[0].bs.green;
		saved_color.s_bscolor[0].blue = new_palette->color[0].bs.blue;
		saved_color.s_flags[0] |= BOTTOMSHADOW;
	}
	if (new_palette->flags[1] & TOPSHADOW) {
		saved_color.s_tscolor[1].red = new_palette->color[1].ts.red;
		saved_color.s_tscolor[1].green =new_palette->color[1].ts.green;
		saved_color.s_tscolor[1].blue = new_palette->color[1].ts.blue;
		saved_color.s_flags[1] |= TOPSHADOW;
	}
}

