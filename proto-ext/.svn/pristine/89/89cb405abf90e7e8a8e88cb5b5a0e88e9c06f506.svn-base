/* Example of stand-alone plugin development
   Distributed in the public domain.
 */

#include <proto/proto_plugin.h>
#include <proto/spatialcomputer.h>

#include "ExampleDistribution.h"
#include "ExampleTimeModel.h"
#include "ExampleLayer.h"

#define DIST_NAME "foo-dist"
#define TIME_NAME "foo-time"
#define LAYER_NAME "foo-layer"
#define DLL_NAME "libexampleplugin"

class ExamplePlugin : public ProtoPluginLibrary {
public:
  void* get_sim_plugin(string type, string name, Args* args, 
                       SpatialComputer* cpu, int n);
  static string inventory();
};
