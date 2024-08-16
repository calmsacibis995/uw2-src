/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/getmac.c	1.1"
#ident	"$Header: $"

#include <stdio.h>
#include <fcntl.h>
#include <sys/dlpi.h>
#include <sys/dlpi_ether.h>
#include <sys/stropts.h>

#define DLGADDR	(('D' << 8) | 5)

/*
 * getmac
 *	Usage: getmac [ device_name ]
 *
 * getmac will generate a file, /etc/inet/macaddr which contains
 * info on the mac address of devices that are in the 
 * /etc/confnet.d/inet/interface file. The purpose of this file is for
 * upgrade of inet pkg to uniquely identify devices of the same kind.
 * Example entries are as follows:
 *
 *	/dev/el16_0	aa:bb:cc:dd
 *	/dev/el16_1	ee:ff:gg:hh
 *
 * if device_name is provided, getmac will print the output to stdout.
 */

static int get_macaddr(char *, FILE *);
static void print_all(void);

main(int argc , char **argv)
{
	if (argc > 2 ) {
		(void)printf("Usage: getmac [ device_name ]\n");
		exit(2);
	}
	if (argc == 2) {
		if (get_macaddr(argv[1], stdout) < 0) {
			perror("get_macaddr failed");
			(void)printf("Usage: getmac [ device_name ]\n");
			exit(1);
		}
		exit(0);
	}
	print_all();
	exit(0);
} 

static int 
get_macaddr(char *device, FILE *outfp)
{
	int fd;
	struct strioctl strioc;
	unsigned char llc_mc[8];

	if ((fd = open(device, O_RDONLY|O_NONBLOCK)) == -1) {
		return(-1);
	} 
	strioc.ic_cmd = DLIOCGENADDR;
	strioc.ic_timout = 15;
	strioc.ic_len = LLC_ADDR_LEN;
	strioc.ic_dp = (char *)llc_mc;
	if (ioctl(fd, I_STR, &strioc) < 0) {
		/*
		 * dlpi token driver does not recognize DLIOCGENADDR,
		 * so try DLGADDR
		 */
		strioc.ic_cmd = DLGADDR;
		strioc.ic_timout = 15;
		strioc.ic_len = LLC_ADDR_LEN;
		strioc.ic_dp = (char *)llc_mc;
		if (ioctl(fd, I_STR, &strioc) < 0) {
			return(-1);
		}
	}
	(void)fprintf(outfp, "%s\t%02X.%02X.%02X.%02X.%02X.%02X\n",
		device, llc_mc[0], llc_mc[1], llc_mc[2],
		llc_mc[3], llc_mc[4], llc_mc[5] );
	return(0);
}

static void
print_all(void)
{
	char	*extractor = "/usr/bin/egrep -v \"^#|^lo:\" /etc/confnet.d/inet/interface | /usr/bin/cut -d: -f 4";
	FILE *fp;
	FILE *outfp;
	char devname[256];
	

	if ((outfp = fopen("/etc/inet/macaddr", "w")) == NULL) {
		perror("open /etc/inet/macaddr failed");
		return;
	}
	if ((fp = popen(extractor, "r")) != NULL) {
		while (fgets(devname, 256, fp) != NULL) {
			devname[strlen(devname) - 1] ='\0';
			(void)get_macaddr(devname, outfp);
		}
		(void)pclose(fp);
	}
	else
		perror("popen failed");
}
