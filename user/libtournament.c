#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// Global variables to maintain tournament state
static int *locks = 0;          // Array of Peterson lock IDs
static int num_processes = 0;   // Number of processes in tournament
static int num_levels = 0;      // Number of levels in tree (log2(num_processes))
static int process_index = -1;  // Index of this process in the tournament (0 to num_processes-1)
static int is_initialized = 0;  // Flag to check if tournament is initialized

// Calculate the number of levels needed for a given number of processes
static int calculate_levels(int processes) {
  int levels = 0;
  while (processes > 1) {
    processes >>= 1;
    levels++;
  }
  return levels;
}

// Check if a number is a power of 2
static int is_power_of_2(int n) {
  return (n != 0) && ((n & (n - 1)) == 0); //bitwise and 
}

// Calculate total number of locks needed
static int calculate_total_locks(int processes) {
  return processes - 1; // In a binary tree, number of internal nodes = num_leaves - 1
}

// Calculate the role for this process at a given level
static int calculate_role(int level) {
  return (process_index & (1 << (num_levels - level - 1))) >> (num_levels - level - 1); 
  //extracts the bit that determines which side of the tree (left or right) the process belongs to at each level, which in turn determines its role in the Peterson lock.
}

// Calculate the lock index at a given level in a BFS array
static int calculate_lock_index(int level) {
  // Base lock index within level
  int base_lock_index = process_index >> (num_levels - level);
  
  // Offset to add for this level in the BFS array
  // Sum of 2^k for k from 0 to level-1
  int level_offset = (1 << level) - 1; //equivalent to Σ(2^k) for k from 0 to l-1
  
  // Final array index
  return base_lock_index + level_offset;
}

int tournament_create(int processes) {
  // Validate number of processes (must be power of 2 and up to 16)
  if (!is_power_of_2(processes) || processes <= 0 || processes > 16) {
    return -1;
  }

  num_processes = processes;
  num_levels = calculate_levels(processes);
  int total_locks = calculate_total_locks(processes);
  
  // Allocate memory for lock IDs
  locks = malloc(total_locks * sizeof(int));
  if (!locks) {
    return -1;
  }
  
  // Create Peterson locks for the tournament tree
  for (int i = 0; i < total_locks; i++) {
    locks[i] = peterson_create();
    if (locks[i] < 0) {
      // Failed to create a lock
      // Note: We don't clean up here as per the assignment
      return -1;
    }
  }
  
  // Fork processes
  int pid;
  for (int i = 1; i < processes; i++) {
    pid = fork();
    if (pid < 0) {
      // Fork failed
      return -1;
    } else if (pid == 0) {
      // Child process
      process_index = i;
      is_initialized = 1;
      return i; // Return tournament ID (child's index)
    }
  }
  
  // Parent process
  process_index = 0;
  is_initialized = 1;
  return 0; // Return tournament ID (parent's index)
}

int tournament_acquire(void) {
  if (!is_initialized || process_index < 0) {
    return -1;
  }
  
  // Acquire locks from bottom to top
  for (int level = num_levels - 1; level >= 0; level--) {
    int lock_idx = calculate_lock_index(level);
    int role = calculate_role(level);
    
    if (peterson_acquire(locks[lock_idx], role) < 0) {
      // Failed to acquire lock
      // Release already acquired locks in reverse order
      for (int l = level + 1; l < num_levels; l++) {
        int idx = calculate_lock_index(l);
        int r = calculate_role(l);
        if (peterson_release(locks[idx], r) < 0) {
          // Log an error or handle the failure
          return -1; // Return an error code to indicate failure
        }
      }
      return -1;
    }
  }
  
  return 0;
}

int tournament_release(void) {
  if (!is_initialized || process_index < 0) {
    return -1;
  }
  
  // Release locks from top to bottom (reverse order of acquisition)
  for (int level = 0; level < num_levels; level++) {
    int lock_idx = calculate_lock_index(level);
    int role = calculate_role(level);
    
    if (peterson_release(locks[lock_idx], role) < 0) {
      return -1;
    }
  }
  
  return 0;
}