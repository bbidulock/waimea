/** -*- Mode: C++ -*-
 *
 * @file   WaScreen.hh
 * @author David Reveman <c99drn@cs.umu.se>
 * @date   25-Jul-2001 23:25:51
 *
 * @brief Definition of WaScreen and ScreenEdge classes
 *
 * Function declarations and variable definitions for WaScreen and
 * ScreeeEdge classes.
 *
 * Copyright (C) David Reveman. All rights reserved.
 *
 */

#ifndef __WaScreen_hh
#define __WaScreen_hh

#include <X11/Xlib.h>

#ifdef XFT
#include <X11/Xft/Xft.h>
#endif // XFT

#ifdef SHAPE
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>
#endif //SHAPE

typedef struct {
    Window window;
    int left;
    int right;
    int top;
    int bottom;
} WMstrut;

typedef struct {
    int x;
    int y;
    int width;
    int height;
} Workarea;

class WaScreen;
class ScreenEdge;

typedef struct _WaAction WaAction;
typedef void (WaScreen::*RootActionFn)(XEvent *, WaAction *);

#include "WaImage.hh"
#include "Waimea.hh"
#include "WaMenu.hh"
#include "ResourceHandler.hh"
#include "NetHandler.hh"
#include "DockappHandler.hh"

#define WestDirection  1
#define EastDirection  2
#define NorthDirection 3
#define SouthDirection 4

typedef struct {
    WaColor f_focus, f_unfocus, l_text_focus, l_text_unfocus, b_pic_focus,
        b_pic_unfocus, b_pic_hilite, border_color;
    WaTexture t_focus, t_unfocus, l_focus, l_unfocus, h_focus, h_unfocus,
        b_focus, b_unfocus, b_pressed, g_focus, g_unfocus;
    GC b_pic_focus_gc, b_pic_unfocus_gc, b_pic_hilite_gc;

    char *fontname;
#ifdef XFT
    char *xftfontname;
    XftFont *xftfont;
    XftColor xftfcolor, xftucolor;
    double xftsize;
#else // ! XFT
    GC l_text_focus_gc, l_text_unfocus_gc;
    XFontStruct *font;
#endif // XFT
    int justify, y_pos;
    int handle_width, border_width, title_height;
} WindowStyle;

typedef struct {
    WaColor f_text, f_hilite_text, t_text, border_color;
    WaTexture back_frame, frame, title, hilite;

    char *f_fontname, *t_fontname, *b_fontname, *bullet;
#ifdef XFT
    char *f_xftfontname, *t_xftfontname, *b_xftfontname;
    XftFont *f_xftfont, *t_xftfont, *b_xftfont;
    XftColor f_xftcolor, fh_xftcolor, t_xftcolor;
    double f_xftsize, t_xftsize, b_xftsize;
#else // ! XFT
    GC f_text_gc, fh_text_gc, t_text_gc, b_text_gc, bh_text_gc;
    XFontStruct *f_font, *t_font, *b_font;
#endif // XFT
    int f_justify, t_justify, f_y_pos, t_y_pos, b_y_pos;
    int border_width, title_height, item_height;
} MenuStyle;

class WaScreen : public WindowObject {
public:
    WaScreen(Display *, int, Waimea *);
    virtual ~WaScreen(void);

    void MoveViewportTo(int, int);
    void MoveViewport(int, bool);
    void ScrollViewport(int, bool, WaAction *);
    void MenuMap(XEvent *, WaAction *, bool);
    void MenuRemap(XEvent *, WaAction *, bool);
    void MenuUnmap(XEvent *, WaAction *, bool);
    void UpdateWorkarea(void);
    void AddDockapp(Window window);
    
    inline void MenuMap(XEvent *e, WaAction *ac) {
        MenuMap(e, ac, False);
    }
    inline void MenuMapFocused(XEvent *e, WaAction *ac) {
        MenuMap(e, ac, True);
    }
    inline void MenuRemap(XEvent *e, WaAction *ac) {
        MenuRemap(e, ac, False);
    }
    inline void MenuRemapFocused(XEvent *e, WaAction *ac) {
        MenuRemap(e, ac, True);
    }
    void ViewportMove(XEvent *, WaAction *);
    void Focus(XEvent *, WaAction *);
    inline void MenuUnmap(XEvent *e, WaAction *wa) {
        MenuUnmap(e, wa, False);
    }
    inline void MenuUnmapFocus(XEvent *e, WaAction *wa) {
        MenuUnmap(e, wa, True);
    }
    void Restart(XEvent *, WaAction *);
    void Exit(XEvent *, WaAction *);
    void TaskSwitcher(XEvent *, WaAction *);
    inline void MoveViewportLeft(XEvent *, WaAction *) {
        MoveViewport(WestDirection, True);
    }
    inline void MoveViewportRight(XEvent *, WaAction *) {
        MoveViewport(EastDirection, True);
    }
    inline void MoveViewportUp(XEvent *, WaAction *) {
        MoveViewport(NorthDirection, True);
    }
    inline void MoveViewportDown(XEvent *, WaAction *) {
        MoveViewport(SouthDirection, True);
    }
    inline void ScrollViewportLeft(XEvent *, WaAction *ac) {
        ScrollViewport(WestDirection, True, ac);
    }
    inline void ScrollViewportRight(XEvent *, WaAction *ac) {
        ScrollViewport(EastDirection, True, ac);
    }
    inline void ScrollViewportUp(XEvent *, WaAction *ac) {
        ScrollViewport(NorthDirection, True, ac);
    }
    inline void ScrollViewportDown(XEvent *, WaAction *ac) {
        ScrollViewport(SouthDirection, True, ac);
    }
    inline void MoveViewportLeftNoWarp(XEvent *, WaAction *) {
        MoveViewport(WestDirection, False);
    }
    inline void MoveViewportRightNoWarp(XEvent *, WaAction *) {
        MoveViewport(EastDirection, False);
    }
    inline void MoveViewportUpNoWarp(XEvent *, WaAction *) {
        MoveViewport(NorthDirection, False);
    }
    inline void MoveViewportDownNoWarp(XEvent *, WaAction *) {
        MoveViewport(SouthDirection, False);
    }
    inline void ScrollViewportLeftNoWarp(XEvent *, WaAction *ac) {
        ScrollViewport(WestDirection, False, ac);
    }
    inline void ScrollViewportRightNoWarp(XEvent *, WaAction *ac) {
        ScrollViewport(EastDirection, False, ac);
    }
    inline void ScrollViewportUpNoWarp(XEvent *, WaAction *ac) {
        ScrollViewport(NorthDirection, False, ac);
    }
    inline void ScrollViewportDownNoWarp(XEvent *, WaAction *ac) {
        ScrollViewport(SouthDirection, False, ac);
    }
    void PreviousTask(XEvent *, WaAction *);
    void NextTask(XEvent *, WaAction *);
    
    void EvAct(XEvent *, EventDetail *, list<WaAction *> *);

    Display *display;
    int screen_number, screen_depth, width, height, v_x, v_y, v_xmax, v_ymax;
    Colormap colormap;
    Visual *visual;
    Waimea *waimea;
    NetHandler *net;
    ResourceHandler *rh;
    WaImageControl *ic;
    DockappHandler *dock;
    WindowStyle wstyle;
    MenuStyle mstyle;
    Pixmap fbutton, ubutton, pbutton, fgrip, ugrip;
    unsigned long fbutton_pixel, ubutton_pixel, pbutton_pixel, fgrip_pixel,
        ugrip_pixel;
    char displaystring[1024];
    ScreenEdge *west, *east, *north, *south;
    list<WMstrut *> *strut_list;
    Workarea *workarea;

#ifdef SHAPE
    int shape, shape_event;
#endif // SHAPE

private:
    void CreateVerticalEdges(void);
    void CreateHorizontalEdges(void);
    void CreateColors(void);
    void CreateFonts(void);
    void RenderCommonImages(void);

#ifdef XFT
    void CreateXftColor(WaColor *, XftColor *);
#endif // XFT
};

class ScreenEdge : public WindowObject {
public:
    ScreenEdge(WaScreen *, int, int, int, int, int);
    virtual ~ScreenEdge(void);
    
    WaScreen *wa;
};

#endif // __WaScreen_hh
