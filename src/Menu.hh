/**
 * @file   Menu.hh
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

#ifndef __Menu_hh
#define __Menu_hh

extern "C" {
#include <X11/Xlib.h>
}

class WaMenu;
class WaMenuItem;
class WindowMenu;

typedef struct _WaAction WaAction;
typedef void (WaMenuItem::*MenuActionFn)(XEvent *, WaAction *);

#include "Screen.hh"

#define MenuExecMask  (1L << 0)
#define MenuSubMask   (1L << 1)
#define MenuWFuncMask (1L << 2)
#define MenuRFuncMask (1L << 3)
#define MenuMFuncMask (1L << 4)

class WaMenu : public WindowObject {
public:
    WaMenu(char *);
    virtual ~WaMenu(void);

    void AddItem(WaMenuItem *);
    void Build(WaScreen *);
    void Render(void);
    void Map(int, int);
    void ReMap(int, int);
    void Move(int, int, bool = true);
    void Unmap(bool);
    void UnmapSubmenus(bool);
    void UnmapTree(void);
    void CreateOutline(void);
    void DestroyOutline(void);
    void DrawOutline(int, int);
    void Raise(void);
    void FocusFirst(void);
    
    Waimea *waimea;
    Display *display;
    WaScreen *wascreen;
    WaImageControl *ic;
    
    list<WaMenuItem *> item_list;    

    Window frame, o_west, o_north, o_south, o_east;
    int x, y, width, height, bullet_width, cb_width, extra_width;
    bool mapped, built, has_focus, tasksw, dynamic, dynamic_root, ignore, db,
        cb_db_upd;
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
    
#ifdef RENDER
    Pixmap pixmap;
    bool render_if_opacity;
#endif // RENDER
  
private:
    int f_height, t_height, s_height;
};

class WaMenuItem : public WindowObject {
public:
    WaMenuItem(char *);
    virtual ~WaMenuItem(void);

    void Draw(Drawable = 0, bool = false, int = 0);
    void Render(void);
    
    void Hilite(void);
    void DeHilite(void);
    void Focus(void);
    void MapSubmenu(XEvent *, WaAction *, bool, bool = false);
    void RemapSubmenu(XEvent *, WaAction *, bool);
    void UnmapMenu(XEvent *, WaAction *, bool);
    
    void UnLinkMenu(XEvent *, WaAction *);
    inline void MapSubmenu(XEvent *e, WaAction *ac) {
        MapSubmenu(e, ac, false);
    }
    inline void MapSubmenuOnly(XEvent *e, WaAction *ac) {
        MapSubmenu(e, ac, false, true);
    }
    inline void MapSubmenuFocused(XEvent *e, WaAction *ac) {
        MapSubmenu(e, ac, true);
    }
    inline void MapSubmenuFocusedOnly(XEvent *e, WaAction *ac) {
        MapSubmenu(e, ac, true, true);
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
    inline void Focus(XEvent *, WaAction *) {
        if (! in_window) return;
        Focus();
    }
    inline void UnmapSubmenus(XEvent *, WaAction *) {
        if (! in_window) return;
        menu->UnmapSubmenus(false);
    }
    inline void UnmapTree(XEvent *, WaAction *) {
        if (! in_window) return;
        menu->UnmapTree();
    }
    inline void Raise(XEvent *, WaAction *) {
        if (! in_window) return;
        menu->Raise();
    }
    void ViewportMove(XEvent *, WaAction *);
    void ViewportRelativeMove(XEvent *, WaAction *);
    void ViewportFixedMove(XEvent *, WaAction *);
    void MoveViewportLeft(XEvent *, WaAction *);
    void MoveViewportRight(XEvent *, WaAction *);
    void MoveViewportUp(XEvent *, WaAction *);
    void MoveViewportDown(XEvent *, WaAction *);
    void TaskSwitcher(XEvent *, WaAction *);
    void PreviousTask(XEvent *, WaAction *);
    void NextTask(XEvent *, WaAction *);
    void NextItem(XEvent *, WaAction *);
    void PreviousItem(XEvent *, WaAction *);
    void PointerRelativeWarp(XEvent *, WaAction *);
    void PointerFixedWarp(XEvent *, WaAction *);
    void MenuMap(XEvent *, WaAction *);
    void MenuMapFocused(XEvent *, WaAction *);
    void MenuRemap(XEvent *, WaAction *);
    void MenuRemapFocused(XEvent *, WaAction *);
    void MenuUnmap(XEvent *, WaAction *);
    void MenuUnmapFocus(XEvent *, WaAction *);
    void GoToDesktop(XEvent *, WaAction *);
    void PreviousDesktop(XEvent *, WaAction *);
    void NextDesktop(XEvent *, WaAction *);
    void Restart(XEvent *, WaAction *);
    void Exit(XEvent *, WaAction *);
    inline void Nop(XEvent *, WaAction *) {}
    
    void EvAct(XEvent *, EventDetail *, list<WaAction *> *);
    void UpdateCBox(void);
    int ExpandAll(WaWindow *);
    
    int func_mask, func_mask1, func_mask2, height, width, dy, realheight,
                           cb, cb_y, cb_width, cb_width1, cb_width2;
    bool hilited, move_resize, in_window, sdyn, sdyn1, sdyn2, db;
    char *label, *exec, *param, *sub;
    char *label1, *exec1, *param1, *sub1;
    char *label2, *exec2, *param2, *sub2;
    char *e_label, *e_label1, *e_label2, *e_sub, *e_sub1, *e_sub2;
    char *cbox;
    WwActionFn wfunc, wfunc1, wfunc2;
    MenuActionFn mfunc, mfunc1, mfunc2;
    RootActionFn rfunc, rfunc1, rfunc2;
    WaMenu *menu;
    WaMenu *submenu, *submenu1, *submenu2;
    Window wf;
    WaTexture *texture;
    WaFont *wafont_cb;
    
#ifdef XFT        
    XftDraw *xftdraw;
#endif // XFT
    
#ifdef RENDER
    Pixmap pixmap;
#endif // RENDER

};

class WindowMenu : public WaMenu {
public:
    WindowMenu(void);

    void Build(WaScreen *);
    
private:
    list<WaWindow *> *wawindow_list;
};

#endif // __Menu_hh
