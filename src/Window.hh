/**
 * @file   Window.cc
 * @author David Reveman <david@waimea.org>
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

#ifndef __Window_hh
#define __Window_hh

extern "C" {
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xmd.h>

#ifdef    SHAPE
#  include <X11/extensions/shape.h>
#endif // SHAPE
}

class WaWindow;
class WaChildWindow;

typedef struct _WaAction WaAction;
typedef void (WaWindow::*WwActionFn)(XEvent *, WaAction *);

#include "Event.hh"
#include "Net.hh"

#define DELETED { deleted = true; XUngrabServer(display); return; }
#define WW_DELETED { ww->deleted = true; XUngrabServer(display); return; }

#define MERGED_LOOP \
    list<WaWindow *>::iterator __mw_it = merged.begin(); \
    for (WaWindow *_mw = NULL; _mw != this && \
             (_mw = (__mw_it == merged.end())? this: *__mw_it); __mw_it++)

#define ApplyGravity   1
#define RemoveGravity -1

#define NullMergeType  1
#define CloneMergeType 2
#define HorizMergeType 3
#define VertMergeType  4

typedef struct {
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
    bool focusable;
    bool tasklist;
    bool fullscreen;
    bool hidden;
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
    void Show(void);
    void Hide(void);
    void UpdateTitlebar(void);
    void UpdateAllAttributes(void);
    list <WaAction *> *GetActionList(list<WaActionExtList *> *);
    void SetActionLists(void);
    void RedrawWindow(bool = false);
    void SendConfig(void);
    void Gravitate(int);
    void UpdateGrabs(void);
    void ButtonPressed(WaChildWindow *);
    bool IncSizeCheck(int, int, int *, int *);
    void DrawTitlebar(bool = false);
    void DrawHandlebar(bool = false);
    void FocusWin(void);
    void UnFocusWin(void);
    void Focus(bool);
    void _Maximize(int, int);
    void Merge(WaWindow *, int);
    void Unmerge(WaWindow *);
    bool CheckMoveMerge(int, int, int = 0, int = 0);
    
    void MenuMap(XEvent *, WaAction *, bool);
    void MenuRemap(XEvent *, WaAction *, bool);
    void MenuUnmap(XEvent *, WaAction *, bool);

#ifdef SHAPE
    void Shape(void);
    void ShapeEvent(Window);
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
    void Minimize(XEvent *, WaAction *);
    void UnMinimize(XEvent *, WaAction *);
    void ToggleMinimize(XEvent *, WaAction *);
    void FullscreenOn(XEvent *, WaAction *);
    void FullscreenOff(XEvent *, WaAction *);
    void FullscreenToggle(XEvent *, WaAction *);
    void ViewportMove(XEvent *e, WaAction *);
    void ViewportRelativeMove(XEvent *e, WaAction *);
    void ViewportFixedMove(XEvent *e, WaAction *);
    void MoveViewportLeft(XEvent *, WaAction *);
    void MoveViewportRight(XEvent *, WaAction *);
    void MoveViewportUp(XEvent *, WaAction *);
    void MoveViewportDown(XEvent *, WaAction *);
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
    void AcceptConfigRequestOn(XEvent *, WaAction *);
    void AcceptConfigRequestOff(XEvent *, WaAction *);
    void AcceptConfigRequestToggle(XEvent *, WaAction *);
    void PointerRelativeWarp(XEvent *, WaAction *);
    void PointerFixedWarp(XEvent *, WaAction *);
    inline void RaiseFocus(XEvent *, WaAction *) {
        Raise(NULL, NULL);
        Focus(true);
    }
    void MoveResize(XEvent *, WaAction *);
    void MoveResizeVirtual(XEvent *, WaAction *);
    void MoveWindowToPointer(XEvent *, WaAction *);
    void MoveWindowToSmartPlace(XEvent *, WaAction *);
    inline void moveToSmartPlaceIfUninitialized(XEvent *, WaAction *) {
        if (! pos_init) MoveWindowToSmartPlace(NULL, NULL);
    }
    void GoToDesktop(XEvent *, WaAction *);
    void PreviousDesktop(XEvent *, WaAction *);
    void NextDesktop(XEvent *, WaAction *);
    void DesktopMask(XEvent *, WaAction *);
    void JoinDesktop(XEvent *, WaAction *);
    void PartCurrentJoinDesktop(XEvent *, WaAction *);
    void PartDesktop(XEvent *, WaAction *);
    void PartCurrentDesktop(XEvent *, WaAction *);
    void JoinCurrentDesktop(void);
    void JoinAllDesktops(XEvent *, WaAction *);
    void PartAllDesktopsExceptCurrent(XEvent *, WaAction *);
    void MergeWithWindow(WaAction *, int);
    void CloneMergeWithWindow(XEvent *, WaAction *ac) {
        MergeWithWindow(ac, CloneMergeType);
    }
    void VertMergeWithWindow(XEvent *, WaAction *ac) {
        MergeWithWindow(ac, VertMergeType);
    }
    void HorizMergeWithWindow(XEvent *, WaAction *ac) {
        MergeWithWindow(ac, HorizMergeType);
    }
    void Explode(XEvent *, WaAction *);
    void ToFront(XEvent *, WaAction *);
    void MergeTo(XEvent *, int);
    inline void CloneMergeTo(XEvent *e, WaAction *) {
        MergeTo(e, CloneMergeType);
    }
    inline void VertMergeTo(XEvent *e, WaAction *) {
        MergeTo(e, VertMergeType);
    }
    inline void HorizMergeTo(XEvent *e, WaAction *) {
        MergeTo(e, HorizMergeType);
    }
    inline void UnMergeMaster(XEvent *, WaAction *) {
        if (master) master->Unmerge(this);
    }
    void SetMergeMode(XEvent *, WaAction *);
    void NextMergeMode(XEvent *, WaAction *);
    void PrevMergeMode(XEvent *, WaAction *);
    void Restart(XEvent *, WaAction *);
    void Exit(XEvent *, WaAction *);
    inline void Nop(XEvent *, WaAction *) {}

    void EvAct(XEvent *, EventDetail *, list<WaAction *> *, int);
    
    char *name, *host, *pid;
    int realnamelen;
    bool has_focus, want_focus, mapped, dontsend, deleted, ign_config_req,
                   hidden;
    Display *display;
    Waimea *waimea;
    WaScreen *wascreen;
    int border_w, title_w, handle_w, screen_number, state, restore_shade;
    WaChildWindow *frame, *title, *label, *handle, *grip_r, *grip_l;
    list<WaChildWindow *> buttons;
    WaWindowAttributes attrib, old_attrib, restore_max;
    WaWindowFlags flags;
    SizeStruct size;
    NetHandler *net;
    WMstrut *wm_strut;
    Window transient_for;
    XClassHint *classhint;
    list<Window> transients;
    unsigned int desktop_mask;
    list<WaWindow *> merged;
    list<WaChildWindow *> titles;
    WaWindow *master;
    int mergetype, mergemode;
    bool mergedback;

#ifdef RENDER
    bool render_if_opacity;
#endif // RENDER
    
private:
    void ReparentWin(void);
    void InitPosition(void);
    void CreateOutline(void);
    void DestroyOutline(void);
    void DrawOutline(int, int, int, int);
    void Resize(XEvent *, int);
    void ResizeOpaque(XEvent *, int);
    bool _MoveOpaque(XEvent *, int, int, int, int, list<XEvent *> *);
    
    WaImageControl *ic;
    Window o_west, o_north, o_south, o_east;
    bool move_resize, sendcf, pos_init;
    
#ifdef SHAPE
    bool shaped, been_shaped;
#endif // SHAPE
    
};

class WaChildWindow : public WindowObject {
public:
    WaChildWindow(WaWindow *, Window, int);
    virtual ~WaChildWindow(void);

    void Render(void);
    void Draw(Drawable = 0);
    bool IsDrawable(void);
    
    Display *display;
    WaWindow *wa;
    WaScreen *wascreen;
    WaImageControl *ic;
    WaWindowAttributes attrib;
    WaTexture *f_texture, *u_texture;
    bool pressed;
    ButtonStyle *bstyle;
    int g_x, g_x2;

#ifdef XFT
    XftDraw *xftdraw;
#endif // XFT
    
};

#endif // __Window_hh
