/**
 * @file   Timer.hh
 * @author David Reveman <c99drn@cs.umu.se>
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

#include <sys/time.h>

class Timer;
class Interrupt;

#include "Waimea.hh"

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
    
private:
    struct itimerval timerval;
};

class Interrupt {
public:
    Interrupt(WaAction *, XEvent *);
    
    Window win;
    WaMenuItem *wm;
    WaScreen *ws;
    struct timeval delay;
    WaAction *action;
    XEvent event;
};

void timeout(int);

#endif // __Timer_hh
