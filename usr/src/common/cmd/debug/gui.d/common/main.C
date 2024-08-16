#ident	"@(#)debugger:gui.d/common/main.C	1.11"

#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <signal.h>
#include <stdio.h>

#include "Msgtypes.h"
#include "Message.h"
#include "Msgtab.h"
#include "Transport.h"
#include "UIutil.h"
#include "Label.h"

#include "UI.h"
#include "Dispatcher.h"
#include "Windows.h"
#include "config.h"

extern Transport	transport;
const char		*msg_internal_error;

typedef void (*PFVV)();
extern PFVV	set_new_handler(PFVV);

void
internal_error(int sig)
{
	dispatcher.cleanup();
	write(2, msg_internal_error, strlen(msg_internal_error));
	exit(sig);
}

void
new_handler()
{
	interface_error("new", __LINE__, 1);
}

static void
unset_signals()
{
	signal(SIGQUIT, SIG_DFL);
	signal(SIGILL, SIG_DFL);
	signal(SIGTRAP, SIG_DFL);
	signal(SIGIOT, SIG_DFL);
	signal(SIGEMT, SIG_DFL);
	signal(SIGFPE, SIG_DFL);
	signal(SIGBUS, SIG_DFL);
	signal(SIGSEGV, SIG_DFL);
	signal(SIGSYS, SIG_DFL);
	signal(SIGXCPU, SIG_DFL);
	signal(SIGXFSZ, SIG_DFL);
}

static void
bye(int)
{
	exit(0);
}

void
main(int argc, char **argv)
{
	(void) setlocale(LC_MESSAGES, "");
	(void) set_new_handler(new_handler);

	// initialize the three message catalogs
	labeltab.init();
	init_message_cat();
	Mtable.init();

	msg_internal_error = Mtable.format(ERR_cannot_recover);

	signal(SIGQUIT, internal_error);
	signal(SIGILL, internal_error);
	signal(SIGTRAP, internal_error);
	signal(SIGIOT, internal_error);
	signal(SIGEMT, internal_error);
	signal(SIGFPE, internal_error);
	signal(SIGBUS, internal_error);
	signal(SIGSEGV, internal_error);
	signal(SIGSYS, internal_error);
	signal(SIGXCPU, internal_error);
	signal(SIGXFSZ, internal_error);
	signal(SIGUSR2, bye);	// exit gracefully when cli terminates

	(void) init_gui("debug", "Debug", &argc, argv);

	window_descriptor = make_descriptors();

	Boolean dump_core = FALSE;
	for (int i = 1; i < argc; ++i)
	{
		char *cp = argv[i];
		if (cp[0] == '-' && cp[1] == 'D' && cp[2] == '\0')
			dump_core = TRUE;
	}
	if (dump_core)
		unset_signals();

	// let the debugger know the gui is alive and well and ready to start
	write(fileno(stdout), "1", 1);

	(void) new Window_set();

	// Main loop - never returns
	toolkit_main_loop();
}
