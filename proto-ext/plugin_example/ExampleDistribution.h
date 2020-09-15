/* Example of stand-alone plugin development
   Distributed in the public domain.
 */

#include <proto/spatialcomputer.h>

// Foo Distribution puts points in a spiral out from the center
class FooDistribution : public Distribution {
 public:
  float rad; float d; int i;
  FooDistribution(int n, Rect* volume, Args* args);
  bool next_location(METERS *loc); // called once per device
};
