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

#include <list>

using std::list;

#include <map>

using std::map;
using std::make_pair;

typedef struct _WaAction WaAction;

class Waimea;

#define __m_wastrdup(_str) (((__m_wastrdup_tmp = \
                              new char[strlen(_str) + 1]) && \
                             sprintf(__m_wastrdup_tmp, "%s", _str))? \
                            __m_wastrdup_tmp : __m_wastrdup_tmp)

class WindowObject {
public:
    inline WindowObject(Window win_id, int win_type) {
        id = win_id;
        type = win_type;
        actionlist = NULL;
    }

    Window id;
    int type;
    list<WaAction *> *actionlist;
};

#define EastType  1
#define WestType -1

struct waoptions {
    char *display;
    char *rcfile;
    char *stylefile;
    char *actionfile;
    char *menufile;
};

#define WARNING cerr << "waimea: warning: " << __FUNCTION__ << ": "
#define ERROR cerr << "waimea: error: " << __FUNCTION__ << ": "
			
#define LISTDEL(list) \
    while (! list.empty()) { \
        delete list.back(); \
        list.pop_back(); \
    }

#define LISTDELITEMS(list) \
    while (! list.empty()) { \
        delete list.back(); \
    }

#define LISTPTRDEL(list) \
    while (! list->empty()) { \
        delete list->back(); \
        list->pop_back(); \
    }

#define LISTCLEAR(list) \
    while (! list.empty()) { \
        list.pop_back(); \
    }

#define LISTPTRCLEAR(list) \
    while (! list->empty()) { \
        list->pop_back(); \
    } \
    delete list;

#define LISTPTRDELITEMS(list) \
    while (! list->empty()) { \
        delete list->back(); \
    }

#define MAPCLEAR(map) \
    while (! map.empty()) { \
        map.erase(map.begin()); \
    }

#define MAPPTRCLEAR(map) \
    while (! map->empty()) { \
        map->erase(map->begin()); \
    } \
    delete map;

#define FrameType       (1L << 1)
#define WindowType      (1L << 2)
#define TitleType       (1L << 3)
#define LabelType       (1L << 4)
#define ButtonType      (1L << 5)
#define HandleType      (1L << 6)
#define LGripType       (1L << 7)
#define RGripType       (1L << 8)
#define RootType        (1L << 9)
#define WEdgeType       (1L << 10)
#define EEdgeType       (1L << 11)
#define NEdgeType       (1L << 12)
#define SEdgeType       (1L << 13)
#define MenuTitleType   (1L << 14)
#define MenuItemType    (1L << 15)
#define MenuCBItemType  (1L << 16)
#define MenuSubType     (1L << 17)
#define DockHandlerType (1L << 18)
#define DockAppType     (1L << 19)
#define MenuType        (1L << 20)

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

#include "Screen.hh"
#include "Timer.hh"
#include "Net.hh"

class Waimea {
public:
    Waimea(char **, struct waoptions *);
    virtual ~Waimea(void);

    WindowObject *FindWin(Window, int);

    struct waoptions *options;
    Display *display;
    ResourceHandler *rh;
    EventHandler *eh;
    NetHandler *net;
    Timer *timer;
    Cursor session_cursor, move_cursor, resizeleft_cursor, resizeright_cursor;
    unsigned long double_click, screenmask;
    char *pathenv;
    bool wmerr;
    
    map<Window, WindowObject *> window_table;
    list<WaScreen *> wascreen_list;

#ifdef SHAPE
    int shape, shape_event;
#endif // SHAPE
    
};

bool validateclient(Window);
const bool validateclient_mapped(Window);
void wawarning(char *, ...);
void waerror(char *, ...);
void waexec(const char *, char *);
int xerrorhandler(Display *, XErrorEvent *);
int wmrunningerror(Display *, XErrorEvent *);
void signalhandler(int);
void restart(char *);
void quit(int);
char **commandline_to_argv(char *, char **);
char *expand(char *, WaWindow *);

#endif // __Waimea_hh
