#ident	"@(#)systuner:l_func.C	1.7"
// l_func.c

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

void get_categories ()
{
	char s[100];
	char *y = NULL;
	_category *temp = NULL;

	param_file = fopen (PARAM_FILE, "r");
	if (param_file == NULL) {
		sprintf (s, "%s %s%s%s", DTMSG, "\"", getStr (TXT_fopen_error1), "\"");
		system (s);
		exit (1);
	}
	while (fgets (s, 100, param_file) != NULL) {
		s[strlen (s) - 1] = '\0';
		y = &s[3];
		if (strncmp (s, "%% ", 3) == 0) {
			if (temp != NULL) {
				temp->next = (_category *) malloc (sizeof (_category));
				temp = temp->next;
			} else
				temp = (_category *) malloc (sizeof (_category));
			temp->next = NULL;
			if (c_list == NULL)
				c_list = temp;
			temp->name = (char *) malloc (strlen (y) + 1);
			strcpy (temp->name, y);
			temp->head = NULL;
		}
	}
	fclose (param_file);
}

int skip_space (char s[], int i)
{
	while ((s[i] != '\t') && (s[i] != NULL))
		i++;
	while (s[i] == '\t')
		i++;
	return (i);
}

void get_para_value (_parameter *parameter)
{
	struct link_handle l;
	char s[100];
	int err, i = 0;

	err = setuid (0);
	if (err == 0) {
		link_open (&l, "/etc/conf/bin/idtune", "idtune", "-g", parameter->name, NULL);
		setuid (uid);
		link_read (&l, s);
		link_close (&l);
		i = 0;
		if (strcmp (s, NULL) != 0) {
			if (strncmp (&s[i], "0x", 2) == 0)
				parameter->hex = True;
			parameter->current = (int) strtol (&s[i], (char **) NULL, 0);
			parameter->last = parameter->current;
			i = skip_space (s, i);
			parameter->def_value = (int) strtol (&s[i], (char **) NULL, 0);
			i = skip_space (s, i);
			parameter->min = (int) strtol (&s[i], (char **) NULL, 0);
			i = skip_space (s, i);
			parameter->max = (int) strtol (&s[i], (char **) NULL, 0);
		} else
			parameter->min = parameter->max = 0;
	} else {
		sprintf (s, "%s %s%s%s", DTMSG, "\"", getStr (TXT_setuid_error), "\"");
		system (s);
		exit (1);
	}
}

Boolean check_autoconfig (char *s)
{
//These are the autotunables appearing in /etc/conf/cf.d/mtune_p | mtune_d
	if (strcmp (s, "NINODE") == 0)
		return (True);
	else if (strcmp (s, "SFSNINODE") == 0)
		return (True);
	else if (strcmp (s, "VXFSNINODE") == 0)
		return (True);
	else if (strcmp (s, "BUFHWM") == 0)
		return (True);
	else if (strcmp (s, "DNLCSIZE") == 0)
		return (True);
	else if (strcmp (s, "SEMMNI") == 0)
		return (True);
	else if (strcmp (s, "MSGMNI") == 0)
		return (True);
	else if (strcmp (s, "MSGTQL") == 0)
		return (True);
	else
		return (False);
}

void get_list (_category *c)
{
	char s[100];
	char *y = NULL;
	_parameter *temp = NULL;
	Boolean found = False;

	param_file = fopen (PARAM_FILE, "r");
	if (param_file == NULL) {
		sprintf (s, "%s %s%s%s", DTMSG, "\"", getStr (TXT_fopen_error1), "\"");
		system (s);
		exit (1);
	}
	while ((!found) && (fgets (s, 100, param_file) != NULL)) {
		s[strlen (s) - 1] = '\0';
		y = &s[3];
		if (strncmp (s, "%% ", 3) == 0) {
			if (strcmp (c->name, y) == 0) {
				found = True;
				while ((fgets (s, 100, param_file) != NULL) && (strncmp (s, "%% ", 3) != 0)) {
					if (strncmp (s, "%%%", 3) != 0) {
						s[strlen (s) - 1] = '\0';
						if (temp == NULL) {
							temp = (_parameter *) malloc (sizeof (_parameter));
							c->head = temp;
						} else {
							temp->next = (_parameter *) malloc (sizeof (_parameter));
							temp = temp->next;
						}
						temp->name = (char *) malloc (strlen (s) + 1);
						strcpy (temp->name, s);
						temp->modified = False;
						temp->hex = False;
						temp->autoconfig = check_autoconfig (temp->name);
					}
				}
				temp->next = NULL;
			}
		}
	}
	fclose (param_file);
}

char *format (char s[])
{
	int i, j, k, count;

	i = 0;
	while (s[i] != '\0') {
		if (s[i] == '\t')
			s[i] = ' ';
		i++;
	}
	i = 0;
	while (s[i] != '\0') {
		if (s[i] == ' ') {
			count = 0;
			j = i + 1;
			while (s[j] == ' ') {
				count++;
				j++;
			}
			if (count > 0) {
				j = i + 1;
				k = i + count;
				while (s[k] != '\0') {
					s[j] = s[k];
					j++;
					k++;
				}
				s[j] = '\0';
			}
		}
		i++;
	}
	return (s);
}

char *get_description (char *name)
{
	char s[100], str[DESC_SIZE], *y = NULL;
	int count = 0;
	Boolean found = False;

	desc_file = fopen (DESC_FILE, "r");
	if (desc_file == NULL) {
		sprintf (s, "%s %s%s%s", DTMSG, "\"", getStr (TXT_fopen_error2), "\"");
		system (s);
		exit (1);
	}
	while ((!found) && (fgets (s, 100, desc_file) != NULL)) {
		s[strlen (s) - 1] = '\0';
		y = &s[2];
		if (strncmp (s, "% ", 2) == 0) {
			if (strcmp (name, y) == 0) {
				found = True;
				while ((fgets (s, 100, desc_file) != NULL) && (strncmp (s, "% ", 2) != 0)) {
					s[strlen (s) - 1] = '\0';
					if (count == 0)
						strcpy (str, s);
					else {
						strcat (str, " ");
						strcat (str, s);
					}
					count++;
				}
			}
		}
	}
	fclose (desc_file);
	if (found)
		return (format (str));
	else
		return ((char *) getStr (TXT_no_description));
}
