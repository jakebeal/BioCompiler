/* Example of stand-alone plugin development
   Distributed in the public domain.
*/

#include "ExamplePlugin.h"

/*************** Plugin Library ***************/
void* ExamplePlugin::get_sim_plugin(string type,string name,Args* args, 
                                            SpatialComputer* cpu, int n) {
  if(type == DISTRIBUTION_PLUGIN) {
    if(name == DIST_NAME) { return new FooDistribution(n,cpu->volume,args); }
  } else if(type == TIMEMODEL_PLUGIN) {
    if(name == TIME_NAME) { return new FooTime(args); }
  } else if(type == LAYER_PLUGIN) {
    if(name == LAYER_NAME) { return new FooLayer(args, cpu); }
  }
  return NULL;
}

string ExamplePlugin::inventory() {
  return "# Example plugin\n" +
    registry_entry(DISTRIBUTION_PLUGIN,DIST_NAME,DLL_NAME) +
    registry_entry(TIMEMODEL_PLUGIN,TIME_NAME,DLL_NAME) +
    registry_entry(LAYER_PLUGIN,LAYER_NAME,DLL_NAME);
}

extern "C" {
  ProtoPluginLibrary* get_proto_plugin_library() 
  { return new ExamplePlugin(); }
  const char* get_proto_plugin_inventory()
  { return (new string(ExamplePlugin::inventory()))->c_str(); }
}
