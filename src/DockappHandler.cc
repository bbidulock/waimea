/** -*- Mode: C++ -*-
 *
 * @file   DockappHandler.cc
 * @author David Reveman <c99drn@cs.umu.se>
 * @date   29-Nov-2001 22:13:22
 *
 * @brief Implementation of DockappHandler class  
 *
 * Functions for reading and writing window hints.
 *
 * Copyright (C) David Reveman. All rights reserved.
 *
 */

#include "DockappHandler.hh"

/**
 * @fn    DockappHandler(void)
 * @brief Constructor for DockappHandler class
 *
 * Creates the dockapp handler window.
 */
DockappHandler::DockappHandler(WaScreen *scrn) {
    XSetWindowAttributes attrib_set;
    
    wascreen = scrn;
    waimea = wascreen->waimea;
    display = waimea->display;
    x = 0;
    y = 0;
    width = 70;
    height = 500;
    
    attrib_set.background_pixel = None;
    attrib_set.border_pixel = wascreen->wstyle.border_color.getPixel();
    attrib_set.colormap = wascreen->colormap;
    attrib_set.override_redirect = True;
    attrib_set.event_mask = NoEventMask;
    
    id = XCreateWindow(display, wascreen->id, 0, 0,
                       70, 200, wascreen->wstyle.border_width,
                       wascreen->screen_number, CopyFromParent,
                       wascreen->visual, CWOverrideRedirect | CWBackPixel |
                       CWEventMask | CWColormap | CWBorderPixel, &attrib_set);

    WaTexture *texture = &wascreen->wstyle.t_focus;
    if (texture->getTexture() == (WaImage_Flat | WaImage_Solid)) {
        background = None;
        background_pixel = texture->getColor()->getPixel();
    } else
        background = wascreen->ic->renderImage(width, height, texture);

    if (background)
        XSetWindowBackgroundPixmap(display, id, background);
    else
        XSetWindowBackground(display, id, background_pixel);
    
    XClearWindow(display, id);

    //XMapWindow(display, id);
}

/**
 * @fn    ~DockappHandler(void)
 * @brief Destructor for DockappHandler class
 *
 * Destroys the dockapp handler window
 */
DockappHandler::~DockappHandler(void) {
    XDestroyWindow(display, id);
}
