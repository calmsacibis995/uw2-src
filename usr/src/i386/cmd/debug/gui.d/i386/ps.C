#ident	"@(#)debugger:gui.d/i386/ps.C	1.3"

#include <sys/types.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "Vector.h"
#include "Proctypes.h"
#include "Machine.h"
#include "Proclist.h"
#include "gui_msg.h"

int
do_ps(Vector *vscratch1, Vector *vscratch2)
{
	//
	//This routine gets the 'ps' information, i.e.
	//pid and command string
	//by reading /proc. It returns an array suitable for
	//input to the scrolling list constructors, that is
	//two line pairs, the first element is the
	//pid, the second the command string itself

	//the /proc directory is read, entry by entry
	//each file is opened, the appropriate ioctl is performed
	//and the uid's matched. If they match, that process is added to the
	//return array. 

	//this routine uses the vector class as a variable-sized
	//array handler.
	//the vector `vscratch1' is used to hold the pairs of strings
	//	pid
	//	command
	//returned by the /proc ioctl. In turn, the vector 'vscratch2'
	//is built using the addresses of the strings in vscratch1

	char		pid_buf[MAX_LONG_DIGITS];
	DIR		*dirp;
	int		procp;
	struct dirent	*direntp;
#ifdef OLD_PROC
	prpsinfo_t	ps_data; 
#else
	psinfo_t	ps_data;
#endif
	uid_t		my_id = getuid();
#ifdef OLD_PROC
	char		name[sizeof("/proc/") + MAX_LONG_DIGITS];
#else
	char		name[sizeof("/proc//psinfo") + MAX_LONG_DIGITS];
#endif
	char		*name_endp;
	pid_t		my_pid = getpid();	// gui's pid
	pid_t		parent_pid = getppid();	// debug's pid
	int		total = 0;
	
	if ((dirp = opendir("/proc")) == NULL)
	{
		display_msg(E_ERROR, GE_slash_proc);
		return 0;
	}

	strcpy(name, "/proc/");
	name_endp = name + strlen("/proc/");
	vscratch1->clear();
	vscratch2->clear();
	while((direntp = readdir(dirp)) != NULL)
	{
#ifdef OLD_PROC
		strcpy(name_endp, direntp->d_name);
#else
		sprintf(name_endp, "%s/psinfo", direntp->d_name);
#endif
		if ((procp = open(name, O_RDONLY)) != -1)
		{
#ifdef OLD_PROC
			if(ioctl(procp, PIOCPSINFO, &ps_data) == 0 &&
				ps_data.pr_uid == my_id)
#else
			if((lseek(procp, 0, SEEK_SET) == 0) &&
			   (read(procp, (char *)&ps_data, sizeof(psinfo_t)) > 0) &&
			   (ps_data.pr_uid == my_id))
#endif
			{
				// don't let user try to grab gui, debug,
				// follow process, or process already grabbed
				if (ps_data.pr_pid == my_pid
					|| ps_data.pr_pid == parent_pid
					|| (ps_data.pr_ppid == parent_pid &&
						strncmp(ps_data.pr_psargs, "follow ", sizeof("follow ")-1) == 0)
					|| proclist.find_process(ps_data.pr_pid))
					continue;

				sprintf(pid_buf, "%d", ps_data.pr_pid);
				vscratch1->add(pid_buf, strlen(pid_buf)+1);
				vscratch1->add(ps_data.pr_psargs, strlen(ps_data.pr_psargs)+1);
				total++;
			}
			close(procp);
		}
	}
	closedir(dirp);
	char *p = (char *)vscratch1->ptr();
	for (int i = 0; i < total; i++)
	{
		vscratch2->add(&p, sizeof(char *));
		p += strlen(p)+1;
		vscratch2->add(&p, sizeof(char *));
		p += strlen(p)+1;
	}
	return total;
}
