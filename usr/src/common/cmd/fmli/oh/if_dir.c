/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:oh/if_dir.c	1.45.3.8"

#include <stdio.h>
#include <varargs.h>
#include <string.h>
#include "inc.types.h"		/* abs s14 */
#include "wish.h"
#include "menudefs.h"
#include "vtdefs.h"
#include "token.h"
#include "slk.h"
#include "actrec.h"
#include "typetab.h"
#include "ctl.h"
#include "var_arrays.h"
#include "terror.h"
#include "moremacros.h"
#include "message.h"
#include "sizes.h"
#include <unistd.h>

extern	menu_id folder_make();
extern	char	*path_to_title();

#define MAX_DIRS	(16)
#define SUBSTLEN 2
#define INT_SUBSTLEN 4         /*** 
                                    Length of conversion specification with
                                    precision
                               ***/

/* The activation record for a file folder contains the ott_tab for the
 * folder in the odptr field.  File folders use the menu human-interface,
 * so the id field of the activation record is a menu_id.
 */

/* the sort mode for the current Directory object */
long Sortmodes = OTT_SALPHA;
/* the display mode for the current Directory object */
long Dispmodes = OTT_DOBJ;
/* the last modification time of the pref file */
time_t Prefmodtime = (time_t)0L; /* EFT abs k16 */

static int Dirs_open = 0;	/* count how many open */

/* macro to cast the odptr field of an actrec to my struct odptr */

#define MYODPTR(X)	((struct myodptr *)(X->odptr))

static struct myodptr {
	struct ott_tab *ott;
	time_t dir_mtime;		/* EFT abs k16 */
	time_t ott_mtime;		/* EFT abs k16 */
	time_t pref_mtime;	/* EFT abs k16 */
};

static int 
dir_close(a) 
struct actrec *a; 
{ 
	extern char *Filecabinet;
	extern char Opwd[];
#ifdef _DEBUG
	_debug(stderr, "DIR_CLOSE\n");
#endif
	ott_unlock_inc(MYODPTR(a)->ott); /* unlock the ott so it can be swapped */
	free(MYODPTR(a));
	if (a->path)
		free(a->path);
	Dirs_open--;
	if ( ! Dirs_open ) {
		(void) chdir(Filecabinet);
		sprintf(&Opwd[5], "%.*s", PATHSIZ, Filecabinet);
	}
	return(menu_close(a->id));		/* close the menu id */
}

static struct menu_line
dir_disp(n, ptr)
int n;
struct ott_tab *ptr;
{
    register int i;
    int size = array_len(ptr->parents);
    int d_cols = COLS - FIXED_COLS;
    struct ott_entry *entry;
    struct menu_line m;
    static char ldescr[MAX_WIDTH];
    char *bsd_path_to_title();

    char *i18n_string;
    int   i18n_length;

    m.flags = 0;

    if (n == 0 && size == 0) {	/* empty! */
	m.highlight = gettxt(":90","Empty Folder");
	m.description = NULL;
    } else if (n >= (int)array_len(ptr->parents)) { /* done */
	m.highlight = m.description = NULL;
    } else {
	entry = &(ptr->ott[ptr->parents[n]]);

	switch (ptr->modes & DISMODES) {
	case 0:			/* don't display anything but name */
	    m.description = NULL;
	    m.highlight = bsd_path_to_title(entry->dname,d_cols);
	    break;
	case OTT_DMARK:
	    m.description = NULL;
	    if (entry->objmask & CL_DIR) {
	        strcpy(ldescr, bsd_path_to_title(entry->dname,(d_cols-1)));
		strcat(ldescr, "/");
	    }
	    else if (strcoll(entry->objtype, "EXECUTABLE") == 0 && strcmp(entry->objtype, "EXECUTABLE") == 0) {
	        strcpy(ldescr, bsd_path_to_title(entry->dname,(d_cols-1)));
		strcat(ldescr, "*");
	    }
	    else
	        strcpy(ldescr, bsd_path_to_title(entry->dname,d_cols));
	    m.highlight = ldescr;
	    break;
	default:
	case OTT_DOBJ:		/* display description only */
	    m.description = entry->display;
	    m.highlight = entry->hname;  /* dmd s15 */
	    break;
	case OTT_DODI:		/* display description with odi value */
	{
	    char *key_val, *odi_getkey();

	    key_val = odi_getkey(entry,"UNDELDIR");
	    if ( key_val && *key_val ) {
/* dmd s15
		if ( (i = strlen(entry->dname)) > 20 )
		    i = 20;
		i = d_cols - i - (int)strlen(entry->display) - 11;
*/
		i = d_cols - 20 - (int)strlen(entry->display) - 11;	/* dmd s15 */

		sprintf(ldescr, gettxt(":91","%s - From %s"),entry->display,bsd_path_to_title(key_val, i));
	    } else {

		sprintf(ldescr, gettxt(":92","%s - From unknown"),entry->display);
	    }
	}
	    m.highlight = entry->hname;	/* dmd s15 */
	    m.description = ldescr;
	    break;
	case OTT_DMTIME:	/* display long form */
	{
	    char *p, *ct, *ictime();
	    int len, i;
	    bool usrdisp = FALSE;
	    struct ott_entry *tmp;
	    char *def_display();
	    int	lcv;

	    lcv = array_len(ptr->parents);
	    for (i = 0; !usrdisp && i < lcv; i++) {
		tmp = &(ptr->ott[ptr->parents[i]]);
		p = def_display(tmp->objtype);
		if (tmp->display != p && strcoll(tmp->display, p) != 0)
		    usrdisp = TRUE;
	    }

	    if (entry->display == (p = def_display(entry->objtype)) 
		|| strcoll(entry->display, p) == 0 && strcmp(entry->display, p) == 0 )
		len = sprintf(ldescr, "%-14.14s%*s", 
			      entry->display, usrdisp ? 23 : 2, "");
	    else
		len = sprintf(ldescr, "%-19.19s [%-.14s]%*s ", 
			      entry->display, p, 14-(int)strlen(p), "");
	    ct = ictime(&(entry->mtime));
	    sprintf(ldescr+len, "%12.12s", ct+4);
	}
	    m.highlight = entry->hname;	/* dmd s15 */
	    m.description = ldescr;
	    break;
	}
    }
    return(m);
}

static int
dir_help(a)
struct actrec *a;
{
	extern char *Wastebasket;
	char *help, *title, path[PATHSIZ];
	char *filename(), *anyenv();

	sprintf(path, "%s/.pref", a->path);
	if ((help = anyenv(path, "HELP")) == NULL || *help == '\0') {
		if (strncmp(a->path, Wastebasket, strlen(Wastebasket)) == 0) {
			help = "T.h55.waste";
			title = gettxt(":355","Wastebasket");
		}
		else {
			help = "T.h43.fold";
			title = gettxt(":153","File folder");
		}
	} else {
		title = filename(a->path);
	}

	return(objop("OPEN", "TEXT", "$VMSYS/OBJECTS/Text.mfhelp", help, 
					title, NULL));
}

static int
dir_current(a)
struct actrec *a;
{
	extern char Opwd[];

	make_current(a->path);
	menu_current(a->id);
	chdir(a->path);
	sprintf(&Opwd[5], "%.*s", PATHSIZ, a->path);
	return(SUCCESS);
}

static int
dir_reread(a)
struct actrec *a;
{
	return(dir_init(a, TRUE));
}

static int
dir_reinit(a)
struct actrec *a;
{
	return(dir_init(a, FALSE));
}

static int
dir_init(a, force)
struct actrec *a;
bool force;
{
	struct ott_tab *ott = MYODPTR(a)->ott;

#ifdef _DEBUG
	_debug(stderr, "DIR_REINIT ");
#endif
	MYODPTR(a)->ott = (ott = ott_get(ott->path, Sortmodes, Dispmodes,
				ott->amask, ott->nmask));
	if (ott == NULL)
		return(FAIL);

	if (force || MYODPTR(a)->dir_mtime != ott->dir_mtime || 
				MYODPTR(a)->ott_mtime != ott->ott_mtime  ||
				Prefmodtime != MYODPTR(a)->pref_mtime ) {
#ifdef _DEBUG
		_debug(stderr, "RECREATING WINDOW %s\n", ott->path);
#endif
		MYODPTR(a)->dir_mtime = ott->dir_mtime;
		MYODPTR(a)->ott_mtime = ott->ott_mtime;
		MYODPTR(a)->pref_mtime = Prefmodtime;
		a->id = folder_reinit(a->id, 0, 18, 0, dir_disp, ott);
	}
#ifdef _DEBUG
	else
		_debug(stderr, "No change in directory\n");
#endif

	ott_lock_inc(ott);
	return(SUCCESS);
}

/* these arguments are kludgy, but varargs wouldn't do the trick */

static int
dir_ctl(rec, cmd, arg1, arg2, arg3, arg4, arg5, arg6)
struct actrec *rec;
int cmd;
int arg1, arg2, arg3, arg4, arg5, arg6;
{
#ifdef _DEBUG
    _debug(stderr, "DIR_CTL: cmd=%d\n", cmd);
#endif
    switch (cmd)
    {
    case CTGETARG:
    {
	int line;
	char *path;
	struct ott_entry *entry;
	struct ott_tab *tab = MYODPTR(rec)->ott;
	char *ott_to_path();

	if (array_len(tab->parents) == 0)
	    return(FAIL);
	(void) menu_ctl(rec->id, CTGETPOS, &line);
	entry = &(tab->ott[tab->parents[line]]);
	path = ott_to_path(entry);
	/*if ( **((char ***)(&arg1)))
	  free( **((char ***)(&arg1))); /* les 12/4 */
	**((char ***)(&arg1)) = strsave(path);
	return(SUCCESS);
    }
    case CTSETMSG:
	/* framemsg is always this - miked */
	mess_frame(gettxt(":361","Move to an item with arrow keys and press ENTER to select the item."));
	return(SUCCESS);
    case CTSETLIFE:
	/* lifetime of directory is always longterm */
	return(SUCCESS);
    case CTISDEST:
	**((bool **)(&arg1)) = TRUE;
	return(SUCCESS);
    default:
	return(menu_ctl(rec->id, cmd, arg1, arg2, arg3, arg4, arg5, arg6));
	break;
    }
}

static token
dir_odsh(rec, t)
struct actrec *rec;
register token t;
{
	token menu_stream();

#ifdef _DEBUG
	_debug(stderr, "dir_odsh(%o) => ", t);
#endif
	t = menu_stream(t);
#ifdef _DEBUG
	_debug(stderr, "%o\n", t);
#endif
	if (t == TOK_NEXT)
		t = TOK_NOP;	/* filter out, see menu_stream */
	return(t);
}

int
IF_dir_open(argv)
char *argv[];
{
    register int i;
    char *path;
    long amask, nmask;
    struct ott_tab *ott;
    struct actrec a, *prevdir, *path_to_ar();
    vt_id vid;
    char	*bsd_path_to_title();
    char	*nstrcat();

    struct ott_tab *ott_get();

    char *i18n_string;
    int   i18n_length;


    init_modes();
    amask = 0;
    nmask = M_WB;

    for (i = 0; argv[i]; i++) {
	if (argv[i][0] == '-') {
	    switch (argv[i][1]) {
	    case 'w':		/* wastebasket mode */
		amask = M_WB;
		nmask = 0;
		break;
	    case 'a':		/* set amask only */
		amask = strtol(argv[i]+2);
		break;
	    case 'n':		/* set nmask only */
		nmask = strtol(argv[i]+2);
		break;
	    }
	} else
	    path = argv[i];
    }

    if (prevdir = path_to_ar(path)) {
	ar_current(prevdir, TRUE);
	return(SUCCESS);
    }
    if (Dirs_open > MAX_DIRS) {

	(void)mess_err( gettxt(":93","Too many folders open. Close some, then try again.") ); /* abs s15 */

	return(FAIL);
    }
    if ((ott = ott_get(path, Sortmodes, Dispmodes, amask, nmask)) == NULL) {

        i18n_string=gettxt(":94","Could not open folder %s");
        i18n_length=(int)strlen(i18n_string) - SUBSTLEN;

        io_printf( 'm', NULL, i18n_string,
			  bsd_path_to_title(path, MESS_COLS - i18n_length) );

	return(FAIL);
    }

    if (dir_create(ott, &a, FALSE) == FAIL || !ar_current(ar_create(&a), FALSE))
	return(FAIL);
    Dirs_open++;
    return(SUCCESS);
}

dir_create(ott, a, cover)
struct ott_tab *ott;
struct actrec *a;
bool cover;
{
	static char path_tit[MAX_WIDTH];
	char *bsd_path_to_title();

 	/*if ( a->path )
 		free( a->path );		/* les 12/4 */
 	a->path = strsave(ott->path);
	a->fcntbl[AR_CLOSE] = dir_close;
	a->fcntbl[AR_REREAD] = dir_reread;
	a->fcntbl[AR_REINIT] = dir_reinit;
	a->fcntbl[AR_CURRENT] = dir_current;
	a->fcntbl[AR_TEMP_CUR] = dir_current; /* abs k15. optimize later */
	a->fcntbl[AR_NONCUR] = AR_MEN_NONCUR;
	a->fcntbl[AR_HELP] = dir_help;
	a->fcntbl[AR_CTL] = dir_ctl;
	a->fcntbl[AR_ODSH] = (int (*)())dir_odsh; /* added cast. abs */

	/* we will keep track of what the mod times were on the ott at the
	 * time we first created the window, that way we will be able to 
	 * know when we should recreate the window to reflect the new data.
	 */
	a->odptr = (char *) new(struct myodptr);
	MYODPTR(a)->ott = ott;
	MYODPTR(a)->dir_mtime = ott->dir_mtime;
	MYODPTR(a)->ott_mtime = ott->ott_mtime;
#ifdef _DEBUG
	_debug(stderr, "dir_create mtimes=%d %d\n", ott->dir_mtime, ott->ott_mtime);
#endif

/*
	(void)strncpy(path_tit,path_to_title(ott->path, NULL, 0),MAX_TITLE-1);
	path_tit[MAX_TITLE-1] = '\0';
*/
	(void)strcpy(path_tit,bsd_path_to_title(ott->path, 0));
	a->id = folder_make(-1, path_tit, cover ? VT_COVERCUR : 0,
			VT_UNDEFINED, VT_UNDEFINED, 18, 0, dir_disp, ott);

	if (a->id == FAIL)
		return(FAIL);

	a->lifetime = AR_LONGTERM;
	a->flags = 0;
	a->slks = NULL;

	return SUCCESS;
}

/* selection handlers are not used by wish - they are built in to the
 * ODSH function
 */

/* dmd TCB
int
IF_dsh()
{
}
*/

int
IF_dvi()
{
}

int
IF_dmv(argv)
char *argv[];
{
	return(dir_mv_cp(TRUE, argv));
}

int
IF_dcp(argv)
char *argv[];
{
	return(dir_mv_cp(FALSE, argv));
}

static bool
eq_waste_fc(path, op)
char *path, *op;
{
	char *err = NULL;
	extern char *Filecabinet, *Wastebasket;
	char *filename() , *nstrcat();

        char *i18n_string;
        int   i18n_length;

	if (strcoll(Filecabinet, path) == 0 && strcmp(Filecabinet,path) == 0)
		err = Filecabinet;
	else if (strcoll(Wastebasket, path) == 0 && strcmp(Wastebasket, path) ==0)
		err = Wastebasket;
	if (err) {

                if (strcoll(op,"move") == 0)
                     io_printf( 'm', NULL, gettxt(":95","Can't move your %s"),
                                filename(err) );

                else if (strcoll(op,"copy") == 0)
                       io_printf( 'm', NULL, gettxt(":96","Can't copy your %s"),
                                  filename(err) );

                else if (strcoll(op,"rename") == 0)
                       io_printf( 'm', NULL, gettxt(":97","Can't rename your %s"),
                                  filename(err) );


		return(TRUE);
	} else
		return(FALSE);
}

static int
dir_mv_cp(mv, argv)
bool mv;
char *argv[];
{
    struct ott_entry *ott;
    char command[FILE_NAME_SIZ+PATHSIZ*2+11];
    char *odi, *display, path[PATHSIZ], msg[MESSIZ];
    int l, ret = SUCCESS;
    extern char *Wastebasket, Undel[];
    char *nstrcat(), *filename();
    struct ott_entry *path_to_ott();
    char	*bsd_path_to_title();
    char	*parent();
    char	*spath;
 
    char *i18n_string;
    int   i18n_length;



    working(TRUE);
    if (strncmp(argv[0], argv[1], l = strlen(argv[0])) == 0 &&
	argv[1][0] == '/') {

        if (mv != 0)
          (void)mess_err( gettxt(":98","Can't move a folder inside itself!") );
        else
          (void)mess_err( gettxt(":99","Can't copy a folder inside itself!") );
               

	return(FAIL);
    }

    if (eq_waste_fc(argv[0], mv ? "move" : "copy"))
	return(FAIL);
    if (mv && ckperms(parent(argv[0]), 02) == FAIL)
	return(FAIL);
    if (ckperms(argv[1], 02) == FAIL)
	return(FAIL);
    if (mv && path_isopen(argv[0], mv ? "move" : "copy", TRUE))
	return(FAIL);
    if ((ott = path_to_ott(argv[0])) == NULL)
	return(FAIL);
    odi = strsave(ott->odi);
    display = strsave(ott->display);
    sprintf(command, "dir_%s %s %s %s", mv?"move":"copy", argv[0], argv[1], 
	    argv[2]?argv[2]:"");

    if (waitspawn(sysspawn(command)) == 0) {

	if ( argv[2] ) {

           if (mv != 0) 
             i18n_string=gettxt(":100","%1$s moved to the %2$s folder and named %3$s");
           else
             i18n_string=gettxt(":101","%1$s copied to the %2$s folder and named %3$s");

           i18n_length = (int)strlen(i18n_string) - 3 * INT_SUBSTLEN;
                
            
	   sprintf(msg, i18n_string,
	      bsd_path_to_title( filename(argv[0]), (MESS_COLS-i18n_length)/3 ), 
	      bsd_path_to_title( argv[1],(MESS_COLS-i18n_length)/3 ),
	      bsd_path_to_title( argv[2],(MESS_COLS-i18n_length)/3 ) );


	} else {

           if (mv != 0) 
             i18n_string=gettxt(":102","%1$s moved to the %2$s folder"); 
           else
             i18n_string=gettxt(":103","%1$s copied to the %2$s folder");

           i18n_length = (int)strlen(i18n_string) - 2 * INT_SUBSTLEN;

	   sprintf(msg, i18n_string,
              bsd_path_to_title( filename(argv[0]), (MESS_COLS-i18n_length)/2 ),
	      bsd_path_to_title( argv[1],(MESS_COLS-i18n_length)/2 ) );
	}


/* miked k17
	sprintf(msg, "%s %sed to the ",
	    bsd_path_to_title(argv[2] ? argv[2] : filename(argv[0]),(MESS_COLS-22)/2),
	    mv ? "mov" : "copi");
	l = strlen(msg);
	strcat(msg, bsd_path_to_title(argv[1],MESS_COLS - l - 7));
	strcat(msg, " folder");
*/
	sprintf(path, "%s/%s", argv[1], argv[2] ? argv[2] : filename(argv[0]));
	if ((ott = path_to_ott(path)) == NULL)
	    return(FAIL);
	/*if ( ott->odi )
	  free( ott->odi );   /* les 12/4 */
	ott->odi = odi;
	/*if ( ott->display )
	  free( ott->display ); /* les 12/4 */
	ott->display = display;
	(void) ott_chg_odi(ott);
	if (strncmp(path, Wastebasket, strlen(Wastebasket)) == 0)
	    (void) odi_putkey(ott, Undel, parent(argv[0]));
	else if (strncmp(argv[0], Wastebasket, strlen(Wastebasket)) == 0)
	    (void) odi_putkey(ott, Undel, NULL);
	(void) utime(path, NULL);
	ott_mtime(ott);
	ret = SUCCESS;
    } else {
         if (mv !=0) 
               i18n_string=gettxt(":104","%1$s move failed to the %2$s folder");
         else
               i18n_string=gettxt(":105","%1$s copy failed to the %2$s folder");

         i18n_length=(int)strlen(i18n_string) - 2 * INT_SUBSTLEN;
         sprintf(msg,i18n_string,
	        bsd_path_to_title(argv[2] ? argv[2] : filename(argv[0]),(MESS_COLS-i18n_length)/2),
	        bsd_path_to_title(argv[1],(MESS_COLS - i18n_length)/2) );

	ret = FAIL;
    }
    (void)mess_err(msg);	/* abs s15 */
    return(ret);
}


int
IF_drn(argv)
char *argv[];
{
	register char	*p;
	char	destination[PATHSIZ];
	char	*nstrcat(), *filename(), *parent();
	char	*bsd_path_to_title();

        char *i18n_string;
        int   i18n_length;

	p = filename(argv[0]);
	if (ckperms(parent(argv[0]), 02) == FAIL)
		return(FAIL);
	if (path_isopen(argv[0], "rename", TRUE))
		return(FAIL);
	if (eq_waste_fc(argv[0], "rename"))
		return(FAIL);
	sprintf(destination,"%s/%s",parent(argv[0]),argv[1]);	/* dmd s15 */
	if (waitspawn(spawn("/bin/mv", "mv", "-f", argv[0], destination, NULL))) {
                i18n_string=gettxt(":106","%s rename failed");
                i18n_length=(int)strlen(i18n_string) - SUBSTLEN;

                io_printf( 'm', NULL, i18n_string,
		           bsd_path_to_title(p,MESS_COLS - i18n_length) );

		return(FAIL);
	} else {
		mess_temp(nstrcat(bsd_path_to_title(p,(MESS_COLS - 12)/2), " renamed to ", bsd_path_to_title(argv[1],(MESS_COLS - 12)/2), NULL));
		return(SUCCESS);
	}
}

/*
 * note:  This subroutine depends on the fact that its argument is
 * a legal UNIX path, i.e., slash separated strings.
 */

char *
bsd_path_to_title(str, width)
char	*str;
int	width;
{
    static char	title[MAX_WIDTH];
    register int len;
    extern char *parent();
    extern char *filename();

    if ((str == NULL) || (*str == '\0'))
	return str;	/* protect ourselves from the empty devil */

    if (width <= 0)
	width = MAX_TITLE;
    if (width >= MAX_WIDTH)
	width = MAX_WIDTH - 1;  /* lets not exceed the buffer */

    if ((len = strlen(str)) > width) {
	register int	flen;

	flen = strlen(filename(str));
	if (flen >= (width-1)) {
	    if (len == flen || len == flen+1) {
		(void) strncpy(title, str, (width-1));
	    } else {
		(void) strcpy(title, "</");
		(void) strncat(title, filename(str), (width-3));
	    }
	    (void) strcpy(&title[width-1], ">");
	} else if (flen) {
	    if ( (int)strlen(parent(str)) <= 4 ) {   /* EFT k16 */
		(void) strcpy(title, "</");
		(void) strcat(title, filename(str));
	    } else {
		(void) strcpy(title, path_to_title(parent(str),"X",(width-flen-1)));
		(void) strcat(title, "/");
		(void) strcat(title, filename(str));
	    }
	} else
	    (void) strcpy(title, path_to_title(str,"X",width));
    } else
	strcpy(title, str);
    return strdup(title);
}
