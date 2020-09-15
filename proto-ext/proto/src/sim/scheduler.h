/* Special-purpose priority queue variant
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include <stdlib.h>
#include <math.h>

#ifndef __SCHEDULER__
#define __SCHEDULER__

// The scheduler is a priority queue designed for simulations where
// most devices are evolving cyclically at a fairly similar rate.
// It performs well when devices execution is well-spread through time and
// poorly when they are closely synchronized.

struct Event {
  void* target;
  double true_time;
  double internal_time;
  int type;
  int uid;
};

/*****   DATA STRUCTURE   *****/
// At 20 bytes per event, the total memory needed is about 28*N
// which is about 3MB for 100K nodes, and is acceptable.
struct evtList {
  struct evtList* prev;
  struct evtList* next;
  Event e;
};

class Scheduler {
  int num_slots;                   // number of total slots
  int cur_slot;                    // pointer to start looking for next event
  double slot_cycle_time;          // time covered by the set of timeslots
  evtList **queue;                 // one cycle worth of timeslots
  double working_min, working_max; // bounds of current cycle
  Event* scratch;                  // for simplifying a return problem
  int bound_slot;                  // slot where searching may stop
  double bound_time;               // time when searching will stop
  int cycle_safety;                // infinite loop preventer
  
  // internal routines
  Event* insert_evt(int slot, double time);
  int get_next_from_cur_slot();
  void advance_cur_slot();
    
 public:
  Scheduler(int num_users, double cycle_time);
  ~Scheduler();
  static void test(); // a regression test fn

  void schedule_event(void* target, double true_time, double internal_time,
                      int type, int uid);
  // The bound is a governor intended to prevent the system from going too
  // far ahead in a burst of pulls.  It is not intended for tight control
  // of time evolution; events may not return when they are close to the
  // bound (within 2*cycle_time/num_users seconds)
  void set_bound(double time);
  // tests whether there's an event in the next cycle.  If so, removes the
  // event from the queue, puts its contents in evt, and returns true
  int pop_next_event(Event *evt);
  // There is no remove method: when a target dies, its events are
  // left in the queue and should be discarded when they appear.
  // This is because there are generally few events per target.
  // UID is included to allow reuse of target memory for different targets.
};

#endif // __SCHEDULER__
