/**
 * @file   Timer.cc
 * @author David Reveman <c99drn@cs.umu.se>
 * @date   05-Aug-2002 09:05:11
 *
 * @brief Implementation of Timer and Interrupt classes
 *
 * Timer implementation, used for delayed actions.
 *
 * Copyright (C) David Reveman. All rights reserved.
 *
 */

#include <signal.h>
#include <unistd.h>
#include <string.h>

#include "Timer.hh"

Timer *timer;

/**
 * @fn    Timer(void)
 * @brief Constructor for Timer class
 *
 * Sets SIGALRM signal handling and some initial values. Starts timer.
 *
 * @param wa Pointer to waimea object
 */
Timer::Timer(Waimea *wa) {
    struct sigaction action;

    timer = this;
    waimea = wa;
    timerval.it_interval.tv_sec = 0;
    timerval.it_interval.tv_usec = 0;
    timerval.it_value.tv_sec = 0;
    timerval.it_value.tv_usec = 0;
    action.sa_handler = timeout;
    action.sa_mask = sigset_t();
    action.sa_flags = 0;
    sigaction(SIGALRM, &action, NULL);

    Start();
}

/**
 * @fn    ~Timer(void)
 * @brief Destructor for Timer class
 *
 * Removes all interrupts, stops timer and resets SIGALRM signal handling.
 */
Timer::~Timer(void) {
    struct sigaction action;

    action.sa_handler = SIG_DFL;
    action.sa_mask = sigset_t();
    action.sa_flags = 0;
    timerval.it_value.tv_sec = 0;
    timerval.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &timerval, NULL);
    sigaction(SIGALRM, &action, NULL);
    LISTCLEAR3(interrupts);
}

/**
 * @fn    AddInterrupt(Interrupt *i)
 * @brief Adds interrupt to timer
 *
 * Inserts a new interrupt in the interrupt list and next timeout for the timer
 * is updated if needed.
 *
 * @param i Interrupt that should be added
 */
void Timer::AddInterrupt(Interrupt *i) {
    Pause();
    
    if (interrupts.empty())
        interrupts.push_back(i);
    else {
        list<Interrupt *>::iterator it = interrupts.begin();
        for (; it != interrupts.end(); ++it) {
            if (i->delay.tv_sec <= (*it)->delay.tv_sec &&
                i->delay.tv_usec < (*it)->delay.tv_usec) {
                interrupts.insert(it, i);
                break;
            }
        }
        if (it == interrupts.end())
            interrupts.push_back(i);
    }
    Start();
}

/**
 * @fn    Start(void)
 * @brief Starts timer
 *
 * Starts timer or if timer is paused it continues timer.
 */
void Timer::Start(void) {
    if (! interrupts.empty()) {
        timerval.it_value.tv_sec = interrupts.front()->delay.tv_sec;
        timerval.it_value.tv_usec = interrupts.front()->delay.tv_usec;
        if (timerval.it_value.tv_sec < 0) timerval.it_value.tv_sec = 0;
        if (timerval.it_value.tv_usec < 0) timerval.it_value.tv_usec = 0;
        if (timerval.it_value.tv_sec == 0 && timerval.it_value.tv_usec == 0)
            timerval.it_value.tv_usec = 1;
        setitimer(ITIMER_REAL, &timerval, NULL);
    }    
}

/**
 * @fn    Pause(void)
 * @brief Pause timer
 *
 * Stops timer but doesn't discard any interrupts. Call to Start function
 * will unpauses timer.
 */
void Timer::Pause(void) {
    struct itimerval remainval;
    struct timeval elipsedval;

    if (interrupts.empty()) return;
    
    timerval.it_value.tv_sec = 0;
    timerval.it_value.tv_usec = 0;
    getitimer(ITIMER_REAL, &remainval);
    setitimer(ITIMER_REAL, &timerval, NULL);
   
    elipsedval.tv_sec = interrupts.front()->delay.tv_sec - 
        remainval.it_value.tv_sec;
    elipsedval.tv_usec = interrupts.front()->delay.tv_usec -
        remainval.it_value.tv_usec;
    
    if (elipsedval.tv_usec < 0) {
        elipsedval.tv_sec--;
        elipsedval.tv_usec += 1000000;
    }

    list<Interrupt *>::iterator it = interrupts.begin();
    for (; it != interrupts.end(); ++it) {
        (*it)->delay.tv_sec -= elipsedval.tv_sec;
        (*it)->delay.tv_usec -= elipsedval.tv_usec;

        if ((*it)->delay.tv_usec < 0) {
            (*it)->delay.tv_sec--;
            (*it)->delay.tv_usec += 1000000;
        }
    }
}

/**
 * @fn    ValidateInterrupts(XEvent *e)
 * @brief Validates interrupt list
 *
 * Checks if the XEvent e invalidates any of the interrupts in the interrupt
 * list, invalid interrupts are thrown away and the timers next timeout is
 * updated if needed.
 *
 * @param e XEvent used for invalidation check
 */
void Timer::ValidateInterrupts(XEvent *e) {
    if (interrupts.empty()) return;
    
    Pause();
    list<Interrupt *>::iterator it = interrupts.begin();
    for (; it != interrupts.end(); ++it) {
        list<int>::iterator dit = (*it)->action->delay_breaks->begin();
        for (; dit != (*it)->action->delay_breaks->end(); ++dit) {
            if (*dit == e->type &&
                (*it)->event.xany.window == e->xany.window) {
                interrupts.remove(*it);
                it = interrupts.begin();
                break;
            }
        }
    }    
    Start();        
}


/**
 * @fn    Interrupt(void)
 * @brief Constructor for Interrupt class
 *
 */
Interrupt::Interrupt(WaAction *ac, XEvent *e) {
    memcpy(&event, e, sizeof(XEvent));
    action = ac;    
    delay.tv_sec = ac->delay.tv_sec;
    delay.tv_usec = ac->delay.tv_usec;
    win = (Window) 0;
    wm = NULL;
    ws = NULL;
}

/**
 * @fn    timeout(int signal)
 * @brief Timeout handler function
 *
 * Handles the SIGALRM signal. Invokes actions for interrupt and updates
 * timevalues for the interrupts.
 *
 * @param signal The signal we received
 */
void timeout(int signal) {
    Interrupt *i = timer->interrupts.front();
    timer->interrupts.pop_front();

    list<Interrupt *>::iterator it = timer->interrupts.begin();
    for (; it != timer->interrupts.end(); ++it) {
        (*it)->delay.tv_sec -= i->delay.tv_sec;
        (*it)->delay.tv_usec -= i->delay.tv_usec;

        if ((*it)->delay.tv_usec < 0) {
            (*it)->delay.tv_sec--;
            (*it)->delay.tv_usec += 1000000;
        }
    }

    if (i->win) {
        list<WaWindow *>::iterator wit = timer->waimea->wawindow_list->begin();
        for (; wit != timer->waimea->wawindow_list->end(); ++wit) {
            if ((*wit)->id == i->win) {
                if (i->action->exec)
                    waexec(i->action->exec, (*wit)->wascreen->displaystring);
                else {
                    ((*(*wit)).*(i->action->winfunc))(&i->event, i->action);
                    XSync((*wit)->display, false);
                }
            }
        }
    } else if (i->ws) {
        if (i->action->exec)
            waexec(i->action->exec, i->ws->displaystring);
        else {
            ((*(i->ws)).*(i->action->rootfunc))(&i->event, i->action);
            XSync(i->ws->display, false);
        }
    } else if (i->wm) {
        if (i->action->exec)
            waexec(i->action->exec, i->wm->menu->wascreen->displaystring);
        else {
            ((*(i->wm)).*(i->action->menufunc))(&i->event, i->action);
            XSync(i->wm->menu->display, false);
        }
    }
    delete i;
    timer->Start();
}
