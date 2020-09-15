/* 
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#define BUTTON_PIN      PIND
#define BUTTON_PORT     PORTD
#define BUTTON_DDR      DDRD
#define BUTTON          2
#define BUTTON_READ     ((BUTTON_PIN >> 2) & 0x1)


#define SINGLE_CLICK 10 // 250 ms.
#define DOUBLE_CLICK 11 // just a flag

#define BUTTON_PRESSED ((BUTTON_PIN & _BV(BUTTON)) == 0)

#define BUTTON_DEBOUNCE (25 / RTC_PERIOD)     // in ms
#define DOUBLE_CLICK_TIMEOUT (500 / RTC_PERIOD) // 1/2 second
#define REMOTE_DEBOUNCE_TIMER_RELOAD (200 / RTC_PERIOD)

void enableButton(void);
void disableButton(void);
int8_t getButton(void);
