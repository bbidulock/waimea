/** -*- Mode: C++ -*-
 *
 * @file   WaMenu.hh
 * @author David Reveman <c99drn@cs.umu.se>
 * @date   02-Aug-2001 22:40:01
 *
 * @brief Definition of WaMenu and WaMenuItem classes
 *
 * Function declarations and variable definitions for WaMenu and WaMenuItem
 * classes.
 *
 * Copyright (C) David Reveman. All rights reserved.
 *
 */

#ifndef __WaMenu_hh
#define __WaMenu_hh

#include <X11/Xlib.h>

class WaMenu;
class WaMenuItem;

typedef void (WaMenuItem::*MenuActionFn)(XEvent *);

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
    void Unmap(void);
    void UnmapSubmenus(void);
    void UnmapTree(void);
    void CreateOutlineWindows(void);
    void ToggleOutline(void);
    void DrawOutline(int, int);
    void Raise(void);
    
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
    
    void Hilite();
    void DeHilite();
    
    void UnLinkMenu(XEvent *);
    void MapSubmenu(XEvent *);
    void ReMapSubmenu(XEvent *);
    void UnmapMenu(XEvent *);
    void Exec(XEvent *);
    void Func(XEvent *);
    void Move(XEvent *);
    void MoveOpaque(XEvent *);
    void Lower(XEvent *);
    inline void UnmapSubmenus(XEvent *e) { menu->UnmapSubmenus(); }
    inline void UnmapTree(XEvent *e) { menu->UnmapTree(); }
    inline void Raise(XEvent *e) { menu->Raise(); }
    void EvAct(XEvent *, EventDetail *, list<WaAction *> *);
    
    
    int func_mask, height, width, dy, realheight;
    bool hilited;
    char *label, *exec, *sub;
    WwActionFn wfunc;
    MenuActionFn mfunc;
    RootActionFn rfunc;
    WaMenu *menu;
    WaMenu *submenu;
    WaMenu *map_menu;
#ifdef XFT        
    XftDraw *xftdraw;
#endif // XFT
};

#endif // __WaMenu_hh
