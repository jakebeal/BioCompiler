/* Utilities & standard top-level types
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include "utils.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>

#include "config.h"

using namespace std;

flo
urnd(flo min, flo max)
{
  // FIXME: What a crock!
  return min + (((max - min) * rand()) / RAND_MAX);
}

/*****************************************************************************
 *  NOTIFICATION FUNCTIONS                                                   *
 *****************************************************************************/

void
uerror(const char *message, ...)
{
  va_list ap;
  va_start(ap, message);
  vprintf(message, ap);
  printf("\n Aborting on fatal error\n");
  fflush(stdout);
  va_end(ap);
  exit(1);
}

void
debug(const char *dstring, ...)
{
  char buf[1024];
  va_list ap;
  va_start(ap, dstring);
  vsnprintf(buf, sizeof buf, dstring, ap);
  va_end(ap);
  fputs(buf, stderr);
  fflush(stderr);
}

void
post(const char *pstring, ...)
{
  va_list ap;
  va_start(ap, pstring);
  vprintf(pstring, ap);
  va_end(ap);
  fflush(stdout);
}

/*****************************************************************************
 *  MEMORY MANAGEMENT                                                        *
 *****************************************************************************/

// FIXME: MALLOC and FREE are a losing hack only for the
// paleocompiler.  Kill!

#define GC_malloc malloc
#define GC_free   free

void *
MALLOC(size_t size)
{
  return malloc(size);
}

void
FREE(void *ptr_)
{
  // FIXME: WTF?
  void **ptr = (void**)ptr_;
  if (*ptr == 0) uerror("TRYING TO FREE ALREADY FREED MEMORY\n");
  GC_free(*ptr);
  *ptr = 0;
}

/*****************************************************************************
 *  COMMAND LINE ARGUMENT SUPPORT                                            *
 *****************************************************************************/

void
Args::remove(size_t i)
{
  for (size_t j = 0; j < save_ptrs_.size(); j++)
    if (save_ptrs_[j] > i)
      save_ptrs_[j]--;

  // Shrink the size of the list.
  argc--;

  // Shuffle the arguments backward.
  for (; i < argc; i++)
    argv[i] = argv[i + 1];
}

// ARG_SAFE is used to check if two different things modules request the
// same argument
#define ARG_SAFE true
static vector<string> switch_rec;  // remember a set of switch arguments

bool
Args::extract_switch(const char *sw, bool warn)
{
  if (ARG_SAFE && warn) {
    string s = sw;
    // Warns the user if a switch is overloaded.
    for (size_t i = 0; i < switch_rec.size(); i++)
      if (s == switch_rec[i]) {
	debug("WARNING: Switch '%s' used more than once.\n", sw);
        break;
      }

    switch_rec.push_back(s);
  }

  if (!find_switch(sw))
    return false;

  remove(argp_);
  return true;
}

bool
Args::find_switch(const char *sw)
{
  for (argp_ = 1; argp_ < argc; argp_++) {
    if (strcmp(argv[argp_], sw) == 0) {
      last_switch_ = argv[argp_];
      return true;
    }
  }

  return false;
}

void
Args::goto_first()
{
  argp_ = 1;
}

char *
Args::pop_next()
{
  if (argp_ >= argc)
    return 0;
  char *save = argv[argp_];
  remove(argp_);
  return save;
}

char *
Args::peek_next() const
{
  if (argp_ >= argc)
    return 0;
  return argv[argp_];
}

double
Args::pop_number()
{
  // FIXME: Use a real number parser here...
  char *arg = pop_next();
  if (arg == 0 || !str_is_number(arg)) {
    if (arg != 0 && arg[0] == '0' && arg[1]=='x' && str_is_number(&arg[2])) {
      unsigned int hexnum;
      sscanf(arg, "%x", &hexnum);
      return hexnum;
    }
    uerror("Missing numerical parameter after '%s'", last_switch_);
  }
  return atof(arg);
}

int
Args::pop_int()
{
  return static_cast<int>(pop_number());
}

// . If only positive is present, value is set to true.
// . If only negative is present, value is set to false.
// . If neither, value is unchanged.
// . If both, value is set to false.
// FIXME: Why not just return it?
void
Args::undefault(bool *value, const char *positive, const char *negative)
{
  bool pp = extract_switch(positive), np = extract_switch(negative);
  if (pp)
    *value = !np;
  else if (np)
    *value = false;
}

void
Args::save_ptr()
{
  save_ptrs_.push_back(argp_);
}

void
Args::restore_ptr()
{
  argp_ = save_ptrs_.back();
  save_ptrs_.pop_back();
}

// Read args from optional .[appname] and ~/.[appname] files.

void
Args::add_defaults()
{
  char *dirstrip = strrchr(argv[0], DIRECTORY_SEP);
  string appname(dirstrip ? &dirstrip[1] : argv[0]);
  ifstream fin;

  string name = "." + appname;
  fin.open(name.c_str());
  if (fin.is_open()) {
    parse_argstream(&fin);
    fin.close();
  }

  const char *homedir = getenv("HOME");
  if (homedir) {
    name = string(homedir) + "/." + appname;
    fin.open(name.c_str());
    if (fin.is_open()) {
      parse_argstream(&fin);
      fin.close();
    }
  }
}

void
Args::parse_argstream(istream *s)
{
  string newargstr = "";

  while (!s->eof()) {
    string line;
    getline(*s, line, '\n');
    newargstr += ((line[0] != '#') ? line : " ");
  }

  if ((newargstr.find('"') != string::npos)
      || (newargstr.find('\'') != string::npos))
    debug("WARNING: default arg-file parsing ignores quotes\n");

  // Put into a mutable, long-term allocated c-str.
  char *newargs = static_cast<char *>(malloc(newargstr.size()));
  newargstr.copy(newargs, newargstr.size());
  newargs[newargstr.size()]=0;

  // Extract tokens.
  vector<char *> tokens;
  size_t n = 0;

  char *tok = strtok(newargs, " '\"");
  while (tok) {
    tokens.push_back(tok);
    n++;
    tok = strtok(0, " '\"");
  }

  // Merge with existing collection.
  char **newargv = static_cast<char **>(calloc(argc + n, sizeof(char *)));
  for (size_t i = 0; i < argc; i++) { newargv[i] = argv[i]; }
  for (size_t i = 0; i < n; i++) { newargv[i + argc] = tokens[i]; }

  // Note: ignoring old argv memory leaking: it's small.
  argc += n;
  argv = newargv;
}

/*****************************************************************************
 *  STRING UTILITIES                                                         *
 *****************************************************************************/

// Does the string contain a number after any initial whitespace?

bool
str_is_number(const char *str)
{
  // FIXME: Null test masks legitimate errors.
  if (str == 0)
    return false;

  size_t i = 0;

  // Remove whitespace.
  while (isspace(str[i]))
    i++;

  // Initial sign.
  if ((str[i] == '+') || (str[i] == '-'))
    i++;

  bool decimal_point = false, exponent_marker = false;
  do {
    if (!isdigit(str[i])) {
      if ((str[i] == '.') && !decimal_point && !exponent_marker)
        decimal_point = true;
      else if (((str[i] == 'e') || (str[i] == 'E')) && !exponent_marker)
        exponent_marker = true;
      else
        return false;
    }
    i++;
  } while(str[i] && !isspace(str[i]));

  return true;
}

const char *
bool2str(bool b)
{
  return b ? "true" : "false";
}

const char *
flo2str(float num, unsigned int precision)
{
  static char buffer[32] = "", tmpl[10] = "";
  snprintf(tmpl, sizeof tmpl, "%s%u%s", "%.", precision, "f");
  //printf("\nflo2str: %s\nFor precision: %d\n", tmpl, precision);
  snprintf(buffer, sizeof buffer, tmpl, num);
  return buffer;
}

const char *
int2str(int num)
{
  static char buffer[32] = "";
  snprintf(buffer, sizeof buffer, "%d", num);
  return buffer;
}

void
print_indented(size_t n, const string &s, bool trim_trailing_newlines)
{
  size_t loc = 0;
  while (true) {
    size_t newloc = s.find('\n', loc);
    if ((!trim_trailing_newlines) || newloc != string::npos) {
      for (size_t i = 0; i < n; i++) cout << " ";
      cout << s.substr(loc, (newloc - loc)) << endl;
    }
    if (newloc == string::npos)
      return;
    loc = newloc + 1;
  }
}

bool
ends_with(const string &base, const string &tail)
{
  size_t b_n = base.size(), t_n = tail.size();
  if (b_n < t_n)
    return false;
  for (size_t i = 0; i < t_n; i++)
    if (tail[i] != base[i + b_n - t_n])
      return false;
  return true;
}

void
ensure_extension(string &base, const string &extension)
{
  if (!ends_with(base, extension))
    base += extension;
}

/*****************************************************************************
 *  POPULATION CLASS                                                         *
 *****************************************************************************/

size_t
Population::add(void *member)
{
  size_t i;

  if (recycled_.empty()) {
    i = vector_.size();
    vector_.push_back(member);
  } else {
    i = recycled_.front();
    recycled_.pop();
    if (vector_[i] != 0)
      uerror("Population tried to over-write a node!");
    vector_[i] = member;
  }

  population_size_++;
  return i;
}

void *
Population::remove(size_t i)
{
  void *member = vector_.at(i);

  if (member != 0) {
    vector_[i] = 0;
    recycled_.push(i);
    population_size_--;
  }

  return member;
}

void
Population::destroy(size_t i)
{
  void *member = remove(i);
  if (member)
    free(member);
}

void
Population::clear()
{
  vector_.clear();
  while (!recycled_.empty())
    recycled_.pop();
  population_size_ = 0;
}

void *
Population::get(size_t i) const
{
  // Note: i is always non-negative because it is a size_t, which is unsigned
  if (i < vector_.size()) // 0 <= i && 
    return vector_[i];
  uerror("Attempted to access out of bounds member of Population!");
  return 0;
}

/*****************************************************************************
 *  EVENTCONSUMER                                                            *
 *****************************************************************************/

set<string> EventConsumer::colors_registered;

void
EventConsumer::ensure_colors_registered(const string &classname)
{
  if(!colors_registered.count(classname)) {
    register_colors();
    colors_registered.insert(classname);
  }
}

/*****************************************************************************
 *  GETTING TIME                                                             *
 *****************************************************************************/

#ifdef __WIN32__
// Winblows

# include <windows.h>
double
get_real_secs()
{
  DWORD tv = timeGetTime();
  return static_cast<double>(tv / 1000.0);
}

#else
// Unix

# include <sys/time.h>
double
get_real_secs ()
{
  struct timeval t;
  gettimeofday(&t, 0);
  return static_cast<double>(t.tv_sec)
    + static_cast<double>(t.tv_usec) / 1000000.0;
}

#endif
