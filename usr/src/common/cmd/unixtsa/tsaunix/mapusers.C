#ident	"@(#)unixtsa:common/cmd/unixtsa/tsaunix/mapusers.C	1.2"

#include <sys/types.h>
#include <smstypes.h>
#include <pwd.h>

/*
 * GetUIDbyNWname - map the given user name(may be netware) to UNIX uid.
 *
 * synopsis : int GetUIDByNWname(char *UserName)
 *
 * Description :
 *
 * Return : UNIX uid on success
 *          -1 on failure
 */
int GetUIDByNWname(char *UserName)
{
	/*
	struct passwd *pwent ;

	if ((pwent = getpwnam(UserName)) != NULL) {
		return((int)pwent->pw_uid);
	}
	*/
	return(-1);
}

/*
 * GetUIDbyNWuid - map the given foreign uid(may be netware) to UNIX uid.
 *
 * synopsis : int GetUIDByNWuid(UINT32 OwnerID)
 *
 * Description :
 *
 * Return : UNIX uid on success
 *          -1 on failure
 */
int GetUIDByNWuid(UINT32 OwnerID)
{
	/*
	struct passwd *pwent ;

	if ((pwent = getpwuid(OwnerID)) != NULL) {
		return((int)pwent->pw_uid);
	}
	*/
	return(-1);
}

/*
 * GetNWNameByuid - map the UNIX uid to user name(may be netware).
 *
 * synopsis : char *GetNWNameByuid(int uid)
 *
 * Description :
 *
 * Return : pointer to the static buffer containing the name.
 *          NULL on failure
 */
char *GetNWNameByuid(int uid)
{
	/*
	uid_t userID = uid ;
	struct passwd *pwent ;

	if ((pwent = getpwuid(userID)) != NULL) {
		return(pwent->pw_name);
	}
	*/
	return(NULL);
}
