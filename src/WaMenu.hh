/** -*- Mode: C++ -*-
 *
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

class WaMenu {
public:
    WaMenu(char *);
    virtual ~WaMenu(void);

    void AddItem(WaMenuItem *);
    void RemoveItem(WaMenuItem *);
    void Build(WaScreen *);
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
    
    list<WaMenuItem *> *item_list;    

    Window frame, o_west, o_north, o_south, o_east;
    int x, y, width, height, bullet_width;
    bool mapped, built, o_mapped;
    char *name;
    Pixmap pbackframe, pframe, ptitle, philite, psub, psubhilite;
    unsigned long backframe_pixel, frame_pixel, title_pixel, hilite_pixel,
        sub_pixel, subhilite_pixel;
    WaMenu *root_menu;
    WaMenuItem *root_item;
    bool tasksw, focus;

    int ftype; 
    Window wf;
    WaScreen *rf;
    WaMenuItem *mf;
  
private:
    WaImageControl *ic;
    int f_height, t_height, s_height;
};

class WaMenuItem : public WindowObject {
public:
    WaMenuItem(char *);
    virtual ~WaMenuItem(void);

    void DrawFg(void);
    
    void Hilite(void);
    void DeHilite(void);
    void Focus(void);
    void MapSubmenu(XEvent *, WaAction *, bool);
    void RemapSubmenu(XEvent *, WaAction *, bool);
    void UnmapMenu(XEvent *, WaAction *, bool);
    
    void UnLinkMenu(XEvent *, WaAction *);
    inline void MapSubmenu(XEvent *e, WaAction *ac) {
        MapSubmenu(e, ac, False);
    }
    inline void MapSubmenuFocused(XEvent *e, WaAction *ac) {
        MapSubmenu(e, ac, True);
    }
    inline void RemapSubmenu(XEvent *e, WaAction *ac) {
        RemapSubmenu(e, ac, False);
    }
    inline void RemapSubmenuFocused(XEvent *e, WaAction *ac) {
        RemapSubmenu(e, ac, True);
    }
    inline void UnmapMenu(XEvent *e, WaAction *wa) {
        UnmapMenu(e, wa, False);
    }
    inline void UnmapMenuFocus(XEvent *e, WaAction *wa) {
        UnmapMenu(e, wa, True);
    }
    void Exec(XEvent *, WaAction *);
    void Func(XEvent *, WaAction *);
    void Move(XEvent *, WaAction *);
    void MoveOpaque(XEvent *, WaAction *);
    void Lower(XEvent *, WaAction *);
    inline void Focus(XEvent *, WaAction *) { Focus(); }
    inline void UnmapSubmenus(XEvent *, WaAction *) { menu->UnmapSubmenus(False); }
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
    
    void EvAct(XEvent *, EventDetail *, list<WaAction *> *);
    
    
    int func_mask, height, width, dy, realheight;
    bool hilited, focus;
    char *label, *exec, *sub;
    WwActionFn wfunc;
    MenuActionFn mfunc;
    RootActionFn rfunc;
    WaMenu *menu;
    WaMenu *submenu;
    Window wf;
#ifdef XFT        
    XftDraw *xftdraw;
#endif // XFT
};


class TaskSwitcher : public WaMenu {
public:
    TaskSwitcher(void);

    void Build(WaScreen *);
    
private:
    list<WaWindow *> *wawindow_list;
};

#endif // __WaMenu_hh
