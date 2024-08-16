#ident	"@(#)ktool:common/ktool/kstuff/kprompt.awk	1.3"
#ident  "$Header:"

function normalize(root, mach) {
	if (root == "")
		return ""
	if (mach == "")
		mach = "i386at"

	cmd = "sh -ce \"cd " root "/" mach "; /bin/pwd\" 2>&1"
	cmd | getline dirname
	close cmd
	if (dirname ~ /ERROR|does not exist/)
		dirname = ""

	return dirname
}

function display_menu_section(menu, base, count) {
	rows = int((count + columns - 1) / columns)
	for (i = 1; i <= rows; ++i) {
		line = ""
		for (j = 0; j < columns; ++j) {
			item = i + j * rows
			if (item > count)
				break
			line = line sprintf(fmt, base + item, menu[item])
		}
		write_output(line)
	}
}

function display_menu() {
	columns = int(79 / (maxwidth + 6))
	fmt = "  %2d) %-" maxwidth "s"
	entry = 0
	if (bundle_entries > 0) {
		write_output("\nAvailable Kernel Bundles:\n")
		display_menu_section(bundle_menu, 0, bundle_entries)
	}
	if (tree_entries > 0) {
		write_output("\nAvailable Kernel Trees:\n")
		display_menu_section(tree_menu, bundle_entries, tree_entries)
	}
	write_output("\n   q) quit\n\nEnter Kernel Number: \\c")
}

function add_menu(root, mach, bundle, pre_built_flag, desc) {
	conf_root = normalize(root, mach)
	if (bundle == "")
		bundle = conf_root "/etc/conf/kstuff.d/bundle.Z"
	if (pre_built_flag) {
		cmd = "/bin/ls -l " bundle " 2>&1"
		cmd | getline info
		close cmd
		if (info ~ /ERROR|No such file or directory/)
			return
		if (desc == "")
			desc = bundle
		split(info, field)
		desc = desc " [" field[6] " " field[7] "]"
		++bundle_entries
		bundle_menu[bundle_entries] = desc
		bundle_bundle[bundle_entries] = bundle
		bundle_root[bundle_entries] = conf_root
		entry_no[conf_root " A"] = bundle_entries;
	} else if (conf_root == "" || entry_no[conf_root " T"] || \
		   entry_no[conf_root " A"]) {
		return
	} else {
		if (desc == "")
			desc = root
		++tree_entries
		tree_menu[tree_entries] = desc
		tree_bundle[tree_entries] = bundle
		tree_root[tree_entries] = conf_root
		entry_no[conf_root " T"] = tree_entries;
	}
	width = length(desc)
	if (width > maxwidth)
		maxwidth = width
}

function scan_db_file(file) {
	while ((getline input < file) == 1) {
		split(input, field, ":")
		add_menu(field[1], field[2], field[3], field[4], field[5])
	}
	close file
}

function init_output() {
	output = "echo '"
}

function flush_output() {
	output = output "\\c' >&2'"
	system(output)
}

function write_output(line) {
	output = output line "\n"
}

function get_selection() {
	if (bundle_entries + tree_entries == 0) {
		print "echo 'No kernels are available.' >&2"
		print "exit 1"
		exit
	}
	init_output()
	for (;;) {
		display_menu()
		flush_output()
		getline input < "/dev/tty"
		if (input ~ /^[qQ]/) {
			print "exit"
			exit
		}
		init_output()
		if (input !~ /^[0-9][0-9]*$/) {
			line = sprintf("%s: not a number\n", input)
			write_output(line)
		} else if (0 + input < 1 || \
			   0 + input > (bundle_entries + tree_entries)) {
			printf "=%d, =%d\n", a, b > "/dev/tty"
			line = sprintf("%s: out of range\n", input)
			write_output(line)
		} else if (0 + input <= bundle_entries) {
			print "PRE_BUILT=1"
			print "BUNDLE=" bundle_bundle[input]
			print "CONF_ROOT=" bundle_root[input]
			print "DESC=\"" bundle_menu[input] "\""
			break
		} else {
			input -= bundle_entries
			print "PRE_BUILT=0"
			print "BUNDLE=" tree_bundle[input]
			print "CONF_ROOT=" tree_root[input]
			print "DESC=\"" tree_menu[input] "\""
			break
		}
	}
}

NR == 1 {
	ROOT=$1; MACH=$2
}
NR == 2 {
	for (i = 1; i <= NF; ++i)
		scan_db_file($i)
}
END {
	add_menu(ROOT, MACH, "", "", "")
	get_selection()
}
