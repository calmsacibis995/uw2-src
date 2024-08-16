/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

#ident	"@(#)stand:i386at/standalone/boot/at386/boot.c	1.2.2.22"
#ident	"$Header: $"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/bootinfo.h>
#include <sys/cram.h>
#include <boothdr/bootcntl.h>
#include <boothdr/boot.h>
#include <boothdr/bootlink.h>
#include <boothdr/bootdef.h>
#include <boothdr/vxvm_boot.h>
#include <boothdr/libfm.h>
#include <boothdr/error.h>

extern	off_t	boot_delta;
extern	off_t	root_delta;
extern	bfstyp_t boot_fs_type;
extern	bfstyp_t root_fs_type;
extern  unsigned char	*gbuf;

extern	void	goany();
extern	int	bt_logo();
extern	int	getfhdr();
extern	int	checkkbd();
extern	void	update_bootcntl();
extern 	struct	lpcb	lpcb[];

/*	Initalize the function pointers for the utility routines
 *	used by the boot supportive programs. Note changes here
 *	must be reflected in the initprog/initprog.h file and in
 *	bootlink.h.
 */

struct	bootfuncs	bf = {
		bprintf, bstrcpy, bstrncpy, bstrcat, bstrlen, bmemcpy, bmemcmp,
		bgetchar, bputchar,
		bgets, ischar, doint, goany, CMOSread, bmemset, shomem,
		NULL, /* file read_routine */
		bootabort, bt_malloc,
		BL_file_init, BL_file_open, BL_file_read, BL_file_close,
		BL_file_lseek, 
		no_op, /* default file_compress */
		no_op, /* default logo display*/
		getfhdr, /* reads elf file header */
		checkkbd
	};

int	logo_up = FAILURE;
int	memrng_updated = FALSE;
struct	bootcntl *btcntlp; 

int	setjmpenv[6];
int	boot_restart;

unsigned short	hdisk_id = BOOTDRV_HD;
#if !defined(WINI) | defined(HDTST)
int	slow_boot = 5;
#else
int	slow_boot = 0;
#endif

#ifdef HDTST
static char	diskid[VOL_UUID_SZ];
#endif

extern	char	flatdesc;
extern	int	end;
extern  daddr_t	part_start;	/* start of active partition */
extern	paddr_t secboot_mem_loc;

extern	short	bps;		/* bytes per sector */
extern	short	spt;		/* disk sectors per track */
extern	short	spc;		/* disk sectors per cylinder */
extern 	int	goprot();
extern 	int	_start();

extern	char	standboot[];

#ifdef BOOT_DEBUG
uint	dread_cnt=0;
#endif

main()
{
	int	i;
	char	ans[3];
        char    *dest;
        char    *fname;
        char    *bootstr;
	int	status;
	off_t		save_delta;
	bfstyp_t	save_fs_type;

	btcntlp = (struct bootcntl *)secboot_mem_loc;

	/* zero out the bootinfo/bootenv  struct */
	for (dest = (char *)BOOTENV; dest != (char *)(BOOTENV + 1);)
		*dest++ = '\0';

	/*
	 * reserve the first argument for kernel pathname
	 */
	BTE_INFO.bargc = 1;

	/*
         * bootflags must be initialized before the call to BL_file_init()
         * because the floppy boot flag will get OR'ed in if needed in
         * get_fs() which is called by BL_file_init().
         */
        BTE_INFO.bootflags = btcntlp->bc_bootflags;

	/*
	 * initialize the stand alone disk driver
	 */
#if defined(WINI) & !defined(HDTST)
	BL_file_init(BOOTDRV_HD);
#else
	BL_file_init(BOOTDRV_FP);
#endif

	/*
	 * If /stand/boot exists, it will be used to override 
	 * default values in the bootcntl structure.  
	 * If the boot file does NOT exist, the default
	 * values will be used unchanged (see bootcntl.c) .
	 */

	update_bootcntl(btcntlp);

	printf("\n%s \n", btcntlp->bootmsg);

	kb_flush();
	if (btcntlp->autoboot == FALSE){
		parse_user(btcntlp);
		printf("\n%s \n", btcntlp->bootmsg);
	}

	/* wait for user intervention w/ slowboot */
	while ( slow_boot-- ){	
		wait1s();
		if (checkkbd()){
			parse_user(btcntlp);
			printf("\n%s \n", btcntlp->bootmsg);
			break;
		}
	};

	if ( (boot_restart=setjmp(setjmpenv)) != 0){
		bf.b_read = NULL;
		bf.decompress = no_op;
		bf.logo = no_op;
		gbuf=0;
		BOOTENV->bf_resv_base=0;
		BTE_INFO.bootflags = btcntlp->bc_bootflags;
#if defined(WINI) & !defined(HDTST)
		BL_file_init(BOOTDRV_HD);
#else
		BL_file_init(BOOTDRV_FP);
#endif
	}

	/*
	 * Initialize the boot environment from the boot control block
	 */
	BOOTENV->db_flag = btcntlp->bc_db_flag;
	BOOTENV->bootsize = (paddr_t) ptob(btopr((paddr_t)&end)) ;
	BOOTENV->memrng_updated = memrng_updated;

	BOOTENV->memrngcnt = btcntlp->bc_memrngcnt;
	for (i = 0; i < (int)btcntlp->bc_memrngcnt; i++) 
		BOOTENV->memrng[i] = btcntlp->bc_memrng[i];


	/* initialize the loader */
	btload_init();
	/*
	 * DCMP - loading -initialization
	 * This must come first allowing sip, mip, et. al to be compressed.
	 */
	if ((btload(&lpcb[DCMP]) != FAILURE) && 
		(( (int (*)()) lpcb[DCMP].lp_entry) (&bf, &lpcb[DCMP]) == SUCCESS))
		bt_dcmp_init();

	/*
	 * SIP - loading
	 */
	if (btload(&lpcb[SIP]) == FAILURE) 
		bootabort("Fatal error loading SIP");

	/*
	 * SIP - initialization
	 */
	( (int (*)()) lpcb[SIP].lp_entry) (&bf, SIP_INIT, &lpcb[SIP], btcntlp);

	if( bf.logo(DISPLAY_LOGO) == E_OK)
		logo_up = SUCCESS;
	else
		logo_up = FAILURE;

	/*
	 * MIP - loading
	 */
	if (btload(&lpcb[MIP]) == FAILURE) 
		bootabort("Fatal error loading MIP");

#ifdef BOOT_DEBUG2
	if (BOOTENV->db_flag & BOOTDBG) {
		printf("Init SIP: entry= 0x%x MIP: entry= 0x%x\n", 
			lpcb[SIP].lp_entry, lpcb[MIP].lp_entry);
		printf("part_start= 0x%x sbml= 0x%x bps= %d spt= %d spc= %d\n",
			part_start, secboot_mem_loc, bps, spt, spc);
		printf("bootdbflags = %x\n", BOOTENV->db_flag);
		printf("bootmsg= %s \n", btcntlp->bootmsg);
		for (i = 0; i < (int)btcntlp->bc_memrngcnt; i++) 
			printf("btcntl_mrng= 0x%x\n",
				btcntlp->bc_memrng[i].base);
		printf("ADDR:bootenv %x, avail %x, used %x, bend %x[%x]\n",
			BOOTENV, &BTE_INFO.memavail[0],
			&BTE_INFO.memused[0], &end, BOOTENV->bootsize);
		printf("ADDRESS: _start= 0x%x main= 0x%x goprot= 0x%x \n",
			_start, main, goprot);
		printf("PRI_REGS: stack_ptr= 0x%x\n", &status);
		goany();
	}
#endif

#ifdef BOOT_DEBUG
	if (BOOTENV->db_flag)
		goany();
#endif

#ifdef HDTST
	/* switch from floppy to hard disk */
	BL_file_init( hdisk_id );
	vxvm_get_diskid(diskid);

#endif /* HDTST */

	/*
	 * MIP - initialization
	 */
	( (int (*)()) lpcb[MIP].lp_entry) (&bf, MIP_INIT, &lpcb[MIP]);

	/*
	 * --------------------------------------------------------
	 * N.B. all calls to bt_malloc() must have been executed by
	 * this point.  FAILURE to follow this rule will potentially
	 * cause memory corruption via memtest() and bload().
	 *
	 * The first used memory segment is the boot (us). This will get 
	 * changed later to an extent of RESERVED_SIZE.
	 */

	/*
	 * SIP - prepare kernel loading
	 */
	( (int (*)()) lpcb[SIP].lp_entry) (&bf, SIP_KPREP, &lpcb[SIP]);

	BTE_INFO.memused[0].base = 0L;
	BTE_INFO.memused[0].extent = BOOTENV->bootsize;
	BTE_INFO.memused[0].flags = B_MEM_BOOTSTRAP;

	/*
	 * If the value of memrng and memrngcnt in bootcntl are other than the
	 * default value, make sure to reset memrng and memrngcnt in BOOTENV
	 * after MIP initialization is compleated since MIP can change these
	 * values. 
	 * The values set by MIP should prevail iff the values of memrng
	 * and memrngcnt in bootcntl are the default value.
	 */
	if ( BOOTENV->memrng_updated == TRUE )
	{
		BOOTENV->memrngcnt = btcntlp->bc_memrngcnt;
	        for (i = 0; i < (int)btcntlp->bc_memrngcnt; i++)
	                BOOTENV->memrng[i] = btcntlp->bc_memrng[i];
	}


	save_fs_type = boot_fs_type;
	save_delta = boot_delta;

	/*
	 * KERNEL loading: present the bootmsg, bootprompt, etc.
	 */
	status = SUCCESS;
	do {
		if ( status == FAILURE ){
			parse_user(btcntlp);
			if( bf.logo(DISPLAY_LOGO) == E_OK)
				logo_up = SUCCESS;
			else{
				logo_up = FAILURE;
				printf("\n%s \n", btcntlp->bootmsg);
			}
			longjmp(setjmpenv,3);
		}

		kb_flush();

		if( (status = ( (int (*)()) lpcb[SIP].lp_entry) (&bf, SIP_KLOAD, &lpcb[KERNEL])) == SUCCESS)
			/* load the resource database */
			status = ((int (*)()) lpcb[SIP].lp_entry) (&bf, SIP_KLOAD, &lpcb[RM_DATABASE]);

#ifdef SFBOOT

/* NOT_YET_MEMFSROOT
		if (( (int (*)()) lpcb[SIP].lp_entry) (&bf, SIP_KLOAD, &lpcb[MEMFSROOT_META]) == FAILURE)
			bootabort("Error loading MEMFS METADATA");
		else
			if (( (int (*)()) lpcb[SIP].lp_entry) (&bf, SIP_KLOAD, &lpcb[MEMFSROOT_FS]) == FAILURE)
				bootabort("Error loading MEMFS FILESYSTEM DATA");
*/

#endif /* SFBOOT */
	} while (status == FAILURE);

	/*
	 * Copy the booted program name into bargv[0]
	 */
        dest = BTE_INFO.bargv[0];
	fname = btcntlp->bootstring;

        if (boot_fs_type == BFS) {
                /*
                 * Since the bfs file code in the boot code ignores
                 * everything except the final component in the path,
                 * I need to do the same.
                 */
                for (bootstr = btcntlp->bootstring; *bootstr != '\0';) {
                        if (*bootstr++ == '/')
                                fname = bootstr;
                }

                bstrcpy( dest, "/stand/" );
                dest += bstrlen( "/stand/" );
        }

        bstrcpy(dest, fname);

#ifdef BOOT_DEBUG
	if (BOOTENV->db_flag & BOOTTALK) {
		printf("boot: kernel dread_cnt= %d\n", dread_cnt);
		for (i = 0; i < (int)BTE_INFO.bargc; i++) 
			printf("[%d] argv= %s\n", i, BTE_INFO.bargv[i]);
		goany();
	}
#endif

	/*
	 * MIP - execute final machine setup procedure
	 */
	((int (*)()) lpcb[MIP].lp_entry) (&bf, MIP_END, &lpcb[MIP]);

	if ( logo_up == SUCCESS ){
		bf.logo(REMOVE_LOGO);
		logo_up = FAILURE;
	};

	/*
	 * invoke the system startup routine
	 */
	((int (*)()) lpcb[SIP].lp_entry) (&bf, SIP_KSTART, &lpcb[KERNEL]);
}


/*
 * Flush keyboard
 */

kb_flush()
{
	while (ischar())
		(void)bgetchar();
}


/*
 * Terminate boot program
 */
void
bootabort(reason)
char	*reason;
{
	printf("%s\n",reason);
	kb_flush();
	parse_user(btcntlp);
	if( bf.logo(DISPLAY_LOGO) == E_OK)
		logo_up = SUCCESS;
	else{
		logo_up = FAILURE;
		printf("\n%s \n", btcntlp->bootmsg);
	}
	longjmp(setjmpenv,6);
}

extern char		*getdef();
extern unsigned long	atol();

#define DEFERROR(x)	printf("\nboot: %s argument missing or incorrect\n", x)

#define SET_BY_BOOT	(B_MEM_BOOTSTRAP|B_MEM_KTEXT|B_MEM_KDATA)


void
write_bootcntl( btcntlp , buf)
struct bootcntl *btcntlp;
char	*buf;
{
	int	i,n;
	int	memrngcnt;
	struct	bootmem	*mp;
	char	*p, *q;
	unsigned long	t;
	char	parmsbuf[B_STRSIZ];

	/* 
	 * search for valid options; this is inefficient, but
	 * at least it's relatively clean
	 */

	/* if reached limit or it's a comment, skip it */
	if ( BTE_INFO.bargc == B_MAXARGS || buf[0] == '#' )
		return;

	/*
	 * Copy buffer to argv before doing anything else,
	 * but only increment argc if it's OK.
	 * Thus, we can stomp on buf during the parsing.
	 */

	bstrncpy(BTE_INFO.bargv[BTE_INFO.bargc], buf, B_STRSIZ);

	if ((p = getdef(buf, "BOOTMSG")) != NULL) {

		bstrncpy(btcntlp->bootmsg, p, BC_STRSIZ);
		btcntlp->bootmsg[BC_STRSIZ - 1] = '\0';

	} else if ((p = getdef(buf, "BOOTPROMPT")) != NULL) {

		bstrncpy(btcntlp->bootprompt, p, B_STRSIZ);

#if defined(WINI) | defined(HDTST)
	} else if ((p = getdef(buf, "KERNEL")) != NULL) {

		bstrncpy(btcntlp->bootstring, p, B_PATHSIZ);
		btcntlp->bootstring[B_PATHSIZ - 1] = '\0';

#ifndef HDTST
	} else if ((p = getdef(buf, "MIP")) != NULL) {

		bstrncpy(btcntlp->mip, p, B_PATHSIZ);
		btcntlp->mip[B_PATHSIZ - 1] = '\0';

	} else if ((p = getdef(buf, "SIP")) != NULL) {

		bstrncpy(btcntlp->sip, p, B_PATHSIZ);
		btcntlp->sip[B_PATHSIZ - 1] = '\0';
#endif

	} else if ((p = getdef(buf, "RESMGR")) != NULL) {

		bstrncpy(btcntlp->rmdatabase, p, B_PATHSIZ);
		btcntlp->rmdatabase[B_PATHSIZ - 1] = '\0';
		BTE_INFO.bargc++;	/* add to kernel args as well */

#ifdef HDTST
	} else if ((p = getdef(buf, "DISK")) != NULL) {

		if (*(p+1) == ':') {
			i = *p;
			if (i >= 'c' && i <= 'z') {
				hdisk_id = BOOTDRV_HD + i - 'c';
			} else if (i >= 'C' && i <= 'Z') {
				hdisk_id = BOOTDRV_HD + i - 'C';
			} else {
				printf("Boot drive must be C: through Z:\n");
			}
		}else
			printf("Boot drive must be C: through Z:\n");
#endif
	} else if ((p = getdef(buf, "AUTOBOOT")) != NULL) {

		if ( bstrncmp(p, "YES", 3) == 0 )
			btcntlp->autoboot = TRUE;
		else if ( bstrncmp(p, "NO", 2) == 0 )
			btcntlp->autoboot = FALSE;
		else
			DEFERROR("AUTOBOOT");
	} else if ((p = getdef(buf, "SLOWBOOT")) != NULL) {

		if ( (t = atol(p)) < 0 )
			DEFERROR("SLOWBOOT");
		else
			slow_boot = (int)t;

	} else if ((p = getdef(buf, "TIMEOUT")) != NULL) {

		if ( (t = atol(p)) < 0 )
			DEFERROR("TIMEOUT");
		else
			btcntlp->timeout = (int)t;

#endif	/* WINI | HDTST */
	} else if ((p = getdef(buf, "MEMRANGE")) != NULL) {

		memrng_updated = TRUE;
		memrngcnt = 0;
		q = bstrtok(p, "-");

		do {	
			mp = &btcntlp->bc_memrng[memrngcnt];

			/* start of the range */

			if (q == NULL) {
				DEFERROR("MEMRANGE");
				break;
			}
			if ((t = atol(q)) == -1L) {
				DEFERROR("MEMRANGE");
				break;
			}
			mp->base = (paddr_t)ptob( btop(t) );

			/* end of the range */

			if ( (q = bstrtok(NULL, ":")) == NULL ) {
				DEFERROR("MEMRANGE");
				break;
			}

			if ( (t = atol(q)) == -1L) {
				DEFERROR("MEMRANGE");
				break;
			}
			mp->extent = ptob( btop(t - (long)mp->base) );

			/* flags */

			if ( (q = bstrtok(NULL, ",\n")) == NULL) {
				DEFERROR("MEMRANGE");
				break;
			}
			if ( (t = atol(q)) == -1L) {
				DEFERROR("MEMRANGE");
				break;
			}
			mp->flags = (ushort)t & ~SET_BY_BOOT;

			memrngcnt++;
			q = bstrtok(NULL, "-");

#ifdef BOOT_DEBUG
printf("write_bootcntl:  base %x, extent %x, flags %x\n",mp->base, mp->extent, mp->flags);
printf("memrng_updated: %s\n",BOOTENV->memrng_updated == TRUE ? "TRUE" : "FALSE");
printf("memrng_updated: 0x%x\n",BOOTENV->memrng_updated);
goany();
#endif
		} while ( (q != NULL) && (memrngcnt < B_MAXMEMR) );

		btcntlp->bc_memrngcnt = memrngcnt;

	/*
	 * If the string doesn't seem to be well formed, 
	 * punt silently, so that we don't yell 
	 * when processing /etc/TIMEZONE shell scripts.
	 */

	} else if (getdef(buf, NULL) == NULL) {
		printf("Format error: PARM=value\n"); 
	} else 
		BTE_INFO.bargc++;
}

void
update_bootcntl( btcntlp )
struct bootcntl *btcntlp;
{
	off_t	offset = 0;
	int	n;
	int	status;
	char	*dest;
	char	parmsbuf[B_STRSIZ];

	BL_file_open(standboot, &status);
	if (status == E_OK)
		for( ; ((n = bfgets(parmsbuf, B_STRSIZ, offset)) != 0); offset += n+1 )
			write_bootcntl(btcntlp, parmsbuf);
}



/* 
 * get default entry from string buffer.
 * returns a pointer to the arguments associated with key,
 * NULL if key is not present or argument(s) are missing.
 */

char *
getdef(buf, key)
char	*buf;
char	*key;
{
	if ( (key != NULL) && (bstrncmp( buf, key, bstrlen(key)) != 0) )
		return(NULL);

	/* find beginning of arg string, and return a pointer to it */

	for ( ; *buf; buf++ )
		if ( (*buf == '=') )
			return( buf+1 );

	/* if not found, return NULL */

	return( NULL );
}


/* 
 * sort of, but not exactly, like the libc atol().
 * We extract a positive integer from the string s,
 * allowing for the 'k' (*1024) and 'm' (*1024^2)
 * multiplier abbreviations.
 * returns the integer if successful, (unsigned long)-1L if not.
 */

unsigned long
atol( p )
register char	*p;
{
	register unsigned long n;

	if ( *p == 0 )
		return(-1L);

	/* gobble white space */

	while ((*p == ' ') || (*p == '\t'))
		p++;

	/* grab digits */

	n = 0;
	while ( (*p >= '0') && (*p <= '9') ) 
		n = n * 10 + *p++ - '0';

	/* modifiers */

	switch( *p ) {
	case ('M'):
	case ('m'):
		n *= 1024;
	case ('K'):
	case ('k'):
		n *= 1024;
		p++;
	}

	return( ((*p == '\0') || (*p == '\n')) ? n : -1L );
}

no_op()
{
	/* dummy routine for sundry uses .... doesn't do much */
	return(~E_OK);
}

checkkbd ()
{
return(ischar());
}
#define UNKCMD 0
#define LIST 1
#define EXIT 2
#define HELP 3
#define REMOVE 4

parse_user(btcntlp)
struct bootcntl *btcntlp;
{
	char	inline[B_STRSIZ], dirname[20], dskname[2];
	struct	bootmem	*mp;
	int	memrngcnt, t, cnt, found_user=FAILURE;
	int entry;

	printf("\n%s ",btcntlp->bootprompt);
	kb_flush();
	while (1){
		printf("\n[boot]# ");
		for (t = btcntlp->timeout; !ischar() &&
			(btcntlp->timeout == 0 || t-- > 0 || found_user==SUCCESS );)
			wait1s();

		if (!ischar())
			return;

		found_user = SUCCESS;
		if ( bgets(inline, B_STRSIZ) != 0) {
			switch (gettok(inline))
			{
			case LIST:

#if defined(WINI) | defined(HDTST)
				printf("KERNEL=%s\n",btcntlp->bootstring);
#ifndef HDTST
				printf("MIP=%s\n",btcntlp->mip);
				printf("SIP=%s\n",btcntlp->sip);
#endif
				printf("RESMGR=%s\n",btcntlp->rmdatabase);
#ifdef HDTST
				dskname[0]=hdisk_id-0x3d;
				dskname[1]='\0';
				printf("DISK=%s:\n",dskname);
#endif
#endif

				memrngcnt = 0;
				printf("MEMRANGE:\n");
				do{
					mp = &btcntlp->bc_memrng[memrngcnt++];
					printf("    %d MEMBASE: 0x%x EXTENT: 0x%x FLAGS: 0x%x\n",memrngcnt,mp->base,mp->extent,mp->flags);
				} while(memrngcnt<(int)btcntlp->bc_memrngcnt);

				for (t=0; t < BTE_INFO.bargc; t++)
					printf("%s\n",BTE_INFO.bargv[t]);

#if !defined(SFBOOT)
				printf("\nfiles in /stand...\n");
				cnt = 0;
				entry = 2;
				while ((t = BL_file_readdir(entry++, dirname, sizeof(dirname))) != -1){
					dirname[14] = '\0';
					if (cnt++ == 6) {
						printf("\n");
						cnt = 0;
					}
					printf("  %s", dirname);
				};
				printf("\n");
#endif
				
				break;
			case REMOVE:
				if (BTE_INFO.bargc > 1)
					BTE_INFO.bargc--;
				break;
			case EXIT:
				return 0;
				break;
			case HELP:
				printf("BOOT commands:\nlist -- list parameters/values\ngo -- continue system loading.\nremove -- deletes last system parameter \n? -- help\n");
				break;
			default:
				write_bootcntl( btcntlp, inline);
				break;
			}
		}
	
	}

}

gettok(buf)
char buf[];
{
	if (bstrncmp("list", buf, 5) == 0){
		return(LIST);
	}
	if (bstrncmp("remove", buf, 7) == 0){
		return(REMOVE);
	}
	if (bstrncmp("go", buf, 3) == 0){
		return(EXIT);
	}
	if (bstrncmp("?", buf, 2) == 0){
		return(HELP);
	}
	return(UNKCMD);
}

#ifdef HDTST
int
vxvm_get_diskid(diskid)
	char *diskid;
{
	char dgname[NAME_LEN + 1];
	char buf[NBPSCTR];
	char *bufp;
	char *dest;

	/*
	 * search for a VxVM private partition by tag.  If one is found
	 * on the boot disk, get the diskid and pass that into the
	 * kernel in the argument list.
	 */

	if (vxvm_priv_slice < 0) {
		return 0;
	}

	disk(vxvm_priv_delta, (paddr_t)buf, (short)1);
	bufp = buf;

	if (bstrncmp(bufp, "PRIVHEAD", sizeof("PRIVHEAD") - 1) != 0) {
		return 0;
	}

	bufp += sizeof("PRIVHEAD") - 1 + VOL_ULONG_BYTES +
		2 * VOL_USHORT_BYTES +
		VOL_ULONG_BYTES + VOL_SEQNO_BYTES +
		2 * VOL_VOFF_BYTES;
	memcpy(diskid, bufp, VOL_UUID_LEN);
	diskid[VOL_UUID_LEN] = '\0';
	bufp += 3 * VOL_UUID_LEN;
	memcpy(dgname, bufp, NAME_LEN);
	dgname[NAME_LEN] = '\0';

	if (bstrncmp(dgname, "rootdg", NAME_LEN) == 0) {
		dest = BTE_INFO.bargv[BTE_INFO.bargc++];
		bstrcpy(dest, "VXDISKID=");
		memcpy(dest + sizeof("VXDISKID=") - 1,
		       diskid, B_STRSIZ - sizeof("VXDISKID="));
		if (bstrlen(diskid) > B_STRSIZ - sizeof("VXDISKID=")) {
			dest = BTE_INFO.bargv[BTE_INFO.bargc++];
			bstrcpy(dest, "VXDISKID2=");
			bstrcpy(dest + sizeof("VXDISKID2=") - 1,
			       diskid + B_STRSIZ - sizeof("VXDISKID="));
		}
		return 1;
	}else
		return 0;
}
#endif
