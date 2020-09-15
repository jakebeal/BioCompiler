/* Example of stand-alone plugin development
   Distributed in the public domain.
*/

#include "ExampleTimeModel.h"

/*************** FooTimer ***************/
// run timer faster farther from the center

void FooTimer::next_transmit(SECONDS* d_true, SECONDS* d_internal) {
  const flo* pos = device->body->position();
  float dist = sqrt((pos[0]*pos[0])+(pos[1]*pos[1])+(pos[2]*pos[2]));
  *d_true = *d_internal = 1/(1+dist/k)/2;
}
void FooTimer::next_compute(SECONDS* d_true, SECONDS* d_internal) {
  const flo* pos = device->body->position();
  float dist = sqrt((pos[0]*pos[0])+(pos[1]*pos[1])+(pos[2]*pos[2]));
  *d_true = *d_internal = 1/(1+dist/k);
}

FooTime::FooTime(Args* args) {
  k = args->extract_switch("-foo-k")?args->pop_number():10;
  if(k<=0) uerror("Cannot specify non-positive time steps.");
}
