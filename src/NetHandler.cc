/** -*- Mode: C++ -*-
 *
 * @file   NetHandler.cc
 * @author David Reveman <c99drn@cs.umu.se>
 * @date   11-Oct-2001 22:36:12
 *
 * @brief Implementation of NetHandler class  
 *
 * Functions for reading and writing window hints.
 *
 * Copyright (C) David Reveman. All rights reserved.
 *
 */

#include <X11/Xatom.h>

#include "NetHandler.hh"

/**
 * @fn    NetHandler(Waimea *wa)
 * @brief Constructor for NetHandler class
 *
 * Creates atom identifiers, allocates wm_hints and size_hints.
 *
 * @param wa Waimea object
 */
NetHandler::NetHandler(Waimea *wa) {
    waimea = wa;
    display = waimea->display;

    wm_hints = XAllocWMHints();
    size_hints = XAllocSizeHints();
    mwm_hints_atom = XInternAtom(display, "_MOTIF_WM_HINTS", False);
    wm_state = XInternAtom(display, "WM_STATE", False);
    net_desktop_viewport = XInternAtom(display, "_NET_DESKTOP_VIEWPORT",
                                       False);
    net_virtual_pos = XInternAtom(display, "_NET_VIRTUAL_POS", False);
}

/**
 * @fn    ~NetHandler(void)
 * @brief Destructor for NetHandler class
 *
 * Frees wm_hints and size_hints.
 */
NetHandler::~NetHandler(void) {
    XFree(wm_hints);
    XFree(size_hints);
}

/**
 * @fn    GetWMHints(WaWindow *ww)
 * @brief Read WM hints
 *
 * Reads WaWindows WM hints.
 *
 * @param ww WaWindow object
 */
void NetHandler::GetWMHints(WaWindow *ww) {
    ww->state = NormalState;
    XGrabServer(display);
    if (validateclient(ww->id))
    if ((wm_hints = XGetWMHints(display, ww->id)))
        if (wm_hints->flags & StateHint)
            ww->state = wm_hints->initial_state;
    XUngrabServer(display);
}

/**
 * @fn    GetMWMHints(WaWindow *ww)
 * @brief Read MWM Hints
 *
 * Reads WaWindows MWM hints.
 *
 * @param ww WaWindow object
 */
void NetHandler::GetMWMHints(WaWindow *ww) {
    ww->flags.title = ww->flags.border = ww->flags.handle = True;
    
    XGrabServer(display);
    if (validateclient(ww->id))
        if (XGetWindowProperty(display, ww->id, mwm_hints_atom, 0L, 20L,
                               False, mwm_hints_atom, &real_type,
                               &real_format, &items_read, &items_left,
                               (unsigned char **) &mwm_hints) == Success
            && items_read >= PropMotifWmHintsElements) {
            if (mwm_hints->flags & MwmHintsDecorations
                && !(mwm_hints->decorations & MwmDecorAll)) {
                ww->flags.title  =
                    (mwm_hints->decorations & MwmDecorTitle)  ? True: False;
                ww->flags.border =
                (mwm_hints->decorations & MwmDecorBorder) ? True: False;
                ww->flags.handle =
                    (mwm_hints->decorations & MwmDecorHandle) ? True: False;
            }
        }
    XUngrabServer(display);
    
    ww->border_w = (ww->flags.border * ww->wascreen->wstyle.border_width);
    ww->title_w  = (ww->flags.title  * ww->wascreen->wstyle.title_height);
    ww->handle_w = (ww->flags.handle * ww->wascreen->wstyle.handle_width);
}

/**
 * @fn    GetWMNormalHints(WaWindow *ww)
 * @brief Read size hints
 *
 * Reads WaWindows size hints.
 *
 * @param ww WaWindow object
 */
void NetHandler::GetWMNormalHints(WaWindow *ww) {
    long dummy;

    ww->size.max_width = ww->size.max_height = 65536;
    ww->size.min_width = ww->size.min_height = ww->size.width_inc =
        ww->size.height_inc = 1;
    ww->size.base_width = ww->size.min_width;
    ww->size.base_height = ww->size.min_height;
    ww->size.win_gravity = NorthWestGravity;

    size_hints->flags = 0;
    XGrabServer(display);
    if (validateclient(ww->id))
        XGetWMNormalHints(display, ww->id, size_hints, &dummy);
    XUngrabServer(display);

    if (size_hints->flags & PMaxSize) {
        ww->size.max_width = size_hints->max_width;
        ww->size.max_height = size_hints->max_height;
    }
    if (size_hints->flags & PMinSize) {
        ww->size.min_width = size_hints->min_width;
        ww->size.min_height = size_hints->min_height;
    }

    if (size_hints->flags & PResizeInc) {
        ww->size.width_inc = size_hints->width_inc;
        ww->size.height_inc = size_hints->height_inc;
    }
    if (size_hints->flags & PBaseSize) {
        ww->size.base_width = size_hints->base_width;
        ww->size.base_height = size_hints->base_height;
    }
    if (size_hints->flags & PWinGravity)
        ww->size.win_gravity = size_hints->win_gravity;

    
    if (ww->size.min_width < ((ww->title_w - 4) * 3 + 8))
        ww->size.min_width = (ww->title_w - 4) * 3 + 8;
    if (ww->size.min_width < (50 + ww->border_w))
        ww->size.min_width = 50 + ww->border_w;
}

/**
 * @fn    GetState(WaWindow *ww)
 * @brief Read state hint
 *
 * Reads WaWindows state hint.
 *
 * @param ww WaWindow object
 */
void NetHandler::GetState(WaWindow *ww) {
    CARD32 *data;

    ww->state = WithdrawnState;
    XGrabServer(display);
    if (validateclient(ww->id))
        if (XGetWindowProperty(display, ww->id, wm_state, 0L, 2L, False,
                               wm_state, &real_type, &real_format, &items_read,
                               &items_left, (unsigned char **) &data) == Success &&
            items_read) {
            ww->state = *data;
            XFree(data);
    }
    XUngrabServer(display);
        
}

/**
 * @fn    SetState(WaWindow *ww, int newstate)
 * @brief Write state hint.
 *
 * Changes the state of the window and writes state hint.
 *
 * @param ww WaWindow object
 * @param newstate State to change to
 */
void NetHandler::SetState(WaWindow *ww, int newstate) {
    CARD32 data[2];
    
    ww->state = newstate;
    switch (ww->state) {
        case IconicState:
        case NormalState:
            XGrabServer(display);
            if (validateclient(ww->id))
                XMapWindow(display, ww->id);
            XUngrabServer(display);
            if (ww->title_w) {
                XMapWindow(display, ww->title->id);
                XMapWindow(display, ww->label->id);
                XMapWindow(display, ww->button_min->id);
                XMapWindow(display, ww->button_max->id);
                XMapWindow(display, ww->button_c->id);
            }
            if (ww->handle_w) {
                XMapWindow(display, ww->grip_l->id);
                XMapWindow(display, ww->handle->id);
                XMapWindow(display, ww->grip_r->id);
            }
            XMapWindow(display, ww->frame->id);
            ww->mapped = True;
            break;
    }
    if (ww->want_focus && ww->mapped) {
        XGrabServer(display);
        if (validateclient(ww->id))
            XSetInputFocus(display, ww->id, RevertToPointerRoot, CurrentTime);
        XUngrabServer(display);
    }

    ww->want_focus = False;
    
    data[0] = ww->state;
    data[1] = None;

    XGrabServer(display);
    if (validateclient(ww->id))
        XChangeProperty(display, ww->id, wm_state, wm_state,
                        32, PropModeReplace, (unsigned char *) data, 2);
    XUngrabServer(display);
    ww->SendConfig();
}

/**
 * @fn    GetDesktopViewPort(WaScreen *ws)
 * @brief Reads viewport hint
 *
 * Reads WaScreens viewport hint.
 *
 * @param ws WaScreen object
 */
void NetHandler::GetDesktopViewPort(WaScreen *ws) {
    CARD32 *data;
    
    if (XGetWindowProperty(display, ws->id, net_desktop_viewport, 0L, 2L, 
                           False, XA_CARDINAL, &real_type,
                           &real_format, &items_read, &items_left, 
                           (unsigned char **) &data) == Success && 
        items_read >= 2) { 
        ws->MoveViewportTo(data[0], data[1]);
        XFree(data);
    }
}

/**
 * @fn    SetDesktopViewPort(WaScreen *ws)
 * @brief Writes viewport hint
 *
 * Writes WaScreens viewport hint.
 *
 * @param ws WaScreen object
 */
void NetHandler::SetDesktopViewPort(WaScreen *ws) {
    CARD32 data[2];

    data[0] = ws->v_x;
    data[1] = ws->v_y;
    
    XChangeProperty(display, ws->id, net_desktop_viewport, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *) &data, 2);
}

/**
 * @fn    GetVirtualPos(WaWindow *ww)
 * @brief Reads virtual position hint
 *
 * Reads WaWindows virtual position hint.
 *
 * @param ww WaWindow object
 */
void NetHandler::GetVirtualPos(WaWindow *ww) {
    CARD32 *data;
    
    XGrabServer(display);
    if (validateclient(ww->id))
        if (XGetWindowProperty(display, ww->id, net_virtual_pos, 0L, 2L, 
                               False, XA_CARDINAL, &real_type, &real_format, 
                               &items_read, &items_left, 
                               (unsigned char **) &data) == Success && 
            items_read >= 2) {
            ww->attrib.x = data[0] - ww->wascreen->v_x;
            ww->attrib.y = data[1] - ww->wascreen->v_y;
            XFree(data);
        }
    XUngrabServer(display);
}

/**
 * @fn    SetVirtualPos(WaWindow *ww)
 * @brief Writes virtual position hint
 *
 * Writes WaWindows virtual position hint.
 *
 * @param ww WaWindow object
 */
void NetHandler::SetVirtualPos(WaWindow *ww) {
    CARD32 data[2];
    
    ww->Gravitate(RemoveGravity);
    data[0] = ww->wascreen->v_x + ww->attrib.x;
    data[1] = ww->wascreen->v_y + ww->attrib.y;
    ww->Gravitate(ApplyGravity);

    XGrabServer(display);
    if (validateclient(ww->id))
        XChangeProperty(display, ww->id, net_virtual_pos, XA_CARDINAL, 32,
                        PropModeReplace, (unsigned char *) &data, 2);
    XUngrabServer(display);
}
