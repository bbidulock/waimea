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

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif //SHAPE

class WaWindow;
class WaChildWindow;

typedef struct _WaAction WaAction;
typedef void (WaWindow::*WwActionFn)(XEvent *, WaAction *);

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
    bool shaded;
    bool max;
    bool all;
    bool alwaysontop;
    bool alwaysatbottom;
} WaWindowFlags;

typedef struct {
    int x;
    int y;
    int width;
    int height;
    int misc0;
    int misc1;
    Colormap colormap;
} WaWindowAttributes;

class WaWindow : public WindowObject {
public:
    WaWindow(Window, WaScreen *);
    virtual ~WaWindow(void);

    void MapWindow(void);
    void UpdateAllAttributes(void);
    void RedrawWindow(void);
    void SendConfig(void);
    void Gravitate(int);
    void ButtonPressed(int);
    void UpdateGrabs(void);
    bool IncSizeCheck(int, int, int *, int *);
    void DrawIconifyButtonFg(void);
    void DrawCloseButtonFg(void);
    void DrawMaxButtonFg(void);
    void DrawLabelFg(void);
    void FocusWin(void);
    void UnFocusWin(void);
    void ButtonHilite(int);
    void ButtonDehilite(int);
    void Focus(bool);
    void _Maximize(int, int);
    void MenuMap(XEvent *, WaAction *, bool);
    void MenuRemap(XEvent *, WaAction *, bool);
    void MenuUnmap(XEvent *, WaAction *, bool);

#ifdef SHAPE
    void Shape(void);
#endif // SHAPE
    
    void Raise(XEvent *, WaAction *);
    void Lower(XEvent *, WaAction *);
    inline void Focus(XEvent *e, WaAction *ac) {
        Focus(false);
    }
    inline void FocusVis(XEvent *e, WaAction *ac) {
        Focus(true);
    }
    void Move(XEvent *, WaAction *);
    void MoveOpaque(XEvent *, WaAction *);
    inline void ResizeRight(XEvent *e, WaAction *) {
        Resize(e, EastType);
    }
    inline void ResizeLeft(XEvent *e, WaAction *) {
        Resize(e, WestType);
    }
    inline void ResizeRightOpaque(XEvent *e, WaAction *) {
        ResizeOpaque(e, EastType);
    }
    inline void ResizeLeftOpaque(XEvent *e, WaAction *) {
        ResizeOpaque(e, WestType);
    }
    void EndMoveResize(XEvent *, WaAction *);
    inline void Maximize(XEvent *, WaAction *) { _Maximize(-1, -1); } 
    void UnMaximize(XEvent *, WaAction *);
    void ToggleMaximize(XEvent *, WaAction *);
    void Close(XEvent *, WaAction *);
    void Kill(XEvent *, WaAction *);
    void CloseKill(XEvent *, WaAction *);
    inline void MenuMap(XEvent *e, WaAction *ac) {
        MenuMap(e, ac, false);
    }
    inline void MenuMapFocused(XEvent *e, WaAction *ac) {
        MenuMap(e, ac, true);
    }
    inline void MenuRemap(XEvent *e, WaAction *ac) {
        MenuRemap(e, ac, false);
    }
    inline void MenuRemapFocused(XEvent *e, WaAction *ac) {
        MenuRemap(e, ac, true);
    }
    inline void MenuUnmap(XEvent *e, WaAction *wa) {
        MenuUnmap(e, wa, false);
    }
    inline void MenuUnmapFocus(XEvent *e, WaAction *wa) {
        MenuUnmap(e, wa, true);
    }
    void Shade(XEvent *, WaAction *);
    void UnShade(XEvent *, WaAction *);
    void ToggleShade(XEvent *, WaAction *);
    void Sticky(XEvent *, WaAction *);
    void UnSticky(XEvent *, WaAction *);
    void ToggleSticky(XEvent *, WaAction *);
    void ViewportMove(XEvent *e, WaAction *);
    void MoveViewportLeft(XEvent *, WaAction *);
    void MoveViewportRight(XEvent *, WaAction *);
    void MoveViewportUp(XEvent *, WaAction *);
    void MoveViewportDown(XEvent *, WaAction *);
    void ScrollViewportLeft(XEvent *, WaAction *);
    void ScrollViewportRight(XEvent *, WaAction *);
    void ScrollViewportUp(XEvent *, WaAction *);
    void ScrollViewportDown(XEvent *, WaAction *);
    void MoveViewportLeftNoWarp(XEvent *, WaAction *);
    void MoveViewportRightNoWarp(XEvent *, WaAction *);
    void MoveViewportUpNoWarp(XEvent *, WaAction *);
    void MoveViewportDownNoWarp(XEvent *, WaAction *);
    void ScrollViewportLeftNoWarp(XEvent *, WaAction *);
    void ScrollViewportRightNoWarp(XEvent *, WaAction *);
    void ScrollViewportUpNoWarp(XEvent *, WaAction *);
    void ScrollViewportDownNoWarp(XEvent *, WaAction *);
    void TaskSwitcher(XEvent *, WaAction *);
    void PreviousTask(XEvent *, WaAction *);
    void NextTask(XEvent *, WaAction *);
    void DecorTitleOn(XEvent *, WaAction *);
    void DecorHandleOn(XEvent *, WaAction *);
    void DecorBorderOn(XEvent *, WaAction *);
    void DecorAllOn(XEvent *, WaAction *);
    void DecorTitleOff(XEvent *, WaAction *);
    void DecorHandleOff(XEvent *, WaAction *);
    void DecorBorderOff(XEvent *, WaAction *);
    void DecorAllOff(XEvent *, WaAction *);
    void DecorTitleToggle(XEvent *, WaAction *);
    void DecorHandleToggle(XEvent *, WaAction *);
    void DecorBorderToggle(XEvent *, WaAction *);
    void AlwaysontopOn(XEvent *, WaAction *);
    void AlwaysatbottomOn(XEvent *, WaAction *);
    void AlwaysontopOff(XEvent *, WaAction *);
    void AlwaysatbottomOff(XEvent *, WaAction *);
    void AlwaysontopToggle(XEvent *, WaAction *);
    void AlwaysatbottomToggle(XEvent *, WaAction *);
    void PointerWarp(XEvent *, WaAction *);
    inline void RaiseFocus(XEvent *e, WaAction *ac) {
        Raise(e, ac);
        Focus(true);
    }
    inline void Nop(XEvent *, WaAction *) {}

    void EvAct(XEvent *, EventDetail *, list<WaAction *> *, int);
    
    char *name;
    bool has_focus, want_focus, mapped, dontsend, deleted;
    Display *display;
    Waimea *waimea;
    WaScreen *wascreen;
    int border_w, title_w, handle_w, screen_number, state, restore_shade;
    WaChildWindow *frame, *title, *label, *handle, *button_c, *button_max,
                   *button_min, *grip_r, *grip_l;
    WaWindowAttributes attrib, old_attrib, restore_max;
    WaWindowFlags flags;
    SizeStruct size;
    NetHandler *net;
    WMstrut *wm_strut;
    list<WaAction *> *frameacts, *awinacts, *pwinacts, *titleacts, *labelacts,
        *handleacts, *cbacts, *ibacts, *mbacts, *lgacts, *rgacts;
    
private:
    void ReparentWin(void);
    void InitPosition(void);
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
    bool o_mapped, move_resize;
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
    
#ifdef SHAPE
    bool shaped;
#endif // SHAPE
};

class WaChildWindow : public WindowObject {
public:
    WaChildWindow(WaWindow *, Window, int);
    virtual ~WaChildWindow(void);
    
    WaWindowAttributes attrib;
    WaWindow *wa;
};

#endif // __WaWindow_hh
