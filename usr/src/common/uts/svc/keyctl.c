/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:svc/keyctl.c	1.12"
#ident  "$Header: $"

#include <acc/priv/privilege.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/clock.h>
#include <svc/keyctl.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ghier.h>
#include <util/param.h>
#include <util/types.h>


typedef struct {
	int msize;		/* allocated size */
	int csize;		/* current length */
	uchar_t *val;
} MPNUM;


struct	maxvalue {
	int	userval;
	int	procval;
};

#define K_PE_EDITION		0
#define K_AS_EDITION		1
#define K_MP_UPGRADE		2

void keyctl_init(void);
void keyctl_exit(void);
void limit(int, int *);

STATIC int validate(int, void *, int, struct maxvalue *);
STATIC int modtime(void);
STATIC int keyvalue(uchar_t *);
STATIC int getproduct(uchar_t *);
STATIC void strtokey(uchar_t *, uchar_t *);
STATIC void snumtokey(uchar_t *, uchar_t *);
STATIC int ctl_curlogusers(int);
STATIC uchar_t *serialkey(uchar_t *, uchar_t *);
static MPNUM *multicreate(uchar_t *, int);
static MPNUM *multialloc(size_t);
static void multizero(MPNUM *);
static void multinormalize(MPNUM *);
static void multifree(MPNUM *);
static int shiftleft(MPNUM *, int);
static int shiftright(MPNUM *, int);
static int multiadd(MPNUM *, MPNUM *, MPNUM *);
static int multisub(MPNUM *, MPNUM *, MPNUM *);
static int multimult(MPNUM *, MPNUM *, MPNUM *);
static int multidiv(MPNUM *, MPNUM *, MPNUM *, MPNUM *);
static int multipowmod(MPNUM *, int, MPNUM *, MPNUM *);

#ifdef DEBUG
STATIC int skey_debug;
#endif

struct keyctl_arg {
	int	cmd;
	void	*arg;
	int	nskeys;
};

#define KEYCTL_HIER_BASE	KERNEL_HIER_BASE

STATIC lock_t	keyctl_lock;
STATIC LKINFO_DECL(keyctl_lockinfo, "SU:keyctl:keyctl_lock", 0);

/*
 * void
 * keyctl_start(void)
 *	Initialize the lock used by keyctl system call.
 *
 * Calling/Exit State:
 *
 * Description:
 *
 */
void
keyctl_init(void)
{
	LOCK_INIT(&keyctl_lock, KEYCTL_HIER_BASE, PLBASE, &keyctl_lockinfo,
		  KM_NOSLEEP);
}


/*
 * int
 * keyctl(struct keyctl_arg *uap, rval_t *rvp)
 *
 *	System call to get/set the limit of active CPUs/users allowed.
 *
 * Calling/Exit State:
 *
 *	This is a system call.
 *
 * Description:
 *
 *	System call to:
 *	   * Get and/or Set the limit of:
 *
 *		- online processors allowed in the system.
 *
 *		- concurrent number of users logged in the system.
 *
 *	   * Increment the number of:
 *
 *		- current users logged in the system iff limit not exceeded.
 *
 *	   * Validate key pairs.
 *
 */
int
keyctl(struct keyctl_arg *uap, rval_t *rvp)
{
	int	error;
	struct	maxvalue	mval;

	switch (uap->cmd & K_ACTIONMASK) {
	case K_GETLIMIT:
		limit(uap->cmd, &(rvp->r_val1));
		break;

	case K_VALIDATE:
		if (pm_denied(CRED(), P_SYSOPS))
			return EPERM;

		if (uap->nskeys <= 0)
			return EINVAL;

		return validate(uap->cmd, uap->arg, uap->nskeys,
				(struct maxvalue *)0);

	case K_SETLIMIT:
		if (pm_denied(CRED(), P_SYSOPS))
			return EPERM;

		if ((error = modtime()) != 0)
			return error;

		if (uap->nskeys < 0)
			return EINVAL;

		mval.userval = 0;
		mval.procval = 0;

		if ((error = validate(uap->cmd, uap->arg, uap->nskeys, &mval))
		    == 0 ||
		    error == EEXIST || error == EINVAL) {
			limit(K_SETUSERLIMIT, &(mval.userval));
			limit(K_SETPROCLIMIT, &(mval.procval));
		}
		return error;

	case K_INCRUSE:
	case K_PINCRUSE:
		switch (uap->cmd & K_RESOURCEMASK) {
		case K_USER:
			if (pm_denied(CRED(), P_SYSOPS))
				return EPERM;
			return  ctl_curlogusers(uap->cmd);
		default:
			return EINVAL;
		}

	case K_DECRUSE:
		/*
		 * Will NOT EVER allow an entry point from user level code
		 * to decrement the number of simultaneous users on the system.
		 */
		return EINVAL;

	default:
		error = EINVAL;
	}

	return 0;
}

/*
 * The following is the basekey for the system
 */
#define K_BASEKEY	"\010\017\062\005\010\065\004\001"


/*
 * int
 * validate(int cmd, void *arg, int nskeys, struct maxvalue *mval)
 *
 *	Validates the key pairs and determines their value if applicable.
 *
 * Calling/Exit State:
 *
 *	Each key pair may have a value associated with it. This function
 *	validates each key pair. If cmd is "K_SETLIMIT" it additionaly 
 *	determines the key pair value and computes the total value of all
 *	rlevant valid key pairs. The total value is returned in `mval'.
 *
 *	Returns the errno on error, zero on sucess.
 */
STATIC int
validate(int cmd, void *arg, int nskeys, struct maxvalue *mval)
{
	static uchar_t	basekey[SKLEN] = K_BASEKEY;
	uchar_t		serialnumkey1[SKLEN], serialnumkey2[SKLEN];
	uchar_t		serkey[SKLEN];
	k_skey_t	*kp1, *kp2;
	boolean_t	keyisadup = B_FALSE;
	boolean_t	invalkeys = B_FALSE;
	uchar_t		*result;
	k_skey_t	*kbuf;
	int		k_value;
	int		k_product;
	int		system_type = -1;
	int		proc_persystem = 0;
	int		proc_perupgrade = 0;
	int		num_as_pe_keys = 0;
	int		error;
	int		i, j;


	if (nskeys == 0)
		return 0;

	if (nskeys > MAXSKEYS)
		nskeys = MAXSKEYS;

	kbuf = kmem_alloc(nskeys * sizeof(k_skey_t), KM_SLEEP);

	if (copyin(arg, kbuf, nskeys * sizeof(k_skey_t)) == -1) {
		kmem_free(kbuf, nskeys * sizeof(k_skey_t));
		return EFAULT;
	}

	kp1 = kbuf;

	error = 0;
	for (i = 0; i < nskeys; i++, kp1++) {

		snumtokey(serialnumkey1, kp1->sernum);

#ifdef DEBUG
		if (skey_debug) {
			int n;
			cmn_err(CE_CONT, "keyctl(): sernum string value   ");
			cmn_err(CE_CONT, ":%s:\n",kp1->sernum);
			cmn_err(CE_CONT, "keyctl(): sernum key value   ");
			for (n = 0; n < SKLEN; n++)
				cmn_err(CE_CONT, ":%x: ",serialnumkey1[n]);
			cmn_err(CE_CONT, "\n");
			cmn_err(CE_CONT, "keyctl(): serkey string value   ");
			cmn_err(CE_CONT, ":%x:\n",kp1->serkey[n]);
		}
#endif
		kp2 = kbuf;

		for (j = 0; j < i; j++, kp2++) {
			snumtokey(serialnumkey2, kp2->sernum);
			if (bcmp(serialnumkey2, serialnumkey1, SKLEN) == 0) {
				keyisadup = B_TRUE;
				error = EEXIST;
				break;
			}
		}
		if (keyisadup) {
			keyisadup = B_FALSE;
			continue;
		}
		result = serialkey(serialnumkey1, basekey);
#ifdef DEBUG
		if (skey_debug) {
			int n;
			cmn_err(CE_CONT, "keyctl(): serkey computed ");
			for (n = 0; n < SKLEN; n++)
				cmn_err(CE_CONT, ":%x: ", result[n]);
			cmn_err(CE_CONT, "\n");
		}
#endif

		strtokey(serkey, kp1->serkey);
		if (bcmp(serkey, result, SKLEN)) {
			invalkeys = B_TRUE;
			continue;
		}

		if (cmd == K_SETLIMIT) {

			/*
			 * Each valid key pair will be processed to determine
			 *  - system type (i.e., ASE or PED)
			 *  - number of processors per system type (key value)
			 *  - number of processors per upgrade key pairs
			 */

			k_value = keyvalue(kp1->sernum);

			k_product = getproduct(kp1->sernum);

			switch (k_product) {

			case K_PE_EDITION:

			case K_AS_EDITION:
				/*
				 * Process only the first valid ASE or PED
				 * key pair. Ignore the rest.
				 */
				if (num_as_pe_keys == 0) {

					system_type = k_product;

					proc_persystem = k_value;

					num_as_pe_keys++;
				}
				break;

			case K_MP_UPGRADE:
				if ((proc_perupgrade + k_value) >= K_UNLIMITED)
					proc_perupgrade = K_UNLIMITED;
				else
					proc_perupgrade += k_value;
				break;

			default:
				break;
			}
		}

	}

	if (cmd == K_SETLIMIT && mval != 0) {

		/*
		 * After all key pairs have been processed, determine
		 * the user and processor limits for the system.
		 */

		switch (system_type) {

		case K_PE_EDITION:

			mval->userval = 2;
			mval->procval = proc_persystem;
			break;

		case K_AS_EDITION:

			mval->userval = K_UNLIMITED;
			if ((proc_persystem + proc_perupgrade) >= K_UNLIMITED)
			       mval->procval = K_UNLIMITED;
			else
			       mval->procval = proc_persystem + proc_perupgrade;
			break;
		}
	}

	kmem_free(kbuf, nskeys * sizeof(k_skey_t));
	if (invalkeys)
		error = EINVAL;
	return error;
}


#define	PRODNAME_LEN	3

/*
 * int
 * getproduct(uchar_t str[])
 *
 * Calling/Exit State:
 *
 */
STATIC int
getproduct(uchar_t str[])
{
	struct product {
		char	*name;
		int	number;
	} prodtab[] = {
		{"PED",	K_PE_EDITION},
		{"ASE",	K_AS_EDITION},
		{"MPU",	K_MP_UPGRADE}
	};
	int	i;
	int	product = -1;
	int	val_nproducts;
	char	productname[PRODNAME_LEN +1];

	val_nproducts = (sizeof prodtab / sizeof (struct product));

	/*
	 * Get the product name (first 3 char) from the input string
	 */
	strncpy(productname, (char *)str, PRODNAME_LEN);
	productname[PRODNAME_LEN] = '\0';

	/*
	 * For the purpose of setting user/processor limits, there
	 * are only a few products that are valid/relevant.
	 * So, if the given product name is found in the table,
	 * return is aasociated product number, else return -1
	 */
	for (i = 0; i < val_nproducts; i++) {
		if (strcmp(productname, prodtab[i].name) == 0) {
			product = prodtab[i].number;
			break;
		}
	}
	return product;
}


/*
 * int
 * keyvalue(uchar_t key[])
 *
 *	Determines the associated value of the key pair.
 *
 * Calling/Exit State:
 *
 */
STATIC int
keyvalue(uchar_t str[])
{
	int value;

	/*
	 * Determine the decimal value of the fourth character of the str.
	 */
	if (str[3] >= '0' && str[3] <= '9')
		value = str[3] - '0';
	else 
		value = -1;
	if (value < 0)
		return 0;
	else if (value == 0)
		return K_UNLIMITED;
	else
		return value;
}


/*
 * void
 * strtokey(uchar_t key[], uchar_t *str)
 *
 *	converts serialkey string into serialkey keys.
 *
 * Calling/Exit State:
 * 
 * Description:
 *	Translates a max of 11 character string (all alpha) into an 8 byte key
 *
 */
STATIC void
strtokey(uchar_t key[], uchar_t *str)
{
	uchar_t block[(SKLEN + 1) * 8];
	uchar_t	c;
	int	i, j;

	/*
	 * Fill block with SKLEN * 8 null characters.
	 */
	for (i = 0; i < SKLEN * 8; i++)
		block[i] = '\0';

	/*
	 * Translate string characters (6-bit values) into block.
	 */
	for (i = 0; (c = *str++) != 0 && c != '\n' && c != ' '
				&& c != '\t' && i * 6 < SKLEN * 8; i++ ) {
		if (c > 'Z')
			c -= 6;
		if (c > '9')
			c -= 7;
		if (c > '.')
			c -= 1;
		c -= '-';
		for (j = 0; j < 6; j++)
			if (i * 6 + j < SKLEN * 8)
				block[(i * 6) + j] = (c >> (5 - j)) & 01;
	}
	
	/*
	 * Translate block into key.
	 */
	for (i = 0; i < SKLEN; i++) {
		key[i] = '\0';
		for (j = 0; j < 8; j++) {
			key[i] = key[i] << 1;
			key[i] |= block[(i * 8) + j];
		}
	}
}


/*
 * void
 * snumtokey(uchar_t key[], uchar_t *str)
 *
 *	converts the serialnumber string into serialnumber key.
 *
 * Calling/Exit State:
 *
 * Description:
 * 
 *	Translates a 14 character string (4 alpha + 10 digits) into  8 byte key
 *
 */
STATIC void
snumtokey(uchar_t key[], uchar_t *str)
{
	uchar_t block[(SKLEN + 1) * 8];
	uchar_t	c;
	int	i, j;

	/*
	 * Fill block with SKLEN * 8 null characters.
	 */
	for (i = 0; i < SKLEN * 8; i++)
		block[i] = '\0';

	/*
	 * Translate the first 4 string characters (6-bit values) into block.
	 * This will fill in block[0] to block[23].
	 */
	for (i = 0; (c = *str) != 0 && c != '\n' && c != ' '
				&& c != '\t' && i <= 3; i++, str++) {
		if (c > 'Z')
			c -= 6;
		if (c > '9')
			c -= 7;
		if (c > '.')
			c -= 1;
		c -= '-';
		for (j = 0; j < 6; j++)
			block[(i * 6) + j] = (c >> (5 - j)) & 01;
	}
	
	/*
	 * Translate the last 10 string digits (4-bit values) into block.
	 * This will fill in block[24] to block[63].
	 */
	for (i = 0; (c = *str++) != 0 && c != '\n' && c != ' '
				&& c != '\t' && i * 4 < SKLEN * 8; i++ ) {
		if (c > 'Z')
			c -= 6;
		if (c > '9')
			c -= 7;
		if (c > '.')
			c -= 1;
		c -= '-';
		for (j = 0; j < 4; j++)
			if ((24 + (i * 4) + j) < SKLEN * 8)
				block[24 + (i * 4) + j] = (c >> (3 - j)) & 01;
	}
	
	/*
	 * Translate 64 byte block into and 8 byte key.
	 */
	for (i = 0; i < SKLEN; i++) {
		key[i] = '\0';
		for (j = 0; j < 8; j++) {
			key[i] = key[i] << 1;
			key[i] |= block[(i * 8) + j];
		}
	}
}

#define TENSECONDS	(10 * (HZ))

/*
 * int
 * modtime(void)
 *
 *	Checks and/or modifies `modtime'
 *
 * Calling/Exit State:
 *
 *	When checking for last time modified, returns ETIME if
 *	less than ten seconds has passed, zero otherwise.
 */
STATIC int
modtime(void)
{
	static clock_t	modtime;

	TIME_LOCK();
	if (modtime == 0 || (lbolt - modtime) >= TENSECONDS) {
		modtime = lbolt;
		TIME_UNLOCK();
		return 0;
	}
	TIME_UNLOCK();

	return ETIME;
}


/*
 * The following are the default values for numbers of users and number of
 * processors in the system.
 */
#define K_NUSERS	2
#define K_NPROCS	1


/*
 * void
 * limit(int cmd, int *maxval)
 *
 *	Gets and/or sets `maxcpuonline' or `max_nusers'
 *
 * Calling/Exit State:
 *
 *	The value of `maxcpuonline' or `max_nusers' is:
 *
 *		- returned in `maxval' for a GET command.
 *
 *		- set to `maxval' for a SET command.
 *
 *	No locks held on entry and no locks held on exit.
 *	Relying on atomic int access.
 */
void
limit(int cmd, int *maxval)
{
	static struct limval {
		int curval;
		int minval;
	} limval[] = {
		{ K_NUSERS, K_NUSERS },		/* K_USER */
		{ K_NPROCS, K_NPROCS }		/* K_PROC */
	};
	struct limval *limp;

	limp = &limval[cmd & K_RESOURCEMASK];

	switch (cmd & K_ACTIONMASK) {
	case K_GETLIMIT:
		*maxval = limp->curval;
		break;
	case K_SETLIMIT:
		if (*maxval < limp->minval)
			limp->curval = limp->minval;
		else
			limp->curval = *maxval;
		break;
	}
}


/*
 * int
 * ctl_curlogusers(int cmd)
 *
 *	Controls the current number of logged users in the system.
 *
 * Calling/Exit State:
 *
 */
STATIC int
ctl_curlogusers(int cmd)
{
	static int	cur_nusers = 0;
	int 		max_nusers;

	ASSERT(getpl() == PLBASE);

	switch (cmd) {
	case K_PINCRCURNUSER:
	case K_INCRCURNUSER:
		limit(K_GETUSERLIMIT, &max_nusers);
		if (cmd == K_PINCRCURNUSER)
			max_nusers++;

		(void)LOCK_PLMIN(&keyctl_lock);

		/*
		 * If already counted, we are set.
		 */
		if (u.u_procp->p_perusercnt) {
			UNLOCK_PLMIN(&keyctl_lock, PLBASE);
			break;
		}

		if (cur_nusers >= max_nusers && max_nusers != K_UNLIMITED) {
			UNLOCK_PLMIN(&keyctl_lock, PLBASE);
			return EUSERS;
		}

		cur_nusers++;

		/*
		 * set flag to indicate that this process has been counted as
		 * a user session. 
		 */
		u.u_procp->p_perusercnt = B_TRUE;

		UNLOCK_PLMIN(&keyctl_lock, PLBASE);

		break;

	case K_DECRCURNUSER:
		/*
		 * ``K_DECRCURNUSER'' could never be executed on behalf of user
		 * level code. No entry points are (and should NEVER be)
		 * available to user level code.
		 * 
		 */
		(void)LOCK_PLMIN(&keyctl_lock);

		ASSERT(cur_nusers > 0);
		cur_nusers--;

		/*
		 * clear the user flag for this process.
		 */
		u.u_procp->p_perusercnt = B_FALSE;

		UNLOCK_PLMIN(&keyctl_lock, PLBASE);

		break;
	}

	return 0;
}

/*
 * void
 * keyctl_exit(void)
 *
 * Calling/Exit State:
 *
 */
void
keyctl_exit(void)
{
	if (u.u_procp->p_perusercnt)
		(void)ctl_curlogusers(K_DECRCURNUSER);
}

#define max(a,b)	((a) > (b) ? (a) : (b))
#define MULTIFREE(x)	{ if ((x) != NULL) multifree((x)); }

/*
 * Multiple-precision math error codes
 */
#define MP_EOVFL	1		/* overflow */
#define MP_EDOM		2		/* domain */
#define MP_ERANGE	3		/* range */

#define BASE		256
#define COFLEN		12


/*
 * static MPNUM *
 * multicreate(uchar_t num[], int len)
 *
 *	Creates and initializes a multi-precision number structure.
 *
 * Calling/Exit State:
 *
 *	Returns a pointer to the created multi-precision structure.
 *
 */
static MPNUM *
multicreate(uchar_t num[], int len)
{
	MPNUM *new;
	int i;

	new = multialloc(len);
	multizero(new);
	new->csize = len;
	for (i = 0; i < len; i++) {
		new->val[i] = num[i];
	}
	multinormalize(new);
	return new;
}


/*
 * uchar_t *
 * serialkey(uchar_t num[], uchar_t basekey[])
 *
 *	One-way function to compute serial key given, a pair of keys.	
 *
 * Calling/Exit State:
 *
 *	Returns a pointer to the computed serial key. The value of
 *	serial key will be set to NULL if an error was encountered
 *	during its computaion.
 */
STATIC uchar_t *
serialkey(uchar_t num[], uchar_t basekey[])
{
	static uchar_t key[SKLEN];
	static uchar_t a1[COFLEN] = {0x00, 0x00, 0x00, 0x00, 0x8A, 0xff, 0xff,
				     0xff, 0xff, 0xff, 0xff, 0xc5};
	static uchar_t a2[COFLEN] = {0x00, 0x00, 0x00, 0x00, 0x7B, 0xff, 0xff,
				     0xff, 0xff, 0xff, 0xff, 0xc5};
	static uchar_t a3[COFLEN] = {0x00, 0x00, 0x00, 0x00, 0x6F, 0xff, 0xff,
				     0xff, 0xff, 0xff, 0xff, 0xc5};
	static uchar_t a4[COFLEN] = {0x00, 0x00, 0x00, 0x00, 0x50, 0xff, 0xff,
				     0xff, 0xff, 0xff, 0xff, 0xc5};
	static uchar_t a5[COFLEN] = {0x00, 0x00, 0x00, 0x00, 0x5C, 0xff, 0xff,
				     0xff, 0xff, 0xff, 0xff, 0xc5};
	static uchar_t p1[COFLEN] = {0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
				     0xff, 0xff, 0xff, 0xff, 0xc5};
	MPNUM *thekey, *thenum, *param;

	/* partial sums */
	MPNUM *temp1, *temp2, *temp3;

	/* terms */
	MPNUM *term1, *term2, *term3, *term4, *term5;

	/* coefficients */
	MPNUM *c1, *c2, *c3, *c4, *c5, *prime;

	uchar_t *retval = key;
	int i, j;

	param = multialloc(SKLEN * 65);
	temp1 = multialloc(SKLEN * 65);
	temp2 = multialloc(SKLEN * 65);
	temp3 = multialloc(SKLEN * 65);

	term1 = multialloc(SKLEN * 65);
	term2 = multialloc(SKLEN * 65);
	term3 = multialloc(SKLEN * 65);
	term4 = multialloc(SKLEN * 65);
	term5 = multialloc(SKLEN * 65);

	thekey = multicreate(basekey, SKLEN);
	thenum = multicreate(num, SKLEN);
	if (multiadd(thekey, thenum, temp1)) goto errexit;
	if (multiadd(temp1, temp1, param)) goto errexit;

	c1 = multicreate(a1, COFLEN);
	c2 = multicreate(a2, COFLEN);
	c3 = multicreate(a3, COFLEN);
	c4 = multicreate(a4, COFLEN);
	c5 = multicreate(a5, COFLEN);
	prime = multicreate(p1, COFLEN);

	/* x^n  */
	if (multipowmod(param, 16777233, term1, prime)) goto errexit;

	/* x^n2 */
	if (multipowmod(param, 16777219, temp1, prime)) goto errexit;

	/* a1 * x^n2 */
	if (multimult(temp1, c1, term2)) goto errexit;

	/* x^3  */
	if (multipowmod(param, 3, temp1, prime)) goto errexit;    

	/* a2 * x^3  */
	if (multimult(temp1, c2, term3)) goto errexit;

	/* x^2  */
	if (multipowmod(param, 2, temp1, prime)) goto errexit;

	/* a3 * x^2  */
	if (multimult(temp1, c3, term4)) goto errexit;  

	/* a4 * x    */
	if (multimult(param, c4, term5)) goto errexit; 

	/* x^n + (a1 * x^n2) */
	if (multiadd(term1, term2, temp1)) goto errexit;  

	/* (x^n + (a1 * x^n2)) + (a2 * x^3) */
	if (multiadd(temp1, term3, temp2)) goto errexit;

	/* ((x^n + (a1 * x^n2)) + (a2 * x^3)) + (a3 * x^2) */
	if (multiadd(temp2, term4, temp1)) goto errexit;

	/* (((x^n + (a1 * x^n2)) + (a2 * x^3)) + (a3 * x^2)) + (a4 * x) */
	if (multiadd(temp1, term5, temp2)) goto errexit;

	/* ((((x^n + (a1 * x^n2)) + (a2 * x^3)) + (a3 * x^2)) + (a4 * x)) + a5 */
	if (multiadd(temp2, c5, temp1)) goto errexit;

	/* p(x) mod P */
	if (multidiv(temp1, prime, temp2, temp3)) goto errexit;

	multinormalize(temp3);

	/* zero it out */
	for (i = 0; i < SKLEN; i++) {
		key[i] = '\0';
	}

	j = SKLEN - 1;
	for (i = temp3->csize - 1; i >= 0; i--) {
		key[j--] = temp3->val[i];
	}
	goto freeit;

errexit:

#ifdef DEBUG
	cmn_err(CE_WARN, "keyctl(): unable to evaluate polynomial.");
#endif
	retval = NULL;

freeit:
	MULTIFREE(temp1);
	MULTIFREE(temp2);
	MULTIFREE(temp3);
	MULTIFREE(term1);
	MULTIFREE(term2);
	MULTIFREE(term3);
	MULTIFREE(term4);
	MULTIFREE(term5);
	MULTIFREE(thekey);
	MULTIFREE(thenum);
	MULTIFREE(param);
	MULTIFREE(c1);
	MULTIFREE(c2);
	MULTIFREE(c3);
	MULTIFREE(c4);
	MULTIFREE(c5);
	MULTIFREE(prime);
	return retval;
}


/*
 * void
 * multizero(MPNUM *n)
 *
 *	Initializes to zeros the multi-precision number structure.
 *
 * Calling/Exit State:
 *
 */
static void
multizero(MPNUM *n)
{
	bzero(n->val, n->msize);
	(n)->csize = 1;
}


/*
 * void
 * multinormalize(MPNUM *n)
 *
 *
 * Calling/Exit State:
 *
 */
static void
multinormalize(MPNUM *n)
{
	int i, j;

	for (i = 0; i < n->csize; ++i) {
		if ((int)(n->val[i]) != 0) break;
	}
	if (i >= n->csize) multizero(n);	/* no nonzero digits */
	else if (i == 0) return;			/* already normalized */
	else {
		n->csize -= i;
		for (j = 0; j < n->csize; ++j) n->val[j] = n->val[j + i];
	}
}


/*
 * static MPNUM *
 * multialloc(size_t size)
 *
 *	Allocates space for the multi-precision number structure.
 *
 * Calling/Exit State:
 *	returns a pointer to the new multi-precision number structure.
 *
 */
static MPNUM *
multialloc(size_t size)
{
	MPNUM *n;
	uchar_t *v;

	n = kmem_alloc(sizeof(MPNUM), KM_SLEEP);

	v = kmem_alloc(size, KM_SLEEP);

	n->msize = size;
	n->val = v;

	multizero(n);
	return n;
}


/*
 * void
 * multifree(MPNUM *n)
 *	Frees space of the multi-precision number structure pointed to by n.
 *
 * Calling/Exit State:
 *
 */
static void
multifree(MPNUM *n)
{
	kmem_free(n->val, n->msize);
	kmem_free(n, sizeof(MPNUM));
}


/*
 * int
 * multicompare(MPNUM *n1, MPNUM *n2)
 *
 * Calling/Exit State:
 *
 */
static int
multicompare(MPNUM *n1, MPNUM *n2)
{
	int i;

	multinormalize(n1);
	multinormalize(n2);

	if (n1->csize < n2->csize) return -1;
	else if (n1->csize > n2->csize) return 1;
	else for (i=0; i<n1->csize; ++i) {
		if (n1->val[i] < n2->val[i]) return -1;
		else if (n1->val[i] > n2->val[i]) return 1;
	}
	return 0;
}


/*
 * int
 * shiftleft(MPNUM *n, int b)
 *
 * Calling/Exit State:
 *
 */
static int
shiftleft(MPNUM *n, int b)
{
	int ii;
	uchar_t mask, savebits;

	if ( b < 0 )
		return shiftright(n, -b);

	while (b > 8) {
		if (n->csize == n->msize) return MP_EOVFL;
		n->val[n->csize++] = 0;
		b -= 8;
	}
	if ( b != 0 ) {
		mask = (0xFF << (8 - b)) & 0xFF;
		savebits = 0;
		for ( ii = n->csize - 1; ii >= 0; ii--) {
			int temp = ((int)(n->val[ii]) << b) | savebits;
			savebits = (int)(n->val[ii] & mask) >> (8 - b);
			n->val[ii] = (uchar_t)(temp & 0xFF);
		}
		if (savebits) {
			if (n->csize == n->msize) return MP_EOVFL;
			for (ii = n->csize - 1; ii >= 0; --ii)
				 n->val[ii+1] = n->val[ii];
			n->val[0] = savebits;
			++n->csize; 
		}
	}
	return 0;
}


/*
 * int
 * shiftright(MPNUM *n, int b)
 *
 * Calling/Exit State:
 *
 */
static int
shiftright(MPNUM *n, int b)
{
	int ii;
	uchar_t mask, savebits;

	if ( b < 0 ) return shiftleft(n, -b);

	while (b > 8) {
		n->val[--n->csize] = 0;
		if (n->csize < 1) n->csize = 1;
		b -= 8;
	}

	if ( b != 0 ) {
		mask = (0x00FF >> (8 - b)) & 0xFF;
		savebits = 0;
		for ( ii = 0; ii < n->csize; ii++) {
			int temp = ((int)(n->val[ii]) >> b) | savebits;
			savebits = ((int)(n->val[ii]) & mask) << (8 - b);
			n->val[ii] = (uchar_t)(temp & 0xFF);
		}
	}
	return 0;
}


/*
 * Return the digit at position p (counting from the right starting
 * at 1) in the number n.  Return zeros on the left.
 */

#define DIGIT(n,p) (((p)<=(n)->csize)?((n)->val[(n)->csize-(p)]):0)


/*
 * int
 * multiadd(MPNUM *n1, MPNUM *n2, MPNUM *s)
 *
 * Calling/Exit State:
 *	Returns MP_EOVFL on failure, zero on success.
 *
 */
static int
multiadd(MPNUM *n1, MPNUM *n2, MPNUM *s)
{
	int i, k, ss;
	unsigned int p;

	multizero(s);
	ss = max(n1->csize, n2->csize);
	if (ss > s->msize) 
		return MP_EOVFL;

	k = 0;
	for (i = 1; i <= ss; ++i) {
		p = DIGIT(n1,i) + DIGIT(n2,i) + k;
		k = (p > BASE-1);
		s->val[ss - i] = (uchar_t)(p & (BASE-1));
	}
	s->csize = ss;
	if (k) {
		if (s->csize == s->msize) 
			return MP_EOVFL;
		for (i = s->csize++; i > 0; --i) s->val[i] = s->val[i-1];
		s->val[0] = (uchar_t)k;
	}
	return 0;
}


/*
 * int
 * multisub(MPNUM *n1, MPNUM *n2, MPNUM *d)
 *
 * Calling/Exit State:
 *
 */
static int
multisub(MPNUM *n1, MPNUM *n2, MPNUM *d)
{
	int i, k, ds, d1;

	multizero(d);
	ds = max(n1->csize, n2->csize);
	if (ds > d->msize) return MP_EOVFL;
	d->csize = ds;

	k = 0;
	for (i = 1; i <= ds; ++i) {
		d1 = (int)DIGIT(n1,i) - (int)DIGIT(n2,i);
		d->val[d->csize - i] = (d1 + BASE - k) & (BASE-1);
		k = (d1 < k);
	}
	if (k) return MP_EDOM;		/* negative result */

	return 0;
}


#undef DIGIT

/*
 * These definitions help iron out the 1-based arrays used
 * in Knuth and make the math clearer.
 */
#define U(x) (n1->val[(x)-1])
#define V(x) (n2->val[(x)-1])
#define W(x) (p->val[(x)-1])


/*
 * int
 * multimult(MPNUM *n1, MPNUM *n2, MPNUM *p)
 *
 * Calling/Exit State:
 *	Returns MP_EOVFL on failure, zero on success.
 *
 */
static int
multimult(MPNUM *n1, MPNUM *n2, MPNUM *p)
{
	int i, j, ps;
	uint_t t, k;

	multizero(p);
	ps = n1->csize + n2->csize;
	if (p->msize < ps) 
		return MP_EOVFL;
	p->csize = ps;

	for (j = n2->csize; j > 0; --j) {
		/* Optional zero test left out here */
		k = 0;
		for (i = n1->csize; i > 0; --i) {
			t = (int)U(i) * (int)V(j) + (int)W(i+j) + k;
			W(i+j) = (uchar_t)(t & (BASE-1));
			k = (t >> 8) & (BASE-1);
		}
		W(j) = (uchar_t)k;
	}
	return 0;
}


#undef U
#undef V
#undef W

/*
 * See Knuth, _Seminumerical Algoritms_, pp. 256-260
 */
#define U(x) (dvd->val[x])
#define V(x) (dsr->val[(x)-1])
#define Q(x) (q->val[x])
#define R(x) (r->val[(x)-1])


/*
 * int
 * multidiv(MPNUM *n1, MPNUM *n2, MPNUM *q, MPNUM *r)
 *
 * Calling/Exit State:
 *
 */
static int
multidiv(MPNUM *n1, MPNUM *n2, MPNUM *q, MPNUM *r)
{
	MPNUM *dvd, *dsr, *t1, *t2;
	int i, j, k, m, n, rval = MP_EOVFL;
	uint t, qhat, shift;

	if (multicompare(n1, n2) == -1) {	/* check for zero quotient */
		multizero(q);
		r->csize = n1->csize;
		bcopy(n1->val, r->val, n1->csize);
		return 0;
	}
	dvd = multialloc(n1->csize + 1);
	dvd->csize = n1->csize + 1;
	bcopy(n1->val, dvd->val + 1, n1->csize);
	dvd->val[0] = 0;

	dsr = multialloc(n2->csize);
	dsr->csize = n2->csize;
	bcopy(n2->val, dsr->val, n2->csize);
	multinormalize(dsr);
	n = dsr->csize;
	m = n1->csize - n;

	t1 = multialloc(n + 1);
	t2 = multialloc(n + 1);

	rval = MP_EDOM;		/* Test for Div by 0 */
	if ((j = (int)V(1)) == 0) goto derr;
	shift = 0;
	while ((j & 0x80) == 0) {
		++shift;
		j <<= 1;
	}
	shiftleft(dsr, shift);
	shiftleft(dvd, shift);

	for (j = 0; j <= m; ++j) {
		t = (((uint)U(j) << 8) + U(j+1)) & 0xFFFF;
		if (U(j) == V(1)) qhat = BASE-1;
		else qhat = t / (uint)V(1);

		while (qhat * (int)V(2) > 
		       ((t - qhat * (int)V(1)) << 8) + (int)U(j+2)) {
			--qhat;
		}
		/*
		 * R = qhat * V
		 */
		multizero(r);
		r->csize = n+1;
		k = 0;
		for (i = n; i > 0; --i) {
			t = (int)V(i) * qhat + (int)R(i+1) + k;
			R(i+1) = (uchar_t)(t & (BASE-1));
			k = (t >> 8) & (BASE-1);
		}
		R(1) = (uchar_t)k;
		/*
		 * U(j..j+n) -= R
		 */
		t1->csize = n+1;
		for (i = 0; i <= n; ++i) t1->val[i] = U(j+i);
		k = multisub(t1, r, t2);
		for (i = 0; i <= n; ++i) U(j+i) = t2->val[i];

		Q(j) = (uchar_t)qhat;
		if (k != 0) {
			--Q(j);
			k = 0;
			for (i = n; i >= 0; --i) {
				t = (int)U(j+i) + ((i>0)?V(i):0) + k;
				U(j+i) = (uchar_t)(t & 0xFF);
				k = (t >= BASE);
			}
		}
	}
	q->csize = m+1;
	multizero(r);
	for (i = 1; i <= n; ++i) R(i) = U(m+i);

	r->csize = n;
	shiftright(r, (int)shift);

	rval = 0;
derr:
	multifree(t2);
	multifree(t1);
	multifree(dsr);
	multifree(dvd);

	return rval;
}


/*
 * int
 * multipowmod(MPNUM *n, int exp, MPNUM *r, MPNUM *modulus)
 *
 * Calling/Exit State:
 *	Returns MP_EOVFL on failure, zero on success.
 *
 */
static int
multipowmod(MPNUM *n, int exp, MPNUM *r, MPNUM *modulus)
{
	int rval = MP_EOVFL;
	MPNUM *b, *t, *q;

	b = multialloc(r->msize);
	b->csize = n->csize;
	bcopy(n->val, b->val, n->csize);
	multinormalize(b);

	t = multialloc(r->msize);
	q = multialloc(r->msize);
	multizero(r);
	r->val[0] = 1;

	for (; ;) {
		if (exp & 1) {
			if (multimult(r, b, t) != 0) goto error2;
			if (multidiv(t, modulus, q, r) != 0) goto error2;
		}
		if ((exp >>= 1) == 0) 
			break;

		if (multimult(b, b, t) != 0) goto error2;
		if (multidiv(t, modulus, q, b) != 0) goto error2;
	}
	rval = 0;
error2:
	multifree(q);
	multifree(t);
	multifree(b);
	return rval;
}

#undef U
#undef V
#undef Q
#undef R
