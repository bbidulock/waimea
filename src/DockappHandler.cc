/**
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

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#include "DockappHandler.hh"

/**
 * @fn    DockappHandler(void)
 * @brief Constructor for DockappHandler class
 *
 * Creates the dockapp handler window.
 *
 * @param scrn WaScreen to create dockapp handler window on.
 * @param ds Style structure to use when creating this dockapphandler
 */
DockappHandler::DockappHandler(WaScreen *scrn, DockStyle *ds) :
    WindowObject(0, DockHandlerType) {
    XSetWindowAttributes attrib_set;

    style = ds;
    wascreen = scrn;
    waimea = wascreen->waimea;
    display = waimea->display;
    background = (Pixmap) 0;
    x = 1;
    y = 1;
    if (style->geometry & XValue ||
        style->geometry & YValue) {
        if (style->geometry & XValue) {
            x = style->x;
        }
        if (style->geometry & YValue) {
            y = style->y;
        }
    } else
        style->geometry = XValue | YValue | XNegative;
    
    width = 0;
    height = 0;

    dockapp_list = new list<Dockapp *>;

    attrib_set.background_pixel = None;
    attrib_set.border_pixel = style->style.border_color.getPixel();
    attrib_set.colormap = wascreen->colormap;
    attrib_set.override_redirect = true;
    attrib_set.event_mask = SubstructureRedirectMask | ButtonPressMask |
        EnterWindowMask | LeaveWindowMask;

    id = XCreateWindow(display, wascreen->id, 0, 0, 1, 1,
                       style->style.border_width, wascreen->screen_depth,
                       CopyFromParent, wascreen->visual, CWOverrideRedirect |
                       CWBackPixel | CWEventMask | CWColormap | CWBorderPixel,
                       &attrib_set);

    if (style->stacking == AlwaysOnTop)
        wascreen->always_on_top_list.push_back(id);
    else
        wascreen->always_at_bottom_list.push_back(id);

    if (! style->inworkspace) {
        wm_strut = new WMstrut;
        wm_strut->window = id;
        wm_strut->left = 0;
        wm_strut->right = 0;
        wm_strut->top = 0;
        wm_strut->bottom = 0;
        wascreen->strut_list.push_back(wm_strut);
    }
    waimea->window_table.insert(make_pair(id, this));
}

/**
 * @fn    ~DockappHandler(void)
 * @brief Destructor for DockappHandler class
 *
 * Removes all dockapps and destroys the dockapp handler window.
 */
DockappHandler::~DockappHandler(void) {
    if (style->stacking == AlwaysOnTop)
        wascreen->always_on_top_list.remove(id);
    else
        wascreen->always_at_bottom_list.remove(id);
    LISTPTRDELITEMS(dockapp_list);
    XDestroyWindow(display, id);
    if (! style->inworkspace) {
        wascreen->strut_list.remove(wm_strut);
        delete wm_strut;
    }
    waimea->window_table.erase(id);

    delete dockapp_list;
}


/**
 * @fn    Update(void)
 * @brief Update the dockapp handler
 *
 * Repositions all dockapps. Moves and resizes dockapp handler.
 */
void DockappHandler::Update(void) {
    int dock_x = style->gridspace;
    int dock_y = style->gridspace;    
    
    if (dockapp_list->empty()) {
        if (! style->inworkspace) {
            wm_strut->left = 0;
            wm_strut->right = 0;
            wm_strut->top = 0;
            wm_strut->bottom = 0;
            wascreen->UpdateWorkarea();
        }
        XUnmapWindow(display, id);
        return;
    }
    map_x = x;
    map_y = y;
    width = style->gridspace;
    height = style->gridspace;

    list<Dockapp *>::reverse_iterator d_it;
    list<Dockapp *> *tmp_list = new list<Dockapp *>;
    list<char *>::reverse_iterator s_it = style->order->rbegin();
    for (; s_it != style->order->rend(); ++s_it) {
        d_it = dockapp_list->rbegin();
        if (**s_it == 'U') {
            for (; d_it != dockapp_list->rend(); ++d_it) {
                if (! (*d_it)->c_hint)
                    tmp_list->push_front(*d_it);
            }
        } else if (**s_it == 'N') {
            for (; d_it != dockapp_list->rend(); ++d_it) {
                if ((*d_it)->c_hint &&
                    (! strcmp(*s_it + 2, (*d_it)->c_hint->res_name)))
                    tmp_list->push_front(*d_it);
            }
        } else if (**s_it == 'C') {
            for (; d_it != dockapp_list->rend(); ++d_it) {
                if ((*d_it)->c_hint &&
                    (! strcmp(*s_it + 2, (*d_it)->c_hint->res_class)))
                    tmp_list->push_front(*d_it);
            }
        }
    }
    while (! dockapp_list->empty())
        dockapp_list->pop_back();
    delete dockapp_list;
    dockapp_list = tmp_list;
    
    list<Dockapp *>::iterator it = dockapp_list->begin();
    for (; it != dockapp_list->end(); ++it) {
        switch (style->direction) {
            case VerticalDock:
                if (((*it)->width + style->gridspace * 2) > width)
                    width = (*it)->width + style->gridspace * 2;
                break;
            case HorizontalDock:
                if (((*it)->height + style->gridspace * 2) > height)
                    height = (*it)->height + style->gridspace * 2;
                break;
        }
    }
    it = dockapp_list->begin();
    XGrabServer(display);
    for (; it != dockapp_list->end(); ++it) {
        if (validateclient((*it)->id)) {
            switch (style->direction) {
                case VerticalDock:
                    dock_y = height;
                    height += (*it)->height + style->gridspace;
                    dock_x = (((width - style->gridspace * 2) -
                               (*it)->width) / 2) + style->gridspace;
                    break;
                case HorizontalDock:
                    dock_x = width;
                    width += (*it)->width + style->gridspace;
                    dock_y = (((height - style->gridspace * 2) -
                               (*it)->height) / 2) + style->gridspace;
                    break;
            }
            (*it)->x = dock_x;
            (*it)->y = dock_y;
            XMoveWindow(display, (*it)->id, dock_x, dock_y);
        }
    }
    XUngrabServer(display);

    if (! style->inworkspace)
        wm_strut->left = wm_strut->right = wm_strut->top =
            wm_strut->bottom = 0;
    if (style->geometry & XNegative) {
        map_x = wascreen->width - style->style.border_width * 2 -
            width + x;
        if (! style->inworkspace)
            wm_strut->right = wascreen->width - map_x;
    } else {
        if (! style->inworkspace)
            wm_strut->left = map_x + style->style.border_width * 2 + width;
    }
    
    if (style->geometry & YNegative) {
        map_y = wascreen->height - style->style.border_width * 2 -
            height + y;
        if (style->direction == HorizontalDock && (! style->inworkspace)) {
            wm_strut->bottom = wascreen->height - map_y;
            wm_strut->right = wm_strut->left = 0;
        }
    } else
        if (style->direction == HorizontalDock && (! style->inworkspace)) {
            wm_strut->top = map_y + style->style.border_width * 2 + height;
            wm_strut->right = wm_strut->left = 0;
        }

    if (style->centered) {
        switch (style->direction) {
            case VerticalDock: map_y = wascreen->height / 2 - height / 2;
                break;
            case HorizontalDock: map_x = wascreen->width / 2 - width / 2;
                break;
        }
    }
    XResizeWindow(display, id, width, height);
    XMoveWindow(display, id, map_x, map_y);
    XMapWindow(display, id);
    Render();
    wascreen->UpdateWorkarea();
}

/**
 * @fn    Render(void)
 * @brief Render background
 *
 * Renders background for dockapp holder.
 */
void DockappHandler::Render(void) {
    WaTexture *texture = &style->style.texture;
    
#ifdef XRENDER
    if (texture->getOpacity()) {
        background = XCreatePixmap(wascreen->pdisplay, wascreen->id, width,
                                   height, wascreen->screen_depth);
    }
#endif // XRENDER
    
    if (texture->getTexture() == (WaImage_Flat | WaImage_Solid)) {
        background = None;
        background_pixel = texture->getColor()->getPixel();
#ifdef XRENDER        
        if (texture->getOpacity()) {
            background = wascreen->ic->xrender(None, width, height, texture,
                                               wascreen->xrootpmap_id,
                                               map_x +
                                               style->style.border_width,
                                               map_y +
                                               style->style.border_width,
                                               background);
            XSetWindowBackgroundPixmap(display, id, background);
        } else
            XSetWindowBackground(display, id, background_pixel);
#else // ! XRENDER
        XSetWindowBackground(display, id, background_pixel);
#endif // XRENDER
        
    } else {
        background = wascreen->ic->renderImage(width, height, texture

#ifdef XRENDER
                                               , wascreen->xrootpmap_id,
                                               map_x +
                                               style->style.border_width,
                                               map_y +
                                               style->style.border_width,
                                               background
#endif // XRENDER
                                               
                                               );
        XSetWindowBackgroundPixmap(display, id, background);
    }
    XClearWindow(display, id);
    
#ifdef XRENDER    
    if (texture->getOpacity()) XFreePixmap(wascreen->pdisplay, background);
#endif // XRENDER
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
    deleted = false;

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
        XSelectInput(display, id, StructureNotifyMask |
                     SubstructureNotifyMask);
    } else {
        XUngrabServer(display);
        delete this;
        return;
    }
    XUngrabServer(display);
    dh->waimea->window_table.insert(make_pair(id, this));
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
    dh->waimea->window_table.erase(id);
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
