/* Special-purpose priority queue variant
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include "config.h"
#include <stdio.h>
#include "scheduler.h"

/*****   IMPLEMENTATION *****/
// Assuming an even distribution of N users into S time-slots, 
// there is expected to be 4*N/S users per slot (each user has an
// execute event and a transmit event, both generally in the front half
// Cycle time is the expected time of a devices' round
Scheduler::Scheduler(int num_users, double cycle_time) {
  cur_slot=0;
  num_slots = num_users;
  queue = (evtList**)calloc(sizeof(evtList*),num_slots);
  slot_cycle_time = cycle_time*2;
  working_min = 0; working_max = slot_cycle_time;
  cycle_safety = num_slots*10;
}

Scheduler::~Scheduler() {
  for(int i=0;i<num_slots;i++) {
    evtList *l = queue[i];
    while(l) { evtList* next=l->next; free(l); l=next; }
  }
  free(queue);
}

Event* Scheduler::insert_evt(int slot, double time) {
  evtList *new_evt = (evtList*)malloc(sizeof(evtList));
  evtList *l = queue[slot];
  while(l) {
    if(time<l->e.true_time) {
      new_evt->prev=l->prev; new_evt->next=l; l->prev=new_evt;
      if(new_evt->prev) new_evt->prev->next=new_evt;
      if(queue[slot]==l) queue[slot]=new_evt;
      return &new_evt->e;
    } else if(!l->next) {
      break;
    } else {
      l=l->next;
    }
  }
  if(!queue[slot]) queue[slot]=new_evt;
  new_evt->prev=l; new_evt->next=NULL; if(l) l->next=new_evt;
  return &new_evt->e;
}

void copy_evt(Event* from, Event* to) {
  to->target=from->target; to->true_time=from->true_time; 
  to->internal_time=from->internal_time; 
  to->type=from->type; to->uid=from->uid;
}

void Scheduler::schedule_event(void* target, double true_time, 
                               double internal_time, int type, int uid) {
  // wrap time to choose slot
  double wraps = fmod(true_time,slot_cycle_time);
  int slot = (int)((wraps*num_slots)/slot_cycle_time);
  //printf("Event for %f added to slot %d\n",true_time,slot);
  Event* e = insert_evt(slot,true_time); // find a usable event
  e->target = target; e->true_time=true_time; e->internal_time=internal_time; 
  e->type=type; e->uid=uid;
}

// returns 1 if the list is well structured, 0 otherwise
int test_validity(evtList* l, double prev_time) {
  if(!l) return 1;
  return (l->e.true_time>prev_time) && test_validity(l->next, l->e.true_time);
}

/*** deletion routines ***/

// find if there's an event in the working range.  If so, put it in scratch
// and shrink the contents of the slot
// return 1 when successful
int Scheduler::get_next_from_cur_slot() {
  evtList *l = queue[cur_slot];
  if(l && l->e.true_time<=bound_time) { // used to be working_max
    copy_evt(&l->e,scratch);
    // remove event from list and deallocate
    if(l->next) l->next->prev=l->prev;
    if(l->prev) { l->prev->next=l->next; } else { queue[cur_slot]=l->next; }
    free(l);
    return 1;
  } else return 0;
}

// move the current slot forward, wrapping if necessary
void Scheduler::advance_cur_slot() {
  cur_slot++;
  if(cur_slot >= num_slots) {
    cur_slot=0;
    working_min = working_max; working_max += slot_cycle_time;
  }
}

// interface functions
void Scheduler::set_bound(double time) {
  bound_time = time;
  bound_slot = ((int)((time-working_min)/slot_cycle_time*num_slots))%num_slots;
  //printf("Bounds set to time %f and slot %d\n",bound_time,bound_slot);
}
int Scheduler::pop_next_event(Event *evt) {
  int i=0;
  scratch=evt; // set return location
  while(1) {
    if(get_next_from_cur_slot()) return 1; // when true, evt contains answer
    if(cur_slot==bound_slot && bound_time<working_max) return 0;
    advance_cur_slot();
    if(++i>cycle_safety) {
      printf("WARNING: Scheduler cycled %d times, to slot %d in ",i,cur_slot);
      printf("in range (%.2f to %.2f) with bound %.2f\n",working_min,
	   working_max,bound_time);
      return 0;
    }
  }
}


// Test for correct behavior:
// Events should return in order: 4 0 2 1 3 _ _ 5 6 _
void Scheduler::test() {
  printf("Starting scheduler...\n");
  Scheduler *sch = new Scheduler(10, 1.0);
  printf("Loading events...\n");
  sch->schedule_event((void*)2000, 0.3    ,0.1,3000,4000);
  sch->schedule_event((void*)2001, 0.6002 ,0.2,3001,4001);
  sch->schedule_event((void*)2002, 0.6001 ,0.3,3002,4002);
  sch->schedule_event((void*)2003, 0.6003 ,0.4,3003,4003);
  sch->schedule_event((void*)2004, 0.1    ,0.5,3004,4004);
  sch->schedule_event((void*)2005, 5.3    ,0.6,3005,4005);
  sch->schedule_event((void*)2006, 6.1    ,0.7,3006,4006);
  printf("Extracting events...\n");
  Event e;
  int i;
  for(i=0;i<10;i++) {
    sch->set_bound((i+1)*0.7);
    int got = sch->pop_next_event(&e);
    if(got) {
      printf("[%p, %f, %f %d %d]\n",e.target,e.true_time,e.internal_time,
             e.type,e.uid);
    } else {
      printf("Pop returned no event through slot %d after %f\n",
             sch->cur_slot,sch->working_min);
    }
  }
  printf("Done.\n");
}

/*
int main(int argc, char *argv[]) {
  Scheduler::test();
}
*/
