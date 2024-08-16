/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libresmgr:libresmgr.c	1.11"

#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<stdlib.h>
#include	<string.h>
#include	<errno.h>
#include	<sys/resmgr.h>
#include	<sys/confmgr.h>
#include	<sys/cm_i386at.h>

#define	MODNAME		1
#define	UNIT		2
#define	IPL		3
#define	ITYPE		4
#define	IRQ		5
#define	IOADDR		6
#define	MEMADDR		7
#define	DMAC		8
#define	BINDCPU		9
#define	BRDBUSTYPE	10
#define	BRDID		12
#define	SLOT		13
#define	ENTRYTYPE	14
#define	BOOTHBA		15
#define	UNKPARAM	16

#define	UNK_VAL		1
#define	STR_VAL		2
#define	NUM_VAL		3
#define	RNG_VAL		4

#define	VB_ILEN		256

#define	MATCH(s1, s2)	(strcmp(s1, s2) == 0)
#define	NOVAL(p)	(!p || (*p == '-' && *(p+1) == '\0'))

static	void	*val_buf;
static	size_t	vb_len;

static	int	rm_fd = -1;

static char *
getval(char *s, char **ns, char delim)
{
	char	*sp2;

	if(s == NULL) {
		*ns = NULL;
		return(NULL);
	}

	for(sp2 = s; *sp2 == delim; sp2++);

	if(*sp2 == '\0') {
		*ns = NULL;
		return(NULL);
	}

	s = sp2;

	for(sp2 = s; *sp2 != delim && *sp2 != '\0'; sp2++);

	if(*sp2 == '\0') {
		*ns = NULL;
	} else {
		*sp2++ = '\0';
		*ns = sp2;
	}

	return(s);
}

static int
param_type(char *param)
{
	switch(*param) {

		case 'B':
			if(MATCH(param, CM_BRDBUSTYPE)) {
				return(BRDBUSTYPE);
			}
			if(MATCH(param, CM_BRDID)) {
				return(BRDID);
			}
			if(MATCH(param, CM_BINDCPU)) {
				return(BINDCPU);
			}
			if(MATCH(param, CM_BOOTHBA)) {
				return(BOOTHBA);
			}
			return(UNKPARAM);

		case 'D':
			if(MATCH(param, CM_DMAC)) {
				return(DMAC);
			}
			return(UNKPARAM);

		case 'E':
			if(MATCH(param, CM_ENTRYTYPE)) {
				return(ENTRYTYPE);
			}
			return(UNKPARAM);

		case 'I':
			if(MATCH(param, CM_IRQ)) {
				return(IRQ);
			}
			if(MATCH(param, CM_IOADDR)) {
				return(IOADDR);
			}
			if(MATCH(param, CM_IPL)) {
				return(IPL);
			}
			if(MATCH(param, CM_ITYPE)) {
				return(ITYPE);
			}
			return(UNKPARAM);

		case 'M':
			if(MATCH(param, CM_MEMADDR)) {
				return(MEMADDR);
			}
			if(MATCH(param, CM_MODNAME)) {
				return(MODNAME);
			}
			return(UNKPARAM);

		case 'S':
			if(MATCH(param, CM_SLOT)) {
				return(SLOT);
			}
			return(UNKPARAM);

		case 'U':
			if(MATCH(param, CM_UNIT)) {
				return(UNIT);
			}
			return(UNKPARAM);

		default:
			return(UNKPARAM);
	}
}

static int
val_type(char *param)
{
	char	*pp;

	for(pp = param; *pp != ',' && *pp != '\0'; pp++);

	if(*pp == ',') {
		*pp = '\0';

		switch(*(pp+1)) {
			case 'n':
				return(NUM_VAL);

			case 'r':
				return(RNG_VAL);

			case 's':
				return(STR_VAL);

			default:
				return(UNK_VAL);
		}
	}

	switch(param_type(param)) {

		case UNIT:
		case IPL:
		case ITYPE:
		case IRQ:
		case DMAC:
		case BINDCPU:
		case BRDBUSTYPE:
		case SLOT:
		case ENTRYTYPE:
		case BOOTHBA:
			return(NUM_VAL);

		case MODNAME:
		case BRDID:
			return(STR_VAL);

		case IOADDR:
		case MEMADDR:
			return(RNG_VAL);

		default:
			return(UNK_VAL);
	}
}

static int
readparam(rm_key_t key, const char *param, int n)
{
	struct	rm_args	rma;
	void	*tvb;
	int	rc;

	if(strlen(param) >= RM_MAXPARAMLEN) {
		return(EINVAL);
	}

	strcpy(rma.rm_param, param);

	rma.rm_key = key;
	rma.rm_val = val_buf;
	rma.rm_vallen = vb_len;
	rma.rm_n = n;

	if(ioctl(rm_fd, RMIOC_GETVAL, &rma) < 0) {
		if(errno == ENOSPC) {
			if((tvb = malloc(rma.rm_vallen)) == NULL) {
				return(ENOSPC);
			}
			free(val_buf);
			val_buf = tvb;
			vb_len = rma.rm_vallen;
			rma.rm_val = val_buf;

			if(ioctl(rm_fd, RMIOC_GETVAL, &rma) < 0) {
				return(errno);
			}
			return(0);
		}
		return(errno);
	}
	return(0);
}

int
RMopen(int oflag)
{
	if(rm_fd != -1) {
		return(EINVAL);
	}

	if((rm_fd = open(DEV_RESMGR, oflag)) < 0) {
		return(errno);
	}

	if((val_buf = malloc(VB_ILEN)) == NULL) {
		close(rm_fd);
		rm_fd = -1;
		return(ENOSPC);
	}
	vb_len = VB_ILEN;

	return(0);
}

int
RMclose(void)
{
	if(rm_fd < 0) {
		return(EINVAL);
	}

	close(rm_fd);
	rm_fd = -1;
	free(val_buf);

	return(0);
}


int
RMgetfd()
{
	return(rm_fd);
}



int
RMdelkey(rm_key_t key)
{
	struct	rm_args	rma;

	if(rm_fd < 0) {
		return(EINVAL);
	}

	rma.rm_key = key;

	if(ioctl(rm_fd, RMIOC_DELKEY, &rma) < 0) {
		return(errno);
	}

	return(0);
}

int
RMnextkey(rm_key_t *keyp)
{
	struct	rm_args	rma;

	if(rm_fd < 0) {
		return(EINVAL);
	}

	rma.rm_key = *keyp;

	if(ioctl(rm_fd, RMIOC_NEXTKEY, &rma) < 0) {
		return(errno);
	}

	*keyp = rma.rm_key;
	return(0);
}

int
RMnewkey(rm_key_t *keyp)
{
	struct	rm_args	rma;

	if(rm_fd < 0) {
		return(EINVAL);
	}

	if(ioctl(rm_fd, RMIOC_NEWKEY, &rma) < 0) {
		return(errno);
	}

	*keyp = rma.rm_key;
	return(0);
}

int
RMgetvals(rm_key_t key, const char *param_list, int n,
	  char *val_list, int val_size)
{
	char	*pl, *vl, *vp, *param;
	int	nlen, rc, vt;

	if(rm_fd < 0) {
		return(EINVAL);
	}

	if((pl = malloc(strlen(param_list)+1)) == NULL) {
		return(ENOMEM);
	}
	if((vl = malloc(val_size+40)) == NULL) {
		free(pl);
		return(ENOMEM);
	}
	*vl = '\0';

	strcpy(pl, param_list);
	vp = vl;

	param = strtok(pl, " ");

	while(param != NULL) {
		vt = val_type(param);
		rc = readparam(key, param, n);

		switch(vt) {
			case STR_VAL:
				if(rc)
					sprintf(vp, "- ");
				else
					sprintf(vp, "%s ", val_buf);
				break;

			case NUM_VAL:
				if(rc)
					sprintf(vp, "- ");
				else
					sprintf(vp, "%d ", *(int *)val_buf);
				break;

			case RNG_VAL:
				if(rc)
					sprintf(vp, "- - ");
				else
					sprintf(vp, "%lx %lx ",
						((struct cm_addr_rng *)val_buf)->startaddr,
						((struct cm_addr_rng *)val_buf)->endaddr);
				break;

			default:
				free(pl);
				free(vl);
				return(UNKPARAM);
		}
		nlen = strlen(vp);
		if(nlen >= val_size) {
			free(pl);
			free(vl);
			return(ENOMEM);
		}
		vp = vp + nlen;

		param = strtok(NULL, " ");
	}

	if (vp > vl && *--vp == ' ')	{
		*vp = '\0';
	}

	strcpy(val_list, vl);
	free(pl);
	free(vl);
	return(0);
}

int
RMputvals_d(rm_key_t key, const char *param_list,
	    const char *val_list, char delim)
{
	char	*pl, *vl, *param;
	char	*vps, *vpn;
	void	*tvb;
	struct	rm_args	rma;
	int	vt;

	if(rm_fd < 0) {
		return(EINVAL);
	}

	if((pl = malloc(strlen(param_list)+1)) == NULL) {
		return(ENOMEM);
	}
	if((vl = malloc(strlen(val_list)+1)) == NULL) {
		free(pl);
		return(ENOMEM);
	}

	rma.rm_key = key;
	rma.rm_val = val_buf;

	strcpy(pl, param_list);
	strcpy(vl, val_list);

	vps = vl;

	param = strtok(pl, " ");

	while(param != NULL) {
		if(vps == NULL) {
			free(pl);
			free(vl);
			return(EINVAL);
		}

		vt = val_type(param);

		if(strlen(param) >= RM_MAXPARAMLEN) {
			free(pl);
			free(vl);
			return(EINVAL);
		}

		switch(vt) {

			case STR_VAL:
				vps = getval(vps, &vpn, delim);

				if(NOVAL(vps)) {
					goto skipval;
				}

				rma.rm_vallen = strlen(vps) + 1;
				if(rma.rm_vallen > vb_len) {
					if((tvb = malloc(rma.rm_vallen)) == NULL) {
						free(pl);
						free(vl);
						return(ENOMEM);
					}
					free(val_buf);
					val_buf = tvb;
					vb_len = rma.rm_vallen;
					rma.rm_val = val_buf;
				}
				strcpy(val_buf, vps);
				break;

			case NUM_VAL:
				vps = getval(vps, &vpn, delim);

				if(NOVAL(vps)) {
					goto skipval;
				}

				*((int *)val_buf) = strtol(vps, NULL, 10);
				rma.rm_vallen = sizeof(cm_num_t);
				break;

			case RNG_VAL:
				vps = getval(vps, &vpn, delim);

				if(NOVAL(vps)) {
					vps = vpn;
					vps = getval(vps, &vpn, delim);
					goto skipval;
				}

				((struct cm_addr_rng *)val_buf)->startaddr = strtoul(vps, NULL, 16);

				if((vps = vpn) == NULL) {
					free(vl);
					free(pl);
					return(EINVAL);
				}
				vps = getval(vps, &vpn, delim);

				if(NOVAL(vps)) {
					goto skipval;
				}

				((struct cm_addr_rng *)val_buf)->endaddr = strtoul(vps, NULL, 16);
				rma.rm_vallen = sizeof(struct cm_addr_rng);
				break;

			default:
				free(pl);
				free(vl);
				return(UNKPARAM);
		}
		strcpy(rma.rm_param, param);
		if(ioctl(rm_fd, RMIOC_ADDVAL, &rma) < 0) {
			free(pl);
			free(vl);
			return(errno);
		}

skipval:
		vps = vpn;
		param = strtok(NULL, " ");
	}

	free(vl);
	free(pl);

	return(0);
}

int
RMputvals(rm_key_t key, const char *param_list, const char *val_list)
{
	return (RMputvals_d(key, param_list, val_list, ' '));
}

int
RMdelvals(rm_key_t key, const char *param_list)
{
	char	*pl, *param;
	struct	rm_args	rma;

	if(rm_fd < 0) {
		return(EINVAL);
	}

	if((pl = malloc(strlen(param_list)+1)) == NULL) {
		return(ENOMEM);
	}

	rma.rm_key = key;

	strcpy(pl, param_list);
	param = strtok(pl, " ");

	while(param != NULL) {
		if(strlen(param) >= RM_MAXPARAMLEN) {
			free(pl);
			return(EINVAL);
		}
		(void) val_type(param);
		strcpy(rma.rm_param, param);
		if(ioctl(rm_fd, RMIOC_DELVAL, &rma) < 0) {
			if(errno != ENOENT) {
				free(pl);
				return(errno);
			}
		}
		param = strtok(NULL, " ");
	}

	free(pl);
	return(0);
}

int
RMgetbrdkey(const char * modname, cm_num_t brdinst, rm_key_t *keyp)
{
	struct	rm_args	rma;
	rm_key_t	rmkey;
	cm_num_t	inst;

	rmkey = NULL;
	rma.rm_key = NULL;
	rma.rm_n = 0;

	inst = 0;

	while(ioctl(rm_fd, RMIOC_NEXTKEY, &rma) >= 0) {
		if(rma.rm_key == NULL) {
			break;
		}

		strcpy(rma.rm_param, CM_MODNAME);
		rma.rm_val = val_buf;
		rma.rm_vallen = vb_len;

		if(ioctl(rm_fd, RMIOC_GETVAL, &rma) < 0) {
			continue;
		}

		if(!MATCH(rma.rm_val, modname)) {
			continue;
		}

		if(inst == brdinst) {
			rmkey = rma.rm_key;
			break;
		}

		inst++;
	}

	if(rmkey != NULL) {
		*keyp = rmkey;
		return(0);
	}

	return(ENOENT);
}
