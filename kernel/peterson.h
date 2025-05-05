#include "types.h"
#include "spinlock.h"

#define NPETERSON 16  // Support up to 16 Peterson locks

struct peterson {
  int active;          // Whether the lock is active (created)
  int flag[2];         // Flag for each role (0 or 1)
  int turn;            // Whose turn it is
  struct spinlock lk;  // Kernel-side protection for the peterson lock
};

extern struct peterson petersonlocks[NPETERSON];

// Function prototypes for internal kernel functions
void peterson_init(void);
int peterson_create(void);
int peterson_acquire(int lock_id, int role);
int peterson_release(int lock_id, int role);
int peterson_destroy(int lock_id);