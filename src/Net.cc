/**
 * @file   Net.cc
 * @author David Reveman <david@waimea.org>
 * @date   11-Oct-2001 22:36:12
 *
 * @brief Implementation of NetHandler class  
 *
 * Functions for reading and writing window hints.
 *
 * Copyright (C) David Reveman. All rights reserved.
 *
 */

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

extern "C" {
#include <X11/Xatom.h>
}

#include "Net.hh"

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
    utf8_string = XInternAtom(display, "UTF8_STRING", false);
    mwm_hints_atom =
        XInternAtom(display, "_MOTIF_WM_HINTS", false);
    wm_state =
        XInternAtom(display, "WM_STATE", false);
    net_supported =
        XInternAtom(display, "_NET_SUPPORTED", false);
    net_supported_wm_check =
        XInternAtom(display, "_NET_SUPPORTING_WM_CHECK", false);
    net_client_list =
        XInternAtom(display, "_NET_CLIENT_LIST", false);
    net_client_list_stacking =
        XInternAtom(display, "_NET_CLIENT_LIST_STACKING", false);
    net_active_window =
        XInternAtom(display, "_NET_ACTIVE_WINDOW", false);
    net_state =
        XInternAtom(display, "_NET_WM_STATE", false);
    net_state_sticky =
        XInternAtom(display, "_NET_WM_STATE_STICKY", false);
    net_state_shaded =
        XInternAtom(display, "_NET_WM_STATE_SHADED", false);
    net_maximized_vert =
        XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", false);
    net_maximized_horz =
        XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", false);
    net_state_decor =
        XInternAtom(display, "_NET_WM_STATE_DECOR", false);
    net_state_decortitle =
        XInternAtom(display, "_NET_WM_STATE_DECOR_TITLE", false);
    net_state_decorhandle =
        XInternAtom(display, "_NET_WM_STATE_DECOR_HANDLE", false);
    net_state_decorborder =
        XInternAtom(display, "_NET_WM_STATE_DECOR_BORDER", false);
    net_state_aot =
        XInternAtom(display, "_NET_WM_STATE_STAYS_ON_TOP", false);
    net_state_aab =
        XInternAtom(display, "_NET_WM_STATE_STAYS_AT_BOTTOM", false);
    net_state_parentrelative_background =
        XInternAtom(display, "_NET_WM_STATE_PARENTRELATIVE_BACKGROUND", false);
    net_maximized_restore =
        XInternAtom(display, "_NET_MAXIMIZED_RESTORE", false);
    net_virtual_pos =
        XInternAtom(display, "_NET_VIRTUAL_POS", false);
    net_desktop_viewport =
        XInternAtom(display, "_NET_DESKTOP_VIEWPORT", false);
    net_desktop_geometry =
        XInternAtom(display, "_NET_DESKTOP_GEOMETRY", false);
    net_wm_strut =
        XInternAtom(display, "_NET_WM_STRUT", false);
    net_workarea =
        XInternAtom(display, "_NET_WORKAREA", false);
    net_wm_name =
        XInternAtom(display, "_NET_WM_NAME", false);
    net_wm_visible_name =
        XInternAtom(display, "_NET_WM_VISIBLE_NAME", false);
    net_restart =
        XInternAtom(display, "_NET_RESTART", false);
    net_shutdown =
        XInternAtom(display, "_NET_SHUTDOWN", false);
    net_wm_pid =
        XInternAtom(display, "_NET_WM_PID", false);
    net_wm_window_type =
        XInternAtom(display, "_NET_WM_WINDOW_TYPE", false);
    net_wm_window_type_desktop =
        XInternAtom(display, "_NET_WM_WINDOW_TYPE_DESKTOP", false);
    net_wm_window_type_dock =
        XInternAtom(display, "_NET_WM_WINDOW_TYPE_DOCK", false);
    net_wm_window_type_toolbar =
        XInternAtom(display, "_NET_WM_WINDOW_TYPE_TOOLBAR", false);
    net_wm_window_type_menu =
        XInternAtom(display, "_NET_WM_WINDOW_TYPE_MENU", false);
    net_wm_window_type_splash =
        XInternAtom(display, "_NET_WM_WINDOW_TYPE_SPLASH", false);
    net_wm_window_type_normal =
        XInternAtom(display, "_NET_WM_WINDOW_TYPE_NORMAL", false);
    net_wm_window_type_dialog =
        XInternAtom(display, "_NET_WM_WINDOW_TYPE_DIALOG", false);
    net_wm_window_type_utility =
        XInternAtom(display, "_NET_WM_WINDOW_TYPE_UTILITY", false);
    net_current_desktop =
        XInternAtom(display, "_NET_CURRENT_DESKTOP", false);
    net_number_of_desktops =
        XInternAtom(display, "_NET_NUMBER_OF_DESKTOPS", false);
    net_desktop_names =
        XInternAtom(display, "_NET_DESKTOP_NAMES", false);
    net_close_window =
        XInternAtom(display, "_NET_CLOSE_WINDOW", false);
    net_wm_desktop =
        XInternAtom(display, "_NET_WM_DESKTOP", false);
    net_wm_desktop_mask =
        XInternAtom(display, "_NET_WM_DESKTOP_MASK", false);

#ifdef RENDER
    xrootpmap_id =
        XInternAtom(display, "_XROOTPMAP_ID", false);
#endif // RENDER    
    
    xa_xdndaware = XInternAtom(display, "XdndAware", false);
    xa_xdndenter = XInternAtom(display, "XdndEnter", false);
    xa_xdndleave = XInternAtom(display, "XdndLeave", false);

    event.type = ClientMessage;
    event.xclient.display = display;
    event.xclient.format = 32;
    event.xclient.data.l[0] = event.xclient.data.l[1] =
        event.xclient.data.l[2] = event.xclient.data.l[3] =
        event.xclient.data.l[4] = 0l;
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
    XTextProperty text_prop;
    char **list;
    int num;
    char *__m_wastrdup_tmp;
    
    ww->state = NormalState;
    XGrabServer(display);
    if (validateclient(ww->id)) {
        if ((wm_hints = XGetWMHints(display, ww->id)))
            if (wm_hints->flags & StateHint)
                ww->state = wm_hints->initial_state;
        ww->classhint = XAllocClassHint();
        XGetClassHint(ww->display, ww->id, ww->classhint);
        if (XGetWMClientMachine(ww->display, ww->id, &text_prop)) {
            if (XTextPropertyToStringList(&text_prop, &list, &num)) {
                XFree(text_prop.value);
                ww->host = __m_wastrdup(*list);
                XFreeStringList(list);
            }
        }
    } else ww->deleted = true;
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
    Window trans;
    int status;
    ww->flags.title = ww->flags.border = ww->flags.handle = true;
    
    XGrabServer(display);
    if (validateclient(ww->id)) {
        status = XGetWindowProperty(display, ww->id, mwm_hints_atom, 0L, 20L,
                                    false, mwm_hints_atom, &real_type,
                                    &real_format, &items_read, &items_left,
                                    (unsigned char **) &mwm_hints);
    } else WW_DELETED;
    XUngrabServer(display);
    
    if (status == Success && items_read >= PropMotifWmHintsElements) {
        if (mwm_hints->flags & MwmHintsDecorations
            && !(mwm_hints->decorations & MwmDecorAll)) {
            ww->flags.title  =
                (mwm_hints->decorations & MwmDecorTitle)  ? true: false;
            ww->flags.border =
                (mwm_hints->decorations & MwmDecorBorder) ? true: false;
            ww->flags.handle =
                (mwm_hints->decorations & MwmDecorHandle) ? true: false;
        }
    }
    if (ww->wascreen->config.transient_above) {
        XGrabServer(display);
        if (validateclient(ww->id)) {
            status = XGetTransientForHint(display, ww->id, &trans);
        } else WW_DELETED;
        XUngrabServer(display);
        if (status && trans && (trans != ww->id)) {
            if (trans == ww->wascreen->id) {
                list<WaWindow *>::iterator it =
                    ww->wascreen->wawindow_list.begin();
                for (;it != ww->wascreen->wawindow_list.end(); ++it)
                    (*it)->transients.push_back(ww->id);
                ww->want_focus = true;
            } else {
                map<Window, WindowObject *>::iterator it;
                if ((it = waimea->window_table.find(trans)) !=
                    waimea->window_table.end()) {
                    if (((*it).second)->type == WindowType) {
                        ww->transient_for = trans;
                        ((WaWindow *)
                         (*it).second)->transients.push_back(ww->id);
                        if (waimea->eh && trans == waimea->eh->focused)
                            ww->want_focus = true;
                    }
                }
            }
        }
    }
    ww->flags.all = ww->flags.title && ww->flags.handle && ww->flags.border;
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
    int status;

    ww->size.max_width = ww->size.max_height = 65536;
    ww->size.min_width = ww->size.min_height = ww->size.width_inc =
        ww->size.height_inc = 1;
    ww->size.base_width = ww->size.min_width;
    ww->size.base_height = ww->size.min_height;

    size_hints->flags = 0;
    XGrabServer(display);
    if (validateclient(ww->id))
        status = XGetWMNormalHints(display, ww->id, size_hints, &dummy);
    else WW_DELETED;
    XUngrabServer(display);

    if (status) {
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

        if (ww->size.width_inc == 0) {
            ww->size.base_width = 0;
            ww->size.width_inc = 1;
        }
        if (ww->size.height_inc == 0) {
            ww->size.base_height = 0;
            ww->size.height_inc = 1;
        }
    }
    
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
    if (validateclient(ww->id)) {
        if (XGetWindowProperty(display, ww->id, wm_state, 0L, 1L, false,
                               wm_state, &real_type, &real_format, &items_read,
                               &items_left, (unsigned char **) &data) ==
            Success && items_read) {
            ww->state = *data;
            XFree(data);
        }
    } else ww->deleted = true;
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
            ww->MapWindow();
    }
    if (ww->want_focus && ww->mapped && !ww->hidden) {
        XGrabServer(display);
        if (validateclient(ww->id))
            XSetInputFocus(display, ww->id, RevertToPointerRoot, CurrentTime);
        else WW_DELETED;
        XUngrabServer(display);
    }

    ww->want_focus = false;
    
    data[0] = ww->state;
    data[1] = None;

    XGrabServer(display);
    if (validateclient(ww->id))
        XChangeProperty(display, ww->id, wm_state, wm_state,
                        32, PropModeReplace, (unsigned char *) data, 2);
    else WW_DELETED;
    XUngrabServer(display);
    ww->SendConfig();
}

/**
 * @fn    GetWmState(WaWindow *ww)
 * @brief Reads _NET_WM_STATE atom
 *
 * Reads _NET_WM_STATE atom to get the current state of the window.
 *
 * @param ww WaWindow object
 */
void NetHandler::GetWmState(WaWindow *ww) {
    CARD32 *data;
    bool vert = false, horz = false, shaded = false, title = false,
        handle = false, border = false, decor = false;
    unsigned int i;
    int status;

    XGrabServer(display);
    if (validateclient(ww->id)) {
        status = XGetWindowProperty(display, ww->id, net_state, 0L, 10L,
                                    false, XA_ATOM, &real_type,
                                    &real_format, &items_read, &items_left, 
                                    (unsigned char **) &data);
    } else WW_DELETED;
    XUngrabServer(display);
        
    if (status == Success && items_read)
        for (i = 0; i < items_read; i++) {
            if (data[i] == net_state_sticky) ww->flags.sticky = true;
            else if (data[i] == net_state_shaded) shaded = true;
            else if (data[i] == net_maximized_vert) vert = true;
            else if (data[i] == net_maximized_horz) horz = true;
            else if (data[i] == net_state_decor) decor = true;
            else if (data[i] == net_state_decortitle) title = true;
            else if (data[i] == net_state_decorhandle) handle = true;
            else if (data[i] == net_state_decorborder) border = true;
            else if (data[i] == net_state_aot) {
                ww->flags.alwaysontop = true;
                ww->wascreen->wawindow_list_stacking_aot.push_back(ww);
                ww->wascreen->WaRaiseWindow(0);
            }
            else if (data[i] == net_state_aab) {
                ww->flags.alwaysatbottom = true;
                ww->wascreen->wawindow_list_stacking_aab.push_back(ww);
                ww->wascreen->WaLowerWindow(ww->frame->id);
            }
        }
    if (decor) {
        ww->flags.title = ww->flags.handle = ww->flags.border = false;
        if (title) ww->flags.title = true;
        if (handle) ww->flags.handle = true;
        if (border) ww->flags.border = true;
        ww->flags.all = ww->flags.title && ww->flags.handle &&
            ww->flags.border;
    }
    XFree(data);
    
    if (vert && horz) {
        XGrabServer(display);
        if (validateclient(ww->id)) {
            status = XGetWindowProperty(display, ww->id, net_maximized_restore,
                                        0L, 6L, false, XA_CARDINAL, &real_type,
                                        &real_format, &items_read,
                                        &items_left, (unsigned char **) &data);
        } else WW_DELETED;
        XUngrabServer(display);

        if (status == Success && items_read >= 6) {
            ww->_Maximize(data[4], data[5]);
            ww->restore_max.x = data[0];
            ww->restore_max.y = data[1];
            ww->restore_max.width = data[2];
            ww->restore_max.height = data[3];
            XFree(data);
        }
    }
    if (shaded) ww->flags.shaded = true;
}

/**
 * @fn    SetWmState(WaWindow *ww)
 * @brief Sets _NET_WM_STATE atom
 *
 * Sets _NET_WM_STATE atom to the state of the window.
 *
 * @param ww WaWindow object
 */
void NetHandler::SetWmState(WaWindow *ww) {
    int i = 0;
    CARD32 data[10];
    CARD32 data2[6];

    XGrabServer(display);
    if (validateclient(ww->id)) {
        if (ww->flags.sticky) data[i++] = net_state_sticky;
        if (ww->flags.shaded) data[i++] = net_state_shaded;
        data[i++] = net_state_decor;
        if (ww->flags.title) data[i++] = net_state_decortitle;
        if (ww->flags.handle) data[i++] = net_state_decorhandle;
        if (ww->flags.border) data[i++] = net_state_decorborder;
        if (ww->flags.alwaysontop) data[i++] = net_state_aot;
        if (ww->flags.alwaysatbottom) data[i++] = net_state_aab;
        if (ww->flags.max) {
            data[i++] = net_maximized_vert;
            data[i++] = net_maximized_horz;
            
            data2[0] = ww->restore_max.x;
            data2[1] = ww->restore_max.y;
            data2[2] = ww->restore_max.width;
            data2[3] = ww->restore_max.height;
            data2[4] = ww->restore_max.misc0;
            data2[5] = ww->restore_max.misc1;
            XChangeProperty(display, ww->id, net_maximized_restore,
                            XA_CARDINAL, 32, PropModeReplace,
                            (unsigned char *) data2, 6);
        }
        XChangeProperty(display, ww->id, net_state, XA_ATOM, 32,
                        PropModeReplace, (unsigned char *) data, i);
    } else ww->deleted = true;
    XUngrabServer(display);
}

/**
 * @fn    SetSupported(WaScreen *ws)
 * @brief Writes _NET_SUPPORTED hint
 *
 * Sets _NET_SUPPORTED hint to the atoms Waimea supports.
 *
 * @param ws WaScreen object
 */
void NetHandler::SetSupported(WaScreen *ws) {
    CARD32 data[40];
    int i = 0;
    
    data[i++] = net_current_desktop;
    data[i++] = net_number_of_desktops;
    data[i++] = net_desktop_geometry;
    data[i++] = net_desktop_viewport;
    data[i++] = net_desktop_names;
    data[i++] = net_active_window;
    data[i++] = net_workarea;
    data[i++] = net_client_list;
    data[i++] = net_client_list_stacking;
    data[i++] = net_wm_window_type;
    data[i++] = net_wm_window_type_desktop;
    data[i++] = net_wm_window_type_dock;
    data[i++] = net_wm_window_type_toolbar;
    data[i++] = net_wm_window_type_menu;
    data[i++] = net_wm_window_type_splash;
    data[i++] = net_wm_window_type_normal;
    data[i++] = net_wm_window_type_dialog;
    data[i++] = net_wm_window_type_utility;
    data[i++] = net_state;
    data[i++] = net_state_sticky;
    data[i++] = net_state_shaded;
    data[i++] = net_maximized_vert;
    data[i++] = net_maximized_horz;
    data[i++] = net_wm_strut;
    data[i++] = net_wm_name;
    data[i++] = net_wm_visible_name;
    data[i++] = net_wm_desktop;
    data[i++] = net_wm_desktop_mask;

    data[i++] = net_state_decor;
    data[i++] = net_state_decortitle;
    data[i++] = net_state_decorhandle;
    data[i++] = net_state_decorborder;
    data[i++] = net_state_aot;
    data[i++] = net_state_aab;

#ifdef RENDER
    data[i++] = net_state_parentrelative_background;
#endif // RENDER
    
    data[i++] = net_maximized_restore;
    data[i++] = net_virtual_pos;
    data[i++] = net_restart;
    data[i++] = net_shutdown;
    XChangeProperty(display, ws->id, net_supported, XA_ATOM, 32,
                    PropModeReplace, (unsigned char *) data, i);
}

/**
 * @fn    SetSupportedCheck(WaScreen *ws, Window child)
 * @brief Writes _NET_SUPPORTED_WM_CHECK hint
 *
 * Sets _NET_SUPPORTED_WM_CHECK to child window and sets child windows
 * _NET_WM_NAME hint to the window manager name.
 *
 * @param ws WaScreen object
 & @param child Window to use as child window
*/
void NetHandler::SetSupportedWMCheck(WaScreen *ws, Window child) {
    XChangeProperty(display, ws->id, net_supported_wm_check, XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *) &child, 1);

    XChangeProperty(display, child, net_wm_name, utf8_string, 8,
                    PropModeReplace, (unsigned char *) PACKAGE,
                    strlen(PACKAGE));
}

/**
 * @fn    SetClientList(WaScreen *ws)
 * @brief Writes _NET_CLIENT_LIST hint
 *
 * Updates _NET_CLIENT_LIST hint to the current window list.
 *
 * @param ws WaScreen object
 */
void NetHandler::SetClientList(WaScreen *ws) {
    CARD32 *data;
    int i = 0;

    data = new CARD32[ws->wawindow_list_map_order.size() + 1];

    list<WaWindow *>::iterator it =
        ws->wawindow_list_map_order.begin();
    for (; it != ws->wawindow_list_map_order.end(); ++it) {
        if (! (*it)->flags.forcedatbottom)
            data[i++] = (*it)->id;
    }

    if (i > 0)
        XChangeProperty(display, ws->id, net_client_list, XA_WINDOW, 32,
                        PropModeReplace, (unsigned char *) data, i);

    delete [] data;
}

/**
 * @fn    SetClientListStacking(WaScreen *ws)
 * @brief Writes _NET_CLIENT_LIST_STACKING hint
 *
 * Updates _NET_CLIENT_LIST_STACKING hint to the current stacking order.
 *
 * @param ws WaScreen object
 */
void NetHandler::SetClientListStacking(WaScreen *ws) {
    CARD32 *data;
    list<WaWindow *>::reverse_iterator rit;
    list<WindowObject *>::reverse_iterator writ;
    list<WaWindow *>::iterator it;    
    int i = 0;

    data = new CARD32[ws->wawindow_list.size() + 1];
    
    it = ws->wawindow_list_stacking_aab.begin();
    for (; it != ws->wawindow_list_stacking_aab.end(); ++it)
        data[i++] = (*it)->id;
    writ = ws->wa_list_stacking.rbegin();
    for (; writ != ws->wa_list_stacking.rend(); ++writ) {
        if ((*writ)->type == WindowType)
            data[i++] = ((WaWindow *) (*writ))->id;
    }
    rit = ws->wawindow_list_stacking_aot.rbegin();
    for (; rit != ws->wawindow_list_stacking_aot.rend(); ++rit)
        data[i++] = (*rit)->id;

    if (i > 0)
        XChangeProperty(display, ws->id, net_client_list_stacking, XA_WINDOW,
                        32, PropModeReplace, (unsigned char *) data, i);

    delete [] data;
}

/**
 * @fn    GetClientList(WaScreen *ws)
 * @brief Reads _NET_CLIENT_LIST_STACKING hint
 *
 * Updates window stacking order to order in _NET_CLIENT_LIST_STACKING hint.
 *
 * @param ws WaScreen object
 */
void NetHandler::GetClientListStacking(WaScreen *ws) {
    CARD32 *data;
    WaWindow *ww;
    unsigned int i;
    
    if (XGetWindowProperty(display, ws->id, net_client_list_stacking,
                           0L, ws->wawindow_list.size(),
                           false, XA_WINDOW, &real_type,
                           &real_format, &items_read, &items_left, 
                           (unsigned char **) &data) == Success && 
        items_read) {
        for (i = 0; i < items_read; i++) {
            ww = (WaWindow *) waimea->FindWin(data[i], WindowType);
            if (ww) {
                if (ww->flags.alwaysontop) {
                    ws->wawindow_list_stacking_aot.remove(ww);
                    ws->wawindow_list_stacking_aot.push_front(ww);
                }
                else if (ww->flags.alwaysatbottom) {
                    ws->wawindow_list_stacking_aab.remove(ww);
                    ws->wawindow_list_stacking_aab.push_back(ww);
                }
                else if (! ww->flags.forcedatbottom) {
                    ws->wa_list_stacking.remove(ww);
                    ws->wa_list_stacking.push_front(ww);
                }
            }
        }
        ws->WaLowerWindow((Window) 0);
    }
}

/**
 * @fn    SetActiveWindow(WaScreen *ws, WaWindow *ww)
 * @brief Writes _NET_ACTIVE_CLIENT hint
 *
 * Updates _NET_ACTIVE_CLIENT hint to the current list of last active
 * windows. The currently active window at the front.
 *
 * @param ws WaScreen object
 * @param ww WaWindow that was set active or NULL if root window.
 */
void NetHandler::SetActiveWindow(WaScreen *ws, WaWindow *ww) {
    CARD32 *data;
    list<WaWindow *>::iterator it;
    int i = 0;

    data = new CARD32[ws->wawindow_list.size() + 1];
    
    if (ww) {
        ws->focus = false;
        ws->wawindow_list.remove(ww);
        ws->wawindow_list.push_front(ww);
    } else {
        ws->focus = true;
        data[i++] = None;
    }

    it = ws->wawindow_list.begin();
    for (; it != ws->wawindow_list.end(); ++it)
        data[i++] = (*it)->id;
    
    XChangeProperty(display, ws->id, net_active_window, XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *) data, i);

    delete [] data;
}

/**
 * @fn    GetActiveWindow(WaScreen *ws)
 * @brief Reads _NET_ACTIVE_CLIENT hint
 *
 * Sorts active window list after _NET_ACTIVE_CLIENT hint and sets input
 * focus to first window in _NET_ACTIVE_CLIENT list.
 *
 * @param ws WaScreen object
 */
void NetHandler::GetActiveWindow(WaScreen *ws) {
    WaWindow *ww;
    CARD32 *data;
    int i;
    
    if (XGetWindowProperty(display, ws->id, net_active_window, 0L,
                           ws->wawindow_list.size(), 
                           false, XA_WINDOW, &real_type, &real_format, 
                           &items_read, &items_left, 
                           (unsigned char **) &data) == Success && 
        items_read) {
        for (i = items_read - 1; i >= 0; i--) {
            if (i == 0 && data[0] == None) {
                ws->Focus(NULL, NULL);
                break;
            }
            ww = (WaWindow *) waimea->FindWin(data[i], WindowType);
            if (ww) {
                ws->wawindow_list.remove(ww);
                ws->wawindow_list.push_front(ww);
                if (i == 0)
                    ww->Focus(false);
            }
        }
        XFree(data);
    }
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
    int *data;
    
    XGrabServer(display);
    if (validateclient(ww->id)) {
        if (XGetWindowProperty(display, ww->id, net_virtual_pos, 0L, 2L, 
                               false, XA_INTEGER, &real_type, &real_format, 
                               &items_read, &items_left, 
                               (unsigned char **) &data) == Success && 
            items_read >= 2) {
            ww->attrib.x = data[0] - ww->wascreen->v_x;
            ww->attrib.y = data[1] - ww->wascreen->v_y;
            if (ww->flags.sticky) {
                ww->attrib.x = ww->attrib.x % ww->wascreen->width;
                ww->attrib.y = ww->attrib.y % ww->wascreen->height;
            }
            XFree(data);
        }
    } else ww->deleted = true;
    XUngrabServer(display);
}

/**
 * @fn    GetXaName(WaWindow *ww)
 * @brief Reads window title
 *
 * Reads WM_NAME hint and if hint exists the window title is set to this
 * and _NET_WM_VISIBLE_HINT is updated.
 *
 * @param ww WaWindow object
 */
void NetHandler::GetXaName(WaWindow *ww) {
    char *data = NULL;
    int status;
    char *__m_wastrdup_tmp;
    
    XGrabServer(display);
    if (validateclient(ww->id)) {
        status = XFetchName(display, ww->id, &data);
    } else ww->deleted = true;
    XUngrabServer(display);

    if (status && data) {
        delete [] ww->name;
        ww->name = __m_wastrdup(data);
        XFree(data);
        ww->SetActionLists();
    }
        
    XGrabServer(display);
    if (validateclient(ww->id)) {
        XChangeProperty(display, ww->id, net_wm_visible_name,
                            XA_STRING, 8, PropModeReplace,
                            (unsigned char *) ww->name, strlen(ww->name));
    } else ww->deleted = true;
    XUngrabServer(display);
}

/**
 * @fn    GetNetName(WaWindow *ww)
 * @brief Reads window title
 *
 * Reads _NET_WM_NAME hint and if hint exists the window title is set to this
 * and _NET_WM_VISIBLE_HINT is updated.
 *
 * @param ww WaWindow object
 *
 * @return True if _NET_WM_NAME hint did exist otherwise false
 */
bool NetHandler::GetNetName(WaWindow *ww) {
    char *data;
    int status;
    char *__m_wastrdup_tmp;
    
    XGrabServer(display);
    if (validateclient(ww->id)) {
        status = XGetWindowProperty(display, ww->id, net_wm_name, 0L, 8192L, 
                                    false, utf8_string, &real_type,
                                    &real_format, &items_read, &items_left, 
                                    (unsigned char **) &data);
    } else ww->deleted = true;
    XUngrabServer(display);
    
    if (status == Success && items_read) {
        delete [] ww->name;
        ww->name = __m_wastrdup(data);
        ww->SetActionLists();
        XFree(data);

        XGrabServer(display);
        if (validateclient(ww->id)) {
            XChangeProperty(display, ww->id, net_wm_visible_name,
                            utf8_string, 8, PropModeReplace,
                            (unsigned char *) ww->name, strlen(ww->name));
        } else ww->deleted = true;
        XUngrabServer(display);

        return true;
    }
    return false;
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
    int data[2];
    
    ww->Gravitate(RemoveGravity);
    data[0] = ww->wascreen->v_x + ww->attrib.x;
    data[1] = ww->wascreen->v_y + ww->attrib.y;
    ww->Gravitate(ApplyGravity);

    XGrabServer(display);
    if (validateclient(ww->id))
        XChangeProperty(display, ww->id, net_virtual_pos, XA_INTEGER, 32,
                        PropModeReplace, (unsigned char *) data, 2);
    else ww->deleted = true;
    XUngrabServer(display);
}

/**
 * @fn    GetWmStrut(WaWindow *ww)
 * @brief Reads WM_STRUT hint
 *
 * Reads windows WM_STRUT hint, adds it to the strut list and updates
 * workarea.
 *
 * @param ww WaWindow object
 */
void NetHandler::GetWmStrut(WaWindow *ww) {
    CARD32 *data;
    WMstrut *wm_strut;
    bool found = false;
    int status;

    XGrabServer(display);
    if (validateclient(ww->id)) {
        status = XGetWindowProperty(display, ww->id, net_wm_strut, 0L, 4L, 
                                    false, XA_CARDINAL, &real_type,
                                    &real_format, &items_read, &items_left, 
                                    (unsigned char **) &data);
    } else WW_DELETED;
    XUngrabServer(display);
    
    if (status == Success && items_read >= 4) {
        list<WMstrut *>::iterator it = ww->wascreen->strut_list.begin();
        for (; it != ww->wascreen->strut_list.end(); ++it) {
            if ((*it)->window == ww->id) {
                (*it)->left = data[0];
                (*it)->right = data[1];
                (*it)->top = data[2];
                (*it)->bottom = data[3];
                found = true;
                ww->wascreen->UpdateWorkarea();
            }
        }
        if (! found) {
            wm_strut = new WMstrut;
            wm_strut->window = ww->id;
            wm_strut->left = data[0];
            wm_strut->right = data[1];
            wm_strut->top = data[2];
            wm_strut->bottom = data[3];
            ww->wm_strut = wm_strut;
            ww->wascreen->strut_list.push_back(wm_strut);
            ww->wascreen->UpdateWorkarea();
        }
        XFree(data);
    }
}

/**
 * @fn    GetWmPid(WaWindow *ww)
 * @brief Reads _NET_WM_PID hint
 *
 * Reads windows _NET_WM_PID hint and stores it in WaWindows pid variable.
 *
 * @param ww WaWindow object
 */
void NetHandler::GetWmPid(WaWindow *ww) {
    char tmp[32];
    CARD32 *data;

    XGrabServer(display);
    if (validateclient(ww->id)) {
        if (XGetWindowProperty(ww->display, ww->id, net_wm_pid, 0L, 1L, 
                               false, XA_CARDINAL, &real_type,
                               &real_format, &items_read, &items_left, 
                               (unsigned char **) &data) == Success && 
            items_read) {
            sprintf(tmp, "%d" , (unsigned int) *data);
            ww->pid = new char[strlen(tmp) + 1];
            sprintf(ww->pid, "%s" , tmp);
            XFree(data);
        }
    } else ww->deleted = true;
    XUngrabServer(display);
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
                           false, XA_CARDINAL, &real_type,
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
                    PropModeReplace, (unsigned char *) data, 2);
}

/**
 * @fn    SetDesktopGeometry(WaScreen *ws)
 * @brief Writes desktop geometry hint
 *
 * Sets _NET_DESKTOP_GEOMETRY hint.
 *
 * @param ws WaScreen object
 */
void NetHandler::SetDesktopGeometry(WaScreen *ws) {
    CARD32 data[2];

    data[0] = ws->v_xmax + ws->width;
    data[1] = ws->v_ymax + ws->height;    
    XChangeProperty(display, ws->id, net_desktop_geometry, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *) data, 2);
}

/**
 * @fn    SetNumberOfDesktops(WaScreen *ws)
 * @brief Writes number of desktops hint
 *
 * Sets _NET_NUMBER_OF_DESKTOPS hint.
 *
 * @param ws WaScreen object
 */
void NetHandler::SetNumberOfDesktops(WaScreen *ws) {
    CARD32 data[1];

    data[0] = ws->desktop_list.size();
    XChangeProperty(display, ws->id, net_number_of_desktops, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *) data, 1);
}
    
/**
 * @fn    SetCurrentDesktop(WaScreen *ws)
 * @brief Writes current desktop hint
 *
 * Sets _NET_CURRENT_DESKTOP hint.
 *
 * @param ws WaScreen object
 */
void NetHandler::SetCurrentDesktop(WaScreen *ws) {
    CARD32 data[1];
    
    data[0] = ws->current_desktop->number;
    XChangeProperty(display, ws->id, net_current_desktop, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *) data, 1);
}

/**
 * @fn    GetCurrentDesktop(WaScreen *ws)
 * @brief Reads current desktop hint
 *
 * Reads _NET_CURRENT_DESKTOP hint and sets desktop accordingly.
 *
 * @param ws WaScreen object
 */
void NetHandler::GetCurrentDesktop(WaScreen *ws) {
    CARD32 *data;

    if (XGetWindowProperty(display, ws->id, net_current_desktop, 0L, 1L, 
                           false, XA_CARDINAL, &real_type, &real_format, 
                           &items_read, &items_left, 
                           (unsigned char **) &data) == Success && 
        items_read) {
        ws->GoToDesktop(data[0]);
        XFree(data);
    }
}


/**
 * @fn    SetDesktopNames(WaScreen *ws, char *names)
 * @brief Writes desktop names hint
 *
 * Sets _NET_DESKTOP_NAMES.
 *
 * @param ws WaScreen object
 * @param names String to parse desktop names from
 */
void NetHandler::SetDesktopNames(WaScreen *ws, char *names) {
    int i;
    
    for (i = 0; i < 8192; i++) {
        if (names[i] == ',') names[i] = '\0';
        else if (names[i] == '\0') break;
    }
    names[i] = '\0';

    if (i)
        XChangeProperty(display, ws->id, net_desktop_names, utf8_string, 8,
                        PropModeReplace, (unsigned char *) names, i + 1);
}

/**
 * @fn    SetWorkarea(Workarea *workarea)
 * @brief Sets window manager workarea
 *
 * Sets the window manager workarea, used for placing icons and maximizing
 * windows.
 *
 * @param workarea Structure containing workarea parameters
 */
void NetHandler::SetWorkarea(WaScreen *ws) {
    CARD32 data[4];

    data[0] = ws->current_desktop->workarea.x;
    data[1] = ws->current_desktop->workarea.y;
    data[2] = ws->current_desktop->workarea.width;
    data[3] = ws->current_desktop->workarea.height;
    
    XChangeProperty(waimea->display, ws->id, net_workarea, XA_CARDINAL,
                    32, PropModeReplace, (unsigned char *) data, 4);
}

/**
 * @fn    wXDNDMakeAwareness(Window window)
 * @brief Make window DND aware
 *
 * Makes drag'n'drop awareness for window.
 *
 * @param window Window to make DND aware
 */
void NetHandler::wXDNDMakeAwareness(Window window) {
    long int xdnd_version = 3;

    XChangeProperty(waimea->display, window, xa_xdndaware, XA_ATOM,
                    32, PropModeReplace, (unsigned char *) &xdnd_version, 1);
}

/**
 * @fn    wXDNDClearAwareness(Window window)
 * @brief Removes DND awareness from window
 *
 * Removes drag'n'drop awareness for window.
 *
 * @param window Window to remove DND awareness from
 */
void NetHandler::wXDNDClearAwareness(Window window) {
    XDeleteProperty (waimea->display, window, xa_xdndaware);
}

/**
 * @fn    DeleteSupported(WaScreen *ws)
 * @brief Deletes supported hints
 *
 * Deletes all hints set on the root window that we don't have any use
 * of next time Waimea starts.
 *
 * @param ws WaScreen object
 */
void NetHandler::DeleteSupported(WaScreen *ws) {
    XDeleteProperty(display, ws->id, net_desktop_geometry);
    XDeleteProperty(display, ws->id, net_workarea);
    XDeleteProperty(display, ws->id, net_supported_wm_check);
    XDeleteProperty(display, ws->id, net_supported);
}

#ifdef RENDER
/**
 * @fn    GetXRootPMapId(WaScreen *ws)
 * @brief Reads XROOTPMAPID hint
 *
 * Reads XROOTPMAPID hint, which if it exist is the pixmap ID for the root
 * window.
 *
 * @param ws WaScreen object
 */
void NetHandler::GetXRootPMapId(WaScreen *ws) {
    CARD32 *data;

    XSync(ws->display, false);
    if (XGetWindowProperty(ws->pdisplay, ws->id, xrootpmap_id, 0L, 1L, 
                           false, XA_PIXMAP, &real_type,
                           &real_format, &items_read, &items_left, 
                           (unsigned char **) &data) == Success && 
        items_read) { 
        ws->xrootpmap_id = (Pixmap) (*data);
        XFree(data);
    }
    else
        ws->xrootpmap_id = (Pixmap) 0;

    XSync(ws->display, false);
    XSync(ws->pdisplay, false);
}
#endif // RENDER

/**
 * @fn    GetType(WaWindow *ww)
 * @brief Read type hint
 *
 * Reads _NET_WM_WINDOW_TYPE hint and sets window properties accordingly.
 *
 * @param ww WaWindow object
 */
void NetHandler::GetWmType(WaWindow *ww) {
    CARD32 *data;
    int status;
    
    XGrabServer(display);
    if (validateclient(ww->id)) {
        status = XGetWindowProperty(display, ww->id, net_wm_window_type,
                                    0L, 8L, false, XA_ATOM,
                                    &real_type, &real_format, &items_read,
                                    &items_left, (unsigned char **) &data);
    } else WW_DELETED;
    XUngrabServer(display);
    
    if (status == Success && items_read) {
        for (unsigned int i = 0; i < items_read; ++i) {
            if (data[i] == net_wm_window_type_desktop) {
                ww->desktop_mask = (1L << 16) - 1;
                ww->flags.tasklist = false;
                ww->flags.sticky = true;
                ww->flags.border = ww->flags.title = ww->flags.handle =
                    ww->flags.all = false;
                ww->size.max_width = ww->wascreen->width;
                ww->size.min_width = ww->wascreen->width;
                ww->size.max_height = ww->wascreen->height;
                ww->size.min_height = ww->wascreen->height;
                ww->attrib.x = 0;
                ww->attrib.y = 0;
                if (ww->flags.alwaysontop)
                    ww->wascreen->wawindow_list_stacking_aot.remove(ww);
                if (ww->flags.alwaysatbottom)
                    ww->wascreen->wawindow_list_stacking_aab.remove(ww);
                ww->flags.alwaysatbottom = ww->flags.alwaysontop = false;
                ww->flags.forcedatbottom = true;
                ww->wascreen->always_at_bottom_list.push_back(ww->frame->id);
                ww->wascreen->WaLowerWindow(ww->frame->id);
            }
            else if (data[i] == net_wm_window_type_toolbar ||
                     data[i] == net_wm_window_type_dock) {
                ww->desktop_mask = (1L << 16) - 1;
                ww->flags.tasklist = false;
                ww->flags.sticky = true;
                ww->flags.border = ww->flags.title = ww->flags.handle =
                    ww->flags.all = false;
                if (ww->flags.alwaysontop)
                    ww->wascreen->wawindow_list_stacking_aot.remove(ww);
                if (ww->flags.alwaysatbottom)
                    ww->wascreen->wawindow_list_stacking_aab.remove(ww);
                ww->flags.alwaysontop = true;
                ww->wascreen->wawindow_list_stacking_aot.push_front(ww);
                ww->wascreen->WaRaiseWindow(0);
            }
            else if (data[i] == net_wm_window_type_splash ||
                     data[i] == net_wm_window_type_menu) {
                ww->flags.tasklist = false;
                ww->flags.border = ww->flags.title = ww->flags.handle =
                    ww->flags.all = false;
                if (ww->flags.alwaysontop)
                    ww->wascreen->wawindow_list_stacking_aot.remove(ww);
                if (ww->flags.alwaysatbottom)
                    ww->wascreen->wawindow_list_stacking_aab.remove(ww);
                ww->flags.alwaysontop = true;
                ww->wascreen->wawindow_list_stacking_aot.push_front(ww);
                ww->wascreen->WaRaiseWindow(0);
            }
            else {
                if (ww->attrib.x == 0) {
                    if (ww->wascreen->current_desktop->workarea.x >
                        ww->attrib.x)
                        ww->attrib.x =
                            ww->wascreen->current_desktop->workarea.x;
                }
                if (ww->attrib.y == 0) {
                    if (ww->wascreen->current_desktop->workarea.y >
                        ww->attrib.y)
                        ww->attrib.y =
                            ww->wascreen->current_desktop->workarea.y;
                }
            }
        }
        XFree(data);
    } else {
        if (ww->attrib.x == 0) {
            if (ww->wascreen->current_desktop->workarea.x > ww->attrib.x)
                ww->attrib.x = ww->wascreen->current_desktop->workarea.x;
        }
        if (ww->attrib.y == 0) {
            if (ww->wascreen->current_desktop->workarea.y > ww->attrib.y)
                ww->attrib.y = ww->wascreen->current_desktop->workarea.y;
        }
    }
}

/**
 * @fn    SetDesktop(WaWindow *ww)
 * @brief Write net_wm_desktop hint
 *
 * Sets _NET_WM_DESKTOP hint to current desktop, if window is not a member
 * of the current desktop then hint is set to the lowest desktop number that
 * the window is a member of. If window is a member of all desktops then
 * hint is set to 0xffffffff.
 *
 * @param ww WaWindow object
 */
void NetHandler::SetDesktop(WaWindow *ww) {
    CARD32 data[1];

    data[0] = 0;
    if (ww->desktop_mask & (1L << ww->wascreen->current_desktop->number))
        data[0] = ww->wascreen->current_desktop->number;
    else {
        int i;
        for (i = 0; i < 16; i++)
            if (ww->desktop_mask & (1L << i)) {
                data[0] = i;
                break;
            }
    }
    if (ww->desktop_mask == ((1L << 16) - 1))
        data[0] = 0xffffffff;
    
    XGrabServer(display);
    if (validateclient(ww->id)) {    
        XChangeProperty(display, ww->id, net_wm_desktop, XA_CARDINAL, 32,
                        PropModeReplace, (unsigned char *) data, 1);
    } else ww->deleted = true;
    XUngrabServer(display);
}

/**
 * @fn    SetDesktopMask(WaWindow *ww)
 * @brief Write net_wm_desktop_mask hint
 *
 * Sets _NET_WM_DESKTOP_MASK hint to the current desktop mask of the window.
 *
 * @param ww WaWindow object
 */
void NetHandler::SetDesktopMask(WaWindow *ww) {
    CARD32 data[1];

    data[0] = ww->desktop_mask;

    XGrabServer(display);
    if (validateclient(ww->id)) {
        XChangeProperty(display, ww->id, net_wm_desktop_mask, XA_CARDINAL, 32,
                        PropModeReplace, (unsigned char *) data, 1);
    } else ww->deleted = true;
    XUngrabServer(display);
}

/**
 * @fn    GetDesktop(WaWindow *ww)
 * @brief Reads net_wm_desktop and net_desktop_mask hints
 *
 * Reads _NET_WM_DESKTOP and _NET_DESKTOP_MASK hints and sets desktop_mask
 * for window accordingly.
 *
 * @param ww WaWindow object
 */
void NetHandler::GetDesktop(WaWindow *ww) {
    CARD32 *data;

    XGrabServer(display);
    if (validateclient(ww->id)) {
        if (XGetWindowProperty(display, ww->id, net_wm_desktop, 0L, 1L, 
                               false, XA_CARDINAL, &real_type, &real_format, 
                               &items_read, &items_left,
                               (unsigned char **) &data) == Success && 
            items_read) {
            if (data[0] == 0xffffffff)
                ww->desktop_mask = ((1L << 16) - 1);
            else if (data[0] < 15 && data[0] >= 0) {
                ww->desktop_mask = (1L << data[0]);
            }
            XFree(data);
        }
        if (XGetWindowProperty(display, ww->id, net_wm_desktop_mask, 0L, 1L, 
                               false, XA_CARDINAL, &real_type, &real_format, 
                               &items_read, &items_left, 
                               (unsigned char **) &data) == Success && 
            items_read) {
            ww->desktop_mask = data[0];
            XFree(data);
        }
    } else ww->deleted = true;
    XUngrabServer(display);
}
