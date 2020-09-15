/* Simulator's extensions to the kernel
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include "config.h"
#include <stdlib.h>
#include "sim-hardware.h"

void my_platform_operation(uint8_t op) {
  extern SimulatedHardware* hardware;
  hardware->dispatchOpcode(op);
}
