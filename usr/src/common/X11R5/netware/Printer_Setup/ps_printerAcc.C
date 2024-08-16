#ident	"@(#)prtsetup2:ps_printerAcc.C	1.9"
//--------------------------------------------------------------
// ps_printerAcc.c
//--------------------------------------------------------------

#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <limits.h>
#include <errno.h>
#include <Xm/Xm.h>

#include "ps_hdr.h"
#include "ps_printer.h"

extern "C" {
#include "lpsys.h"
}

#ifndef UID_MAX
#define UID_MAX 60002
#endif

#ifndef LOWEST_USER_UID
#define	LOWEST_USER_UID	100
#endif

static char*					s_allList[] = {"all!all", 0};

extern "C" int					load_userprinter_access (char*,
														 char***,
														 char***);
extern "C" int					deny_user_printer (char**, char*);
extern "C" int					allow_user_printer (char**, char*);

/*----------------------------------------------------------------------------
 *	Returns the first entry in the user array
 */
PrintUser*
PSPrinter::GetFirstUser ()
{
	if (d_userArray.cnt <= 0) {
		return (0);
	}
	d_userArray.cur = 1;
	return (d_userArray.pUsers[0]);
}

/*----------------------------------------------------------------------------
 *
 *	Returns the next entry in the user array
 */
PrintUser*
PSPrinter::GetNextUser ()
{
	if (d_userArray.cur >= d_userArray.cnt) {
		return (0);
	}
	d_userArray.cur++;
	return (d_userArray.pUsers[d_userArray.cur - 1]);
}

/*----------------------------------------------------------------------------
 *	Returns the first entry in the sys array
 */
PrintUser*
PSPrinter::GetFirstSys ()
{
	if (d_sysArray.cnt <= 0) {
		return (0);
	}
	d_sysArray.cur = 1;
	return (d_sysArray.pUsers[0]);
}

/*----------------------------------------------------------------------------
 *
 */
PrintUser*
PSPrinter::GetNextSys ()
{
	if (d_sysArray.cur >= d_sysArray.cnt) {
		return (0);
	}
	d_sysArray.cur++;
	return (d_sysArray.pUsers[d_sysArray.cur - 1]);
}

//--------------------------------------------------------------
// This function gets the access lists for a printer
//				and updates the internal PSPrinter structures.
//
// True if a user list could be constructed.
//			False if user list could not be constructed.
//--------------------------------------------------------------
Boolean
PSPrinter::LoadAccessLists ()
{
	struct passwd*				pwd;
	struct stat					pwStat;
	char**						allowList;
	char**						denyList;
	int							retCode;

	_resetAllFlg = False;
	_currentAllFlg = False;

	//---------------------------------------------------------------------
	//  First, get a list of users on the system. Throw away nobody, noaccess,
	// 	etc.
	//---------------------------------------------------------------------
	while (((retCode = stat ("/etc/passwd", &pwStat)) != 0) && errno == EINTR)
		; 														// try again
	if (retCode != 0) {
		return (False);
	}

	FreePrintUserArray (&d_userArray);					// ?????
	FreePrintUserArray (&d_sysArray);					// ?????
	FreePrintUserArray (&d_otherArray);					// ?????

	while (pwd = getpwent ()) {
		if (pwd->pw_uid > LOWEST_USER_UID && pwd->pw_uid < UID_MAX - 2) {
			AddPrintUser (pwd->pw_name, &d_userArray);
		}
	}
	endpwent ();

	//---------------------------------------------------------------------
	// Now read the user access file (deny or allow). 
	// Handle the special cases all!all and all
	// Otherwise, if the entry is a user on the local system then add an 
	// entry to the d_userArray, with the access permission set. 
	// Otherwise, if the entry is not a user then it should be in the 
	// form system_name!all. Add it to the _denySystem or the _remoteSystem
	// lists. 
	// Ignore everything else. Who knows what it is.
	//--------------------------------------------------------------------- 
	load_userprinter_access (GetLabel (), &allowList, &denyList);
	InitArray (S_ALLOW, &d_userArray);

	while (*denyList) {
		if (strcoll (*denyList, "all!all") == 0) {
			InitArray (S_DENY, &d_userArray);
		}
		else {
			if (strcoll (*denyList, "all") == 0) {
				InitArray (S_DENY, &d_userArray);
			}
			else {
				for (int i = 0; i < d_userArray.cnt; i++) { 	// Is it a user	
					if (!strcmp (d_userArray.pUsers[i]->Name (), *denyList)) {
						InitUser (S_DENY, *denyList);
						break;
					}
				}
				if (i == d_userArray.cnt) {
					AddPrintUser (*denyList, &d_sysArray);
					(d_sysArray.pUsers[d_sysArray.cnt - 1])->InitState (S_DENY);
				}
			}
		}
		denyList++;
	}

	while (*allowList) {
		if (strcoll (*allowList, "all!all") == 0) {
			InitArray (S_ALLOW, &d_userArray);
		}
		else {
			if (strcoll (*allowList, "all") == 0) {
				InitArray (S_ALLOW, &d_userArray);
			}
			else {
				for (int i = 0; i < d_userArray.cnt; i++) { 	// Is it a user	
					if (!strcmp (d_userArray.pUsers[i]->Name (), *allowList)) {
						InitUser (S_ALLOW, *allowList);
						break;
					}
				}
				if (i == d_userArray.cnt) {
					AddPrintUser (*allowList, &d_sysArray);
					(d_sysArray.pUsers[d_sysArray.cnt - 1])->InitState (S_ALLOW);
				}
			}
		}
		allowList++;
	}

	return (TRUE);
}

/*----------------------------------------------------------------------------
 *	Add a user (or system) to a PrintUserArray
 */
PrintUser*
PSPrinter::AddPrintUser (char* name, PrintUserArray* array)
{
	if (array->cnt >= array->allocated) {
		array->allocated += 15;
		array->pUsers = (PrintUser**)XtRealloc ((char*)array->pUsers,
												array->allocated *
												sizeof (PrintUser*));
	}

	array->pUsers[array->cnt] = new PrintUser (name);
	array->cnt++;
	return (array->pUsers[array->cnt - 1]);
}

/*----------------------------------------------------------------------------
 *	Delete a user (or system) from a PrintUserArray
 */
void
PSPrinter::DelPrintUser (char* name, PrintUserArray* array)
{
	for (int i = 0; i < array->cnt; i++) {
		if (!(strcmp (name, array->pUsers[i]->Name ()))) {
			delete (array->pUsers[i]);
			for (int j = i; j < array->cnt - 1; j++) {
				array->pUsers[j] = array->pUsers[j + 1];
			}
			array->cnt--;
			if (array->cur > i) {
				array->cur--;
			}
//			array->pUsers[j]->InitState (S_EMPTY_STATE); // Clear out the state
			break;	
		}
	}
}

/*----------------------------------------------------------------------------
 *	Delete a system from a d_sysArray.
 */
void
PSPrinter::DelSys (char* name)
{
	DelPrintUser (name, &d_sysArray);
}

/*----------------------------------------------------------------------------
 *
 */
void
PSPrinter::FreePrintUserArray (PrintUserArray* array)
{
	for (int i = 0; i < array->cnt; i++) {
		delete (array->pUsers[i]);
	}
	XtFree ((char*)array->pUsers);
	array->cnt = 0;
	array->allocated = 0;
}

//--------------------------------------------------------------
// This function initializes an array to the value passed in.
//--------------------------------------------------------------
void
PSPrinter::InitArray (AllowState state, PrintUserArray* array)
{
	for (int i = 0; i < array->cnt; i++) {
		array->pUsers[i]->InitState (state);
	}
}

//--------------------------------------------------------------
// This function determines what array a user is in.
//				Adds the PrintUser if necc. and initializes its value
//--------------------------------------------------------------
void
PSPrinter::InitUser (AllowState state, char* name)
{
	char*						tmp;

											// THIS DOESN'T WORK !!!!!!!!!!!!!!
	if (strstr (name, "!all") != NULL) {	// Is it a system	//?????????????
		tmp = strchr (name, '!');    // Strip !all
		*tmp = '\0';
		AddPrintUser (name, &d_sysArray);
		(d_sysArray.pUsers[d_sysArray.cnt - 1])->InitState (state);
		return;
	}
	else {
		for (int i = 0; i < d_userArray.cnt; i++) { 	// Is it a user	
			if (!strcmp (d_userArray.pUsers[i]->Name (), name)) {
				(d_userArray.pUsers[i])->InitState (state);
				return;
			}
		}
		// If we get to here this is just something we don't know about.
		// Let's just make an entry for it and write it back out. However,
		// if the user changes the _resetState it might get lost. Oh well

		AddPrintUser (name, &d_otherArray);
		(d_otherArray.pUsers[d_otherArray.cnt - 1])->InitState (state);
	}
}

//--------------------------------------------------------------
// True if update was successful.  False otherwise
//--------------------------------------------------------------
Boolean
PSPrinter::UpdateSystemAccessLists ()
{
	char						msg [MSGMAX];
	char**						list;

	deny_user_printer (s_allList, _updatePrinter.name);	
	allow_user_printer (s_allList, _updatePrinter.name);

	if (list = BuildUserList (0, &d_sysArray, S_DENY)) {
		deny_user_printer (list, _updatePrinter.name);	
	}
	if (list = BuildUserList (0, &d_sysArray, S_ALLOW)) {
		allow_user_printer (list, _updatePrinter.name);
	}

	/* Open a connection to the spooler.  If the open fails, assume the
	 * spooler is not running.
	 */
	if (mopen () == 0) {
		/* Spooler running.  Tell it about the changes. */
		(void)putmessage (msg, S_LOAD_PRINTER, _updatePrinter.name);

		/* Send the message and get the response.  Neither of these
		 * should fail, but if disaster strikes, just ignore the error.
		 * Also assume that the operation works correctly.
		 */
		if (msend (msg) == 0) {
			mrecv (msg, MSGMAX);
		}

		mclose ();
	}

	if (list = BuildUserList (&d_userArray, 0, S_DENY)) {
		deny_user_printer (list, _updatePrinter.name);	
	}

	if (list = BuildUserList (&d_userArray, 0, S_ALLOW)) {
		allow_user_printer (list, _updatePrinter.name);
	}

	/* Open a connection to the spooler.  If the open fails, assume the
	 * spooler is not running.
	 */
	if (mopen () == 0) {
		/* Spooler running.  Tell it about the changes. */
		(void)putmessage (msg, S_LOAD_PRINTER, _updatePrinter.name);

		/* Send the message and get the response.  Neither of these
		 * should fail, but if disaster strikes, just ignore the error.
		 * Also assume that the operation works correctly.
		 */
		if (msend (msg) == 0) {
			mrecv (msg, MSGMAX);
		}

		mclose ();
	}

	return (TRUE);
}

Boolean
PSPrinter::UpdateUserAccessLists ()
{
	return (UpdateSystemAccessLists ());
}

//--------------------------------------------------------------
// This function updates the _resetState based on 
//				the values in the _denyState and _allowState for
//				all the users in the d_userArray.
//--------------------------------------------------------------
void
PSPrinter::UpdateUserArray ()
{
	for (int i = 0; i < d_userArray.cnt; i++) {
		d_userArray.pUsers[i]->UpdateResetState (S_NO_STATE);
	}
}

//--------------------------------------------------------------
// This function updates the _resetState based on 
//				the values in the _denyState and _allowState for
//				all the users in the d_sysArray.
//--------------------------------------------------------------
void
PSPrinter::UpdateSysArray ()
{
	for (int i = 0; i < d_sysArray.cnt; i++) {
		d_sysArray.pUsers[i]->UpdateResetState (S_NO_STATE);
	}
}

//--------------------------------------------------------------
// Checks to see if all the PrintUsers in the array
//				are allowed, denied, or both.
//
// S_ALLOW - All PrintUsers are allowed access
//			S_DENY - All PrintUsers are denied access
//			S_NO_STATE - PrintUsers have mixed access.
//			S_MIXED - PrintUsers have mixed access.
//--------------------------------------------------------------
AllowState
PSPrinter::CheckArray (PrintUserArray* array)
{
	AllowState		state;
	short			i;

	if (array->cnt > 0) {
		if ((state = array->pUsers[0]->ResetState()) == S_NO_STATE) {
//			state = S_NO_STATE;
			for (i = 0; i < array->cnt; i++) {
				if((state = array->pUsers[i]->ResetState ()) != S_NO_STATE) {
					state = S_MIXED;
					break;
				}
			}
		}
	
		switch (state) {
		case S_DENY:
			for (i = 0; i < array->cnt; i++) {
				state = array->pUsers[i]->ResetState();
				if (state == S_ALLOW) {
					state = S_MIXED;		
					break;
				}
			}
			break;

		case S_ALLOW:
			for (i = 0; i < array->cnt; i++) {
				state = array->pUsers[i]->ResetState();
				if (state == S_DENY) {
					state = S_MIXED;		
					break;
				}
			}
			break;

		case S_NO_STATE:
		default:
			break;
		}
	}
	else {
		state = S_NO_STATE;
	}
	return (state);
}

//--------------------------------------------------------------
// This function builds an array of character pointers
//				that is terminated with a NULL. This array is 
//				suitable to be passed into allow_user_printer or
//				deny_user_printer.
//--------------------------------------------------------------
char**
PSPrinter::BuildUserList (PrintUserArray* user,
						  PrintUserArray* systems,
						  AllowState state)
{
	short 						i;
	short						cnt = 0;
	int							userCount = 0;
	Boolean						all = FALSE;
	char**						list = NULL;
	char*						tmp;

	// Get count
	if (user != NULL) {
		for (i = 0; i < user->cnt; i++) {
			if (user->pUsers[i]->ResetState() == state) {
				cnt++;
			}
			userCount++;
		}
		if (userCount != 0 && userCount == cnt) {
			all = TRUE;
		}
	}
	if (systems != NULL) {
		for (i = 0; i < systems->cnt; i++) {
			if (systems->pUsers[i]->ResetState() == state) {
				cnt++;
			}
		}
	}
	if (cnt == 0) {
		return (0);
	}

	if (cnt != 0) {
		cnt++;								// Add one for the NULL
	}

	if (cnt)	 {
		list = (char**)XtMalloc (sizeof (char*) * cnt);
	}
	if (list != NULL) {
		memset (list, 0, sizeof (char*) * cnt);
	}
	cnt = 0;	
	if (user != NULL) {
		if (all) {
			list[cnt] = "all";
			cnt++;
		}
		else {
			for (i = 0; i < user->cnt; i++) {
				if (user->pUsers[i]->ResetState () == state) {
					list[cnt] = user->pUsers[i]->Name ();				
					cnt++;
				}
			}
		}
	}
	if (systems != NULL) {
		for (i = 0; i < systems->cnt; i++) {
			if (systems->pUsers[i]->ResetState () == state) {
				tmp = XtMalloc (strlen (systems->pUsers[i]->Name ()) + 2);
				tmp[0] = '\0';
				strcat (tmp, systems->pUsers[i]->Name ());
				list[cnt] = tmp;				
				cnt++;
			}
		}
	}

	return (list);
}

//--------------------------------------------------------------
// Changes the AllowState from the current state
//				to the state passed in. For example, if the state
//				passed in is S_DENY, then the user will be added to 
//				the S_DENY list and removed from the S_ALLOW list.
//--------------------------------------------------------------
Boolean
PSPrinter::ChangeUserAllowState (char* name, AllowState state)
{
	for (int i = 0; i < d_userArray.cnt; i++) {
		if (!strcmp (d_userArray.pUsers[i]->Name(), name)) {
			d_userArray.pUsers[i]->ChangeState (state);
			return (True);
		}
	}
	return (False);
}

//--------------------------------------------------------------
// Changes the AllowState from the current state
//				to the state passed in. For example, if the state
//				passed in is S_DENY, then the user will be added to 
//				the S_DENY list. However, unlike ChangeUserAllowState
//				above the status of the S_ALLOW list is not changed.
//--------------------------------------------------------------
void
PSPrinter::AddSysAllowState (char* name, AllowState state)
{
	PrintUser*					sys;

	for (int i = 0; i <  d_sysArray.cnt; i++) {
		if (!strcmp (d_sysArray.pUsers[i]->Name(), name)) {
			d_sysArray.pUsers[i]->AddState (state);
			return;
		}
	}
	sys = AddPrintUser (name, &d_sysArray);
	sys->InitState (S_NO_STATE);
	sys->ChangeState (state);
}

Boolean
PSPrinter::ChangeSysAllowState (char* name, AllowState state)
{
	for (int i = 0; i < d_sysArray.cnt; i++) {
		if (!strcmp (d_sysArray.pUsers[i]->Name(), name)) {
			d_sysArray.pUsers[i]->ChangeState (state);
			return (True);
		}
	}
	return (False);
}

Boolean
PSPrinter::ClearSysAllowState (char* name)
{
	for (int i = 0; i < d_sysArray.cnt; i++) {
		if (!strcmp (d_sysArray.pUsers[i]->Name(), name)) {
			d_sysArray.pUsers[i]->AddState (S_NO_STATE);
			return (True);
		}
	}
	return (False);
}

//--------------------------------------------------------------
// Function to load NAME_NONE, NAME_ALL, or 
//				ALL_BANG_ALL into an array of character pointers.
//--------------------------------------------------------------
char**
PSPrinter::LoadSpecialList (char* str)
{
	static char**				list;
	
	list = (char**)XtMalloc (sizeof (char*) * 2);
	memset (list, 0, sizeof (char*) * 2);
	list[0] = strdup (str);
	return (list);
}

//--------------------------------------------------------------
// Resets the allowStates for all users in the 
//--------------------------------------------------------------
void
PSPrinter::ResetUserList ()
{
	ResetList (&d_userArray);
}

//--------------------------------------------------------------
//	Resets the allowStates for all users in the 
//--------------------------------------------------------------
void
PSPrinter::ResetList (PrintUserArray* array)
{
	for (int i = 0; i < array->cnt; i++) {
		array->pUsers[i]->ChangeState (array->pUsers[i]->ResetState ());
	}
}

