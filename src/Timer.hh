/**
 * @file   Timer.hh
 * @author David Reveman <david@waimea.org>
 * @date   05-Aug-2002 09:05:11
 *
 * @brief Definition of Timer and Interrupt classes
 *
 * Function declarations and variable definitions for Timer and Interrupt
 * classes.
 *
 * Copyright (C) David Reveman. All rights reserved.
 *
 */

#ifndef __Timer_hh
#define __Timer_hh

extern "C" {
#ifdef    TIME_WITH_SYS_TIME
#  include <sys/time.h>
#  include <time.h>
#else // !TIME_WITH_SYS_TIME
#  ifdef    HAVE_SYS_TIME_H
#    include <sys/time.h>
#  else // !HAVE_SYS_TIME_H
#    include <time.h>
#  endif // HAVE_SYS_TIME_H
#endif // TIME_WITH_SYS_TIME
}

class Timer;
class Interrupt;

#include "Menu.hh"

class Timer {
public:
    Timer(Waimea *);
    virtual ~Timer(void);

    void AddInterrupt(Interrupt *);
    void Start(void);
    void Pause(void);
    void ValidateInterrupts(XEvent *e);

    Waimea *waimea;
    list<Interrupt *> interrupts;
    bool paused;
    
private:
    struct itimerval timerval;
};

class Interrupt {
public:
    Interrupt(WaAction *, XEvent *, Window);
    
    Window id;
    WaMenuItem *wm;
    WaScreen *ws;
    struct timeval delay;
    WaAction *action;
    XEvent event;
};

void timeout(int);

#endif // __Timer_hh
