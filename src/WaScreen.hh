/**
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

#ifdef    SHAPE
#  include <X11/Xutil.h>
#  include <X11/extensions/shape.h>
#endif // SHAPE

#ifdef    XRENDER
#  include <X11/extensions/Xrender.h>
#endif // XRENDER

#ifdef    XFT
#  include <X11/Xft/Xft.h>
#endif // XFT

class WaScreen;
class ScreenEdge;

typedef struct _WaAction WaAction;
typedef void (WaScreen::*RootActionFn)(XEvent *, WaAction *);

typedef struct {
    Window window;
    int left;
    int right;
    int top;
    int bottom;
} WMstrut;

typedef struct {
    bool xft;
    char *font;
} WaFont;

#include "WaImage.hh"
#include "EventHandler.hh"
#include "NetHandler.hh"
#include "DockappHandler.hh"

typedef struct {
    int x;
    int y;
    int width;
    int height;
} Workarea;

#define WestDirection  1
#define EastDirection  2
#define NorthDirection 3
#define SouthDirection 4

typedef struct {
    WaColor l_text_focus, l_text_unfocus, border_color, outline_color;
    WaTexture t_focus, t_unfocus, l_focus, l_unfocus, h_focus, h_unfocus,
        g_focus, g_unfocus;
    WaFont wa_font;
    
#ifdef XFT
    XftFont *xftfont;
    XftColor *xftfcolor, *xftucolor;
    double xftsize;
#endif // XFT
    
    GC l_text_focus_gc, l_text_unfocus_gc;
    XFontStruct *font;
    bool font_ok;
    
    int justify, y_pos;
    unsigned int handle_width, border_width, title_height;

    list<ButtonStyle *> *buttonstyles;
    int b_num;
} WindowStyle;

typedef struct {
    WaColor f_text, f_hilite_text, t_text, border_color;
    WaTexture back_frame, title, hilite;
    WaFont wa_f_font, wa_t_font, wa_b_font, wa_ct_font, wa_cf_font;
    char *bullet, *checkbox_true, *checkbox_false;
    
#ifdef XFT
    XftFont *f_xftfont, *t_xftfont, *b_xftfont, *ct_xftfont, *cf_xftfont;
    XftColor *f_xftcolor, *fh_xftcolor, *t_xftcolor;
#endif // XFT
    
    GC f_text_gc, fh_text_gc, t_text_gc, b_text_gc, bh_text_gc,
        ct_text_gc, cth_text_gc, cf_text_gc, cfh_text_gc;
    XFontStruct *f_font, *t_font, *b_font, *ct_font, *cf_font;
    bool f_font_ok, t_font_ok, b_font_ok, ct_font_ok, cf_font_ok;
    
    int f_justify, t_justify, f_y_pos, t_y_pos, b_y_pos, ct_y_pos, cf_y_pos;
    unsigned int border_width, title_height, item_height;
} MenuStyle;

class WaScreen : public WindowObject {
public:
    WaScreen(Display *, int, Waimea *);
    virtual ~WaScreen(void);

    void MoveViewportTo(int, int);
    void MoveViewport(int);
    void ScrollViewport(int, bool, WaAction *);
    void MenuMap(XEvent *, WaAction *, bool);
    void MenuRemap(XEvent *, WaAction *, bool);
    void MenuUnmap(XEvent *, WaAction *, bool);
    void UpdateWorkarea(void);
    void AddDockapp(Window window);
    
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
    void ViewportMove(XEvent *, WaAction *);
    void EndMoveResize(XEvent *, WaAction *);
    void Focus(XEvent *, WaAction *);
    inline void MenuUnmap(XEvent *e, WaAction *wa) {
        MenuUnmap(e, wa, false);
    }
    inline void MenuUnmapFocus(XEvent *e, WaAction *wa) {
        MenuUnmap(e, wa, true);
    }
    void Restart(XEvent *, WaAction *);
    void Exit(XEvent *, WaAction *);
    void TaskSwitcher(XEvent *, WaAction *);
    void PreviousTask(XEvent *, WaAction *);
    void NextTask(XEvent *, WaAction *);
    void PointerRelativeWarp(XEvent *, WaAction *);
    void PointerFixedWarp(XEvent *, WaAction *);
    void ViewportRelativeMove(XEvent *, WaAction *);
    void ViewportFixedMove(XEvent *, WaAction *);

    inline void MoveViewportLeft(XEvent *, WaAction *) {
        MoveViewport(WestDirection);
    }
    inline void MoveViewportRight(XEvent *, WaAction *) {
        MoveViewport(EastDirection);
    }
    inline void MoveViewportUp(XEvent *, WaAction *) {
        MoveViewport(NorthDirection);
    }
    inline void MoveViewportDown(XEvent *, WaAction *) {
        MoveViewport(SouthDirection);
    }
    inline void Nop(XEvent *, WaAction *) {}
    
    void EvAct(XEvent *, EventDetail *, list<WaAction *> *);

    Display *display;
    int screen_number, screen_depth, width, height, v_x, v_y, v_xmax, v_ymax;
    Colormap colormap;
    Visual *visual;
    Waimea *waimea;
    NetHandler *net;
    ResourceHandler *rh;
    WaImageControl *ic;
    WindowStyle wstyle;
    MenuStyle mstyle;
    WaFont default_font;
    XFontStruct *def_font;

    Pixmap fgrip, ugrip;
    
#ifdef XRENDER
    Pixmap xrootpmap_id;
#endif // XRENDER

    unsigned long fbutton_pixel, ubutton_pixel, pbutton_pixel, fgrip_pixel,
        ugrip_pixel;
    char displaystring[1024];
    ScreenEdge *west, *east, *north, *south;
    Workarea *workarea;
    Window wm_check;
    bool focus;
    
    list<WMstrut *> *strut_list;
    list<DockappHandler *> *docks;

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

    int move;
};

class ScreenEdge : public WindowObject {
public:
    ScreenEdge(WaScreen *, int, int, int, int, int);
    virtual ~ScreenEdge(void);
    
    WaScreen *wa;
};

#endif // __WaScreen_hh
