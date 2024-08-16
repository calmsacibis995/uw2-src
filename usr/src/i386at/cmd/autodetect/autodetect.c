/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)autodetect:autodetect.c	1.10"

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/corollary.h>
#include <sys/cbus.h>

#define M_FLG_SRGE		1       /* sig scattered in a range of memory */
#define MAX_SIGNATURE_LEN	10
#define MAX_DESCRIPTION_LEN	40

struct machconfig {
        char    *sigaddr;       /* machine signature location   */
        size_t   range;         /* signature search range       */
        unsigned char   sigid[MAX_SIGNATURE_LEN]; /* signature to match */
        unsigned char   m_flag;      /* search style flag */
        unsigned char   name[MAX_DESCRIPTION_LEN]; /* desc. of the machine */
	int	rval;		/* return value */
};

#define NULL	0

struct machconfig mconf[] = {
	(char *)0x9fc00, 0x400, "_MP_", M_FLG_SRGE, "PC+MP", 6,
	(char *)0xf0000, 0xffff, "_MP_", M_FLG_SRGE, "PC+MP", 7,
	(char *)0xfffe0000, 0xffff, "Corollary", M_FLG_SRGE, "Cbus", 0,
        (char *)0xfffe4, 0x100, "COMPAQ", M_FLG_SRGE, "Compaq", 1,
	(char *)0xf0000, 0xffff, "EBI2", M_FLG_SRGE, "AST Manhattan", 2,
	(char *)0xfe000, 0x100, "ACER", 0, "Acer Frame3000MP", 3,
	(char *)0xf4000, 0xffff, "ACER", 0, "Acer Altos", 3,
	(char *)0xf0800, 0x100, "Tricord", M_FLG_SRGE, "Tricord MP", 4,
	(char *)0xfe076,0x200, "Advanced L", M_FLG_SRGE, "ALR machine", 5,
        (char *)0xfffe0000, 0x100, "OLIVETTI", M_FLG_SRGE, "Olivetti", 8
};

#define NSIGS   (sizeof(mconf)/sizeof(struct machconfig))

extern int errno;

void	*bufp;
int	fdmem, fdpmem;
#ifdef DEBUG
int	wfd;
#endif

char *
membrk(char *s1, char *s2, int n1, int n2)
{
        char    *os = s1;
        int     n;

        for (n = n1 - n2 ; n >= 0; n--) {
                if (memcmp(s1++, s2, n2) == 0) {
                        return(s1);
                }
        }
        return(0);
}

void
cleanup()
{
	if (fdpmem != -1)
		close(fdpmem);

	if (fdmem != -1)
		close(fdmem);

#ifdef DEBUG
	if (wfd != -1)
		close(wfd);
#endif

	if (bufp != NULL)
		free(bufp);

	return;

}

main(argc, argv)
int argc;
char *argv[]; 
{
	int i;

	bufp = NULL;

	if ((fdpmem = open("/dev/pmem", O_RDONLY)) == -1)
		printf("auto_detect:/dev/pmem open fails! errno=%d\n", errno);

	if ((fdmem = open("/dev/mem", O_RDONLY)) == -1)
		printf("auto_detect:/dev/mem open fails! errno=%d\n", errno);

	if (fdmem == -1 && fdpmem == -1)
		printf("auto_detect:Cannot open /dev/[p]mem. Operation aborted");

#ifdef DEBUG
	if ((wfd = open("./outfile", O_RDWR|O_CREAT)) == -1)
		printf("auto_detect:./outfile open fails! errno=%d\n", errno);
#endif


        for (i = 0; i < NSIGS; i++) {

		if ((bufp = malloc(mconf[i].range)) == NULL) {
			printf("auto_detect:malloc for i=%d fails.\n", i);
			cleanup();
			return(-1);
		}

		if (lseek(fdpmem, mconf[i].sigaddr, 0) != -1) {
			if (read(fdpmem, bufp, mconf[i].range) != -1)
				goto search;
		}

		if (lseek(fdmem, mconf[i].sigaddr, 0) == -1) {
			printf("auto_detect:lseek fails for i=%d.\n", i);
			cleanup();
			return(-1);
		} else {
			if (read(fdmem, bufp, mconf[i].range) == -1) {
				printf("auto_detect:read fails for i=%d.\n", i);
				cleanup();
				return(-1);
			}
		}

search:

#ifdef DEBUG
		write(wfd, bufp, mconf[i].range);
		write(wfd, "\n\n", 2);
#endif

		if (mconf[i].m_flag & M_FLG_SRGE) {

			if ((membrk((void *)bufp, (void *)mconf[i].sigid,
			     mconf[i].range, strlen(mconf[i].sigid))) != 0)  {	/* found the footprint */

				if (strcmp("Corollary", mconf[i].sigid) == 0) {
#ifdef DEBUG
					printf("auto_detect:Find %s\n", 
						mconf[i].name);
#endif
					iscorollary(fdpmem, fdmem, 
							argc, argv, i);
					/* NOTREACHED */

				} else {	/* !Corollary */
					if (strcmp("OLIVETTI",
					   (char *)&(mconf[i].sigid[0])) == 0) {

						uchar_t mach_type, *add = 0xffffd;

						lseek(fdmem, add, SEEK_SET);
						read(fdmem, &mach_type, 1);
						if (mach_type != 0x71) {
							printf("\nOlivetti PSM for ");
							printf("line 50xx only ");
							printf("supports 5050 model ");
							printf("(Pentium, APIC).\n\n");
							exit(-1);
						} else {
#ifdef DEBUG
							printf("auto_detect:Find %s\n", mconf[i].name);
#endif
							cleanup();
							return(mconf[i].rval);
						}

					} else {	/* !OLIVETTI */
						if (strcmp("COMPAQ", mconf[i].sigid) != 0) {
#ifdef DEBUG
							printf("auto_detect:Find %s\n", mconf[i].name);
#endif
							cleanup();
							return(mconf[i].rval);
						} else {	/* !COMPAQ */
							if (*(char *)bufp == 'E') {
#ifdef DEBUG
								printf("auto_detect:Find %s MP\n", mconf[i].name);
#endif
								cleanup();
								return(mconf[i].rval);
							} else {
#ifdef DEBUG
								printf("auto_detect:Find %s UP\n", mconf[i].name);
#endif
								break;
							}
						}
					}	/* !OLIVETTI */
				}	/* !Corollary */
                        }	/* found the footprint */
                } else {	/* !M_FLG_SRGE */
                        if (memcmp((void *)bufp, (void *)mconf[i].sigid,
                            strlen(mconf[i].sigid)) == 0) {

				if (strcmp("COMPAQ", mconf[i].sigid) != 0) {
#ifdef DEBUG
					printf("auto_detect:Find %s\n", mconf[i].name);
#endif
					cleanup();
					return(mconf[i].rval);
				} else {
					if (*(char *)bufp == 'E') {
#ifdef DEBUG
						printf("auto_detect:Find %s MP\n", mconf[i].name);
#endif
						cleanup();
						return(mconf[i].rval);
					 } else {
#ifdef DEBUG
						printf("auto_detect:Find %s UP\n", mconf[i].name);
#endif
						break;
					}
				}
                        }	/* found the footprint */
                }	/* !M_FLG_SRGE */

		free(bufp);
		bufp = NULL;
        }

	cleanup();
	return(-1);
}

unsigned print_debug;

unsigned char *
get_struct(fd1, fd2, location, size)
int fd1, fd2;
unsigned location;
unsigned size;
{
	int		good_fd;
	unsigned char	*mem;

	if (lseek(fd1, location, 0) != -1) {
		good_fd = fd1;
	} else if (lseek(fd2, location, 0) != -1) {
		good_fd = fd2;
	} else {
		printf("auto_detect:lseek fails for seek=0x%x.\n", location);
		cleanup();
		exit(-1);
	}

	mem = (unsigned char *)malloc(size);
	
	if (mem == NULL) {
		printf("auto_detect:malloc for size=%d fails.\n", size);
		cleanup();
		exit(-1);
	}

	if (read(good_fd, mem, size) != size) {
		printf("auto_detect: read failed\n", size);
		cleanup();
		exit(-1);
	}

	return mem;
}

free_struct(ptr)
unsigned *ptr;
{
	free(ptr);
}

iscorollary(fd1, fd2, argc, argv, indx)
int fd1, fd2;
int argc;
char *argv[];
int indx;
{
	unsigned char		*rrd_ram;
	unsigned		corollary_string = 0xdeadbeef;
	struct configuration	*config_ptr;
	int			type;

	rrd_ram = get_struct(fd1, fd2, RRD_RAM, 0x8000);

	if (argc == 2)
		if (strcmp(argv[1], "-d") == 0)
			print_debug = 1;

	/* 
	 * search the ram for the info check word 
	 */
	config_ptr = (struct configuration *)
		membrk(rrd_ram, &corollary_string, 0x8000, sizeof(int));

	if (config_ptr == 0) {
		printf("autodetect: can't find identifier\n");
		free_struct(rrd_ram);
		cleanup();
	}

	config_ptr = (struct configuration *)((char *)config_ptr - 1);

	type = corollary_find_processors(config_ptr, indx);

	free_struct(rrd_ram);
	cleanup();
	exit(type);
}

int
corollary_find_processors(cptr, indx)
struct configuration	*cptr;
int indx;
{
	struct ext_cfg_header		*ptr_header;
	struct ext_memory_board		*mem_ptr = NULL;
	struct ext_cfg_override		*cfg_ptr = NULL;
	char				*ptr_source;
	int				type;

	ptr_source = (char *)cptr;

	ptr_source += sizeof(configuration);
	ptr_header = (struct ext_cfg_header *)ptr_source;

	if (*(unsigned *)ptr_source == EXT_CHECKWORD) {

		do {
			ptr_source += sizeof(struct ext_cfg_header);

			switch (ptr_header->ext_cfg_checkword) {
			case EXT_MEM_BOARD:
#ifdef DEBUG
				printf("found EXT_MEM_BOARD\n");
#endif
				mem_ptr = (struct ext_memory_board *)ptr_source;
				break;
			case EXT_CHECKWORD:
			case EXT_VENDOR_INFO:
				break;
			case EXT_CFG_OVERRIDE:
#ifdef DEBUG
				printf("found EXT_CFG_OVERRIDE\n");
#endif
				cfg_ptr = (struct ext_cfg_override *)ptr_source;
				break;
			case EXT_ID_INFO:
#ifdef DEBUG
				printf("found EXT_ID_INFO\n");
#endif
				type = corollary_read_ext_ids(
					(struct ext_id_info *)ptr_source);

				return type;
			case EXT_CFG_END:
				break;
			default:
				break;
			}
			
			ptr_source += ptr_header->ext_cfg_length;
			ptr_header = (struct ext_cfg_header *)ptr_source;

		} while (ptr_header->ext_cfg_checkword != EXT_CFG_END);
	}

	/*
	 * This is a pre-XM ROM.
	 */

	/* return the mconf table index value. */
	return mconf[indx].rval;
}

char *cpu_type[] = {
	"None",
	"386",
	"486",
	"Pentium"
};

unsigned cpu_type_num = sizeof(cpu_type) / sizeof(char *);


char *io_type[] = {
	"No I/O ",
	"SIO    ",
	"SCSI   ",
	"SYM    ",
	"Bridge ",
	"Bridge ",
	"----   ",
	"----   ",
	"----   ",
	"Memory "
};

unsigned io_type_num = sizeof(io_type) / sizeof(char *);

#define IS_SYMMETRIC	(ELEMENT_HAS_CBC | ELEMENT_HAS_APIC)

/*
 * read in the extended id information table.  filled in
 * by some of our C-bus licensees, and _all_ of the C-bus2
 * machines.
 */
int
corollary_read_ext_ids(p, indx)
struct ext_id_info *p;
int indx;
{
	register int i;
	char *s;
	unsigned all_sym = 1;
	unsigned any_sym = 0;

	if (print_debug) {
		printf("\nid\ttype\tattr\tfunct\tattr     start    ");
		printf("size     feature\n");
		printf(  "--\t----\t----\t-----\t----     -----    ");
		printf("----     -------\n");
	}

	for (i = 0; i < SMP_MAX_IDS && p->id != 0x7f; i++, p++) {

		if (i == 0)
			continue;

		if ((p->pm == 0) && (p->io_function == IOF_INVALID_ENTRY))
			continue;

		if (print_debug) {
			printf("0x%x\t", p->id);

			if (p->proc_type < cpu_type_num)
				s = cpu_type[p->proc_type];
			else
				s = "UNKNOWN";

			printf("%s\t", s);

			if (p->proc_attr & PA_CACHE_ON) {
				printf("+cache\t");
			} else {
				if (p->id)
					printf("-cache\t");
				else
					printf("      \t");
			}

			if (p->io_function < io_type_num)
				s = io_type[p->io_function];
			else
				s = "UNKNOWN";

			printf("%s\t", s);

			printf("%08x ", p->io_attr);

			printf("%08x ", p->pel_start);

			printf("%08x ", p->pel_size);

			if (p->pel_features & IS_SYMMETRIC)
				printf("SYMMETRIC");
			else
				printf("         ");

			printf("\n");
		}

		if (p->id) {
			if ((p->pel_features & IS_SYMMETRIC) == 0)
				all_sym = 0;
			else
				any_sym = 1;
		}
	}

	if (print_debug)
		printf("\nMachine type: ");

	if (all_sym) {
		if (print_debug)
			printf("ALL SYMMETRIC\n");
		return 101;
	}
	
	if (any_sym) {
		if (print_debug)
			printf("MIXED\n");
		return 102;
	}
		
	if (print_debug)
		printf("CBUS OR CBUS XM\n");

	/* return the mconf table index value */
	return mconf[indx].rval;
}
