/**
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

#ifdef    HAVE_LIST
#  include <list>
#endif // HAVE_LIST

using std::list;

#ifdef    HAVE_MAP
#  include <map>
#endif // HAVE_MAP

using std::map;
using std::make_pair;

class Waimea;

class WindowObject {
public:
    inline WindowObject(Window win_id, int win_type) {
        id = win_id;
        type = win_type;
    }

    Window id;
    int type;
};

typedef struct _WaAction WaAction;

#define EastType  1
#define WestType -1

#include "WaMenu.hh"
#include "Timer.hh"

struct waoptions {
    char *display;
    char *rcfile;
    char *stylefile;
    char *actionfile;
    char *menufile;
};

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

#define LISTCLEAR3(list) \
    while (! list.empty()) { \
        delete list.back(); \
        list.pop_back(); \
    }

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
    ButtonType,
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
    DockHandlerType,
    DockAppType
};

enum {
    MoveType,
    MoveOpaqueType,
    ResizeType,
    ResizeOpaqueType,
    EndMoveResizeType
};

#define MaxCBoxType    1
#define ShadeCBoxType  2
#define StickCBoxType  3
#define TitleCBoxType  4
#define HandleCBoxType 5
#define BorderCBoxType 6
#define AllCBoxType    7
#define AOTCBoxType    8
#define AABCBoxType    9

#define CloseCBoxType  10

class Waimea {
public:
    Waimea(char **, struct waoptions *);
    virtual ~Waimea(void);

    void WaRaiseWindow(Window);
    void WaLowerWindow(Window);
    void UpdateCheckboxes(int);
    WaMenu *GetMenuNamed(char *);
    WaMenu *CreateDynamicMenu(char *);
    
    Display *display;
    WaScreen *wascreen;
    ResourceHandler *rh;
    EventHandler *eh;
    NetHandler *net;
    TaskSwitcher *taskswitch;
    Timer *timer;
    Cursor session_cursor, move_cursor, resizeleft_cursor, resizeright_cursor;
    
    map<long unsigned int, WindowObject *> *window_table;
    list<Window> *always_on_top_list;
    list<Window> *always_at_bottom_list;
    list<WaWindow *> *wawindow_list;
    list<WaWindow *> *wawindow_list_map_order;
    list<WaWindow *> *wawindow_list_stacking;
    list<WaWindow *> *wawindow_list_stacking_aot;
    list<WaWindow *> *wawindow_list_stacking_aab;
    list<WaMenu *> *wamenu_list;
    list<WaMenu *> *wamenu_list_stacking_aot;
    list<WaMenu *> *wamenu_list_stacking_aab;
};

bool validateclient(Window);
const bool validateclient_mapped(Window);
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
char *expand(char *, WaWindow *);

#endif // __Waimea_hh
