/* Reader finds text and turns it into s-expressions
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef PROTO_COMPILER_READER_H
#define PROTO_COMPILER_READER_H

#include <fstream>
#include <list>
#include <string>

class List;
class Obj;

extern Obj *read_object(const char *string, int *start);

extern List *qq_env(const char *str, Obj *val, ...);
extern Obj *read_qq(const char *str, List *env);

extern Obj *read_from_str(const char *str);


// New-style path handling
struct Path {
  std::list<std::string> dirs;

  void add_default_path(const std::string &srcdir);
  void add_to_path(const std::string &addition) { dirs.push_back(addition); }
  std::ifstream *find_in_path(const char *filename) const
    { std::string s(filename); find_in_path(s); }
  std::ifstream *find_in_path(const std::string &filename) const;
};

extern List *read_objects_from_dirs(const std::string &filename,
    const Path *path);

#endif  // PROTO_COMPILER_READER_H
