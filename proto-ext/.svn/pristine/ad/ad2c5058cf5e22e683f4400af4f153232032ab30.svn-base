/* Example of stand-alone plugin development
   Distributed in the public domain.
*/

#include "ExampleLayer.h"

#define FOO_OP "foo boolean scalar"

/*************** FooLayer ***************/
// Cyclic timer
FooLayer::FooLayer(Args* args, SpatialComputer* p) : Layer(p) {
  post("Example plugin online!\n");
  this->parent=parent;
  cycle = args->extract_switch("-foo-cycle")?(int)args->pop_number():3;
  args->undefault(&can_dump,"-Dfoo","-NDfoo");
  // register hardware functions
  parent->hardware.registerOpcode(new OpHandler<FooLayer>(this, &FooLayer::foo_op, FOO_OP));
}

void FooLayer::foo_op(Machine* machine) {
  float want = machine->stack.popNumber(), val = -1;
  if(want) val = ((FooDevice*)device->layers[id])->foo_timer;
  machine->stack.push(val);
}

// Called by the simulator to initialize a new device
void FooLayer::add_device(Device* d) {
  d->layers[id] = new FooDevice(this,d);
}

// Called when recording state to file
void FooLayer::dump_header(FILE* out) {
  if(can_dump) fprintf(out," \"FOOTIME\"");
}


/*************** Per-device instance for FooLayer ***************/
FooDevice::FooDevice(FooLayer* parent,Device* d) : DeviceLayer(d) {
  this->parent=parent;
  foo_timer = parent->cycle;
}

void FooDevice::dump_state(FILE* out, int verbosity) {
  if(verbosity==0) { fprintf(out," %.2f", foo_timer);
  } else { fprintf(out,"Example plugin timer %.2f\n",foo_timer);
  }
}

void FooDevice::update() {
  foo_timer -= (machine->time - machine->last_time);
  if(foo_timer<=0) foo_timer += parent->cycle;
}

BOOL FooDevice::handle_key(KeyEvent* key) {
  // is this a key recognized internally?
  if(key->normal && !key->ctrl) {
    switch(key->key) {
    case 'B': foo_timer += 8; return TRUE;
    }
  }
  return FALSE;
}
