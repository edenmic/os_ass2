#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int lock_id = peterson_create();
  if (lock_id < 0) {
    printf("Failed to create lock\n");
    exit(1);
  }
  
  printf("Created Peterson lock with ID: %d\n", lock_id);
  
  int fork_ret = fork();
  int role = fork_ret > 0 ? 0 : 1; // role = 0 for parent, role = 1 for child
  
  for (int i = 0; i < 100; i++) {
    if (peterson_acquire(lock_id, role) < 0) {
      printf("Failed to acquire lock\n");
      exit(1);
    }
    
    // Critical section
    printf("Process %d (role %d) in critical section, iteration %d\n", 
           getpid(), role, i);
    //sleep(10);  // Sleep for a bit to simulate work
    printf("Process %d (role %d) leaving critical section\n", getpid(), role);
    
    if (peterson_release(lock_id, role) < 0) {
      printf("Failed to release lock\n");
      exit(1);
    }
    
    // Wait a bit before next attempt
    //sleep(5);
  }
  
  if (fork_ret > 0) {
    wait(0);
    printf("Parent process destroying lock\n");
    if (peterson_destroy(lock_id) < 0) {
      printf("Failed to destroy lock\n");
      exit(1);
    }
    printf("Lock destroyed successfully\n");
  }
  
  exit(0);
}