
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  if (argc != 2) {
    fprintf(2, "Usage: tournament <num_processes>\n");
    exit(1);
  }
  
  int n = atoi(argv[1]);
  
  // Tournament ID is the return value from tournament_create
  int tournament_id = tournament_create(n);
  if (tournament_id < 0) {
    fprintf(2, "Failed to create tournament with %d processes\n", n);
    exit(1);
  }
  
  // Try to acquire the lock
  if (tournament_acquire() < 0) {
    fprintf(2, "Process %d failed to acquire lock\n", getpid());
    exit(1);
  }
  
  // Critical section
  printf("Process PID=%d, Tournament ID=%d is in critical section\n", getpid(), tournament_id);
  //sleep(1); // Sleep briefly to demonstrate mutual exclusion
  
  // Release the lock
  if (tournament_release() < 0) {
    fprintf(2, "Process %d failed to release lock\n", getpid());
    exit(1);
  }
  
  exit(0);
}