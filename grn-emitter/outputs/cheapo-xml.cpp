/* Base class for XML emission kludges
Copyright (C) 2009-2013, Raytheon BBN Technologies and contributors listed 
in the AUTHORS file in TASBE BioCompiler distribution's top directory.

This file is part of the TASBE BioCompiler, and is distributed under
the terms of the GNU General Public License, with a linking exception,
as described in the file LICENSE in the TASBE BioCompiler
distribution's top directory. */

#include "cheapo-xml.h"

void XMLitem::addComment(string comment) { addElt(new XMLcomment(comment)); }
void XMLitem::addBreak(int n) { addElt(new XMLbreak(n)); }

