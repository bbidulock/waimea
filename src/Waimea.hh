/** -*- Mode: C++ -*-
 *
 * @file   Waimea.hh
 * @author David Reveman <c99drn@cs.umu.se>
 * @date   02-May-2001 00:49:45
 *
 * @brief Definition of Waimea and WindowObject classes
 *
 * Function declarations and variable definitions for Waimea class.
 *
 * Copyright (C) David Reveman.  All rights reserved.
 *
 */

#ifndef __Waimea_hh
#define __Waimea_hh

#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <hash_map>
#include <list>

struct waoptions {
    char *display;
    char *rcfile;
    char *stylefile;
    char *actionfile;
    char *menufile;
};

class Waimea;

class WindowObject {
public:
    WindowObject(Window, int);
    
    Window id;
    int type;
};

#include "EventHandler.hh"
#include "NetHandler.hh"
#include "ResourceHandler.hh"
#include "WaWindow.hh"
#include "WaScreen.hh"

#define WARNING cerr << "Warning: " << __FUNCTION__ << ": "
#define ERROR cerr << "Error: " << __FUNCTION__ << ": "
			
#define LISTCLEAR(list) \
    while (! list->empty()) { \
        delete list->back(); \
        list->pop_back(); \
    } \
    delete list;

#define LISTCLEAR2(list) \
    while (! list->empty()) { \
        delete list->back(); \
    } \
    delete list;

#define LISTDEL(list) \
    while (! list->empty()) { \
        list->pop_back(); \
    } \
    delete list;

#define HASHDEL(hash) \
    while (! hash->empty()) { \
        hash->erase(hash->begin()); \
    } \
    delete hash;

enum {
    FrameType,
    WindowType,
    TitleType,
    LabelType,
    CButtonType,
    IButtonType,
    MButtonType,
    HandleType,
    LGripType,
    RGripType,
    RootType,
    WEdgeType,
    EEdgeType,
    NEdgeType,
    SEdgeType,
    MenuTitleType,
    MenuItemType,
    MenuCBItemType,
    MenuSubType,
    DockAppType
};

class Waimea {
public:
    Waimea(char **, struct waoptions *);
    virtual ~Waimea(void);

    void WaRaiseWindow(Window);
    void WaLowerWindow(Window);
    void UpdateCheckboxes(int);
    WaMenu *GetMenuNamed(char *);
    
    Display *display;
    WaScreen *wascreen;
    ResourceHandler *rh;
    EventHandler *eh;
    NetHandler *net;
    TaskSwitcher *taskswitch;
    Cursor session_cursor, move_cursor, resizeleft_cursor, resizeright_cursor;
    
    hash_map<long unsigned int, WindowObject *> *window_table;
    list<Window> *always_on_top_list;
    list<Window> *always_at_bottom_list;
    list<WaWindow *> *wawindow_list;
    list<WaWindow *> *wawindow_list_map_order;
    list<WaWindow *> *wawindow_list_stacking;
    list<WaWindow *> *wawindow_list_stacking_aot;
    list<WaWindow *> *wawindow_list_stacking_aab;
    list<WaMenu *> *wamenu_list;
};

Bool validateclient(Window); 
void wawarning(char *, ...);
void waerror(char *, ...);
void waexec(const char *, char *);
int xerrorhandler(Display *, XErrorEvent *);
int wmrunningerror(Display *, XErrorEvent *);
void signalhandler(int);
char *wastrdup(char *);
void restart(char *);
void quit(int);
char **commandline_to_argv(char *, char **);

#endif // __Waimea_hh
