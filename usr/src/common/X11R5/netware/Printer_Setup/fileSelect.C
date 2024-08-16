#ident	"@(#)prtsetup2:fileSelect.C	1.4"
/*----------------------------------------------------------------------------
 *
 */
#include <string.h> 
#include <unistd.h>
#include <assert.h>
#include <malloc.h> 
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <Xm/Text.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/List.h>

#include "fileSelect.h"
#include "MultiPList.h"

/*----------------------------------------------------------------------------
 *	Pixmap files to use
 */
char*							pixFiles[] = {
	"/usr/X/lib/pixmaps/spipe.icon",
	"/usr/X/lib/pixmaps/schrdev.icon",
	"/usr/X/lib/pixmaps/sdir.icon",
	"/usr/X/lib/pixmaps/sdatafile.icon",
	"/usr/X/lib/pixmaps/sblkdev.icon",
	"/usr/X/lib/pixmaps/sdatafile.icon",
	"/usr/X/lib/pixmaps/ssem.icon",
	"/usr/X/lib/pixmaps/sunk.icon",
	"/usr/X/lib/pixmaps/sexec.icon",
};

/*----------------------------------------------------------------------------
 *	Constructors/Destructors
 */
fileSelect::fileSelect (Widget parent, char* startPath)
{
	Widget						pathLabel;
	Widget						listForm;

	d_path = 0;
	d_numNames = 0;
	d_sizeNames = 20;
	if (!(d_names = new char*[d_sizeNames])) {
#ifdef DEBUG
		cerr << "New failed" << endl;
#endif
		d_sizeNames = 0;
	}
	d_updateCallback = 0;

	d_widget = XtCreateManagedWidget ("fileSelectForm",
									  xmFormWidgetClass,
									  parent,
									  NULL,
									  0);
  
	listForm = XtCreateManagedWidget ("listForm",
									  xmFormWidgetClass,
									  d_widget,
									  NULL,
									  0);
	d_list = new MultiPList (listForm, 2, &fileSelect::selectCallback, this);
	for (int i = 0;i < 9;i++) {
		d_list->AddPixmap (pixFiles[i]);
	}
	d_list->SetClearCallback (0);

	XtVaSetValues (listForm,
				   XmNtopAttachment,
				   XmATTACH_FORM,
				   XmNtopOffset,
				   15,
				   XmNleftAttachment,
				   XmATTACH_FORM,
				   XmNleftOffset,
				   15,
				   XmNrightAttachment,
				   XmATTACH_FORM,
				   NULL);

	d_pathWidget = XtVaCreateManagedWidget ("pathText",
											xmTextWidgetClass,
											d_widget,
						 					XmNbottomAttachment,
											XmATTACH_FORM,
						 					XmNleftAttachment,
											XmATTACH_FORM,
											XmNleftOffset,
											15,
						 					XmNrightAttachment,
											XmATTACH_FORM,
											NULL);

  	pathLabel = XtVaCreateManagedWidget ("Selection",
										 xmLabelWidgetClass,
										 d_widget,
					 					 XmNbottomAttachment,
										 XmATTACH_WIDGET,
										 XmNbottomWidget,
										 d_pathWidget,
					 					 XmNleftAttachment,
										 XmATTACH_FORM,
										 XmNleftOffset,
										 15,
					 					 XmNrightAttachment,
										 XmATTACH_FORM,
					 					 XmNalignment,
										 XmALIGNMENT_BEGINNING,
					 					 NULL);

	XtVaSetValues (listForm,
				   XmNbottomAttachment,
				   XmATTACH_WIDGET,
				   XmNbottomWidget,
				   pathLabel,
				   NULL);
	XtAddCallback (d_pathWidget,
				   XmNactivateCallback,
				   &fileSelect::newPathCallback,
				   (XtPointer)this);

	changePath (startPath);
}

fileSelect::~fileSelect ()
{
	if (d_path) {
		delete (d_path);
	}
	for (int i = 0;i < d_numNames;i++) {
		delete (d_names[i]);
	}
	if (d_names) {
		delete (d_names);
	}
}

/*----------------------------------------------------------------------------
 *
 */
char*
fileSelect::getPathName ()
{
	return (XmTextGetString (d_pathWidget));
};

/*----------------------------------------------------------------------------
 *
 */
void
fileSelect::setUpdateCallback (UpdateCallback func, XtPointer data)
{
	d_updateCallback = func;
	d_clientData = data;
}

/*----------------------------------------------------------------------------
 *
 */
void
fileSelect::updateName ()
{
	int							len = 0;
	int							i;

	if (d_path) {
		delete (d_path);
		d_path = 0;
	}
	if (d_numNames == 0) {
		return;
	}
	for (i = 0;i < d_numNames;i++) {
		len += strlen (d_names[i]);
	}
	len += d_numNames;

	d_path = new char[len + 1];
	strcpy (d_path, d_names[0]);
	for (i = 1;i < d_numNames;i++) {
		d_path = strcat (d_path, d_names[i]);
		if (i != d_numNames - 1) {
			d_path = strcat (d_path, "/");
		}
	}
}

/*----------------------------------------------------------------------------
 *
 */
const char*
fileSelect::getName ()
{
	updateName ();
	return (d_path);
}

/*----------------------------------------------------------------------------
 *
 */
int
fileSelect::growNames (int increment)
{
	char**						tmp;

	tmp = d_names;
	d_sizeNames += increment;
	if (!(d_names = new char*[d_sizeNames])) {
#ifdef DEBUG
		cerr << "New failed" << endl;
#endif
		d_names = tmp;
		d_sizeNames -= increment;
		return (0);
	}
	for (int i = 0;i < d_numNames;i++) {
		d_names[i] = tmp[i];
	}
	delete (tmp);
	return (1);
}

/*----------------------------------------------------------------------------
 *
 */
int
fileSelect::appendName (const char* name)
{
	if (!name || !(*name)) {
		return (0);
	}

	if (d_numNames == d_sizeNames) {
		if (!growNames (20)) {
			return (0);
		}
	}

	if (!(d_names[d_numNames] = new char [strlen (name) + 1])) {
#ifdef DEBUG
		cerr << "New failed" << endl;
#endif
		return (0);
	}
	strcpy (d_names[d_numNames], name);
	d_numNames++;

	return (1);
}

/*----------------------------------------------------------------------------
 *
 */
void
fileSelect::deleteName (int pos)
{
	while (d_numNames > pos) {
		delete (d_names[d_numNames - 1]);
		d_numNames--;
	}
}

/*----------------------------------------------------------------------------
 *
 */
void
fileSelect::select (char* item, int pos)
{
	d_list->ClearLists (pos + 1);
	deleteName (pos + 1);

	appendName (item);

	updateName ();
	if (!updatePath (d_path)) {
		d_numNames--;
		updateName ();
	}
	else {
		d_list->NextList ();
	}
	d_list->DisplayLists ();

	XmTextSetString (d_pathWidget, d_path);

	if (d_updateCallback) {
		d_updateCallback (d_clientData);
	}
}

/*----------------------------------------------------------------------------
 *
 */
void
fileSelect::newPath ()
{
	changePath (XmTextGetString (d_pathWidget));
}

/*----------------------------------------------------------------------------
 *
 */
void
fileSelect::changePath (char* path)
{
	XmString					xmstr; 
	char*						tcwd;
	int							len;
	int							i = 0, j;

	if (!path || !(*path)) {
		return;
	}
	len = strlen (path);
	if (!(tcwd = new char[len + 1])) {
		return;
	}
	strcpy (tcwd, path);

	d_list->ClearLists ();
	deleteName (0);

	if (tcwd[0] == '/') {
		if (appendName ("/")) {
			updateName ();
			if (!updatePath (d_path)) {
				d_numNames--;
				updateName ();
			}
			else {
				d_list->NextList ();
			}
		}
		i++;
	}
    for (j = i; j < len + 1; j++) {
		if (tcwd[j] == '/' || !tcwd[j]) {
			tcwd[j] = 0;
			if (appendName (&tcwd[i])) {
				updateName ();
				if (!updatePath (d_path)) {
					d_numNames--;
					updateName ();
					break;
				}
				d_list->NextList ();
			}
			i = j + 1;
		}
	}

	delete (tcwd);

	for (i = 1;i < d_numNames;i++) {
		xmstr = XmStringCreate (d_names[i], XmSTRING_DEFAULT_CHARSET);
		d_list->SelectItem (i - 1, xmstr);
		XmStringFree (xmstr);
	}
	d_list->DisplayLists ();

	XmTextSetString (d_pathWidget, d_path);

	if (d_updateCallback) {
		d_updateCallback (d_clientData);
	}
}

/*----------------------------------------------------------------------------
 *
 */
int
fileSelect::updatePath (char* path, int list, char* item)
{
	XmString					xmstr; 
	DIR*						dir;
	struct dirent*				ent;
	struct stat					stt;
	char*						buf;

	if (!(dir = opendir (path))) {
		return (0);
	}
	for (ent = readdir (dir); ent != NULL; ent = readdir (dir)) {
		if (strcmp (ent->d_name, ".") != 0 &&
					strcmp (ent->d_name, "..") != 0 && ent->d_name[0] != '.') {
			buf = new char[strlen (path) + strlen (ent->d_name) + 2];
			sprintf (buf, "%s/%s", path, ent->d_name);
			stat (buf, &stt);
			delete (buf);
			if (S_ISDIR (stt.st_mode)) {
				xmstr = XmStringCreate (ent->d_name, XmSTRING_DEFAULT_CHARSET);
				d_list->AddListItem (xmstr, 2, this);
			}
		}
	}
	closedir (dir);

	return (1);
}

/*----------------------------------------------------------------------------
 *
 */
void 
fileSelect::selectCallback (XtPointer ptr, char* item, int pathPos, XtPointer)
{
	MultiPList*					list = (MultiPList*)ptr;
	fileSelect*					thisPtr;

	thisPtr = (fileSelect*)list->GetObjClientData ();
	thisPtr->select (item, pathPos);
}

/*----------------------------------------------------------------------------
 *
 */
void
fileSelect::newPathCallback (Widget, XtPointer clientData, XtPointer)
{
    fileSelect*					obj = (fileSelect*)clientData;

	obj->newPath ();
}

