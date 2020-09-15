/* Example of stand-alone plugin development
   Distributed in the public domain.
 */

#include <proto/spatialcomputer.h>

// The FooLayer provides a sensor for a cyclic timer
class FooLayer : public Layer, public HardwarePatch {
 public:
  float cycle;
  FooLayer(Args* args, SpatialComputer* parent);
  void add_device(Device* d); // call to make per-device instances
  void dump_header(FILE* out); // print list of fields to log file "out"
 private:
  void foo_op(Machine* machine);
};

// This is the per-device instance for a FooLayer
class FooDevice : public DeviceLayer {
 public:
  FooLayer* parent;
  float foo_timer;
  FooDevice(FooLayer* parent, Device* container);
  void update();
  bool handle_key(KeyEvent* event);
  void copy_state(DeviceLayer* src) {} // to be called during cloning
  void dump_state(FILE* out, int verbosity); // print state to log file "out"
};
