#include "job.h"
#include "unistd.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void init_jobs(job jobs[]) {
  for (int i = 0; i < NB_JOBS_MAX; i++) {
    jobs[i].pid = -1;
    jobs[i].state = FINISHED;
    jobs[i].command = NULL;
    jobs[i].fd_pipe_out = -1;
  }
}

int add_job(job *jobs, job new_job) {
  for (int i = 0; i < NB_JOBS_MAX; i++) {
    if (jobs[i].state == FINISHED) {
      jobs[i] = new_job;
      return i;
    }
  }
  fprintf(stderr, "Too many jobs\n");
  return -1;
}
void rm_job_pid(job *jobs, int pid) {
  for (int i = 0; i < NB_JOBS_MAX; i++) {
    if (jobs[i].pid == pid) {
      free(jobs[i].command);
      jobs[i].pid = -1;
      jobs[i].state = FINISHED;
      if (jobs[i].fd_pipe_out > 0) {
        close(jobs[i].fd_pipe_out);
      }
      jobs[i].fd_pipe_out = -1;
    }
  }
}
void continue_job_bg_id(job *jobs, int id) {
  if (jobs[id].pid == -1 || id >= NB_JOBS_MAX) {
    fprintf(stderr, "No such job\n");
    return;
  }

  jobs[id].state = ACTIVE; // redondant avec le handler
  kill(jobs[id].pid, SIGCONT);
}
int id_from_pid(job *jobs, int pid) {
  for (int i = 0; i < NB_JOBS_MAX; i++) {
    if (jobs[i].pid == pid) {
      return i;
    }
  }
  printf("No such job\n");
  return -1;
}
void update_status_id(int state, job *jobs, int id) { jobs[id].state = state; }

void update_status_pid(int state, job *jobs, int pid) {
  int id = id_from_pid(jobs, pid);
  if (id != -1) {
    return;
  }
  update_status_id(state, jobs, id);
}

void send_stop_job_pid(job *jobs, int pid) {
  int id = id_from_pid(jobs, pid);
  if (id == -1) {
    return;
  }
  send_stop_job_id(jobs, id);
}

void send_stop_job_id(job *jobs, int id) {
  if (jobs[id].pid == -1 || id >= NB_JOBS_MAX) {
    fprintf(stderr, "No such job\n");
    return;
  }

  jobs[id].state = SUSPENDED; // redondant avec le handler
  kill(jobs[id].pid, SIGSTOP);
}
char *build_command_string(char **cmd) {
  int size = 1;
  for (int i = 0; cmd[i] != NULL; i++) {
    size += strlen(cmd[i]) + 1;
  }
  char *command = malloc(size * sizeof(char));
  command[0] = '\0';
  for (int i = 0; cmd[i] != NULL; i++) {
    strcat(command, cmd[i]);
    strcat(command, " ");
  }
  return command;
}
int wait_job_id(job *jobs, int id) {
  if (jobs[id].pid == -1 || id >= NB_JOBS_MAX) {
    fprintf(stderr, "No such job\n");
    return -1;
  }

  jobs[id].state = ACTIVE;
  kill(jobs[id].pid, SIGCONT);
  return jobs[id].pid;
}

void print_jobs(job *jobs) {
  for (int i = 0; i < NB_JOBS_MAX; i++) {
    if (jobs[i].pid != -1) {
      printf("Job %d\n", i);
      printf("\tPID: %d\n", jobs[i].pid);
      printf("STDOUT -> %d", jobs[i].fd_pipe_out);
      printf("\tState: ");
      switch (jobs[i].state) {
      case ACTIVE:
        printf("Active");
        break;

      case SUSPENDED:
        printf("Suspended");
        break;

      case FINISHED:
        printf("Finished");
        break;
      default:
        printf("Unknown state");
        break;
      }
      printf("\tCommand: %s\n", jobs[i].command);
    }
  }
}
