/**
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

class DockappHandler : public WindowObject {
public:
    DockappHandler(WaScreen *, DockStyle *);
    virtual ~DockappHandler(void);

    void Update(void);
    void Render(void);
    
    Display *display;
    Waimea *waimea;
    WaScreen *wascreen;
    int x, y, map_x, map_y;
    unsigned int width, height;
    Pixmap background;
    unsigned long background_pixel;
    WMstrut *wm_strut;
    DockStyle *style;

    list<Dockapp *> *dockapp_list;
};

class Dockapp : public WindowObject {
public:
    Dockapp(Window, DockappHandler *);
    virtual ~Dockapp(void);

    Window icon_id, client_id;
    Display *display;
    DockappHandler *dh;
    int x, y;
    unsigned int width, height;
    XClassHint *c_hint;
    char *title;
    bool deleted;
    bool added;
};

#endif // __DockappHandler_hh
