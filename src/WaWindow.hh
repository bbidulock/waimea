/** -*- Mode: C++ -*-
 *
 * @file   WaWindow.cc
 * @author David Reveman <c99drn@cs.umu.se>
 * @date   02-May-2001 21:43:03
 *
 * @brief Definition of WaWindow and WaChildWindow classes
 *
 * Function declarations and variable definitions for WaWindow and
 * WaChildWindow classes.
 *
 * Copyright (C) David Reveman. All rights reserved.
 *
 */

#ifndef __WaWindow_hh
#define __WaWindow_hh

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xmd.h>

class WaWindow;
class WaChildWindow;

typedef void (WaWindow::*WwActionFn)(XEvent *);

#include "WaScreen.hh"
#include "WaMenu.hh"
#include "Waimea.hh"
#include "NetHandler.hh"
#include "ResourceHandler.hh"

#define ApplyGravity   1
#define RemoveGravity -1

#define EastType  1
#define WestType -1

typedef struct {
    WaMenuItem *wmi;
    int max_width;
    int max_height;
    int min_width;
    int min_height;
    int width_inc;
    int height_inc;
    int base_width;
    int base_height;
    int win_gravity;
} SizeStruct;

typedef struct {
    bool title;
    bool border;
    bool handle;
    bool sticky;
} WaWindowFlags;

typedef struct {
    int x;
    int y;
    int width;
    int height;
    Colormap colormap;
} WaWindowAttributes;

class WaWindow : public WindowObject {
public:
    WaWindow(Window, WaScreen *);
    virtual ~WaWindow(void);
    
    void SendConfig(void);
    void Gravitate(int);
    void ButtonPressed(int);
    void RedrawWindow(void);
    void DrawIconifyButtonFg(void);
    void DrawCloseButtonFg(void);
    void DrawMaxButtonFg(void);
    void DrawLabelFg(void);
    void FocusWin(void);
    void UnFocusWin(void);
    void ButtonHilite(int);
    void ButtonDehilite(int);
    
    void Raise(XEvent *);
    void Lower(XEvent *);
    void Focus(XEvent *);
    void Move(XEvent *);
    void MoveOpaque(XEvent *);
    inline void ResizeRight(XEvent *e) { Resize(e, EastType); }
    inline void ResizeLeft(XEvent *e) { Resize(e, WestType); }
    inline void ResizeRightOpaque(XEvent *e) { ResizeOpaque(e, EastType); }
    inline void ResizeLeftOpaque(XEvent *e) { ResizeOpaque(e, WestType); }
    void Maximize(XEvent *);
    void UnMaximize(XEvent *);
    void ToggleMaximize(XEvent *);
    void Close(XEvent *);
    void Kill(XEvent *);
    void CloseKill(XEvent *);
    void MenuMap(XEvent *);
    void MenuReMap(XEvent *);
    void MenuUnmap(XEvent *);
    void Shade(XEvent *);
    void UnShade(XEvent *);
    void ToggleShade(XEvent *);
    void Sticky(XEvent *);
    void UnSticky(XEvent *);
    void ToggleSticky(XEvent *);
    void EvAct(XEvent *, EventDetail *, list<WaAction *> *, int);
    
    char *name;
    Bool has_focus, want_focus, mapped, shaded, maximized, dontsend;
    Display *display;
    Waimea *waimea;
    WaScreen *wascreen;
    int border_w, title_w, handle_w, screen_number, state;
    WaChildWindow *frame, *title, *label, *handle, *button_c, *button_max,
        *button_min, *grip_r, *grip_l;
    WaWindowAttributes attrib, old_attrib, restore;
    WaWindowFlags flags;
    SizeStruct size;
    WaMenu *map_menu;
    NetHandler *net;
    
private:
    void ReparentWin(void);
    void InitPosition(void);
    bool IncSizeCheck(int, int, int *, int *);
    void CreateOutlineWindows(void);
    void ToggleOutline(void);
    void DrawOutline(int, int, int, int);
    void Resize(XEvent *, int);
    void ResizeOpaque(XEvent *, int);
    void RenderLabel(void);
    void RenderTitle(void);
    void RenderHandle(void);
    void DrawIconifyButton(void);
    void DrawCloseButton(void);
    void DrawMaxButton(void);
    void DrawLabel(void);
    void DrawHandle(void);
    void DrawLeftGrip(void);
    void DrawRightGrip(void);
    void DrawTitle(void);
    inline void DrawTitlebar(void) { DrawTitle(); 
                                     DrawLabel();
                                     DrawIconifyButton();
                                     DrawCloseButton();
                                     DrawMaxButton(); }
    inline void DrawHandlebar(void) { DrawHandle();
                                      DrawLeftGrip();
                                      DrawRightGrip(); }
    
    WaImageControl *ic;

    Window o_west, o_north, o_south, o_east;
    bool o_mapped;
    Pixmap ftitle, fhandle, flabel;
    Pixmap utitle, uhandle, ulabel;
    Pixmap *ptitle, *phandle, *plabel, *pgrip, *pbutton;
    GC *b_cpic_gc, *b_ipic_gc, *b_mpic_gc;
    unsigned long ftitle_pixel, fhandle_pixel, flabel_pixel;
    unsigned long utitle_pixel, uhandle_pixel, ulabel_pixel;
    unsigned long *title_pixel, *handle_pixel, *label_pixel, *grip_pixel,
        *button_pixel;
#ifdef XFT        
    XftDraw *xftdraw;
    XftColor *xftcolor;
#else // ! XFT
    GC *l_text_gc;
#endif // XFT
};

class WaChildWindow : public WindowObject {
public:
    WaChildWindow(WaWindow *, Window, int, int);
    virtual ~WaChildWindow(void);
    
    WaWindowAttributes attrib;
    WaWindow *wa;
};

#endif // __WaWindow_hh
