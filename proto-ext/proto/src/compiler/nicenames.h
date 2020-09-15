#ifndef PROTO_COMPILER_NICENAMES_H
#define PROTO_COMPILER_NICENAMES_H

#include <string>

class Nameable {
 private:
  std::string assigned_name;
 public:
  Nameable();
  std::string nicename();
};

#endif  // PROTO_SHARED_PLUGIN_H
