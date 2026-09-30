#include <sys/types.h>
#include <sys/resource.h>
#include <unistd.h>
#include "lwp.h"
#include "fp.h" // for FPU_INIT macro
#include "rr.h"
#include <sys/mman.h>
#include <stdlib.h>

#define MEGABYTE (1024 * 1024)

static tid_t next_tid = 1;

static void lwp_wrap(lwpfun fun, void *arg) {
  lwp_exit(fun(arg));
}

struct scheduler *s = &rr_publish;

tid_t lwp_create(lwpfun fun, void *arg) {
  // use sysconf() to get page size
  long pagesize = sysconf(_SC_PAGESIZE);
  if (pagesize < 0) return NO_THREAD;

  // use getrlimit() to get soft resource limit (RLIMIT_STACK)
  struct rlimit stack_limit;
  int status = getrlimit(RLIMIT_STACK, &stack_limit);
  if (status < 0) return NO_THREAD;

  int rlimit;
  // if RLIMIT_STACK is null, set resource limit to 8MB
  if (stack_limit.rlim_cur == 0 || stack_limit.rlim_cur == RLIM_INFINITY) {
    rlimit = 8 * MEGABYTE;
  } else rlimit = (int) stack_limit.rlim_cur; // soft limit

  int stack_size;
  // round the resource limit to a multiple of page size
  if (!(rlimit % pagesize == 0)) { // need to round
    int remainder = rlimit % pagesize;

    stack_size = rlimit + pagesize - remainder;
  } else {
    stack_size = rlimit;
  }

  // mmap the stack
  unsigned long *stack = mmap(NULL, stack_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
  
  if (stack == MAP_FAILED) return NO_THREAD;
  
  
  // allocate memory for the new thread 
  thread created = malloc(sizeof(thread));

  // malloc fail -> unmap requested memory -> return NO_THREAD always an invalid thread id (from lwp.h)
  if (created == NULL) {
    munmap(stack, stack_size);
    return NO_THREAD;
  }

  memset(created, 0, sizeof(thread)); // init all thread struct vals to 0
  
  // set thread struct vals
  created->tid = next_tid++;
  created->stack = stack;
  created->stacksize = stack_size;
  created->status = LWP_LIVE;
  
  // start frame at high-address so that stack can grow towards lower memory addresses
  // we keep the *stack as mmap returned val so that it can be passed to memunmap in case of malloc error
          // simply doing stack + stack_size moves in corresponding type increment so divide by sizeof()
  unsigned long *frame = stack + stack_size / sizeof(unsigned long);
  
  // decrement by sizeof(unsigned long) to next available address space on stack
  frame--;


  // fake return address for lwp_wrap(), correct execution never uses it because
  // lwp_wrap calls lwp_exit(), which does not return

  // since we're manually creating a stack, on x86 a function call has return value right under it (higher memory address)
  *frame = 0x12345678;  

  // swap_rfiles function ret instructions pops this address into RIP (equivalent of pc on x86) starting lwp_wrap
  frame--; 
  *frame = (unsigned long)lwp_wrap;

  // artificial  RBP popped by swap_rfiles leave instruction, just needs to be any value really as long as it's not empty
  frame--;
  *frame = 0;

  created->state.rdi = (unsigned long)fun;
  created->state.rsi = (unsigned long)arg;
  created->state.rbp = (unsigned long)frame;
  created->state.rsp = (unsigned long)frame;
  created->state.fxsave = FPU_INIT;

  RoundRobin->admit(created);
  return created->tid;
}

void lwp_start() {

}

thread tid2thread(tid_t tid) {
  
}

void lwp_yield() {
  thread next = RoundRobin->next();

  if (next == NULL) {
    exit(0); // TODO: Replace with status of current thread
  }


}