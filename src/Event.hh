/**
 * @file   Event.hh
 * @author David Reveman <c99drn@cs.umu.se>
 * @date   11-May-2001 11:48:03
 *
 * @brief Definition of EventHandler class  
 *
 * Function declarations and variable definitions for EventHandler class.
 *
 * Copyright (C) David Reveman. All rights reserved.
 *
 */

#ifndef __Event_hh
#define __Event_hh

#include <X11/Xlib.h>

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

#include <set>

using std::set;

class EventHandler;

typedef struct {
    unsigned int type, mod, detail;
} EventDetail;

#include "Waimea.hh"

#define MoveResizeMask (1L << 25)

#define DoubleClick 36

class EventHandler {
public:
    EventHandler(Waimea *);
    virtual ~EventHandler(void);

    void EventLoop(set<int> *, XEvent *);
    void HandleEvent(XEvent *);
    void EvExpose(XExposeEvent *);
    void EvFocus(XFocusChangeEvent *);
    void EvUnmapDestroy(XEvent *);
    void EvConfigureRequest(XConfigureRequestEvent *);
    void EvAct(XEvent *, Window, EventDetail *);
    
    XEvent *event;
    set<int> *empty_return_mask;
    set<int> *moveresize_return_mask;
    set<int> *menu_viewport_move_return_mask;

    int move_resize;
    Window focused;

private:
    void EvProperty(XPropertyEvent *);
    void EvColormap(XColormapEvent *);
    void EvMapRequest(XMapRequestEvent *);
    void EvClientMessage(XEvent *, EventDetail *);
    
    Waimea *waimea;
    ResourceHandler *rh;
    Window last_click_win;
    unsigned int last_button;
    struct timeval last_click;
};

Bool eventmatch(WaAction *, EventDetail *);

#endif // __EventHandler_hh
