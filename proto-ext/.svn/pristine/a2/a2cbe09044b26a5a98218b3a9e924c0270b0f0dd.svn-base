/* Utilities & standard top-level types
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef PROTO_SHARED_UTILS_H
#define PROTO_SHARED_UTILS_H

#include <dlfcn.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <algorithm>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <vector>

// Kludge to make evil copy and clobber (`assign') constructors fail
// to link.  This should have been the default.

#define DISALLOW_COPY_AND_ASSIGN(T)    \
  void operator=(const T &);           \
  T(const T &)

/*****************************************************************************
 *  NUMBERS AND DIMENSIONS                                                   *
 *****************************************************************************/

// Numbers

#define BOUND bound

template<typename T>
inline const T &
bound(const T &x, const T &y, const T &z)
{
  return std::min(std::max(x, y), z);
}

typedef float flo; // short name that jrb likes

// time and space measurement
typedef double SECONDS;
typedef flo METERS;

struct Rect {
  METERS l, r, b, t; // right>left, top>bottom
  Rect(METERS left, METERS right, METERS bottom, METERS top)
    : l(left), r(right), b(bottom), t(top) {}
  virtual Rect *clone() const { return new Rect(l, r, b, t); }
  virtual int dimensions() const { return 2; }
};

struct Rect3 : public Rect {
  METERS f, c;   // ceiling>floor
  Rect3(METERS left, METERS right, METERS bottom, METERS top, METERS floor,
      METERS ceiling)
    : Rect(left, right, bottom, top), f(floor), c(ceiling) {}
  virtual Rect *clone() const { return new Rect3(l, r, b, t, f, c); }
  virtual int dimensions() const { return 3; }
};

// uniform random numbers
flo urnd(flo min, flo max);

/*****************************************************************************
 *  SMALL MISC EXTENSIONS                                                    *
 *****************************************************************************/

// FIXME: MALLOC and FREE are a losing hack only for the
// paleocompiler.  Kill!

#ifndef MALLOC // avoid conflicts with MALLOC.H
extern void  *MALLOC(size_t size);
#endif
extern void  FREE(void *ptr);

#define for_set(t, x, i)                                        \
  for (std::set<t>::iterator i = (x).begin() ; i != (x).end(); ++i)

#define for_map(tk, td, x, i)                                           \
  for (std::map<tk, td>::iterator i = (x).begin(); i != (x).end(); ++i)

bool str_is_number(const char *str);

// These next three functions do not promise their values will remain
// past the next call to any of the three: they're for string construction.
const char *bool2str(bool b);
const char *flo2str(float num, unsigned int precision = 2);
const char *int2str(int num);

// Prints a block of text with n spaces in front of each line.
void print_indented(size_t n, const std::string &s,
    bool trim_trailing_newlines = false);

// Ensures that a string ends with extension.
// FIXME: Pass a string pointer, not a string reference, for base.
void ensure_extension(std::string &base, const std::string &extension);

/*****************************************************************************
 *  NOTIFICATION FUNCTIONS                                                   *
 *****************************************************************************/

// (since we may want to change the "printf" behavior on some platforms)

extern "C" void uerror(const char *message, ...);
extern "C" void debug(const char *dstring, ...);
extern "C" void post(const char *pstring, ...);

/*****************************************************************************
 *  COMMAND LINE ARGUMENT SUPPORT                                            *
 *****************************************************************************/

// Makes it easy to have multiple routines that pull out switches during boot

// FIXME: Need to switch all this crap to replace char * by string.

class Args {
 public:
  // Number of arguments.
  int argc;

  // Null-terminated array of pointers to argument strings.
  char **argv;

  Args(int argc_, char** argv_)
      : argc(argc_), argv(argv_), argp_(1), last_switch_("(no switch yet)") {
    add_defaults();
  }

  // Test if sw is in the list; leave pointer there.
  bool find_switch(const char *sw);

  // Like find_switch, but delete if found.
  bool extract_switch(const char *sw, bool warn = true);

  // Remove the argument at the pointer and returns it.
  char *pop_next();

  // Return the argument at the pointer without removing.
  char *peek_next() const;

  // Like pop_next, but converts to number.
  double pop_number();

  // pop_number, converted to an int.
  int pop_int();

  // Reset the pointer to the start of the arguments.
  void goto_first();

  // Shrink the list, deleting the ith argument.
  void remove(size_t i);

  // Modify a default switch.
  void undefault(bool *value, const char *positive, const char *negative);

  // Save the current pointer for later recall.
  void save_ptr();

  // Set pointer to the last saved, which is then unsaved.
  void restore_ptr();

 private:
  // Index of the current argument; 0 is the command, starts at 1.
  int argp_;

  // Last successfully found switch.
  const char *last_switch_;

  // Stack of saved pointers.
  std::vector<int> save_ptrs_;

  // Read args from .[appname] and ~/.[appname] files.
  void add_defaults();
  void parse_argstream(std::istream *s);

  DISALLOW_COPY_AND_ASSIGN(Args);
};

/*****************************************************************************
 *  POPULATION                                                               *
 *****************************************************************************/

// A Population is a cross between an array and a list
// It is designed w. fast random access, compact storage, and automatic resize

// FIXME: This should be templatized; void * is wrong.

class Population {
 public:
  Population() : population_size_(0) {}

  size_t add(void *member);     // Add an item; return where it went.
  void *remove(size_t i);       // Remove the item at i and return it.
  void destroy(size_t i);       // Idempotent freeing of item at location i.
  void *get(size_t i) const;    // Return the item at i, or null if empty.
  void clear();                 // Remove every item in the population.

  size_t size() const { return population_size_; }
  size_t max_id() const { return vector_.size(); }

 private:
  size_t population_size_;      // Number of slots that are full.
  std::queue<size_t> recycled_; // Queue of slot indices to be recycled.
  std::vector<void *> vector_;  // Data.

  DISALLOW_COPY_AND_ASSIGN(Population);
};

/*****************************************************************************
 *  STL HELPERS                                                              *
 *****************************************************************************/

// FIXME: insert_at and delete_at are obsolete; they should be spelled
// with the relevant STL routines.

template<class T>
T &
insert_at(std::vector<T> *v, size_t at, T &elt)
{
  size_t size = v->size();
  v->resize(size + 1);          // Add space to end.
  for (size_t i = size; i > at; i--)
    (*v)[i] = (*v)[i - 1];
  (*v)[at] = elt;
  return elt;
}

template<class T>
T
delete_at(std::vector<T> *v, size_t at)
{
  T elt = (*v)[at];
  for (size_t i = at; i < (v->size() - 1); i++)
    (*v)[i] = (*v)[i + 1];
  v->pop_back();
  return elt;
}

template<class T>
ssize_t
index_of(std::vector<T> *v, const T &elt, size_t start = 0)
{
  for (size_t i = start; i < v->size(); i++)
    if ((*v)[i] == elt)
      return i;
  return -1;
}

/*****************************************************************************
 *  GLUT-BASED EVENT MODEL                                                   *
 *****************************************************************************/

// Simple event model derived from GLUT
// The current mouse location in window coordinates
// [(0,0) in top-left corner, X increases rightward, Y increases downward]

// Older versions of GLUT do not support wheels

#ifndef GLUT_WHEEL_UP
# define GLUT_WHEEL_UP 3
#endif
#ifndef GLUT_WHEEL_DOWN
# define GLUT_WHEEL_DOWN 4
#endif

struct MouseEvent {
  int x;
  int y;
  int state; // 0=click, 1=drag start, 2=drag, 3=end drag, -1=drag failed
  bool shift; // only shift modifier, since CTRL and ALT select button on a Mac
  int button; // left, right, middle, or none (-1)?
  MouseEvent() : x(0), y(0), state(0), shift(false), button(-1) {}
};

// Warning: GLUT generates *different* numbers for control keys than normal
// keys.  Alphabetic keys give A=1, B=2, ... Z=26; Space is zero, others are
// not consistent across platforms.  Also, there is no case distinction.
// I do not know why this is, only that GLUT does this.

struct KeyEvent {
  bool normal; // is this a normal key or a GLUT special key?
  bool ctrl; // only CTRL: shift gives case, Macs mix ALT w. META and APPLE
  union {
    unsigned char key; // ordinary characters
    int special; // GLUT directionals and F-keys
  };
};

class EventConsumer {
 public:
  EventConsumer() {}
  virtual ~EventConsumer() {}

  // Returns true iff event consumed.
  virtual bool handle_key(KeyEvent *key) { return false; }

  // Returns true iff event consumed.
  virtual bool handle_mouse(MouseEvent *mouse) { return false; }

  // Draw, assuming a prepared OpenGL context.
  virtual void visualize() {}

  // Move state forward in time to the absolute time limit.  Returns
  // true iff state changed.  FIXME: This can't be right...
  virtual bool evolve(SECONDS limit) { return false; };

  // Visualizer utility: color management for static variables.
  void ensure_colors_registered(const std::string &classname);
  virtual void register_colors() {}

 private:
  static std::set<std::string> colors_registered;
  DISALLOW_COPY_AND_ASSIGN(EventConsumer);
};

// obtains the time in seconds (in a system-dependent manner)
double get_real_secs();

// System-dependent directory separator.

#ifdef __WIN32__
#define DIRECTORY_SEP '\\'
#else
#define DIRECTORY_SEP '/'
#endif

#endif  // PROTO_SHARED_UTILS_H
