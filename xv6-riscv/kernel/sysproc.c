#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

extern struct proc proc[NPROC];

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_memsize(void)
{
  return myproc()->sz;
}

// Coroutine-style yield: hand off execution to another process
// Returns the value passed by the target process, or -1 on error
// 
// Synchronization strategy without modifying proc struct:
// - Use chan field to encode waiting state: chan = (void*)(uint64)(1000000 + target_pid)
// - Use trapframe->a1 to store our outgoing value (so target can retrieve it)
// - Use trapframe->a0 for the return value (what target sends us)
//
// Key insight: If target is already waiting for us, we do DIRECT context switch
// to bypass the scheduler. Otherwise, we sleep and wait for target to yield to us.
uint64
sys_co_yield(void)
{
  int target_pid, value;
  struct proc *p = myproc();
  struct proc *target = 0;
  int i;
  uint64 result;

  // Get arguments: target PID and value to send
  argint(0, &target_pid);
  argint(1, &value);

  // Validate arguments
  if (target_pid <= 0 || target_pid == p->pid) {
    return -1;
  }

  // Find the target process
  for (i = 0; i < NPROC; i++) {
    if (proc[i].pid == target_pid) {
      target = &proc[i];
      break;
    }
  }

  if (!target) {
    return -1; // Target process not found
  }

  acquire(&p->lock);
  acquire(&target->lock);

   // Check if target exists and is not killed
  if (target->state == UNUSED || target->killed || target->state == ZOMBIE ) {
    release(&target->lock);
    release(&p->lock);
    return -1;
  }

  // Check if target is already waiting for co_yield from us
  if (target->state == SLEEPING && 
      target->chan == (void*)(uint64)(1000000 + p->pid)) {
    // Target IS waiting for us! Do DIRECT context switch.
    // Read target's stored value before it can be overwritten
    result = target->trapframe->a1;
    // Send our value to target
    target->trapframe->a0 = value;
    p->state = SLEEPING;
    p->chan = (void*)(uint64)(1000000 + target->pid);
    p->trapframe->a1 = value;
        
    target->state = RUNNING;
    // Update CPU to show target is now running
    struct cpu *c = mycpu();
    c->proc = target;
    
    release(&p->lock);
    
    // Direct context switch: save our context, load target's context
    // When target later co_yields back to us, it will switch us back here
    int intena = mycpu()->intena;
    swtch(&p->context, &target->context);
    mycpu()->intena = intena;
    
    // Control returns here when target yields back to us
    // Clean up the co_yield waiting marker
    p->chan = 0;
    release(&p->lock);
    return result;
  }

  // Target is NOT waiting for us, so we must sleep and wait for target to yield to us
  p->trapframe->a1 = value;
  p->chan = (void*)(uint64)(1000000 + target_pid);
  p->state = SLEEPING;

  release(&target->lock);
  
  swtch(&p->context, &mycpu()->context);

  // After waking up, retrieve the value that target passed to us
  // Target set our a0 when it made us RUNNABLE
  result = p->trapframe->a0;
  
  // Clean up the co_yield waiting marker
  p->chan = 0;
  release(&p->lock);
  return result;
}
