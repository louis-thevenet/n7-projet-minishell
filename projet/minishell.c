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

void handler_sig_tstp() {
  DEBUG_PRINT(("[SIGTSTP received]\n"));
  if (fg_pid != 0) {
    DEBUG_PRINT(("[Stopping %d]\n", fg_pid));

    send_stop_job_pid(jobs, fg_pid);
    fg_pid = 0;
  }
}
void handler_sig_int() {
  DEBUG_PRINT(("[SIGINT received]\n"));
  if (fg_pid != 0) {
    DEBUG_PRINT(("[Killing %d]\n", fg_pid));
    kill(fg_pid, SIGTERM);
    fg_pid = 0;
  }
}

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

void traiter_commande(char **cmd, struct cmdline *commande) {

  if (strcmp(cmd[0], "lj") == 0) {
    print_jobs(jobs);
    return;
  }

  if (strcmp(cmd[0], "sj") == 0) {
    int id = atoi(cmd[1]);
    send_stop_job_id(jobs, id);
    return;
  }

  if (strcmp(cmd[0], "bg") == 0) {
    int id = atoi(cmd[1]);
    continue_job_bg_id(jobs, id);
    return;
  }
  if (strcmp(cmd[0], "fg") == 0) {
    int id = atoi(cmd[1]);
    fg_pid = wait_job_id(jobs, id);
    while (fg_pid != 0) {
      pause();
    }
    return;
  }

  if (strcmp(cmd[0], "susp") == 0) {
    raise(SIGSTOP);
    return;
  }

  int fd_in[2];
  int fd_out[2];
  int fd_input_pipe_chld_stdout_to_target = -1;

  if (commande->in != NULL) {
    if (pipe(fd_in) == -1) {
      printf("Error: Cannot create pipe\n");
      return;
    }
  }

  if (commande->out != NULL) {
    if (pipe(fd_out) == -1) {
      printf("Error: Cannot create pipe\n");
      return;
    }
    fd_input_pipe_chld_stdout_to_target = fd_out[1];
  }

  int pid_fork = fork();

  if (pid_fork == -1) {
    printf("La commande n'a pas fonctionné.");
  }

  if (pid_fork == 0) {

    sigset_t mask_set;
    sigemptyset(&mask_set);
    sigaddset(&mask_set, SIGINT);
    sigaddset(&mask_set, SIGTSTP);
    sigprocmask(SIG_BLOCK, &mask_set, NULL);

    // read from pipe
    if (commande->in != NULL) {
      dup2(fd_in[0], STDIN_FILENO);
      close(fd_in[1]);
    }
    // write stdout to pipe
    if (commande->out != NULL) {
      dup2(fd_out[1], STDOUT_FILENO);
      close(fd_out[0]);
    }

    execvp(cmd[0], cmd);
    printf("La commande n'a pas fonctionné.\n");
    exit(EXIT_FAILURE);

  } else {
    char *cmd_copy = build_command_string(cmd);
    add_job(jobs, (job){pid_fork, fd_input_pipe_chld_stdout_to_target, ACTIVE,
                        cmd_copy});
    if (commande->backgrounded == NULL) {
      fg_pid = pid_fork;
    }
    if (commande->in != NULL) {
      // write to stdin of child
      int src = open(commande->in, O_RDONLY);
      if (src == -1) {
        printf("Le fichier n'a pas pu être lu\n");
        return;
      }
      redirect_pipe(src, fd_in[1]);
    }

    if (commande->out != NULL) {
      // read from stdout of child
      int dest = open(commande->out, O_WRONLY | O_CREAT, S_IRWXU);

      redirect_pipe(fd_out[0], dest);
    }

    if (commande->backgrounded == NULL) {
      while (fg_pid != 0) {
        pause();
      }
    }
  }
}

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

int main(void) {
  setup_sig_action();
  jobs = malloc(sizeof(job) * NB_JOBS_MAX);
  init_jobs(jobs);
  bool fini = false;
  struct cmdline *commande;
  while (!fini) {
    printf("> ");
    commande = readcmd();

    if (commande == NULL) {
      perror("erreur lecture commande \n");

    } else {
      if (commande->err) {
        printf("erreur saisie de la commande : %s\n", commande->err);
      } else {
        int indexseq = 0;
        char **cmd;
        while ((cmd = commande->seq[indexseq])) {
          if (cmd[0]) {
            if (strcmp(cmd[0], "exit") == 0) {
              fini = true;
              printf("Au revoir ...\n");
            } else {
              traiter_commande(cmd, commande);
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
