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
    long flags;
    long functions;
    long decorations;
} MwmHints;

#define _NET_WM_STATE_REMOVE 0
#define _NET_WM_STATE_ADD    1
#define _NET_WM_STATE_TOGGLE 2

#define _NET_WM_MOVERESIZE_SIZE_TOPLEFT      0
#define _NET_WM_MOVERESIZE_SIZE_TOP          1
#define _NET_WM_MOVERESIZE_SIZE_TOPRIGHT     2
#define _NET_WM_MOVERESIZE_SIZE_RIGHT        3
#define _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT  4
#define _NET_WM_MOVERESIZE_SIZE_BOTTOM       5
#define _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT   6
#define _NET_WM_MOVERESIZE_SIZE_LEFT         7
#define _NET_WM_MOVERESIZE_MOVE              8
#define _NET_WM_MOVERESIZE_SIZE_KEYBOARD     9
#define _NET_WM_MOVERESIZE_MOVE_KEYBOARD    10

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
    void SetVisibleName(WaWindow *);
    void RemoveVisibleName(WaWindow *);
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

    void SetAllowedActions(WaWindow *);
    void RemoveAllowedActions(WaWindow *);

    void GetMergedState(WaWindow *);
    void SetMergedState(WaWindow *);
    void SetMergeAtfront(WaWindow *, Window);
    void GetMergeAtfront(WaWindow *);
    void GetMergeOrder(WaWindow *);
    void SetMergeOrder(WaWindow *);

    bool IsSystrayWindow(Window);
    void SetSystrayWindows(WaScreen *);

    Waimea *waimea;
    Display *display;
    XWMHints *wm_hints;
    XSizeHints *size_hints;
    MwmHints *mwm_hints;

    Atom utf8_string;

    Atom mwm_hints_atom;
    Atom wm_state, wm_change_state;

    Atom net_supported, net_supported_wm_check;
    Atom net_client_list, net_client_list_stacking, net_active_window;
    Atom net_desktop_viewport, net_desktop_geometry, net_current_desktop,
        net_number_of_desktops, net_desktop_names, net_workarea;
    Atom net_wm_desktop, net_wm_name, net_wm_visible_name, net_wm_strut,
        net_wm_pid;
    Atom net_wm_state, net_wm_state_sticky, net_wm_state_shaded,
        net_wm_state_hidden, net_wm_maximized_vert, net_wm_maximized_horz,
        net_wm_state_above, net_wm_state_below, net_wm_state_stays_on_top,
        net_wm_state_stays_at_bottom, net_wm_state_fullscreen,
        net_wm_state_skip_taskbar;
    Atom net_wm_allowed_actions, net_wm_action_move, net_wm_action_resize,
        net_wm_action_minimize, net_wm_action_shade, net_wm_action_stick,
        net_wm_action_maximize_horz, net_wm_action_maximize_vert,
        net_wm_action_fullscreen, net_wm_action_change_desktop,
        net_wm_action_close;
    Atom net_wm_window_type, net_wm_window_type_desktop,
        net_wm_window_type_dock, net_wm_window_type_toolbar,
        net_wm_window_type_menu, net_wm_window_type_splash,
        net_wm_window_type_dialog, net_wm_window_type_utility,
        net_wm_window_type_normal;
    Atom net_close_window, net_moveresize_window, net_wm_moveresize;

    Atom waimea_net_wm_state_decor, waimea_net_wm_state_decortitle,
        waimea_net_wm_state_decorhandle,
        waimea_net_wm_state_decorborder;
    Atom waimea_net_maximized_restore, waimea_net_virtual_pos,
        waimea_net_wm_desktop_mask;
    Atom waimea_net_wm_merged_to, waimea_net_wm_merged_type,
        waimea_net_wm_merge_order, waimea_net_wm_merge_atfront;
    Atom waimea_net_restart, waimea_net_shutdown;

    Atom xdndaware, xdndenter, xdndleave;

    Atom kde_net_wm_system_tray_window_for, kde_net_system_tray_windows;

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
