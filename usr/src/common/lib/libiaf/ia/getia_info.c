/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)libiaf:common/lib/libiaf/ia/getia_info.c	1.4.1.8"
#ident  "$Header: getia_info.c 1.2 91/06/25 $"

#include <sys/types.h>
#include <ia.h>
#include <search.h>
#include <fcntl.h>
#include <memory.h>
#include <sys/mman.h>
#include <sys/stat.h>
#ifdef NS
#include <pwd.h>
#include <grp.h>
#include <audit.h>
#include <mac.h>
#include <unistd.h>
#include <errno.h>
#include <dlfcn.h>

static gid_t *groups;

#define NS_GETNISTENT(p)\
{\
	static void *dl;\
	static struct passwd *(*nsptr)();\
	if (dl == NULL){ \
		if ((dl = dlopen("/usr/lib/ns.so", RTLD_LAZY)) == NULL)\
			dl = &dl;\
	}\
	if (dl == &dl)\
		return(-1);\
	if (nsptr == NULL) \
		nsptr = (struct passwd *(*)())dlsym(dl, "nis_getpwentry");\
	if (nsptr == NULL || (p = (*nsptr)(namep)) == NULL)\
		return(-1);\
}
#endif

#define nisname(n) (*n == '+' || *n == '-')

extern	char	*strcpy();
extern	int	strcmp();
extern	void	free();
/*
 * ia_openinfo - passed a pointer to logname and  a uinfo_t pointer.
 *               The routine searchs the index file for logname`s entry, 
 *		 using the length from the entry the routine allocates space
 *               for the uinfo pointer and reads logname's entry from the 
 *		 master file into the space pointer to by uinfo. A 0 is 
 *	  	 returned on success, and -1 if an error occurred.
 *		 If logname is not found uinfo is set to NULL and 0 is returned.
 */
#ifdef NS
ia_openinfo(namep, uinfo)
char	*namep;
uinfo_t	*uinfo;
{
	actl_t actl;
	level_t proc_lid = 0;

	/*
	 * See if namep is in IAF database
	 */
	if (_ia_openinfo(namep, uinfo) == 0)
		return(0);
#ifdef notyet
	/*
	 * If auditing is enabled or the MAC is installed
	 * NIS users are not allowed
	 */
	if (auditctl(ASTATUS, &actl, sizeof(actl_t)) == 0 || errno != ENOPKG)
		return(-1);

	if (lvlproc(MAC_GET, &proc_lid) == 0 || errno != ENOPKG)
		return(-1);
#endif
	/* 
	 * Now check NIS database
	 */
	return(nis_openinfo(namep, uinfo));
}
#endif

#ifdef NS
static int
_ia_openinfo(namep, uinfo)
#else
int
ia_openinfo(namep, uinfo)
#endif
char	*namep;
uinfo_t	*uinfo;
{
	int	fd_indx,fd_mast;
	int	cnt;
	struct  stat    stbuf;
	struct	index	*indx;
	struct	index	*midxp;
	struct	index	index;
	struct	index	*indxp = &index;

	if (uinfo == NULL)
		return(-1);

	*uinfo = NULL;

	/*
	 * Don't look for NIS names in master file
	 */
	if (nisname(namep))
		return(-1);

	(void) strcpy(indxp->name, namep ? namep : "");
	if ((stat(INDEX, &stbuf)) != 0)
		return(-1);

	cnt = (stbuf.st_size/sizeof(struct index));

	if ((fd_indx = open(INDEX,O_RDONLY)) < 0)
		return(-1);

	if ((midxp = (struct index *)mmap(0, stbuf.st_size, PROT_READ, 
		 MAP_SHARED, fd_indx, 0)) < (struct index *) 0) {

			(void) close(fd_indx);
			return(-1);
	}

	indx = bsearch(indxp->name, midxp, cnt, sizeof(struct index), strcmp);

	if (indx == NULL) {
		(void) munmap((caddr_t)midxp, stbuf.st_size);
		(void) close(fd_indx);
		return(-1);
	}
	
	(void) memcpy(indxp, indx, sizeof(struct index));

	(void) munmap((caddr_t)midxp, stbuf.st_size);
	(void) close(fd_indx);
	

	if ((fd_mast = open(MASTER,O_RDONLY)) == -1)
		return(-1);

	if (lseek(fd_mast, indxp->offset, 0) == -1) {
		(void) close(fd_mast);
		return(-1);
	}

	*uinfo = (struct master *) malloc(indxp->length);

	if (*uinfo == NULL) {
		(void) close(fd_mast);
		return(-1);
	}

	if ((read(fd_mast, *uinfo, indxp->length)) != indxp->length) {
		(void) close(fd_mast);
		free(*uinfo);
		*uinfo = NULL;
		return(-1);
	}


	(*uinfo)->ia_lvlp = (level_t *) (((char *) *uinfo) + sizeof(struct master));
	(*uinfo)->ia_sgidp = (gid_t *) (((char *) *uinfo) + sizeof(struct master) 
				+ ((*uinfo)->ia_lvlcnt * sizeof(level_t)) );
	(*uinfo)->ia_dirp = ( ((char *) *uinfo) + sizeof(struct master) 
				+ ((*uinfo)->ia_lvlcnt * sizeof(level_t)) 
				+ ((*uinfo)->ia_sgidcnt * sizeof(gid_t)) );
	(*uinfo)->ia_shellp = ((*uinfo)->ia_dirp + (*uinfo)->ia_dirsz +1);

	(void) close(fd_mast);
	return(0);
}

void
ia_closeinfo(uinfo)
uinfo_t uinfo;
{
	free(uinfo);
}

void
ia_get_uid(uinfo, uid)
uinfo_t uinfo;
uid_t	*uid;
{

	*uid = uinfo->ia_uid;
}

void
ia_get_gid(uinfo, gid)
uinfo_t uinfo;
gid_t	*gid;
{

	*gid = uinfo->ia_gid;
}

int
ia_get_sgid(uinfo, sgid, cnt)
uinfo_t	uinfo;
gid_t	**sgid;
long	*cnt;
{
	*cnt = uinfo->ia_sgidcnt;

	if (uinfo->ia_sgidcnt) {
		*sgid = uinfo->ia_sgidp;
		return(0);
	}
	else {
		*sgid = (gid_t *)NULL;
		return(-1);
	}
}

int
ia_get_lvl(uinfo, lvl, cnt)
uinfo_t	uinfo;
level_t	**lvl;
long	*cnt;
{
	*cnt = uinfo->ia_lvlcnt;

	if (uinfo->ia_lvlcnt) {
		*lvl = uinfo->ia_lvlp;
		return(0);
	}
	else {
		*lvl = (level_t *)NULL;
		return(-1);
	}
}

void
ia_get_mask(uinfo, mask)
uinfo_t	uinfo;
adtemask_t mask;
{
	register i;

	for (i=0; i< ADT_EMASKSIZE; i++)
		mask[i] = uinfo->ia_amask[i];
}

void
ia_get_dir(uinfo, dir)
uinfo_t	uinfo;
char	**dir;
{
	*dir = uinfo->ia_dirp;
}

void
ia_get_sh(uinfo, shell)
uinfo_t	uinfo;
char	**shell;
{
	*shell = uinfo->ia_shellp;
}

void
ia_get_logpwd(uinfo, passwd)
uinfo_t	uinfo;
char	**passwd;
{
	*passwd = uinfo->ia_pwdp;
}

void
ia_get_logchg(uinfo, logchg)
uinfo_t	uinfo;
long	*logchg;
{
	*logchg = uinfo->ia_lstchg;
}

void
ia_get_logmin(uinfo, logmin)
uinfo_t	uinfo;
long	*logmin;
{
	*logmin = uinfo->ia_min;
}

void
ia_get_logmax(uinfo, logmax)
uinfo_t	uinfo;
long	*logmax;
{
	*logmax = uinfo->ia_max;
}


void
ia_get_logwarn(uinfo, logwarn)
uinfo_t	uinfo;
long	*logwarn;
{
	*logwarn = uinfo->ia_warn;
}

void
ia_get_loginact(uinfo, loginact)
uinfo_t	uinfo;
long	*loginact;
{
	*loginact = uinfo->ia_inact;
}


void
ia_get_logexpire(uinfo, logexpire)
uinfo_t	uinfo;
long	*logexpire;
{
	*logexpire = uinfo->ia_expire;
}


void
ia_get_logflag(uinfo, logflag)
uinfo_t	uinfo;
long	*logflag;
{
	*logflag = uinfo->ia_flag;
}

#ifdef NS
static int
getgrps(name, gid)
{
	register struct group *grp;
	register int i;
	long max_grps;
	int ngrps = 0;

	/*
	 * get number of supplementry groups 
	 */
	max_grps = sysconf(_SC_NGROUPS_MAX);
	groups = (gid_t *)calloc((unsigned int)max_grps, sizeof(gid_t));
	
	if (gid >= 0)
		groups[ngrps++] = gid;
	
	setgrent();
	while(grp = getgrent()){
		if (grp->gr_gid == gid)
			continue;
		for (i = 0; grp->gr_mem[i]; i++) {
			if (strcmp(grp->gr_mem[i], name))
				continue;
			if (ngrps == max_grps) {
				goto toomany;
			}
			groups[ngrps++] = grp->gr_gid;
		}
	}
toomany:
	endgrent();
	return(ngrps);
}
static int
nis_openinfo(namep, uinfo)
char	*namep;
uinfo_t	*uinfo;
{
	struct passwd *p;
	size_t len, offset;
	int ngrps, i;
	gid_t *gids;

	NS_GETNISTENT(p);

	ngrps = getgrps(p->pw_name, p->pw_gid);

	len = (sizeof(struct master) + (strlen(p->pw_dir)+1) + 
				  (strlen(p->pw_shell)+1) + (ngrps * sizeof(gid_t)));
	*uinfo = (struct master *) calloc(1,len);
	if (*uinfo == NULL){
		return(-1);
	}
	(void)strcpy((*uinfo)->ia_name, p->pw_name);
	(void)strcpy((*uinfo)->ia_pwdp, p->pw_passwd);
	(*uinfo)->ia_uid = p->pw_uid;
	(*uinfo)->ia_gid = p->pw_gid;
	(*uinfo)->ia_sgidcnt = ngrps;

	offset = sizeof(struct master);
	(*uinfo)->ia_sgidp = (gid_t *) (((char *) *uinfo) + offset); 
	for (i=0, gids = (*uinfo)->ia_sgidp; i < ngrps; i++)
		gids[i] =  groups[i];

	offset += (ngrps * sizeof(gid_t));
	(*uinfo)->ia_dirp = (char *) ((char *)*uinfo+ offset);
	(void)strcpy((*uinfo)->ia_dirp, p->pw_dir);
	(*uinfo)->ia_dirsz = strlen(p->pw_dir);

	offset += ((*uinfo)->ia_dirsz+1);
	(*uinfo)->ia_shellp = (char *) ((char *)*uinfo + offset);
	(void)strcpy((*uinfo)->ia_shellp, p->pw_shell);
	(*uinfo)->ia_shsz = strlen(p->pw_shell);

	(*uinfo)->ia_lstchg = -1;
	(*uinfo)->ia_min = -1;
	(*uinfo)->ia_max = -1;
	(*uinfo)->ia_warn = -1;
	(*uinfo)->ia_inact = -1;
	(*uinfo)->ia_expire = -1;
	(*uinfo)->ia_flag = -1;

	return(0);
}

#endif
