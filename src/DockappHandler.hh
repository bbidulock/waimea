/** -*- Mode: C++ -*-
 *
 * @file   DockappHandler.hh
 * @author David Reveman <c99drn@cs.umu.se>
 * @date   29-Nov-2001 22:13:22
 *
 * @brief Definition of DockappHandler class  
 *
 * Function declarations and variable definitions for DockappHandler and
 * Dockapp classes.
 *
 * Copyright (C) David Reveman. All rights reserved.
 *
 */

#ifndef __DockappHandler_hh
#define __DockappHandler_hh

class DockappHandler;
class Dockapp;

#include "Waimea.hh"

enum {
    VerticalDock,
    HorizontalDock
};

class DockappHandler {
public:
    DockappHandler(WaScreen *);
    virtual ~DockappHandler(void);

    void Update(void);
    
    Window id;
    Display *display;
    Waimea *waimea;
    WaScreen *wascreen;
    int x, y, geometry, direction;
    unsigned int width, height, gridspace;
    Pixmap background;
    unsigned long background_pixel;

    list<Dockapp *> *dockapp_list;
};

class Dockapp : public WindowObject {
public:
    Dockapp(Window, DockappHandler *);
    virtual ~Dockapp(void);

    Window id, icon_id, client_id;
    Display *display;
    bool deleted;
    DockappHandler *dh;
    int x, y;
    unsigned int width, height;
};

#endif // __DockappHandler_hh
