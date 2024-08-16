/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dtcolor:common.c	1.1.1.6"
#include <stdio.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <Xm/Xm.h>
#include "common.h"

char *defpname[] = {
	"Junk",
	"Alpine.vp",
	"Arizona.vp",
	"Black.vp",
	"BlackWhite.vp",
	"Brocia.vp",
	"Cabernet.vp",
	"Camouflage.vp",
	"Charcoal.vp",
	"Chocolate.vp",
	"Cinnamon.vp",
	"Clay.vp",
	"Default.vp",
	"Golden.vp",
	"GrayScale.vp",
	"Lilac.vp",
	"Mustard.vp",
	"Neptune.vp",
	"NorthernSky.vp",
	"Nutmeg.vp",
	"Olive.vp",
	"Orchid.vp",
	"Sand.vp",
	"SanteFe.vp",
	"Savannah.vp",
	"SeaFoam.vp",
	"SoftBlue.vp",
	"SouthWest.vp",
	"Tundra.vp",
	"Urchin.vp",
	"Wheat.vp",
	"White.vp",
	"WhiteBlack.vp",
	"16!BlueBell.vp",
	"16!Cream.vp",
	"16!Default.vp",
	"16!Forest.vp",
	"16!GrayScale.vp",
	"16!Indigo.vp",
	"16!Mauve.vp",
	"16!Olive.vp",
	"16!Pastel.vp",
	"16!Roses.vp",
	NULL
};



/*
 * getsyspdir(void)
 *	Return a pointer to system palette directory path.
 */ 
char *
getsyspdir(void)
{
	char *pdir;
	char *path;
	path = (char *)getenv("XWINHOME");
	if (path == NULL)
		pdir = strdup(DEF_SYS_PDIR);
	else {
		pdir = XtMalloc(strlen(path) + strlen("/lib/palettes"));
		sprintf(pdir, "%s%s", path, "/lib/palettes");
	}
	return pdir;
}


/*
 * gethomepdir(void)
 *	Return a pointer to home palette directory path.
 */ 
char *
gethomepdir(void)
{
	char *hpdir;
	hpdir = (char *)XtMalloc(strlen((char *)getenv(HOME)) + 
		strlen(USER_PALETTE_DIR) + 1);

	/* create the full path name plus file name */
	strcpy(hpdir, (char *)getenv(HOME));
	strcat(hpdir, USER_PALETTE_DIR);
	return hpdir;
}


char * 
#ifdef _NO_PROTO
getdefault(dpy, s_num, depth) 
	Display *dpy,
        int s_num ;
	int depth;
#else
getdefault(Display *dpy, int s_num,  int depth)
#endif /* _NO_PROTO */
{
	Visual *visual;
	visual = XDefaultVisual(dpy, s_num);
	if ((visual->class == GrayScale) || (visual->class == StaticGray))   
		return DEFAULT_GRAYSCALE_PALETTE;
	if (depth == 4)  
		return DEFAULT_16PALETTE;
	return DEFAULT_PALETTE;
}


/*
 * SwitchAItoPS(paletttes *) 
 *    used in a LOW_COLOR system to switch the Acitive and
 *    Inactive colorsets to use the Primary and Secondary colorsets.  It
 *    was determined that this would look much better to the user.
 *
 */
void 
SwitchAItoPS(palettes *new_palette)
{

	new_palette->color[0].bg.red = new_palette->color[3].bg.red;
	new_palette->color[0].bg.green = new_palette->color[3].bg.green;
	new_palette->color[0].bg.blue = new_palette->color[3].bg.blue;

	new_palette->color[0].fg.red = new_palette->color[3].fg.red;
	new_palette->color[0].fg.green = new_palette->color[3].fg.green;
	new_palette->color[0].fg.blue = new_palette->color[3].fg.blue;

	new_palette->color[0].ts.red = new_palette->color[3].ts.red;
	new_palette->color[0].ts.green = new_palette->color[3].ts.green;
	new_palette->color[0].ts.blue = new_palette->color[3].ts.blue;

	new_palette->color[0].bs.red = new_palette->color[3].bs.red;
	new_palette->color[0].bs.green = new_palette->color[3].bs.green;
	new_palette->color[0].bs.blue = new_palette->color[3].bs.blue;

	new_palette->color[0].sc.red = new_palette->color[3].sc.red;
	new_palette->color[0].sc.green = new_palette->color[3].sc.green;
	new_palette->color[0].sc.blue = new_palette->color[3].sc.blue;

	new_palette->color[1].bg.red = new_palette->color[2].bg.red;
	new_palette->color[1].bg.green = new_palette->color[2].bg.green;
	new_palette->color[1].bg.blue = new_palette->color[2].bg.blue;

	new_palette->color[1].fg.red = new_palette->color[2].fg.red;
	new_palette->color[1].fg.blue = new_palette->color[2].fg.blue;
	new_palette->color[1].fg.green = new_palette->color[2].fg.green;

	new_palette->color[1].ts.red = new_palette->color[2].ts.red;
	new_palette->color[1].ts.green = new_palette->color[2].ts.green;
	new_palette->color[1].ts.blue = new_palette->color[2].ts.blue;

	new_palette->color[1].bs.red = new_palette->color[2].bs.red;
	new_palette->color[1].bs.green = new_palette->color[2].bs.green;
	new_palette->color[1].bs.blue = new_palette->color[2].bs.blue;

	new_palette->color[1].sc.red = new_palette->color[2].sc.red;
	new_palette->color[1].sc.green = new_palette->color[2].sc.green;
	new_palette->color[1].sc.blue = new_palette->color[2].sc.blue;
}


/*
 *
 * SetColors
 *       It sets bs and ts if Pixmaps are to be used for topshadow 
 *       and bottomshadow.
 *
 */
static void 
SetColors(int num_of_colors, palettes *new_palette, int FgColor, 
	  Bool UsePixmaps)
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
		/* the values generated by XmCalculateColorRGB are invalid */
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
 * InitializeBW 
 *	initializes the RGB values for the WhiteOnBlack and
 *      BlackOnWhite palettes.  The color passed in is used to determine
 *      if its White on Black or Black on White.
 *
 */
static void 
#ifdef _NO_PROTO
InitializeBW( color, num_of_colors, new_palette )
        unsigned long color ;
        int num_of_colors ;
        palettes *new_palette ;
#else
InitializeBW(
        unsigned long color,
        int num_of_colors,
        palettes *new_palette )
#endif /* _NO_PROTO */
{
	/* 
	 * if color passed in is black(background) its white on black so
	 * set everything to white 
	 */
	if (color == 0L) {
		new_palette->color[num_of_colors].fg.red = 65535; 
		new_palette->color[num_of_colors].fg.blue = 65535;
		new_palette->color[num_of_colors].fg.green = 65535;

		new_palette->color[num_of_colors].sc.red = 0;
		new_palette->color[num_of_colors].sc.blue = 0;
		new_palette->color[num_of_colors].sc.green = 0;

		new_palette->color[num_of_colors].ts.red = 65535; 
		new_palette->color[num_of_colors].ts.blue = 65535;
		new_palette->color[num_of_colors].ts.green = 65535;

		new_palette->color[num_of_colors].bs.red = 65535;
		new_palette->color[num_of_colors].bs.blue = 65535;
		new_palette->color[num_of_colors].bs.green = 65535;
	} else {
		new_palette->color[num_of_colors].fg.red = 0; 
		new_palette->color[num_of_colors].fg.blue = 0;
		new_palette->color[num_of_colors].fg.green = 0;

		new_palette->color[num_of_colors].sc.red = 65535;
		new_palette->color[num_of_colors].sc.blue = 65535;
		new_palette->color[num_of_colors].sc.green = 65535;

		new_palette->color[num_of_colors].ts.red = 0; 
		new_palette->color[num_of_colors].ts.blue = 0;
		new_palette->color[num_of_colors].ts.green = 0;

		new_palette->color[num_of_colors].bs.red = 0;
		new_palette->color[num_of_colors].bs.blue = 0;
		new_palette->color[num_of_colors].bs.green = 0;
	}
}


#define LF '\012'
/*
 *
 * ReadInInfo()
 *	 The routine used to actual parse the data from a palette
 *       file.  It parses 8 different background colors.  
 *
 */
void 
ReadInInfo(char *buf, int nbytes, palettes *new_palette, Display *display,
	   Colormap colormap, int TypeOfMonitor, Boolean FgColor, 
	   int UsePixmaps,  XmColorProc calcRGB)
{
	int bcnt, cnt, result;
	char tmpbuf[50];
	XColor tmp_color;
	int ncolors = 0;
	int first = 0;
	int reset = 0;

	bcnt = 0;
	new_palette->flags[ncolors] = 0;
	/* Read in background colors until end of file */
	while (bcnt < nbytes && buf[bcnt] != '!') {
		cnt = 0;
		/* read in a background color */
		while (buf[bcnt] != LF && buf[bcnt] != ':') 
			tmpbuf[cnt++] = buf[bcnt++];
		tmpbuf[cnt++] = '\0';
		/* Read forgound or select color */
		if (buf[bcnt] == LF)
			reset = 1;
		bcnt++;

		/* skip comment line */
		if (tmpbuf[0] == '!')
			continue;

		/* Parse the color */
		result = XParseColor(display, colormap, tmpbuf, &tmp_color);

		/* 
		 * if the result == 0 then the parse came back bad, uses 
		 * the motif default background (#729FFF) 
		 */
		switch (first) {
		case 0:		/* background */
			if (result == 0) {
				new_palette->color[ncolors].bg.red = 29184;
				new_palette->color[ncolors].bg.green = 40704;
				new_palette->color[ncolors].bg.blue = 65280;
			} else {
				new_palette->color[ncolors].bg.red = tmp_color.red;
				new_palette->color[ncolors].bg.green = tmp_color.green;
				new_palette->color[ncolors].bg.blue = tmp_color.blue;
			}

			/* use the motif routine to generate fg, sc, ts, and bs */
			(*calcRGB)(&tmp_color,&(new_palette->color[ncolors].fg),
				  &(new_palette->color[ncolors].sc),
				  &(new_palette->color[ncolors].ts),
				  &(new_palette->color[ncolors].bs));
			if (TypeOfMonitor == B_W) 
				InitializeBW(tmp_color.red, ncolors, 
						new_palette);
			else
				SetColors(ncolors, new_palette, FgColor, 
						UsePixmaps );
			break;

		case 1:		/* foreground */
			if (result) {
				new_palette->color[ncolors].fg.red = tmp_color.red;
				
				new_palette->color[ncolors].fg.green = tmp_color.green;
				new_palette->color[ncolors].fg.blue = tmp_color.blue;
				new_palette->flags[ncolors] |= FOREGROUND;
			}
			break;

		case 2:
			if (result) {
				new_palette->color[ncolors].sc.red = tmp_color.red;
				new_palette->color[ncolors].sc.green = tmp_color.green;
				new_palette->color[ncolors].sc.blue = tmp_color.blue;
				new_palette->flags[ncolors] |= SELECT;
			}
			break;
			
		case 3:
			if (result) {
				new_palette->color[ncolors].ts.red = tmp_color.red;
				new_palette->color[ncolors].ts.green = tmp_color.green;
				new_palette->color[ncolors].ts.blue = tmp_color.blue;
				new_palette->flags[ncolors] |= TOPSHADOW;
			}
			break;
			
		case 4:
			if (result) {
				new_palette->color[ncolors].bs.red = tmp_color.red;
				new_palette->color[ncolors].bs.green = tmp_color.green;
				new_palette->color[ncolors].bs.blue = tmp_color.blue;
				new_palette->flags[ncolors] |= BOTTOMSHADOW;
			}
			break;
			
		}
		if (reset) {
			first = 0;
			new_palette->num_of_colors++;
			ncolors++;
			new_palette->flags[ncolors] = 0;
			reset = 0;
		} else { 
			first++;
			while (buf[bcnt] == ':') {
				bcnt++;
				first++;
			}
		}
	} 

	/* 
	 * have now read in complete palette file .. set number of colors 
	 * appropriatly because every palette has 8 entries 
	 */
	if (TypeOfMonitor == LOW_COLOR || TypeOfMonitor == B_W)
		new_palette->num_of_colors = 2;

	if (TypeOfMonitor == MEDIUM_COLOR)
		new_palette->num_of_colors = 4;

	if (TypeOfMonitor == LOW_COLOR) {
		savecolor(new_palette);
		SwitchAItoPS(new_palette);
	}
} 


/*
 * getstr(char *msg)
 *	Return an internationalize string.  The string is of the form
 * 	"file:index\001default string"
 */
char *
getstr(char *msg)
{
    char	*sep;
    char	*str;

    sep = strchr (msg, '\001');
    if (sep == NULL)
	return((char *)NULL);
    *sep = 0;
    str = gettxt(msg, sep + 1);
    *sep = '\001';
    return(str);
}

Boolean
verifyname(int depth, int monitor, char **pname, intstr_t istrs)
{

	int plen, len, ret;
	char *pnm = *pname;
	if (!strcmp(pnm, istrs.blackwhite) || !strcmp(pnm, istrs.whiteblack) 
	    || !strcmp(pnm, istrs.white) || !strcmp(pnm, istrs.black)) 
		return ((monitor == B_W) ? True : False);

	if (monitor == B_W) 
		return False; 
	len = strlen(istrs.prefix);
	if (ret =  strncmp(pnm, istrs.prefix, len)) 
		return ((depth == 4) ? False : True);
	if (depth != 4)
		return False;
	plen = strlen(pnm);
	if (plen == len)
		return False;
	strcpy(*pname, pnm + len);
	return True;
}
