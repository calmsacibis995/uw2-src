#
#ident	"%W%"
#
# dtfclass_scan.awk - Scan .dtfclass and upgrade it to the current release.
#
#	It will return 0 if the upgrade was successful,
#		otherwise it returns 1.
#
#	This awk script will -
#		a. added DONT_DELETE to CLASS DATA
#		b. added "_PROG_TYPE 'UNIX Graphical';" line
#		c. replaced "dtedit" with "%_PROG_TO_RUN" in "MENU _Open" line
#			and
#		   added "_PROG_TO_RUN 'dtedit';" line.
#			Note that `MENU _Open' won't change if "dtedit" can't
#				located (twice).
#
#

BEGIN {
	work_in_progress = 0;

	printed_prog_type_string = 0;
	new_classname_string = "'dtmgr:31Datafile';";
	dont_delete_string = "DONT_DELETE";
	old_open3_string = "'##DROP(dtedit)";
	old_open6_string = "dtedit";
	new_open_string = "%_PROG_TO_RUN";
	prog_to_run_string = "\t_PROG_TO_RUN\t'dtedit';";
	prog_type_string = "\t_PROG_TYPE\t'UNIX Graphical';";
}

/^CLASS/ && $2 == "DATA" {
	work_in_progress = 1
}

/^END/ && NF == 1 {
	if (work_in_progress == 1) {
		work_in_progress = 0;
		if (printed_prog_type_string == 0) {
			printed_prog_type_string = 1;
			printf("%s\n", prog_type_string);
		}
	}
}

{
	print_this = 1;

	if (work_in_progress == 1) {
		if ($1 == "CLASS") {
			printf("%s %s\n", $0, dont_delete_string);

			print_this = 0;
		} else if ($1 == "_CLASSNAME") {
			sub($2, new_classname_string, $0);
		} else if ($2 == "_Open" && NF == 8 &&
			   $3 == old_open3_string && $6 == old_open6_string) {

			gsub(old_open6_string, new_open_string, $0);
			printf("%s\n%s\n%s\n",
				prog_to_run_string, prog_type_string, $0);

			printed_prog_type_string = 1;
			print_this = 0;
		}
	}

	if (print_this == 1)
		print $0;
}

END {
	exit(work_in_progress);
}
