/** -*- Mode: C++ -*-
 *
 * @file   EventHandler.hh
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

#ifndef __EventHandler_hh
#define __EventHandler_hh

#include <sys/timeb.h>
#include <X11/Xlib.h>
#include <hash_set.h>

class EventHandler;
typedef struct _EventDetail EventDetail;

#include "Waimea.hh"
#include "WaWindow.hh"
#include "ResourceHandler.hh"

struct _EventDetail {
    unsigned int type, mod, detail;
};

#define DoubleClick 36

class EventHandler {
public:
    EventHandler(Waimea *);
    virtual ~EventHandler(void);

    void EventLoop(hash_set<int> *, XEvent *);
    void EvExpose(XExposeEvent *);
    void EvFocus(XFocusChangeEvent *);
    void EvUnmapDestroy(XEvent *);
    void EvConfigureRequest(XConfigureRequestEvent *);
    void EvAct(XEvent *, Window);

    EventDetail ed;
    XEvent *event;
    hash_set<int> *empty_return_mask;
    hash_set<int> *moveresize_return_mask;
    hash_set<int> *menu_viewport_move_return_mask;

private:
    void EvProperty(XPropertyEvent *);
    void EvColormap(XColormapEvent *);
    void EvMapRequest(XMapRequestEvent *);
    void EvClientMessage(XEvent *);
    
    Waimea *waimea;
    ResourceHandler *rh;
    Window focused, last_click_win;
    struct timeb last_click;
};

Bool eventmatch(WaAction *, EventDetail *);

#endif // __EventHandler_hh
