/** -*- Mode: C++ -*-
 *
 * @file   DockappHandler.cc
 * @author David Reveman <c99drn@cs.umu.se>
 * @date   29-Nov-2001 22:13:22
 *
 * @brief Implementation of DockappHandler class  
 *
 * This class handles docking of 'dockapp' programs.
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
 *
 * @param scrn WaScreen to create dockapp handler window on.
 */
DockappHandler::DockappHandler(WaScreen *scrn) {
    XSetWindowAttributes attrib_set;
    
    wascreen = scrn;
    waimea = wascreen->waimea;
    display = waimea->display;
    x = 0;
    y = 0;
    stacking = waimea->rh->dockstyle.stacking;
    if (waimea->rh->dockstyle.geometry & XValue ||
        waimea->rh->dockstyle.geometry & YValue) {
        geometry = waimea->rh->dockstyle.geometry;
        if (waimea->rh->dockstyle.geometry & XValue) {
            x = waimea->rh->dockstyle.x;
        }
        if (waimea->rh->dockstyle.geometry & YValue) {
            y = waimea->rh->dockstyle.y;
        }
    } else
        geometry = XValue | YValue | XNegative;
    
    direction = waimea->rh->dockstyle.direction;
    gridspace = waimea->rh->dockstyle.gridspace;
    
    width = 0;
    height = 0;

    dockapp_list = new list<Dockapp *>;
    
    attrib_set.background_pixel = None;
    attrib_set.border_pixel = wascreen->wstyle.border_color.getPixel();
    attrib_set.colormap = wascreen->colormap;
    attrib_set.override_redirect = True;
    attrib_set.event_mask = SubstructureRedirectMask | ButtonPressMask |
        EnterWindowMask | LeaveWindowMask;
    
    id = XCreateWindow(display, wascreen->id, 0, 0,
                       1, 1, wascreen->wstyle.border_width,
                       wascreen->screen_number, CopyFromParent,
                       wascreen->visual, CWOverrideRedirect | CWBackPixel |
                       CWEventMask | CWColormap | CWBorderPixel, &attrib_set);

    if (stacking == AlwaysOnTop)
        waimea->always_on_top_list->push_back(id);
    else
        waimea->always_at_bottom_list->push_back(id);

    wm_strut = (WMstrut *) malloc(sizeof(WMstrut));
    wm_strut->window = id;
    wm_strut->left = 0;
    wm_strut->right = 0;
    wm_strut->top = 0;
    wm_strut->bottom = 0;
    wascreen->strut_list->push_back(wm_strut);
}

/**
 * @fn    ~DockappHandler(void)
 * @brief Destructor for DockappHandler class
 *
 * Removes all dockapps and destroys the dockapp handler window.
 */
DockappHandler::~DockappHandler(void) {
    if (stacking == AlwaysOnTop)
        waimea->always_on_top_list->remove(id);
    else
        waimea->always_at_bottom_list->remove(id);
    while (! dockapp_list->empty())
        delete dockapp_list->front();
    XDestroyWindow(display, id);
    wascreen->strut_list->remove(wm_strut);
    free(wm_strut);
    wascreen->UpdateWorkarea();
}


/**
 * @fn    Update(void)
 * @brief Update the dockapp handler
 *
 * Repositions all dockapps. Moves and resizes dockapp handler.
 */
void DockappHandler::Update(void) {
    int dock_x = gridspace;
    int dock_y = gridspace;
    list<Dockapp *>::iterator it;
    bool found_pos;
    Dockapp *tmp_dock;
    
    map_x = x;
    map_y = y;
    width = gridspace;
    height = gridspace;
    
    if (dockapp_list->empty()) {
        XUnmapWindow(display, id);
        return;
    }
    
    list<char *>::reverse_iterator r_it = waimea->rh->dockstyle.order->rbegin();
    for (; r_it != waimea->rh->dockstyle.order->rend(); ++r_it) {
        it = dockapp_list->begin();
        if (**r_it == 'U') {
            for (; it != dockapp_list->end(); ++it) {
                if (! (*it)->c_hint) {
                    tmp_dock = *it;
                    dockapp_list->erase(it);
                    dockapp_list->push_front(tmp_dock);
                }
            }
        } else if (**r_it == 'N') {
            for (; it != dockapp_list->end(); ++it) {
                if ((*it)->c_hint &&
                    (! strcmp(*r_it + 2, (*it)->c_hint->res_name))) {
                    tmp_dock = *it;
                    dockapp_list->erase(it);
                    dockapp_list->push_front(tmp_dock);
                }
            }
        } else if (**r_it == 'C') {
            for (; it != dockapp_list->end(); ++it) {
                if ((*it)->c_hint &&
                    (! strcmp(*r_it + 2, (*it)->c_hint->res_class))) {
                    tmp_dock = *it;
                    dockapp_list->erase(it);
                    dockapp_list->push_front(tmp_dock);
                }
            }
        }
    }    
    
    it = dockapp_list->begin();
    for (; it != dockapp_list->end(); ++it) {
        switch (direction) {
            case VerticalDock:
                if (((*it)->width + gridspace * 2) > width)
                        width = (*it)->width + gridspace * 2;
                break;
            case HorizontalDock:
                if (((*it)->height + gridspace * 2) > height)
                    height = (*it)->height + gridspace * 2;
                break;
        }
    }
    it = dockapp_list->begin();
    XGrabServer(display);
    for (; it != dockapp_list->end(); ++it) {
        if (validateclient((*it)->id)) {
            switch (direction) {
                case VerticalDock:
                    dock_y = height;
                    height += (*it)->height + gridspace;
                    dock_x = (((width - gridspace * 2) - (*it)->width) / 2) +
                        gridspace;
                    break;
                case HorizontalDock:
                    dock_x = width;
                    width += (*it)->width + gridspace;
                    dock_y = (((height - gridspace * 2) - (*it)->height) / 2) +
                        gridspace;
                    break;
            }
            (*it)->x = dock_x;
            (*it)->y = dock_y;
            XMoveWindow(display, (*it)->id, dock_x, dock_y);
        }
    }
    XUngrabServer(display);
    switch (direction) {
        case VerticalDock: height += gridspace; break;
        case HorizontalDock: width += gridspace; break;
    }

    WaTexture *texture = &wascreen->wstyle.t_focus;
    if (texture->getTexture() == (WaImage_Flat | WaImage_Solid)) {
        background = None;
        background_pixel = texture->getColor()->getPixel();
        XSetWindowBackground(display, id, background_pixel);
    } else {
        background = wascreen->ic->renderImage(width, height, texture);
        XSetWindowBackgroundPixmap(display, id, background);
    }
    XClearWindow(display, id);    
    XResizeWindow(display, id, width, height);

    wm_strut->left = wm_strut->right = wm_strut->top = wm_strut->bottom = 0;
    if (geometry & XNegative) {
        map_x = wascreen->width - wascreen->wstyle.border_width * 2 -
            width + x;
        wm_strut->right = wascreen->width - map_x;
    } else
        wm_strut->left = map_x + wascreen->wstyle.border_width * 2 + width;
    
    if (geometry & YNegative) {
        map_y = wascreen->height - wascreen->wstyle.border_width * 2 -
            height + y;
        if (direction == HorizontalDock) {
            wm_strut->bottom = wascreen->height - map_y;
            wm_strut->right = wm_strut->left = 0;
        }
    } else
        if (direction == HorizontalDock) {
            wm_strut->top = map_y + wascreen->wstyle.border_width * 2 + height;
            wm_strut->right = wm_strut->left = 0;
        }
    
    XMoveWindow(display, id, map_x, map_y);
    XMapWindow(display, id);
    wascreen->UpdateWorkarea();
}

/**
 * @fn    Dockapp(Window win, DockappHandler *dhand)
 * @brief Constructor for Dockapp class
 *
 * Reparents window to dockapp handler window and adds it to the window_table
 * hash_map.
 */
Dockapp::Dockapp(Window win, DockappHandler *dhand) :
    WindowObject(win, DockAppType) {
    XWindowAttributes attrib;
    dh = dhand;
    client_id = win;
    display = dh->display;
    deleted = False;

    XWMHints *wmhints = XGetWMHints(display, win);
    if (wmhints) {
      if ((wmhints->flags & IconWindowHint) &&
          (wmhints->icon_window != None)) {
          XUnmapWindow(display, client_id);
          icon_id = wmhints->icon_window;
          id = icon_id;
      } else {
          icon_id = None;
          id = client_id;
      }
      XFree(wmhints);
    } else {
        icon_id = None;
        id = client_id;
    }    
    XGrabServer(display);
    if (validateclient(id)) {
        if (XGetWindowAttributes(display, id, &attrib)) {
            width = attrib.width;
            height = attrib.height;
        } else
            width = height = 64;
        
        XSetWindowBorderWidth(display, id, 0);
        XReparentWindow(display, id, dh->id, dh->width, dh->height);
        XMapRaised(display, id);
        XSelectInput(display, id, StructureNotifyMask | SubstructureNotifyMask);
    } else {
        XUngrabServer(display);
        delete this;
        return;
    }
    XUngrabServer(display);
    dh->waimea->window_table->insert(make_pair(id, this));
    dh->dockapp_list->push_back(this);
}

/**
 * @fn    ~Dockapp(void)
 * @brief Destructor for Dockapp class
 *
 * Reparents dockapp window back to root if it still exists and removes it
 * from the window_table hash_map and dockapp list.
 */
Dockapp::~Dockapp(void) {
    dh->dockapp_list->remove(this);
    dh->waimea->window_table->erase(id);
    if (! deleted) {
        XGrabServer(display);
        if (validateclient(id)) {
            if (icon_id) XUnmapWindow(display, id);
            XReparentWindow(display, id, dh->wascreen->id,
                            dh->map_x + x, dh->map_y + y);
            XMapWindow(display, client_id);
        }
        XUngrabServer(display);
    }
}
