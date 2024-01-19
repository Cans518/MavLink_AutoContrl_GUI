/*
 * Mostly ripped off of console-tools' writevt.c
 */

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char **argv) {
	const char sysctl[] = "/proc/sys/dev/tty/legacy_tiocsti";
	char *term = NULL;
	char *text = NULL;
	int fd;

	term = "/dev/pts/1";
	text = argv[1];
	char *cmd = (char*)malloc(strlen(text)*sizeof(char));
	sprintf(cmd,"%s\n",text);
	text = cmd;
	fd = open(sysctl, O_WRONLY);

	fd = open(term, O_RDONLY);
	if (fd >= 0) {
		while (*text) {
			if (ioctl(fd, TIOCSTI, text)) {
				err(1, "ioctl(TIOCSTI) failed");
			}
			text++;
		}
		close(fd);
	} else {
		err(1, "could not open %s", term);
	}
	return 0;
}

