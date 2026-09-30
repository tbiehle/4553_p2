#include <sys/types.h>
#include <sys/resource.h>
#include <unistd.h>
#include "lwp.h"
#include <sys/mman.h>
#include <rr.h>

#define MEGABYTE (1024 * 1024);

struct scheduler *s = &rr_publish;

tid_t lwp_create(lwpfun fun, void *arg) {
  // use sysconf() to get page size
  long pagesize = sysconf(_SC_PAGESIZE);
  if (pagesize < 0) exit();

  // use getrlimit() to get soft resource limit (RLIMIT_STACK)
  struct rlimit stack_limit;
  int status = getrlimit(RLIMIT_STACK, &stack_limit);
  if (status < 0) exit();

  int rlimit;
  // if RLIMIT_STACK is null, set resource limit to 8MB
  if (stack_limit.rlim_cur == NULL || stack_limit.rlim_cur == RLIM_INFINITY) {
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
  int *stack_base = mmap(NULL, stack_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
  stack_base += stack_size; // stack grows upwards, so move to end of memory to grow backwards later

  // TODO
  // SAVE REGISTER CONTEXT

  // create the thread
  thread lib_one, lib_two, sched_one, sched_two, exited;
  thread t = {
    0, stack_base, stack_size, NULL, // <-- CREATE RFILE
    LWP_LIVE, lib_one, lib_two, sched_one, sched_two, exited
  };

  s->admit(t);
}