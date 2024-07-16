#include "builtin.h"

// returns true if the 'exit' call
// should be performed
//
// (It must not be called from here)
int
exit_shell(char *cmd)
{
	if (cmd[0] == 'e' && cmd[1] == 'x' && cmd[2] == 'i' && cmd[3] == 't' &&
	    cmd[4] == END_STRING) {
		return 1;
	}

	return 0;
}

// returns true if "chdir" was performed
//  this means that if 'cmd' contains:
// 	1. $ cd directory (change to 'directory')
// 	2. $ cd (change to $HOME)
//  it has to be executed and then return true
//
//  Remember to update the 'prompt' with the
//  	new directory.
//
// Examples:
//  1. cmd = ['c','d', ' ', '/', 'b', 'i', 'n', '\0']
//  2. cmd = ['c','d', '\0']

int
cd(char *cmd)
{
	if (cmd[0] == 'c' && cmd[1] == 'd') {
		char buf[FNAMESIZE];
		if (cmd[2] == END_STRING) {
			if (chdir(getenv("HOME")) != 0) {
				printf("ERROR EN CD: %s\n", strerror(errno));
			}
			strcpy(prompt, getenv("HOME"));
			return 1;
		} else if (cmd[2] == SPACE) {
			if (chdir(cmd + 3) != 0) {
				printf("ERROR EN CD: %s\n", strerror(errno));
			}

			strcpy(prompt, getcwd(buf, sizeof(buf)));
			return 1;
		}
	}
	return 0;
}


// returns true if 'pwd' was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
pwd(char *cmd)
{
	if (cmd[0] == 'p' && cmd[1] == 'w' && cmd[2] == 'd' &&
	    cmd[3] == END_STRING) {
		char buf[FNAMESIZE];
		if (getcwd(buf, sizeof(buf)) != NULL) {
			printf("%s\n", buf);
		} else {
			perror("getcwd() error");
		}
		return 1;
	}

	return 0;
}

// returns true if `history` was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
history(char *cmd)
{
	// Your code here

	return 0;
}
