#define NB_JOBS_MAX 5

typedef enum job_state {
  ACTIVE,
  SUSPENDED,
  FINISHED
} job_state;
typedef struct job {
  int pid;
  job_state state;
  char *command;
} job;

void init_jobs(job jobs[]);
int add_job(job jobs[], job new_job);
int rm_job_pid(job jobs[], int pid);
void print_jobs(job jobs[]);
