#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "proc.h"
#include "peterson.h"

struct peterson petersonlocks[NPETERSON];

void
peterson_init(void)
{
  for(int i = 0; i < NPETERSON; i++) {
    petersonlocks[i].active = 0;
    petersonlocks[i].flag[0] = 0;
    petersonlocks[i].flag[1] = 0;
    petersonlocks[i].turn = 0;
    initlock(&petersonlocks[i].lk, "peterson");
  }
}

// Create a new Peterson lock and return its ID
int
peterson_create(void)
{
  for(int i = 0; i < NPETERSON; i++) {
    acquire(&petersonlocks[i].lk);
    if(petersonlocks[i].active == 0) {
      petersonlocks[i].active = 1;
      petersonlocks[i].flag[0] = 0;
      petersonlocks[i].flag[1] = 0;
      petersonlocks[i].turn = 0;
      release(&petersonlocks[i].lk);
      return i;
    }
    release(&petersonlocks[i].lk);
  }
  return -1;
}

// Acquire a Peterson lock with the given role
int
peterson_acquire(int lock_id, int role)
{
  if(lock_id < 0 || lock_id >= NPETERSON || (role != 0 && role != 1))
    return -1;
    
  acquire(&petersonlocks[lock_id].lk);
  if(petersonlocks[lock_id].active == 0) {
    release(&petersonlocks[lock_id].lk);
    return -1;
  }
  release(&petersonlocks[lock_id].lk);

  int other = 1 - role;  // The other role

  // Set flag[role] = 1 indicating intent to enter critical section
  __sync_lock_test_and_set(&petersonlocks[lock_id].flag[role], 1);
  
  // Give priority to the other process
  petersonlocks[lock_id].turn = other;
  __sync_synchronize();  // Memory barrier

  // Wait while the other process wants to enter and it's the other's turn
  while(petersonlocks[lock_id].flag[other] && petersonlocks[lock_id].turn == other) {
    // Instead of busy-waiting, yield the CPU
    yield();
    __sync_synchronize();  // Refresh memory view after returning from yield
  }

  return 0;
}

// Release a Peterson lock with the given role
int
peterson_release(int lock_id, int role)
{
  if(lock_id < 0 || lock_id >= NPETERSON || (role != 0 && role != 1))
    return -1;

  acquire(&petersonlocks[lock_id].lk);
  if(petersonlocks[lock_id].active == 0) {
    release(&petersonlocks[lock_id].lk);
    return -1;
  }
  release(&petersonlocks[lock_id].lk);

  // Set flag[role] = 0 indicating exit from critical section
  __sync_lock_release(&petersonlocks[lock_id].flag[role]);
  __sync_synchronize();  // Ensure the release is visible to other processors

  return 0;
}

// Destroy a Peterson lock by ID
int
peterson_destroy(int lock_id)
{
  if(lock_id < 0 || lock_id >= NPETERSON)
    return -1;

  acquire(&petersonlocks[lock_id].lk);
  if(petersonlocks[lock_id].active == 0) {
    release(&petersonlocks[lock_id].lk);
    return -1;
  }
  
  petersonlocks[lock_id].active = 0;
  petersonlocks[lock_id].flag[0] = 0;
  petersonlocks[lock_id].flag[1] = 0;
  petersonlocks[lock_id].turn = 0;
  release(&petersonlocks[lock_id].lk);
  
  return 0;
}