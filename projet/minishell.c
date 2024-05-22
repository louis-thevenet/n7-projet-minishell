#include "fcntl.h"
#include "job.h"
#include "readcmd.h"
#include "signal.h"
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#define BUFSIZE 64
#ifdef DEBUG
#define LIGHT_GRAY "\033[1;30m"
#define NC "\033[0m"
#define DEBUG_PRINT(x)                                                         \
  printf("%s", LIGHT_GRAY);                                                    \
  printf x;                                                                    \
  printf("%s", NC);
#else
#define DEBUG_PRINT(x)                                                         \
  do {                                                                         \
  } while (0)
#endif

int fg_pid;
job *jobs;

/**
 * Handler for SIGCHLD signal.
 */
void handler_sig_child() {
  int status;
  int pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED);
  if (pid != -1) {
    if (WIFSTOPPED(status)) {
      DEBUG_PRINT(("[Child %d stopped]\n", pid));
      update_status_pid(SUSPENDED, jobs, pid);
    }
    if (WIFCONTINUED(status)) {
      DEBUG_PRINT(("[Child %d continued]\n", pid));
      update_status_pid(ACTIVE, jobs, pid);
    }
    if (WIFSIGNALED(status)) {
      DEBUG_PRINT(("[Child %d signaled]\n", pid));
      rm_job_pid(jobs, pid);
    }
    if (WIFEXITED(status)) {
      DEBUG_PRINT(("[Child %d exited]\n", pid));
      rm_job_pid(jobs, pid);
    }

    if (pid == fg_pid && !WIFCONTINUED(status)) {
      fg_pid = 0;
    }
  }
}

/**
 * Handler for SIGSTP signal. Stops the foreground process.
 */
void handler_sig_tstp() {
  DEBUG_PRINT(("[SIGTSTP received]\n"));
  if (fg_pid != 0) {
    DEBUG_PRINT(("[Stopping %d]\n", fg_pid));

    send_stop_job_pid(jobs, fg_pid);
    fg_pid = 0;
  }
}

/**
 * Handler for SIGINT signal. Kills the foreground process.
 */
void handler_sig_int() {
  DEBUG_PRINT(("[SIGINT received]\n"));
  if (fg_pid != 0) {
    DEBUG_PRINT(("[Killing %d]\n", fg_pid));
    kill(fg_pid, SIGTERM);
    fg_pid = 0;
  }
}

/**
 * Copies the contents from one file descriptor to another then closes both.
 */
void redirect_pipe(int src, int dest) {
  ssize_t bytesRead;
  ssize_t bytesWritten;
  char buf[BUFSIZE];

  do {
    bytesRead = read(src, buf, BUFSIZE);
    bytesWritten = write(dest, buf, bytesRead);
    DEBUG_PRINT(("read %ld, wrote %ld\n", bytesRead, bytesWritten));
  } while (bytesRead > 0);
  close(src);
  close(dest);
}

/**
 * If the command is a built-in one, runs it and return true, returns false
 * otherwise.
 */
bool built_in_commands(char **cmd) {

  if (strcmp(cmd[0], "lj") == 0) {
    print_jobs(jobs);
    return true;
  }

  if (strcmp(cmd[0], "sj") == 0) {
    int id = atoi(cmd[1]);
    send_stop_job_id(jobs, id);
    return true;
  }

  if (strcmp(cmd[0], "bg") == 0) {
    int id = atoi(cmd[1]);
    continue_job_bg_id(jobs, id);
    return true;
  }
  if (strcmp(cmd[0], "fg") == 0) {
    int id = atoi(cmd[1]);
    fg_pid = continue_job_fg_id(jobs, id);
    while (fg_pid != 0) {
      pause();
    }
    return true;
  }

  if (strcmp(cmd[0], "susp") == 0) {
    raise(SIGSTOP);
    return true;
  }
  return false;
}

/**
 * Executes a command in a child process. Handling pipes if needed. Waiting for
 * the child process to finish if it's a foreground process.
 *
 * @param command The commands to execute.
 * @param index The index of the command in the array of commands.
 * @param pipe_in The file descriptor of the input pipe. Either 0 or the
 * previous command's STDOUT
 * @returns File descriptor of the output pipe (STDOUT of last command) or -1 if
 * an error occured
 */
int create_fork(struct cmdline *command, int index, int pipe_in) {
  // is it a built-in command ?
  if (built_in_commands(command->seq[index])) {
    return pipe_in;
  }
  char **cmd = command->seq[index];
  bool last = !command->seq[index + 1]; // is it the last command ?

  /********PIPES********/
  int fd_in[2];    // input file -> STDIN
  int fd_out[2];   // STDOUT -> output file
  int fd_pipe[2];  // current's STDOUT -> next command's STDIN
  int fd_to_store; // in order to close it if the process is killed

  // is there an input file ?
  if (command->in != NULL) {
    if (pipe(fd_in) == -1) {
      printf("Error: Cannot create pipe\n");
      return -1;
    }
  }

  // is there an output file ?
  if (command->out != NULL) {
    if (pipe(fd_out) == -1) {
      printf("Error: Cannot create pipe\n");
      return -1;
    }
    fd_to_store = fd_out[1];
  }

  // pipe between commands
  if (pipe(fd_pipe) == -1) {
    printf("Error: Cannot create pipe\n");
    return -1;
  }
  if (pipe_in > 0) {
    fd_to_store = fd_in[0];
  }

  /********FORK********/
  int pid_fork = fork();

  if (pid_fork == -1) {
    printf("Error: error while initiating child process\n");
  }

  if (pid_fork == 0) {
    /********CHILD********/

    // mask signals for SIGINT, SIGTSTP since the parent process handles them
    sigset_t mask_set;
    sigemptyset(&mask_set);
    sigaddset(&mask_set, SIGINT);
    sigaddset(&mask_set, SIGTSTP);
    sigprocmask(SIG_BLOCK, &mask_set, NULL);

    /********REDIRECTIONS********/
    // input file -> STDIN ?
    if (command->in != NULL) {
      dup2(fd_in[0], STDIN_FILENO);
      close(fd_in[1]);
    }
    // STDOUT -> output file ?
    if (command->out != NULL) {
      dup2(fd_out[1], STDOUT_FILENO);
      close(fd_out[0]);
    }

    /********PIPELINES********/
    // STDOUT -> next command's STDIN through fd_pipe
    if (!last) {
      dup2(fd_pipe[1], STDOUT_FILENO);
      close(fd_pipe[0]);
    }
    // previous command's STDOUT -> this command's STDIN through pipe_in
    if (pipe_in > 0) {
      dup2(pipe_in, STDIN_FILENO);
    }

    execvp(cmd[0], cmd);
    printf("Error: command failed to execute\n");
    exit(EXIT_FAILURE);

  } else {
    /********PARENT********/

    // create a job for the command
    char *cmd_copy = build_command_string(cmd);
    add_job(jobs, (job){pid_fork, fd_to_store, ACTIVE, cmd_copy});

    close(fd_to_store);

    // run in background ?
    if (command->backgrounded == NULL) {
      fg_pid = pid_fork;
    }

    // input file -> STDIN ?
    if (command->in != NULL) {
      int src = open(command->in, O_RDONLY);
      if (src == -1) {
        printf("Error: input file could not be read. Killing subprocess... \n");
        send_stop_job_pid(jobs, pid_fork); // kill the child process
        return -1;
      }
      // we handle the redirection in a separate process to avoid blocking the
      // parent process and support backgrounded commands.
      switch (fork()) {
      case 0:
        redirect_pipe(src, fd_in[1]);
        exit(0);

      case -1:
        printf("Error: could not fork to read input file. Killing "
               "subprocess... \n");
        send_stop_job_pid(jobs, pid_fork);
        break;
      }
      close(src);
      close(fd_in[1]);
    }

    // STDOUT -> output file ?
    if (command->out != NULL) {
      int dest = open(command->out, O_WRONLY | O_CREAT, S_IRWXU);
      if (dest == -1) {
        printf(
            "Error: output file could not be created. Killing subprocess...\n");
        send_stop_job_pid(jobs, pid_fork);
        close(fd_pipe[1]);
        return -1;
      }

      // we also fork for the output file for the same reason
      switch (fork()) {
      case 0:
        redirect_pipe(dest, fd_in[1]);
        exit(0);

      case -1:
        printf("Error: could not fork to write to output file. Killing "
               "subprocess... \n");
        send_stop_job_pid(jobs, pid_fork);
        break;
      }
      close(fd_out[0]);
      close(dest);
    }

    if (command->backgrounded == NULL) {
      // wait for a signal from the child process
      while (fg_pid != 0) {
        pause();
      }
    }
  }
  close(fd_pipe[1]);
  return fd_pipe[0];
}

/**
 * Sets up signal handling for SIGCHLD, SIGTSTP and SIGINT.
 */
void setup_sig_action() {
  struct sigaction sa_chld;
  sa_chld.sa_handler = handler_sig_child;
  sigemptyset(&sa_chld.sa_mask);
  sa_chld.sa_flags = SA_RESTART;
  sigaction(SIGCHLD, &sa_chld, NULL);

  struct sigaction sa_tstp;
  sa_tstp.sa_handler = handler_sig_tstp;
  sigemptyset(&sa_tstp.sa_mask);
  sa_tstp.sa_flags = SA_RESTART;
  sigaction(SIGTSTP, &sa_tstp, NULL);

  struct sigaction sa_int;
  sa_int.sa_handler = handler_sig_int;
  sigemptyset(&sa_int.sa_mask);
  sa_int.sa_flags = SA_RESTART;
  sigaction(SIGINT, &sa_int, NULL);
}

/**
 * Main loop
 */
int main(void) {
  setup_sig_action();
  jobs = malloc(sizeof(job) * NB_JOBS_MAX);
  init_jobs(jobs);
  bool exited = false;
  int fd_stdout_current = 0;
  struct cmdline *commande;
  while (!exited) {
    printf("> ");
    commande = readcmd();

    if (commande == NULL) {
      perror("Error: could not read the command\n");

    } else {
      if (commande->err) {
        printf("Error: error while writing the command %s\n", commande->err);
      } else {
        int indexseq = 0;
        char **cmd;
        fd_stdout_current = 0;
        while ((cmd = commande->seq[indexseq])) {
          if (cmd[0]) {
            if (strcmp(cmd[0], "exit") == 0) {
              exited = true;
              printf("Au revoir ...\n");
            } else {
              fd_stdout_current =
                  create_fork(commande, indexseq, fd_stdout_current);
            }
            if (fd_stdout_current == -1) {
              exited = true;
            }

            indexseq++;
          }
        }
      }
    }
  }
  freecmd(commande);
  free(commande);
  free(jobs);
  return EXIT_SUCCESS;
}
