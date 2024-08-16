#ident	"@(#)prtsetup2:ps_main.C	1.17"
/*----------------------------------------------------------------------------
 *	ps_main.c
 */

#include <iostream.h>
#include <ctype.h>
#include <priv.h>
#include <unistd.h>
#include <Xm/Xm.h>
#include <X11/cursorfont.h>

#include "ps_i18n.h"
#include "dispInfo.h"
#include "ps_application.h"
#include "ps_mainwin.h"

#define NO_MATCH				0
#define ENTRY					1
#define CONTENTS				2
#define INTERFACE				3
#define MODULES					4
#define NAME					5
#define STTY					6
#define TERM					7

static void						initPrinterArray (char*			fileName,
												  PrinterArray*	spt);
static void						createWatchCursor ();

static const char*				WHITESPACE = { " \t\n" };
const char*						DOT_PRINTER = ".printer"; 

typedef struct {
	char*						key;
	int							len;
	int							type;
} KeyWord;

KeyWord keywords[] = {
	{ "entry",		5,	ENTRY },
	{ "contents",	8,	CONTENTS },
	{ "interface",	9,	INTERFACE },
	{ "modules",	7,	MODULES },
	{ "name",		4,	NAME },
	{ "stty",		4,	STTY },
	{ "terminfo",	8,	TERM },
	{ "printer",	7,	INTERFACE },
	{ 0,			0,	NO_MATCH }
};

PrinterArray					s_supportedPrinters;
static PSMainWin*				s_mainWin;
static Cursor					s_watchCursor;
static Display*					s_display;
DispInfo*						s_di;

/*----------------------------------------------------------------------------
 *	This function creates the Application, PSMainWin, and DispInfo objects. It
 *	also calls routines to begin execution of the main application loop.
 */
void
main (int argc, char** argv)
{ 
    Application*				ps;

	initPrinterArray ("/usr/X/desktop/PrintMgr/Printers", &s_supportedPrinters);
	if (s_supportedPrinters.cnt < 1) {
		s_supportedPrinters.warning = PT_WARNING;
	}

    if (!(ps = new Application (argc, argv, GetLocalStr (TXT_appName)))) {
		cerr << "Error:  out of memory" << endl;
		exit (0);
	}
	if (!(s_di = new DispInfo (ps->baseWidget ()))) {
		cerr << "Error:  out of memory" << endl;
		exit (0);
	}

	s_display = s_di->Display ();
	createWatchCursor ();

	XtVaSetValues (ps->baseWidget (), XmNtitle, GetLocalStr (TXT_appName));
    if (!(s_mainWin = new PSMainWin (ps->baseWidget (), "Main_Win", ps))) {
		cerr << "Error:  out of memory" << endl;
		exit (0);
	}

    ps->RealizeLoop (s_di);
	s_di->SetWindow ();
}

/*----------------------------------------------------------------------------
 *	Create an array of pointers to chars to each comma or space seperated
 *	item in a string.
 */
static char**
GetList (char* str)
{ 
	char**						list;
	char*						tmpList[32];
	int							cnt = 0;

	str = strtok (str, " ,");
	while (str) {
		tmpList[cnt++] = strdup (str);
		str = strtok (NULL, " ,");
	}
	tmpList[cnt++] = 0;

	list = (char**)XtMalloc (cnt * sizeof (*list));
	(void)memcpy (list, tmpList, cnt * sizeof (*list));
	return (list);
}

/*----------------------------------------------------------------------------
 *
 */
static int
prtcmp (const void* a1, const void* a2)
{
	SupportedPrinter*			p1 = (SupportedPrinter*)a1;
	SupportedPrinter*			p2 = (SupportedPrinter*)a2;

	return (strcoll (p1->name, p2->name));
}

/*----------------------------------------------------------------------------
 *
 */
static void
initPrinterArray (char* fileName, PrinterArray* spt)
{
	FILE*						printerFile;
	char*						token;
	char						buf[256 + 1];
	int							type;
	int							cnt;
	int							i;

	spt->cnt = 0;
	spt->allocated = 0;
	spt->sPrinters = 0;
	spt->warning = PT_OK;
	cnt = -1;

	if (!(printerFile = fopen (fileName, "r"))) {
#ifdef DEBUG
		cerr << "Cannot open file: " << fileName << endl;
#endif
		return;
	}

	while (fgets (buf, 256, printerFile)) {
		token = strchr (buf, '#');				// Remove comments
		if (token) {
			*token = 0;
		}
		token = buf + strlen (buf) - 1;
		if (token >= buf && *token == '\n') {	// Remove whitespace
			*token = 0;
		}
		token = buf + strspn (buf, WHITESPACE);	

		type = NO_MATCH;
		for (i = 0;keywords[i].key;i++) {		// Find the type of the keyword
			if (!strncmp (keywords[i].key, token, keywords[i].len)) {
				type = keywords[i].type;
				break;
			}	
		}

		token = strchr (token, ':');			// Strip keyword,":",whitespace
		token = token + strspn (token, ": \n\t");

		if (type == ENTRY) {
			cnt++;
			if (cnt >= spt->allocated) {
				spt->allocated += 20;
				spt->sPrinters = (SupportedPrinter*) 
										XtRealloc ((char*)spt->sPrinters,
												   spt->allocated *
												   sizeof (SupportedPrinter)); 
			}

			if (cnt > 0) {
				if (!spt->sPrinters[cnt - 1].name) {
					spt->warning = PT_WARNING;
					cnt--;
				}
			}

			spt->sPrinters[cnt].name = 0;
			spt->sPrinters[cnt].terminfo = 0;
			spt->sPrinters[cnt].contents = 0;
			spt->sPrinters[cnt].interface = 0;
			spt->sPrinters[cnt].stty = 0;
			spt->sPrinters[cnt].modules = 0;
			continue;
		}

		if (cnt < 0) {
			spt->warning = PT_WARNING;
			continue;
		}

		switch (type) {
		case CONTENTS:
			spt->sPrinters[cnt].contents = GetList (token);
			break;

		case INTERFACE:
			spt->sPrinters[cnt].interface = copyString (token);
			break;

		case MODULES:
			spt->sPrinters[cnt].modules = GetList (token);
			break;
		
		case NAME:
			spt->sPrinters[cnt].name = GetName (token);
			break;

		case STTY:
			spt->sPrinters[cnt].stty = copyString (token);
			break;

		case TERM:
			spt->sPrinters[cnt].terminfo = GetList (token);
			break;

		case NO_MATCH:
		default:
			break;
		}	
	}

	if (cnt >= 0) {
		if (!spt->sPrinters[cnt].name) {
			spt->warning = PT_WARNING;
			cnt--;
		}
	}

	spt->cnt = cnt + 1;

	if (fclose (printerFile)) {
#ifdef DEBUG
		cerr << "Cannot close file: " << fileName << endl;
#endif
		spt->warning = PT_WARNING;
	}

	qsort (spt->sPrinters, spt->cnt, sizeof (SupportedPrinter), prtcmp);
}

/*----------------------------------------------------------------------------
 *
 */
static void
createWatchCursor ()
{
	s_watchCursor = XCreateFontCursor (s_display, XC_watch);

	if (s_watchCursor == BadAlloc
		|| s_watchCursor == BadFont
		|| s_watchCursor == BadValue) {
		s_watchCursor = 0;
	}
}

/*----------------------------------------------------------------------------
 *
 */
void
setWatchCursor (Window window, Boolean flush)
{
	if (s_watchCursor) {
		XDefineCursor (s_display, window, s_watchCursor);
	}

	if (flush) {
		XFlush (s_display);
	}
}

/*----------------------------------------------------------------------------
 *
 */
void
resetCursor (Window window, Boolean flush)
{
	XDefineCursor (s_display, window, None);

	if (flush) {
		XFlush (s_display);
	}
}

/*----------------------------------------------------------------------------
 * Clear privilges so that the X application can find it's libraries.
 */
void
noPrivSystem (const char* s)
{
	if (fork () == 0) {
		procprivl (CLRPRV, pm_max (P_ALLPRIVS), (priv_t)0);
		system (s);
		exit (1);
	}
}

/*----------------------------------------------------------------------------
 *
 */
void
helpDisplay (unsigned long type, char* title, char* sectTag)
{
	DtRequest					request;

	DtInitialize (s_mainWin->baseWidget ());

	memset (&request, 0, sizeof (request));

	request.display_help.rqtype = DT_DISPLAY_HELP;
	request.display_help.version = 1;
	request.display_help.client = XtWindow (s_mainWin->baseWidget ());
	request.display_help.app_name = "Printer Setup";
	request.display_help.file_name = helpFile;
	request.display_help.source_type = type;
	request.display_help.title = title;
	request.display_help.sect_tag = sectTag;

	DtEnqueueRequest (XtScreen (s_mainWin->baseWidget ()),
					  _HELP_QUEUE (s_display),
					  _HELP_QUEUE (s_display),
					  XtWindow (s_mainWin->baseWidget ()),
					  &request);
}

/*----------------------------------------------------------------------------
 *
 */
char*
copyString (const char* str)
{
	char*							newstr;

	if (!str) {
		return (0);
	}
	if (!(newstr = (char*)malloc (strlen (str) + 1))) {
		return (0);
	}
	strcpy (newstr, str);
	return (newstr);
}

/*----------------------------------------------------------------------------
 *
 */
Boolean
legalName (char* str)
{
	if (!str || strlen (str) > 255) {
		return (FALSE);
	}
	for (char* pt = str;*pt;pt++) {
		if (!isprint (*pt)) {
			return (FALSE);
		}
		if (*pt == ' ' ||
			*pt == '/' ||
			*pt == '\\' ||
			*pt == ':' ||
			*pt == ';' ||
			*pt == ',' ||
			*pt == '*' ||
			*pt == '?' ||
			*pt == '~') {
			return (FALSE);
		}
	}
	return (TRUE);
}

