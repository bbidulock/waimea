/**
 * @file   NetHandler.hh
 * @author David Reveman <david@waimea.org>
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

extern "C" {
#include <X11/Xlib.h>
#include <X11/Xproto.h>
}

class NetHandler;

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

#define _NET_WM_STATE_REMOVE 0
#define _NET_WM_STATE_ADD    1
#define _NET_WM_STATE_TOGGLE 2

#include "Waimea.hh"

class NetHandler {
public:
    NetHandler(Waimea *);
    virtual ~NetHandler(void);
    
    void GetWMHints(WaWindow *);
    void GetMWMHints(WaWindow *);
    void GetWMNormalHints(WaWindow *);
    void GetState(WaWindow *);
    void SetState(WaWindow *, int);
    void GetWmState(WaWindow *);
    void SetWmState(WaWindow *);
    void GetVirtualPos(WaWindow *);
    void SetVirtualPos(WaWindow *);
    void GetWmStrut(WaWindow *);
    void GetWmPid(WaWindow *);
    void GetXaName(WaWindow *);
    bool GetNetName(WaWindow *);
    void SetDesktop(WaWindow *);
    void SetDesktopMask(WaWindow *);
    void GetDesktop(WaWindow *);

    void SetSupported(WaScreen *);
    void SetSupportedWMCheck(WaScreen *, Window);
    void SetClientList(WaScreen *);
    void SetClientListStacking(WaScreen *);
    void GetClientListStacking(WaScreen *);
    void SetActiveWindow(WaScreen *, WaWindow *);
    void GetActiveWindow(WaScreen *);
    void GetDesktopViewPort(WaScreen *);
    void SetDesktopViewPort(WaScreen *);
    void SetDesktopGeometry(WaScreen *);
    void SetNumberOfDesktops(WaScreen *);
    void SetCurrentDesktop(WaScreen *);
    void GetCurrentDesktop(WaScreen *);
    void SetDesktopNames(WaScreen *, char *);

    void wXDNDMakeAwareness(Window);
    void wXDNDClearAwareness(Window);
    void SetWorkarea(WaScreen *);
    void DeleteSupported(WaScreen *);

#ifdef RENDER
    void GetXRootPMapId(WaScreen *);
#endif // RENDER

    void GetWmType(WaWindow *);
    
    Waimea *waimea;
    Display *display;
    XWMHints *wm_hints;
    XSizeHints *size_hints;
    MwmHints *mwm_hints;

    Atom mwm_hints_atom, wm_state, net_supported, net_supported_wm_check,
        net_client_list, net_client_list_stacking, net_active_window,
        net_state, net_state_sticky, net_state_shaded, net_maximized_vert,
        net_maximized_horz, net_state_decor, net_state_decortitle,
        net_state_decorhandle, net_state_decorborder, net_state_aot,
        net_state_aab, net_state_parentrelative_background,
        net_maximized_restore, net_virtual_pos, net_desktop_viewport,
        net_desktop_geometry, net_wm_strut, net_workarea, xa_xdndaware,
        xa_xdndenter, xa_xdndleave, net_wm_name, net_restart, net_shutdown,
        net_wm_pid, net_wm_window_type, net_wm_window_type_desktop,
        net_wm_window_type_dock, net_wm_window_type_toolbar,
        net_wm_window_type_menu, net_wm_window_type_splash,
        net_wm_window_type_dialog, net_wm_window_type_utility,
        net_wm_window_type_normal, net_current_desktop,
        net_number_of_desktops, net_close_window, net_wm_visible_name,
        net_desktop_names, utf8_string, net_wm_desktop, net_wm_desktop_mask;
    
#ifdef RENDER
    Atom xrootpmap_id;
#endif // RENDER

private:
    XEvent event;
    
    int real_format;
    Atom real_type;
    unsigned long items_read, items_left;
};

#endif // __NetHandler_hh
