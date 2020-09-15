/* 
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/signal.h>
#include <stdio.h>
#include "util.h"
#include "button.h"
#include "main.h"

uint8_t button_status = FALSE;  // not clicked yet
volatile uint16_t button_timer; // for timing things like debounces and multiclicks
uint8_t button_disabled = TRUE;

int8_t getButton(void) {
  uint8_t t;
  
  disableRTC();     // just so we dont conflict here
  if ((button_timer < BUTTON_DEBOUNCE) ||
      ((button_status == SINGLE_CLICK) && 
       (button_timer < DOUBLE_CLICK_TIMEOUT)))
    {
      enableRTC();
      return FALSE;       // wait until we're sure its a single click!
    }

  t = button_status;
  button_status = FALSE; // reset status!
  button_timer = 0;
  enableRTC();
  return t;
}

void enableButton(void) {
  button_disabled = FALSE;
  button_timer = 0;
  button_status = FALSE;
}

void disableButton(void) {
  button_disabled = TRUE;
  button_status = FALSE;
}
