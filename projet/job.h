#define NB_JOBS_MAX 5

typedef enum job_state { ACTIVE, SUSPENDED, FINISHED } job_state;
typedef struct job {
  int pid;
  int fd_pipe_out; // to redirect stdout
  job_state state;
  char *command;
} job;

void init_jobs(job *jobs);
int add_job(job *, job new_job);
void rm_job_pid(job *jobs, int pid);
void send_stop_job_pid(job *jobs, int pid);
void send_stop_job_id(job *jobs, int id);

void update_status_id(int state, job *jobs, int id);

void update_status_pid(int state, job *jobs, int pid);

void continue_job_bg_id(job *jobs, int id);
int wait_job_id(job *jobs, int id);
char *build_command_string(char **cmd);
void print_jobs(job *jobs);
