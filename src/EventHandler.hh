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

#include <X11/Xlib.h>
#include <hash_set>

class EventHandler;
typedef struct _EventDetail EventDetail;

#include "Waimea.hh"
#include "WaWindow.hh"
#include "ResourceHandler.hh"

struct _EventDetail {
    unsigned int type, mod, detail;
};

class EventHandler {
public:
    EventHandler(Waimea *);
    virtual ~EventHandler(void);

    XEvent *EventLoop(hash_set<int> *);
    void EvExpose(XExposeEvent *);
    void EvUnmapDestroy(XEvent *);
    void EvConfigureRequest(XConfigureRequestEvent *);
    void EvAct(XEvent *, Window);

    EventDetail ed;
    hash_set<int> *moveresize_return_mask;
    hash_set<int> *menu_viewport_move_return_mask;
    
private:
    void EvProperty(XPropertyEvent *);
    void EvFocus(XFocusChangeEvent *);
    void EvColormap(XColormapEvent *);
    void EvMapRequest(XMapRequestEvent *);
    
    Waimea *waimea;
    ResourceHandler *rh;
    Window focused;
};

Bool eventmatch(WaAction *, EventDetail *);

#endif // __EventHandler_hh
