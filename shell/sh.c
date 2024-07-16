#include "defs.h"
#include "types.h"
#include "readline.h"
#include "runcmd.h"

char prompt[PRMTLEN] = { 0 };


static void
sigchild_handler(int sig)
{
	if (sig == SIGCHLD) {
		int status;
		pid_t pid;
		char message[BUFLEN];
		// ponemos el while porque pueden haber varios procesos hijos
		// terminados al mismo tiempo, lo que significa que el proceso
		// padre puede recibir múltiples señales SIGCHLD antes de que
		// tenga la oportunidad de manejarlas el 0 de waitpid significa
		// que esperamos a cualquier hijo que sea del mismo grupo que la
		// shell (ya que cambiamos el el id de grupo de todos los
		// procesos menos los background), el WNOHANG significa que no
		// se bloquea si no hay hijos que hayan terminado
		while ((pid = waitpid(0, &status, WNOHANG)) > 0) {
			snprintf(message,
			         sizeof(message),
			         "==> terminado: PID=%d\n",
			         pid);
			printf("%s", message);
		}
	}
}


// runs a shell command
static void
run_shell()
{
	char *cmd;

	// Definir el stack alternativo
	char *alt_stack = malloc(BUFSIZ);
	if (alt_stack == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	// Configurar la estructura sigaltstack
	stack_t ss;
	ss.ss_sp = alt_stack;
	ss.ss_size = BUFSIZ;
	ss.ss_flags = 0;

	// Instalar el stack alternativo
	if (sigaltstack(&ss, NULL) == -1) {
		perror("sigaltstack");
		free(alt_stack);
		exit(EXIT_FAILURE);
	}
	// Configurar el manejador de señales SIGCHLD
	struct sigaction sa = { 0 };
	sa.sa_handler = sigchild_handler;
	sa.sa_flags = SA_RESTART | SA_ONSTACK;
	if (sigaction(SIGCHLD, &sa, NULL)) {
		perror("sigaction");
		free(alt_stack);
		exit(EXIT_FAILURE);
	}

	while ((cmd = read_line(prompt)) != NULL)
		if (run_cmd(cmd) == EXIT_SHELL) {
			free(alt_stack);
			return;
		}
}

// initializes the shell
// with the "HOME" directory
static void
init_shell()
{
	char buf[BUFLEN] = { 0 };
	char *home = getenv("HOME");
	if (chdir(home) < 0) {
		snprintf(buf, sizeof buf, "cannot cd to %s ", home);
		perror(buf);
	} else {
		snprintf(prompt, sizeof prompt, "(%s)", home);
	}
}

int
main(void)
{
	init_shell();

	run_shell();

	return 0;
}
