#include "rr.h"
#include <stddef.h>

static int q_len = 0;
thread curr_thread = NULL;
static thread head = NULL;
static thread end = NULL;

void rr_admit(thread new) {
  if (head == NULL) {
    head = end = new;
    new->sched_one = new;
    new->sched_two = new;
  } else {
    new->sched_one = end;
    new->sched_two = head;
    end->sched_two = new;
    head->sched_one = new;
    end = new;
  }

  q_len++;
}

void rr_remove(thread victim) {
  if (q_len == 0) return;

  if (q_len == 1) {
    head = end = curr_thread = NULL;
  } else {
  
    if (curr_thread == victim) curr_thread = victim->sched_one;
    
    if (head == victim) head = victim->sched_two;

    if (end == victim) end = victim->sched_one;

    victim->sched_one->sched_two = victim->sched_two;
    victim->sched_two->sched_one = victim->sched_one;

  }
  victim->sched_one = NULL;
  victim->sched_two = NULL;

  q_len--;
}

thread rr_next(void) {
  if (head == NULL) return NULL;

  if (curr_thread == NULL) {
    curr_thread = head;
  } else {
    curr_thread = curr_thread->sched_two;
  }

  return curr_thread;
}

int rr_qlen(void) {
  return q_len;
}

struct scheduler rr_publish = {NULL, NULL, rr_admit, rr_remove, rr_next, rr_qlen};
scheduler RoundRobin = &rr_publish;
