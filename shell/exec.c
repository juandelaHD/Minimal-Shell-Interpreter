#include "exec.h"

// sets "key" with the key part of "arg"
// and null-terminates it
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  key = "KEY"
//
static void
get_environ_key(char *arg, char *key)
{
	int i;
	for (i = 0; arg[i] != '='; i++)
		key[i] = arg[i];

	key[i] = END_STRING;
}

// sets "value" with the value part of "arg"
// and null-terminates it
// "idx" should be the index in "arg" where "=" char
// resides
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  value = "value"
//
static void
get_environ_value(char *arg, char *value, int idx)
{
	size_t i, j;
	for (i = (idx + 1), j = 0; i < strlen(arg); i++, j++)
		value[j] = arg[i];

	value[j] = END_STRING;
}

// sets the environment variables received
// in the command line
//
// Hints:
// - use 'block_contains()' to
// 	get the index where the '=' is
// - 'get_environ_*()' can be useful here
static void
set_environ_vars(char **eargv, int eargc)
{
	// Your code here
	for (int i = 0; i < eargc; i++) {
		char key[ARGSIZE];
		char value[ARGSIZE];
		get_environ_key(eargv[i], key);
		int idx = block_contains(eargv[i], '=');
		get_environ_value(eargv[i], value, idx);
		if (setenv(key, value, 1) < 0) {
			perror("fallo setenv");
			_exit(-1);
		}
	}
	//
}

// opens the file in which the stdin/stdout/stderr
// flow will be redirected, and returns
// the file descriptor
//
// Find out what permissions it needs.
// Does it have to be closed after the execve(2) call?
//
// Hints:
// - if O_CREAT is used, add S_IWUSR and S_IRUSR
// 	to make it a readable normal file
static int
open_redir_fd(char *file, int flags)
{
	int fd;
	// S_IWUSR da permisos de escritura al usuario sobre el archivo que creo O_CREAT
	// S_IRUSR da permisos de lectura al usuario sobre el archivo que creo O_CREAT
	if (flags == (O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC)) {
		fd = open(file, flags, S_IRUSR | S_IWUSR);
	} else {
		fd = open(file, flags);
	}
	if (fd < 0) {
		perror("fial to open");
		_exit(-1);
	}
	return fd;
}

// executes a command - does not return
//
// Hint:
// - check how the 'cmd' structs are defined
// 	in types.h
// - casting could be a good option
void
exec_cmd(struct cmd *cmd)
{
	// To be used in the different cases
	struct execcmd *e;
	struct backcmd *b;
	struct execcmd *r;
	struct pipecmd *p;

	// printf("el tipo de comando es: %d\n", cmd->type);


	switch (cmd->type) {
	case EXEC:
		// spawns a command
		//
		// Your code here
		e = (struct execcmd *) cmd;
		set_environ_vars(e->eargv, e->eargc);
		execvp(e->argv[0], e->argv);
		//
		perror("Error, el comando no se ejecuto\n");
		_exit(-1);
		break;

	case BACK: {
		// runs a command in background
		//
		// Your code here
		b = (struct backcmd *) cmd;
		exec_cmd(b->c);
		//
		printf("Background process are not yet implemented\n");
		_exit(-1);
		break;
	}

	case REDIR: {
		// changes the input/output/stderr flow
		//
		// To check if a redirection has to be performed
		// verify if file name's length (in the execcmd struct)
		// is greater than zero
		//
		// Your code here
		r = (struct execcmd *) cmd;
		if (r->in_file[1] != END_STRING) {
			int fd_in =
			        open_redir_fd(r->in_file, O_RDONLY | O_CLOEXEC);
			// O_RDONLY abre el archivo de entrada en modo solo lectura
			dup2(fd_in, 0);  // el 0 es el file descriptor de stdin
		}
		if (r->out_file[1] != END_STRING) {
			int fd_out = open_redir_fd(r->out_file,
			                           O_WRONLY | O_CREAT |
			                                   O_TRUNC | O_CLOEXEC);
			dup2(fd_out, 1);  // el 1 es el file descriptor de stdout
		}
		// O_WRONLY abre el archivo de salida en modo solo escritura
		// O_TRUNC en caso de que el archivo exista primero lo trunca a 0 bytes (lo borra)
		// O_CREAT si el archivo no existe lo crea
		// O_CLOEXEC cierra el archivo cuando el proceso hijo termina

		if (r->err_file[1] != END_STRING) {
			int fd_err;
			if (r->err_file[0] == '&') {
				fd_err = r->err_file[1] -
				         '0';  // paso a int el numero de file descriptor
			} else {
				fd_err = open_redir_fd(r->err_file,
				                       O_WRONLY | O_CREAT |
				                               O_TRUNC |
				                               O_CLOEXEC);
			}
			dup2(fd_err, 2);  // el 2 es el file descriptor de error
		}

		// ¿Deberíamos ejecutar exec_cmd para que luego
		// caiga sobre type EXEC y así, llame a set_environ_vars?
		if (execvp(r->argv[0], r->argv) == -1) {
			perror("fallo execvp");
			_exit(-1);
		}
		//
		perror("Error, el comando de redireccion no se ejecuto\n");
		_exit(-1);
		break;
	}

	case PIPE: {
		// pipes two commands
		p = (struct pipecmd *) cmd;
		// CREO PIPE
		int fds_pipe[2];
		if (pipe(fds_pipe) == -1) {
			perror("ERROR creacion pipe");
			_exit(-1);
		}

		pid_t pid_left = fork();
		if (pid_left < 0) {
			perror("ERROR Fork");
			_exit(-1);
		}

		if (pid_left == 0) {  // HIJO_IZQUIERDO
			// cambiamos el grupo para evitar que el handler de SIGCHLD los capture
			setpgid(0, 0);
			close(fds_pipe[READ]);
			dup2(fds_pipe[WRITE], 1);
			close(fds_pipe[WRITE]);
			exec_cmd(p->leftcmd);
		} else {  // PADRE
			pid_t pid_right = fork();
			if (pid_right < 0) {
				perror("ERROR Fork");
				_exit(-1);
			}
			if (pid_right == 0) {  // HIJO_DERECHO
				// cambiamos el grupo para evitar que el handler de SIGCHLD los capture
				setpgid(0, 0);
				close(fds_pipe[WRITE]);
				dup2(fds_pipe[READ], 0);
				close(fds_pipe[READ]);
				exec_cmd(p->rightcmd);

			} else {  // PADRE
				close(fds_pipe[WRITE]);
				close(fds_pipe[READ]);
				wait(NULL);  // espero al hijo izquierdo
				wait(NULL);  // espero al hijo derecho
				free_command(parsed_pipe);
			}
			exit(0);
			break;
		}


		// free the memory allocated
		// for the pipe tree structure
		perror("Error, el comando PIPE no se ejecuto\n");
		free_command(parsed_pipe);
		break;
	}
	}
}
