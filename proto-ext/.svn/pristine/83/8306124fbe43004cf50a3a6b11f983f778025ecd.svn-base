/* Example of stand-alone plugin development
   Distributed in the public domain.
*/

#include "ExampleDistribution.h"

/*************** FooDistribution ***************/
// create a spiral
FooDistribution::FooDistribution(int n, Rect* volume, Args* args) : Distribution(n,volume) {
  rad = args->extract_switch("-foo-rad")?args->pop_number():0.1;
  d = min(width,height)/n/2; i=0;
}
bool FooDistribution::next_location(METERS *loc) {
  loc[0] = cos(rad*i)*d*i; loc[1] = sin(rad*i)*d*i; loc[2] = 0;
  i++; // increment for next point
  return true; // yes, make the device
}
