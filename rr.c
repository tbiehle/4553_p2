#include <lwp.h>
#include <stdlib.h>

// void   (*init)(void);            /* initialize any structures     */
// void   (*shutdown)(void);        /* tear down any structures      */
// void   (*admit)(thread new);     /* add a thread to the pool      */
// void   (*remove)(thread victim); /* remove a thread from the pool */
// thread (*next)(void);            /* select a thread to schedule   */
// int    (*qlen)(void);            /* number of ready threads       */

int q_len = 0;
thread curr_thread;
thread head;
thread end;

void rr_admit(thread new) {
  if (head == NULL) head = end = new;

  end->sched_two = new;
  head->sched_one = new;
  new->sched_one = end;
  new->sched_two = head;
  end = new;

  q_len++;
}

void rr_remove(thread victim) {
  if (q_len == 0) return;

  victim->sched_one->sched_two = victim->sched_two;
  victim->sched_two->sched_one = victim->sched_one;
  victim->sched_one = NULL;
  victim->sched_two = NULL;

  q_len--;
}

thread rr_next() {
  curr_thread = curr_thread->sched_two;
  return curr_thread;
}

int rr_qlen() {
  return q_len;
}

struct scheduler rr_publish = {NULL, NULL, rr_admit, rr_remove, rr_next, rr_qlen};
scheduler RoundRobin = &rr_publish;
