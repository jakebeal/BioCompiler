/* Registry creation app
Copyright (C) 2005-2010, Jonathan Bachrach, Jacob Beal, and contributors
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include <assert.h>
#include <dirent.h>
#include <ltdl.h>
#include <errno.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>

#include "ir.h"
#include "plugin_manager.h"
#include "proto_plugin.h"
#include "scoped_ptr.h"
#include "spatialcomputer.h"
#include "utils.h"
#include "config.h"

using namespace std;

// Dummy declarations to fill in simulator names required to dlopen plugins.
Device *device = 0;
SimulatedHardware *hardware = 0;
Machine *machine = 0;
void *palette = 0;

// Touch a neocompiler element to ensure it gets linked in.
ProtoBoolean pb;

// Miscellaneous utilities

#define DISALLOW_COPY_AND_ASSIGN(T)    \
  void operator=(const T &);           \
  T(const T &)

// scoped_lt_dlhandle is a kludge rather than a scoped_ptr_malloc<T,
// D> for some D, because the API uses the opaque type lt_dlhandle
// rather than the sensible type `struct lt_dlhandle *' or similar.

class scoped_lt_dlhandle {
 public:
  explicit scoped_lt_dlhandle(lt_dlhandle handle) : handle_(handle) {}
  ~scoped_lt_dlhandle() { if (handle_) (void)lt_dlclose(handle_); }
  lt_dlhandle get(void) const { return handle_; }
  bool operator==(lt_dlhandle handle) const { return (handle == handle_); }
  bool operator!=(lt_dlhandle handle) const { return (handle != handle_); }

 private:
  lt_dlhandle handle_;
  DISALLOW_COPY_AND_ASSIGN(scoped_lt_dlhandle);
};

class ScopedPtrClosedir {
 public:
  inline void operator()(void *x) const
    { (void)closedir(static_cast<DIR *>(x)); }
};

typedef scoped_ptr_malloc<DIR, ScopedPtrClosedir> scoped_dirp;

class file_remover {
 public:
  explicit file_remover(const string &pathname)
    : pathname_(pathname), done_(false) {}
  ~file_remover()
    { if (!done_) { (void)remove(pathname_.c_str()); done_ = true; } }
  void done(void) { done_ = true; }

 private:
  string pathname_;
  bool done_;
  DISALLOW_COPY_AND_ASSIGN(file_remover);
};

#define file_remover(...) You forgot to name your file remover.

class ltdl_initexit {
 public:
  explicit ltdl_initexit(void) { status_ = lt_dlinit(); }
  ~ltdl_initexit() { if (status_ == 0) lt_dlexit(); }
  int status() const { return status_; }

 private:
  int status_;
  DISALLOW_COPY_AND_ASSIGN(ltdl_initexit);
};

#define ltdl_initexit(...) You forgot to name your ltdl initexit.

// Registing plugins

static bool
register_plugin(const string &plugin_directory, const string &filename,
    ofstream *registry_stream)
{
  string pathname = plugin_directory + "/" + filename;

  // Open the plugin.
  scoped_lt_dlhandle handle(lt_dlopenext(pathname.c_str()));
  if (handle == 0) {
    string error(lt_dlerror());
    cerr << "Unable to open plugin `" << pathname << "': " << error << "\n";
    return false;
  }

  // Rummage about for the routine to give us the inventory.
  void *symbol = lt_dlsym(handle.get(), "get_proto_plugin_inventory");
  if (symbol == 0) {
    string error(lt_dlerror());
    cerr << "Unable to register plugin `" << filename << "': "
         << "missing plugin inventory: " << error << "\n";
    return false;
  }

  // Get the inventory.
  cout << "Reading inventory of `" << filename << "'...\n";
  get_inventory_func get_inventory
    = reinterpret_cast<get_inventory_func>(symbol);
  string inventory((*get_inventory)());
  print_indented(2, inventory, true);
  (*registry_stream)
    << "# Inventory of plugin `" << filename << "'\n" << inventory << "\n";

  return true;
}

static bool
plugin_filename_p(const string &filename)
{
  return ((filename.size() > 3) && (filename.substr(0, 3) == "lib"));
}

// Reading the plugin directory

static int
register_plugins(const string &plugin_directory, ofstream *registry_stream)
{
  // Store the plausible plugin filenames in lexicographic order, for
  // consistent output.
  set<string> plausible_plugins;
  size_t n_registered = 0;

  {
    // Open the directory.
    scoped_dirp dirp(opendir(plugin_directory.c_str()));
    if (dirp == 0) {
      string error(strerror(errno));
      cerr << "Unable to open plugin directory `" << plugin_directory << "': "
           << error << "\n";
      return 1;
    }

    // Enumerate each plausibly plugin-named file in the directory.
    const struct dirent *dirent;
    while ((dirent = readdir(dirp.get())) != 0) {
      string filename(dirent->d_name);
      if (plugin_filename_p(filename)) {
        size_t dot = filename.rfind('.');
        if (dot)
          filename = filename.substr(0, dot);
        plausible_plugins.insert(filename);
      }
    }
  }

  {
    // Make sure the libtool dynamic loader library is ready.
    ltdl_initexit ltdl;
    if (ltdl.status() != 0) {
      string error(lt_dlerror());
      cerr << "Unable to initialize ltdl: " << error << "\n";
      return 1;
    }

    for (set<string>::const_iterator i = plausible_plugins.begin();
         i != plausible_plugins.end();
         ++i) {
      const string &filename = (*i);
      if (register_plugin(plugin_directory, filename, registry_stream))
        n_registered += 1;
    }
  }

  // FIXME: This is totally bogus, but I don't understand what's going
  // on with Windows.  If you do understand, please fix this.
#if !defined(_WIN32) && !defined(_CYGWIN)
  if (n_registered == 0) {
    cerr << "No plugins found!\n";
    return 3;
  }
#endif

  return 0;
}

// Main

int
main(int argc, char **argv)
{
  // Parse arguments.
  Args args(argc, argv);
  string plugin_directory
    = (args.extract_switch("--plugin-directory") ? args.pop_next()
       : ProtoPluginManager::PLUGIN_DIR);
  string registry_pathname
    = (args.extract_switch("--registry-file") ? args.pop_next()
       : (plugin_directory + "/" + ProtoPluginManager::REGISTRY_FILE_NAME));

  // Check for excess arguments.
  if (args.argc > 1) {
    cerr << "Usage: " << argv[0]
         << " [--plugin-directory <pathname>]"
         << " [--registry-file <pathname>]\n";
    return 2;
  }

  // FIXME: Need to use a better temporary file creation mechanism, or
  // guarantee that the directory is not world-writable, to avoid
  // symlink attacks.  Unlikely to matter, but good practice.
  string temporary_registry_pathname = (registry_pathname + ".tmp");

  // Open the new file at a temporary pathname.
  ofstream registry_stream(temporary_registry_pathname.c_str());
  if (!registry_stream.good()) {
    string error(strerror(errno));
    cerr << "Unable to open temporary file `" << temporary_registry_pathname
         << "': " << error << "\n";
    return 1;
  }

  // Make sure we nuke the new file if we lose.
  file_remover remove_temporary_registry(temporary_registry_pathname);

  // Register the plugins, stopping here if it fails.
  int result = register_plugins(plugin_directory, &registry_stream);
  if (result != 0)
    return result;

  // Finish up.  Bail if anything went wrong.
  registry_stream.close();
  if (registry_stream.fail()) {
    cerr << "Unable to write registry and I don't know why!\n";
    return 1;
  }

  // Commit the new file to its permanent location.
#ifdef _WIN32
  // Evidently Windows is a brain-damaged steaming pile of balderdash.
  // Yes, this means that the update won't be atomic.  Win loses.
  (void)remove(registry_pathname.c_str());
#endif
  if (rename(temporary_registry_pathname.c_str(), registry_pathname.c_str())
      == -1) {
    string error(strerror(errno));
    cerr << "Unable to commit registry file `" << registry_pathname << "': "
         << error << "\n";
    return 1;
  }
  remove_temporary_registry.done();

  return 0;
}
