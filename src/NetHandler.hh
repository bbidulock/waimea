/** -*- Mode: C++ -*-
 *
 * @file   NetHandler.hh
 * @author David Reveman <c99drn@cs.umu.se>
 * @date   11-Oct-2001 22:36:12
 *
 * @brief Definition of NetHandler class  
 *
 * Function declarations and variable definitions for NetHandler class.
 *
 * Copyright (C) David Reveman. All rights reserved.
 *
 */

#ifndef __NetHandler_hh
#define __NetHandler_hh

class NetHandler;

#include "Waimea.hh"
#include "WaScreen.hh"
#include "WaWindow.hh"

#define MwmHintsDecorations (1L << 1)

#define MwmDecorAll         (1L << 0)
#define MwmDecorBorder      (1L << 1)
#define MwmDecorHandle      (1L << 2)
#define MwmDecorTitle       (1L << 3)
#define MwmDecorMenu        (1L << 4)
#define MwmDecorMinimize    (1L << 5)
#define MwmDecorMaximize    (1L << 6)

#define PropMotifWmHintsElements 3

typedef struct {
    CARD32 flags;
    CARD32 functions;
    CARD32 decorations;
} MwmHints;

class NetHandler {
public:
    NetHandler(Waimea *);
    virtual ~NetHandler(void);
    
    void GetWMHints(WaWindow *);
    void GetMWMHints(WaWindow *);
    void GetWMNormalHints(WaWindow *);
    void SetState(WaWindow *, int);
    void GetState(WaWindow *);
    void SetDesktopViewPort(WaScreen *);
    void GetDesktopViewPort(WaScreen *);
    
    void SetVirtualPos(WaWindow *);
    void GetVirtualPos(WaWindow *);
    
    Waimea *waimea;
    Display *display;
    XWMHints *wm_hints;
    XSizeHints *size_hints;
    MwmHints *mwm_hints;

    Atom mwm_hints_atom, wm_state, net_desktop_viewport, net_virtual_pos;

    int real_format;
    Atom real_type;
    unsigned long items_read, items_left;	    
};

#endif // __NetHandler_hh
