/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libpkg:common/lib/libpkg/pkgactkey.c	1.3"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/keyctl.h>
#include <pfmt.h>

#define	PDLEN		3		/* Product id length. */
#define	ALLOC_SIZ	64
#define	KEYS_FILE	"/etc/config/licensekeys"
#define	MSG_NOARG	"uxpkgtools:796:- key argument is invalid\n"
#define	ERR_MEMORY	"uxpkgtools:6:memory allocation failure, errno=%d"

extern int getkeysid(char *, k_skey_t *);
extern int fgetkeysid(FILE *, char *, k_skey_t *);
extern int prid2list(char ***, char *, const char *);
extern int pkg_validate(k_skey_t *, char *, int);

/*
 * Get a valid serial number/activation key entry from a given file.
 * The first valid entry is returned if no product id is specified.
 * Otherwise the first matched entry from the current file offset is
 * returned.
 */
int
fgetkeysid(FILE *fp, char *prodid, k_skey_t *kp)
{
	int found;
	char *snum;
	register char *p;
	char buf[BUFSIZ];

	if (fp == NULL || prodid == NULL || kp == NULL) {
		return(0);
	}
	if (prodid[0] && strlen(prodid) != PDLEN) {
		return(0);
	}
	found = 0;
	kp->sernum[0] = '\0';
	kp->serkey[0] = '\0';
	while ((p = fgets(buf, BUFSIZ, fp)) != NULL) {
		if ((snum = strtok(p, " \t\n")) == NULL || *p == '#') {
			continue;
		}
		if ((p = strtok(NULL, " \t\n")) == NULL) {
			found = 0;
			break;
		}
		if (prodid[0] == '\0' ||
		    !strncmp(prodid, snum, PDLEN)) {
			found++;
			(void)strncpy((char *)kp->sernum,
					snum + PDLEN, STRLEN - PDLEN);
			(void)strncpy((char *)kp->serkey, p, STRLEN);
			break;
		}
	}
	return(found);
}

/*
 * This getkeysid() routine uses the /etc/config/licensekeys file
 * as the file containing the serial number/activation key pairs
 * of Unixware products.  It calls fgetkeysid() to get the entries.
 */
int
getkeysid(char *prodid, k_skey_t *kp)
{
	static FILE *fp = NULL;

	if (prodid == NULL) {
		/*
		 * This is a request to close the licensekeys file.
		 */
		if (fp) {
			(void)fclose(fp);
			fp = NULL;
		}
		return(1);
	}
	if (kp == NULL) {
		errno = EINVAL;
		pfmt(stderr, MM_ERROR, MSG_NOARG);
		return(-1);
	}
	if (fp == NULL) {
		if ((fp = fopen(KEYS_FILE, "r")) == NULL) {
			return(-1);
		}
	}
	if (prodid[0]) {
		if (fseek(fp, 0L, 0)) {
			return(-1);
		}
	}
	return(fgetkeysid(fp, prodid, kp) ? 0 : -1);
}

int
prid2list(char ***strtab, char *prid, const char *delim)
{
	int i;
	char **pstr;
	register char *pt;

	if (prid == NULL) {
		/*
		 * Free resources associated with strtab.
		 */
		pstr = *strtab;
		if (pstr != NULL && pstr[0] != NULL) {
			(void)free(pstr[0]);
			(void)free(pstr);
		} else if (pstr != NULL) {
			(void)free(pstr);
		}
		*strtab = NULL;
		return 0;
	}
	if ((pt = strdup(prid)) == NULL) {
		progerr(ERR_MEMORY, errno);
		return -1;
	}
	if ((pstr = (char **)calloc(ALLOC_SIZ, sizeof(char *))) == NULL) {
		progerr(ERR_MEMORY, errno);
		return -1;
	}
	i = 0;
	pt = strtok(pt, delim);
	while (pt != NULL) {
		pstr[i++] = pt;
		if ((i % ALLOC_SIZ) == 0) {
			pstr = (char **)realloc(pstr,
					(i + ALLOC_SIZ) * sizeof(char *));
			if (pstr == NULL) {
				progerr(ERR_MEMORY, errno);
				return -1;
			}
		}
		pt = strtok(NULL, delim);
	}
	pstr[i] = NULL;
	pstr = (char **)realloc(pstr, (i + 1) * sizeof(char *));
	*strtab = pstr;
	return pstr == NULL ? -1 : i;
}

int
pkg_validate(k_skey_t *sp, char *prodname, int prefix)
{
	int ret = 1;
	int cnt;
	register int i, n;
	char **ptab;

	/*
	 * Convert product IDs to an array of strings.
	 */
	if ((cnt = prid2list(&ptab, prodname, "|")) <= 0) {
		return ret;
	}
	if (prefix) {
		/*
		 * This portion of the code assumes that if the user
		 * was prompted for the serial number/key pair, then
		 * the serial number supplied doesn't contain the
		 * starting 3-character product ID.  This value therefore
		 * needs to be taken from the PRODUCTNAME specifier
		 * parameter of the package.  This is required in the
		 * sernum field passed to keyctl().
		 */
		for (n = 0; n < cnt; n++) {
			for (i = 0; i < PDLEN; i++) {
				sp->sernum[i] = ptab[n][i];
			}
			if ((ret = keyctl(K_VALIDATE, sp, 1)) == 0) {
				break;
			}
		}
		return ret;
	}
	/*
	 * Verify that the product ID part of the SERIALNUM
	 * exported environment variable matches at least one
	 * of the specified product IDs.
	 */
	for (n = 0; n < cnt; n++) {
		if (strncmp(ptab[n], (const char *)sp->sernum, PDLEN) == 0) {
			break;
		}
	}
	/*
	 * Free memory resources associated with table.
	 */
	(void)prid2list(&ptab, NULL, NULL);
	return n >= cnt ? n : keyctl(K_VALIDATE, sp, 1);
}
