/* Example of stand-alone plugin development
   Distributed in the public domain.
 */

#include <proto/spatialcomputer.h>

// FooTimer runs faster farther from the center
class FooTimer : public DeviceTimer {
 public:
  float k; // stretch factor on acceleration
  FooTimer(float k) { this->k = k; }
  void next_transmit(SECONDS* d_true, SECONDS* d_internal);
  void next_compute(SECONDS* d_true, SECONDS* d_internal);
  DeviceTimer* clone_device() { return new FooTimer(k); };
};

class FooTime : public TimeModel {
 public:
  float k;
  FooTime(Args* args);
  DeviceTimer* next_timer(SECONDS* start_lag) { return new FooTimer(k); }
  SECONDS cycle_time() { return 1; }
};
