#include <sys/types.h>
#include <sys/resource.h>
#include <unistd.h>
#include "lwp.h"
#include "fp.h" // for FPU_INIT macro
#include "rr.h"
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>

#define MEGABYTE (1024 * 1024)

static tid_t next_tid = 1;

static thread head = NULL;
static thread end = NULL;

static thread current = NULL;

static void lwp_wrap(lwpfun fun, void *arg) {
  lwp_exit(fun(arg));
}

scheduler s = &rr_publish;

void lwp_start(void) {
  if (current != NULL) return; //  calling thread is already an LWP.

  thread original = malloc(sizeof(struct threadinfo_st));

  if (original == NULL) exit(EXIT_FAILURE);
  
  // set struct fields for parent thread
  original->tid = next_tid++;
  original->status = LWP_LIVE;

  // no need to allocate new stack
  original->stack = NULL; 
  original->stacksize = 0;

  // 0 all registers rid, rax etc.
  original->state = (rfile){0}; 
  original->state.fxsave = FPU_INIT;

  // make sched thread ptrs null
  original->sched_one = NULL; 
  original->sched_two = NULL;

  // no exit function for parent thread
  original->exited = NULL; 
  // The first yield saves the actual registers into original->state.


  // I'm not handling the case where head == NULL
  // not sure if we need to because that means that start() was called before create()
  original->lib_one = end;
  original->lib_two = head;
  end->lib_two = original;
  head->lib_one = original;
  end = original;
  

  current = original;
  s->admit(original);
  lwp_yield();
}

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
  
  
  thread created = malloc(sizeof(*created));

  // malloc fail -> unmap requested memory -> return NO_THREAD always an invalid thread id (from lwp.h)
  if (created == NULL) {
    munmap(stack, stack_size);
    return NO_THREAD;
  }

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

  s->admit(created);
  if (head == NULL){
    head = end = created;
  }

  head->lib_one = created;
  end->lib_two = created;
  created->lib_one = end;
  created->lib_two = head;
  end = created;
  return created->tid;
}


thread tid2thread(tid_t threadId){
  thread t = head; // where to start looking for the thread
  thread s = head;
  // while the tid is not equal keep iterating through the list
  while (t->tid != threadId){
    t = t->lib_two; // move to next thread

    if (t == s) return NO_THREAD;

  }

  return t;
}



tid_t lwp_gettid(){
  
  if (current == NULL) return NO_THREAD;

  return current->tid;
}

void lwp_set_scheduler(scheduler sched){
  thread curr = s->next();
  while (curr != NULL) {
    sched->admit(curr);
    curr = s->next();
    if (curr == NULL) break;
  }
}

scheduler lwp_get_scheduler() {
  return s;
}


void lwp_yield(){
  thread old = current;

  thread next_thread = s->next();

  if (next_thread == NULL) return exit(EXIT_FAILURE);

  current = next_thread;

  swap_rfiles(&old->state, &current->state);

}



void lwp_exit(int status){

  thread caller = current;
  caller->status = MKTERMSTAT(LWP_TERM, status);
  s->remove(caller);

  thread nextScheduled = s->next();
  if (nextScheduled == NULL) {
    exit(EXIT_FAILURE); // see what the correct exit status should be in spec? 
  }

  current = nextScheduled;
  swap_rfiles(NULL, &current->state); // NULL for first arg since that is "old" thread

  
}

tid_t lwp_wait(int *status){

  

}
