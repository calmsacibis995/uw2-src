#ident	"@(#)systuner:callbacks.C	1.11"
// callbacks.c

//////////////////////////////////////////////////////////////////
// Copyright (c) 1993 Novell
// All Rights Reserved
//
// THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF NOVELL
//
// The copyright notice above does not evidence any
// actual or intended publication of such source code.
///////////////////////////////////////////////////////////////////

#include "caw.h"
#include "link.h"
#include <X11/cursorfont.h>

void watch_cursor (Widget w)
{
	Cursor c1;

	c1 = XCreateFontCursor (XtDisplay (w), XC_watch);
	XDefineCursor (XtDisplay (w), XtWindow (w), c1);
	XFlush (XtDisplay (w));
}

void normal_cursor (Widget w)
{
	XUndefineCursor (XtDisplay (w), XtWindow (w));
	XFlush (XtDisplay (w));
}

void text_focus_CB (Widget w, _parameter *client_data, XmAnyCallbackStruct *call_data)
{
	int ac;
	Arg al[20];
	char s[DESC_SIZE];

	if (last_parameter != NULL) {
		ac = 0;
		XtSetArg (al[ac], XmNshadowThickness, 0); ac++;
		XtSetValues (last_parameter, al, ac);
	}
	last_parameter = w;
	ac = 0;
	XtSetArg (al[ac], XmNshadowThickness, 2); ac++;
	XtSetValues (w, al, ac);
	ac = 0;
	XtSetArg (al[ac], XmNmaximum, client_data->max); ac++;
	XtSetArg (al[ac], XmNminimum, client_data->min); ac++;
	if (client_data->current == (-1)) {
		XtSetArg (al[ac], XmNvalue, client_data->min); ac++;
	} else {
		XtSetArg (al[ac], XmNvalue, client_data->current); ac++;
	}
	XtSetValues (scale, al, ac);
	XtSetSensitive (scale, True);
	if ((current_parameter != NULL) && (current_parameter->autoconfig))
		XtSetSensitive (current_parameter->toggle, False);
	if (client_data->autoconfig)
		XtSetSensitive (client_data->toggle, True);
	current_parameter = client_data;
	strcpy (s, get_description (current_parameter->name));
	XmTextSetString (text, s);
}

void text_losing_focus_CB (Widget w, _parameter *client_data, XmAnyCallbackStruct *call_data)
{
	char s[20];

	strcpy (s, XmTextGetString (client_data->text));
	if (strcmp (s, getStr (TXT_autotuning)) != 0)
		client_data->current = (int) strtol (s, (char **) NULL, 0);
	else
		client_data->current = (-1);
	if (client_data->modified) {
		if (client_data->current >= client_data->max)
			client_data->current = client_data->max;
		else if ((client_data->current < client_data->min) && (client_data->current != (-1)))
			client_data->current = client_data->min;
		if (client_data->hex)
			sprintf (s, "%s%X", "0x", client_data->current);
		else
			sprintf (s, "%d", client_data->current);
		XmTextSetString (current_parameter->text, s);
	}
}

void text_value_changed_CB (Widget w, _parameter *client_data, XmAnyCallbackStruct *call_data)
{
	int ac;
	Arg al[20];
	char s[20];

	if (current_parameter == client_data) {
		strcpy (s, XmTextGetString (client_data->text));
		if (strcmp (s, getStr (TXT_autotuning)) != 0) {
			client_data->current = (int) strtol (s, (char **) NULL, 0);
			if (client_data->current > client_data->max) {
				client_data->current = client_data->max;
				XmScaleSetValue (scale, client_data->current);
			} else if (client_data->current <= client_data->min) {
				client_data->current = client_data->min;
				XmScaleSetValue (scale, client_data->current);
			} else
				XmScaleSetValue (scale, client_data->current);
			if (client_data->autoconfig) {
				ac = 0;
				XtSetArg (al[ac], XmNset, False); ac++;
				XtSetValues (client_data->toggle, al, ac);
			}
		}
		client_data->modified = True;
	}
}

void toggleCB (Widget w, XtPointer client_data, XmAnyCallbackStruct *call_data)
{
	int ac;
	Arg al[20];
	Boolean depressed = False;
	char s[20];

	ac = 0;
	XtSetArg (al[ac], XmNset, &depressed); ac++;
	XtGetValues (current_parameter->toggle, al, ac);
	if (depressed) {
		XmTextSetString (current_parameter->text, getStr (TXT_autotuning));
		current_parameter->current = (-1);
		XmScaleSetValue (scale, current_parameter->min);
	} else {
		current_parameter->current = current_parameter->last;
		sprintf (s, "%d", current_parameter->current);
		XmTextSetString (current_parameter->text, s);
	}
	current_parameter->modified = True;
}

void item_selected (_category *c)
{
	int ac;
	char s[20];
	Arg al[20];
	_parameter *temp;

	set_text (c);
	temp = c->head;
	while (temp != NULL) {
		temp->form = XmCreateForm (rowcolumn, "form", NULL, 0);
		if (temp->max > temp->min) {
			temp->display = True;
			XtManageChild (temp->form);
		} else
			temp->display = False;
		ac = 0;
		XtSetArg (al[ac], XmNtopAttachment, XmATTACH_POSITION); ac++;
		XtSetArg (al[ac], XmNtopPosition, 10); ac++;
		XtSetArg (al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
		XtSetArg (al[ac], XmNwidth, 140); ac++;
		XtSetArg (al[ac], XmNlabelString, XmStringCreateLtoR (temp->name, char_set)); ac++;
		temp->label = XmCreateLabel (temp->form, "label", al, ac);
		XtManageChild (temp->label);
		ac = 0;
		XtSetArg (al[ac], XmNleftAttachment, XmATTACH_WIDGET); ac++;
		XtSetArg (al[ac], XmNleftWidget, temp->label); ac++;
		temp->frame = XmCreateFrame (temp->form, "frame", al, ac);
		XtManageChild (temp->frame);
		if (temp->hex)
			sprintf (s, "%s%X", "0x", temp->current);
		else
			sprintf (s, "%d", temp->current);
		ac = 0;
		XtSetArg (al[ac], XmNcolumns, 12); ac++;
		XtSetArg (al[ac], XmNmaxLength, 10); ac++;
		XtSetArg (al[ac], XmNshadowThickness, 0); ac++;
		XtSetArg (al[ac], XmNmarginHeight, 0); ac++;
		XtSetArg (al[ac], XmNmarginWidth, 0); ac++;
		XtSetArg (al[ac], XmNvalue, s); ac++;
		temp->text = XmCreateText(temp->frame, "text", al, ac);
		XtManageChild (temp->text);
		XtAddCallback (temp->text, XmNfocusCallback, (XtCallbackProc) text_focus_CB, temp);
		XtAddCallback (temp->text, XmNlosingFocusCallback, (XtCallbackProc) text_losing_focus_CB, temp);
		XtAddCallback (temp->text, XmNvalueChangedCallback, (XtCallbackProc) text_value_changed_CB, temp);
		if (temp->autoconfig) {
			ac = 0;
			XtSetArg (al[ac], XmNleftAttachment, XmATTACH_WIDGET); ac++;
			XtSetArg (al[ac], XmNleftWidget, temp->frame); ac++;
			XtSetArg (al[ac], XmNlabelString, XmStringCreate (getStr (TXT_toggle_label), char_set)); ac++;
			temp->toggle = XmCreateToggleButton (temp->form, "toggle", al, ac);
			XtManageChild (temp->toggle);
			XtAddCallback (temp->toggle, XmNvalueChangedCallback, (XtCallbackProc) toggleCB, NULL);
			XtSetSensitive (temp->toggle, False);
		}
		temp = temp->next;
	}
	reApplyMnemInfoOnShell (mPtr, form);
}

void set_text (_category *c)
{
	last_parameter = NULL;
	XmTextSetString (text, get_description (c->name));
}

void scaleCB (Widget w, XtPointer client_data, XmAnyCallbackStruct *call_data)
{
	int i, ac;
	Arg al[20];
	char s[20];
	Boolean depressed = False;

	XmScaleGetValue (w, &i);
	if (i < 0)
		current_parameter->current = current_parameter->max;
	else
		current_parameter->current = i;
	if (current_parameter->hex)
		sprintf (s, "%s%X", "0x", current_parameter->current);
	else
		sprintf (s, "%d", current_parameter->current);
	XmTextSetString (current_parameter->text, s);
	current_parameter->modified = True;
	if (current_parameter->autoconfig) {
		ac = 0;
		XtSetArg (al[ac], XmNset, &depressed); ac++;
		XtGetValues (current_parameter->toggle, al, ac);
		if (depressed) {
			ac = 0;
			XtSetArg (al[ac], XmNset, False); ac++;
			XtSetValues (current_parameter->toggle, al, ac);
		}
	}
}

void menuCB (Widget w, char *client_data, XmAnyCallbackStruct *call_data)
{
	int ac;
	Arg al[20];
	_parameter *temp;
	_category *c_temp;
	Boolean found = False;

	watch_cursor (form);
	XtSetSensitive (last_item, True);
	ac = 0;
	XtSetArg (al[ac], XmNshadowThickness, 0); ac++;
	XtSetValues (last_parameter, al, ac);
	temp = last_list->head;
	XtUnmanageChild (rowcolumn);
	while (temp != NULL) {
		XtUnmanageChild (temp->form);
		temp = temp->next;
	}
	c_temp = c_list;
	while ((!found) && (c_temp != NULL)) {
		if (strcmp (client_data, c_temp->name) == 0)
			found = True;
		else
			c_temp = c_temp->next;
	}
	if (c_temp->head == NULL) {
		get_list (c_temp);
		temp = c_temp->head;
		while (temp != NULL) {
			get_para_value (temp);
			temp = temp->next;
		}
		item_selected (c_temp);
		XtManageChild (rowcolumn);
	} else {
		set_text (c_temp);
		temp = c_temp->head;
		while (temp != NULL) {
			if (temp->display)
				XtManageChild (temp->form);
			temp = temp->next;
		}
		XtManageChild (rowcolumn);
	}
	XtSetSensitive (w, False);
	last_item = w;
	last_list = c_temp;
	temp = c_temp->head;
	if (temp->max > temp->min) {
		ac = 0;
		XtSetArg (al[ac], XmNminimum, temp->min); ac++;
		XtSetArg (al[ac], XmNmaximum, temp->max); ac++;
		XtSetArg (al[ac], XmNvalue, temp->current); ac++;
		XtSetValues (scale, al, ac);
	}
	XtSetSensitive (scale, False);
	current_parameter = NULL;
	normal_cursor (form);
}

void dialog_CB (Widget w, int client_data, XmAnyCallbackStruct *call_data)
{
	struct link_handle l;
	_parameter *t1;
	_category *t2;
	char s[20];

	switch (client_data) {
		case 1:		// Discard changes? YES
			exit (0);
		case 2:		// Discard changes? NO
			mtune_modified = False;
			watch_cursor (form);
			t2 = c_list;
			while (t2 != NULL) {
				t1 = t2->head;
				while (t1 != NULL) {
					if (t1->modified && t1->current != t1->last) {
						mtune_modified = True;
						if (t1->hex)
							sprintf (s, "%s%X", "0x", t1->current);
						else
							sprintf (s, "%d", t1->current);
						setuid (0);
						link_open (&l, "/etc/conf/bin/idtune", "idtune", "-f", t1->name, s);
						setuid (uid);
						link_close (&l);
						t1->last = t1->current;
					}
					t1 = t1->next;
				}
				t2 = t2->next;
			}
			normal_cursor (form);
			XtUnmanageChild (w);
			if (mtune_modified)
			 	XtManageChild (q_dialog2);
			else
				exit(0);
			break;
		case 3:		// Discard changes?/Rebuild kernel now? CANCEL
			XtUnmanageChild (w);
			break;
		case 4:		// Rebuild kernel now? YES
			XtUnmanageChild (w);
			XtManageChild (m_dialog3);
			XFlush (XtDisplay (toplevel));
			XtAppAddTimeOut ((XtAppContext) context, 5000, (XtTimerCallbackProc) system_time, m_dialog3);
			break;
		case 5:		// Rebuild kernel now? NO
			XtUnmanageChild (w);
			XtManageChild (m_dialog1);
			break;
		case 6:		// Do you want to reboot system now? OK
			setuid (0);
			system ("/usr/sbin/init 6");
			setuid (uid);
			exit (0);
		case 7:		// Do you want to reboot system now? CANCEL
			XtUnmanageChild (w);
			XtManageChild (m_dialog2);
			break;
		case 8:		// Kernel will be rebuild in next reboot - OK
			setuid (0);
			system ("/etc/conf/bin/idbuild");
			setuid (uid);
			exit (0);
		case 9:		// New kernel installed in next reboot - OK
			exit (0);
		case 10:	// Error during kernel rebuilding - OK
			exit (1);
	}
}

void system_time (XtPointer client_data, XtIntervalId id)
{
	struct stat *file_buf;
	XtAppContext cxt = XtWidgetToApplicationContext (m_dialog3);

	if (XtWindow ((Widget) client_data)) {
		file_buf = (struct stat *) malloc (sizeof (struct stat));
 		watch_cursor(toplevel);
		setuid (0);
		system ("/etc/conf/bin/idbuild -B 2> /tmp/kernel_status &");
		setuid (uid);
		system("/usr/bin/ps -ef | /usr/bin/awk '/idbuild/ {print$0}' > /tmp/idbuildalive");
		// The temporary file is necessary for now.  If I can find some way to see if a process is
		// alive then I can use that.  I was doing a stat on /etc/conf/cf.d/unix for the size, but
		// if the build fails and the file is not created an endless loop would occur
		stat ("/tmp/idbuildalive", file_buf);
		while(file_buf->st_size!=0)
		{
			system("/usr/bin/ps -ef | /usr/bin/awk '!/awk/ && /idbuild/ {print$0}' > /tmp/idbuildalive");
			stat ("/tmp/idbuildalive", file_buf);
			sleep(5);
			XmUpdateDisplay(toplevel);         // Update the display during the kernel rebuild.
		}
		XtUnmanageChild (m_dialog3);
		stat ("/etc/conf/cf.d/unix", file_buf);
		if (file_buf->st_size > 0)
			XtManageChild (q_dialog3);
		else
			XtManageChild (e_dialog1);
		XFlush (XtDisplay (toplevel));
		system("/usr/bin/rm /tmp/idbuildalive");
		setuid (0);
		stat("/tmp/kernel_status",file_buf);
		if(file_buf->st_size==0)
			system("/usr/bin/rm /tmp/kernel_status");
		setuid (uid);
		free(file_buf);
		normal_cursor(toplevel);
	} else
		XtAppAddTimeOut ((XtAppContext) context, 5000, (XtTimerCallbackProc) system_time, client_data);
}

void buttonCB (Widget w, int client_data, XmAnyCallbackStruct *call_data)
{
	int ac;
	Arg al[20];
	_parameter *temp;
	_category *temp2;
	char s[20];
	struct link_handle l;

	switch (client_data) {
		case OK:
			mtune_modified = FALSE;
			watch_cursor (form);
			if (!dialogs_created)
				create_dialogs ();
			temp2 = c_list;
			while (temp2 != NULL) {
				temp = temp2->head;
				while (temp != NULL) {
					if (temp->modified && (temp->current != temp->last)){
						mtune_modified = True;
						if (temp->hex)
							sprintf (s, "%s%X", "0x", temp->current);
						else
							sprintf (s, "%d", temp->current);
						setuid (0);
						link_open (&l, "/etc/conf/bin/idtune", "idtune", "-f", temp->name, s);
						setuid (uid);
						link_close (&l);
						temp->last = temp->current;
					}
					temp = temp->next;
				}
				temp2 = temp2->next;
			}
			normal_cursor (form);
			if (mtune_modified)
				XtManageChild (q_dialog2);
			else
				exit(0);
			break;
		case RESET:
			XmScaleSetValue (scale, current_parameter->last);
			temp2 = c_list;
			while (temp2 != NULL) {
				//temp = last_list->head;
				temp = temp2->head;
				while (temp != NULL) {
					if ((temp->modified) && (temp->display)) {
						temp->current = temp->last;
						if (temp->hex)
							sprintf (s, "%s%X", "0x", temp->current);
						else
							sprintf (s, "%d", temp->current);
						XmTextSetString (temp->text, s);
						if (temp->autoconfig) {
							ac = 0;
							XtSetArg (al[ac], XmNset, False); ac++;
							XtSetValues (temp->toggle, al, ac);
						}
					}
					temp = temp->next;
				}
				temp2 = temp2->next;
			}
			break;
		case RESET_FACTORY:
			XmScaleSetValue (scale, current_parameter->last);
			temp2 = c_list;
			while (temp2 != NULL) {
				//temp = last_list->head;
				temp = temp2->head;
				while (temp != NULL) {
					if (temp->display) {
						temp->modified = True;
						temp->current = temp->def_value;
						if (temp->hex)
							sprintf (s, "%s%X", "0x", temp->current);
						else
							sprintf (s, "%d", temp->current);
						XmTextSetString (temp->text, s);
						if (temp->autoconfig) {
							ac = 0;
							XtSetArg (al[ac], XmNset, False); ac++;
							XtSetValues (temp->toggle, al, ac);
						}
					}
					temp = temp->next;
				}
				temp2 = temp2->next;
			}
			break;
		case CANCEL:
			mtune_modified = FALSE;
			watch_cursor (form);
			if (!dialogs_created)
				create_dialogs ();
			temp2 = c_list;
			while (temp2 != NULL) {
				temp = temp2->head;
				while (temp != NULL) {
					if (temp->modified && (temp->current != temp->last))
						mtune_modified = True;
					temp = temp->next;
				}
				temp2 = temp2->next;
			}
			normal_cursor (form);
			if (mtune_modified){
				XtManageChild (q_dialog1);
				XtManageChild(no_button);
			}
			else
				exit (0);
			break;
		case HELP:
			displayHelp (toplevel, &systunerHelp);
			break;
	}
}
