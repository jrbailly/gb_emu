#
# Regular cron jobs for the gbemu package.
#
0 4	* * *	root	[ -x /usr/bin/gbemu_maintenance ] && /usr/bin/gbemu_maintenance
