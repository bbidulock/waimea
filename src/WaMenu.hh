/**
 * @file   WaMenu.hh
 * @author David Reveman <c99drn@cs.umu.se>
 * @date   02-Aug-2001 22:40:01
 *
 * @brief Definition of WaMenu, WaMenuItem and TaskSwitcher classes
 *
 * Function declarations and variable definitions for WaMenu, WaMenuItem
 * and TaskSwitcher classes.
 *
 * Copyright (C) David Reveman. All rights reserved.
 *
 */

#ifndef __WaMenu_hh
#define __WaMenu_hh

#include <X11/Xlib.h>

class WaMenu;
class WaMenuItem;
class TaskSwitcher;

typedef struct _WaAction WaAction;
typedef void (WaMenuItem::*MenuActionFn)(XEvent *, WaAction *);

#include "Waimea.hh"
#include "WaWindow.hh"
#include "WaScreen.hh"
#include "WaImage.hh"
#include "ResourceHandler.hh"

#define MenuExecMask  (1L << 0)
#define MenuSubMask   (1L << 1)
#define MenuWFuncMask (1L << 2)
#define MenuRFuncMask (1L << 3)
#define MenuMFuncMask (1L << 4)

#define MaxCBoxType    1
#define ShadeCBoxType  2
#define StickCBoxType  3
#define TitleCBoxType  4
#define HandleCBoxType 5
#define BorderCBoxType 6
#define AllCBoxType    7
#define AOTCBoxType    8
#define AABCBoxType    9

class WaMenu {
public:
    WaMenu(char *);
    virtual ~WaMenu(void);

    void AddItem(WaMenuItem *);
    void Build(WaScreen *);
    
#ifdef XRENDER
    void Render(void);
#endif // XRENDER

    void Map(int, int);
    void ReMap(int, int);
    void Move(int, int);
    void Unmap(bool);
    void UnmapSubmenus(bool);
    void UnmapTree(void);
    void CreateOutlineWindows(void);
    void ToggleOutline(void);
    void DrawOutline(int, int);
    void Raise(void);
    void FocusFirst(void);
    
    Waimea *waimea;
    Display *display;
    WaScreen *wascreen;
    WaImageControl *ic;
    
    list<WaMenuItem *> *item_list;    

    Window frame, o_west, o_north, o_south, o_east;
    int x, y, width, height, bullet_width, cb_width, extra_width;
    bool mapped, built, o_mapped, has_focus, tasksw;
    char *name;
    Pixmap pbackframe, ptitle, philite, psub, psubhilite;
    unsigned long backframe_pixel, title_pixel, hilite_pixel,
        sub_pixel, subhilite_pixel;
    WaMenu *root_menu;
    WaMenuItem *root_item;

    int ftype; 
    Window wf;
    WaScreen *rf;
    WaMenuItem *mf;
    
#ifdef XRENDER
    Pixmap pixmap;
#endif // XRENDER
  
private:
    int f_height, t_height, s_height;
};

class WaMenuItem : public WindowObject {
public:
    WaMenuItem(char *);
    virtual ~WaMenuItem(void);

    void DrawFg(void);
    void Render(void);
    
    void Hilite(void);
    void DeHilite(void);
    void Focus(void);
    void MapSubmenu(XEvent *, WaAction *, bool);
    void RemapSubmenu(XEvent *, WaAction *, bool);
    void UnmapMenu(XEvent *, WaAction *, bool);
    
    void UnLinkMenu(XEvent *, WaAction *);
    inline void MapSubmenu(XEvent *e, WaAction *ac) {
        MapSubmenu(e, ac, false);
    }
    inline void MapSubmenuFocused(XEvent *e, WaAction *ac) {
        MapSubmenu(e, ac, true);
    }
    inline void RemapSubmenu(XEvent *e, WaAction *ac) {
        RemapSubmenu(e, ac, false);
    }
    inline void RemapSubmenuFocused(XEvent *e, WaAction *ac) {
        RemapSubmenu(e, ac, true);
    }
    inline void UnmapMenu(XEvent *e, WaAction *wa) {
        UnmapMenu(e, wa, false);
    }
    inline void UnmapMenuFocus(XEvent *e, WaAction *wa) {
        UnmapMenu(e, wa, true);
    }
    void Exec(XEvent *, WaAction *);
    void Func(XEvent *, WaAction *);
    void Move(XEvent *, WaAction *);
    void MoveOpaque(XEvent *, WaAction *);
    void EndMoveResize(XEvent *, WaAction *);
    void Lower(XEvent *, WaAction *);
    inline void Focus(XEvent *, WaAction *) { Focus(); }
    inline void UnmapSubmenus(XEvent *, WaAction *) {
        menu->UnmapSubmenus(false); }
    inline void UnmapTree(XEvent *, WaAction *) { menu->UnmapTree(); }
    inline void Raise(XEvent *, WaAction *) { menu->Raise(); }
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
    void NextItem(XEvent *, WaAction *);
    void PreviousItem(XEvent *, WaAction *);
    void PointerWarp(XEvent *, WaAction *);
    void Restart(XEvent *, WaAction *);
    void Exit(XEvent *, WaAction *);
    inline void Nop(XEvent *, WaAction *) {}
    
    void EvAct(XEvent *, EventDetail *, list<WaAction *> *);
    void UpdateCBox(void);
    
    int func_mask, func_mask1, func_mask2, height, width, dy, realheight,
                   cb, cb_y, cb_width, cb_width1, cb_width2;
    bool hilited, move_resize;
    char *label, *exec, *param, *sub;
    char *label1, *exec1, *param1, *sub1;
    char *label2, *exec2, *param2, *sub2;
    char *cbox;
    WwActionFn wfunc, wfunc1, wfunc2;
    MenuActionFn mfunc, mfunc1, mfunc2;
    RootActionFn rfunc, rfunc1, rfunc2;
    WaMenu *menu;
    WaMenu *submenu, *submenu1, *submenu2;
    Window wf;
    WaTexture *texture;
    
#ifdef XFT        
    XftDraw *xftdraw;
    XftFont *cbox_xft_font;
#else // !XFT
    GC *cbox_gc;
#endif // XFT

#ifdef XRENDER
        Pixmap pixmap;
#endif // XRENDER

};


class TaskSwitcher : public WaMenu {
public:
    TaskSwitcher(void);

    void Build(WaScreen *);
    
private:
    list<WaWindow *> *wawindow_list;
};

#endif // __WaMenu_hh
