/**
 * @file   Resources.hh
 * @author David Reveman <c99drn@cs.umu.se>
 * @date   18-Jul-2001 00:31:06
 *
 * @brief Definition of ResourceHandler and StrComp classes
 *
 * Function declarations and variable definitions for ResourceHandler and
 * and StrComp classes.
 *
 * Copyright (C) David Reveman. All rights reserved.
 *
 */

#ifndef __Resources_hh
#define __Resources_hh

#include <X11/Xlib.h>
#include <X11/Xresource.h>

#ifdef    HAVE_STDIO_H
#  include <stdio.h>
#endif // HAVE_STDIO_H

class ResourceHandler;
class Define;
class WaActionExtList;
class StrComp;

typedef struct _WaAction WaAction;
typedef struct _DockStyle DockStyle;
typedef struct _ButtonStyle ButtonStyle;

#include "Window.hh"
#include "Menu.hh"
#include "Waimea.hh"
#include "Regex.hh"

#define IS_ENV_CHAR(ch) (isalnum(ch) || ch == '_')

#define ACTLISTCLEAR(list) \
    while (! list.empty()) { \
        if (list.back()->exec) \
            delete [] list.back()->exec; \
        if (list.back()->param) \
            delete [] list.back()->param; \
        delete list.back(); \
        list.pop_back(); \
    }

#define ACTLISTPTRCLEAR(list) \
    while (! list->empty()) { \
        if (list->back()->exec) \
            delete [] list->back()->exec; \
        if (list->back()->param) \
            delete [] list->back()->param; \
        delete list->back(); \
        list->pop_back(); \
    }


struct _WaAction {
    WwActionFn winfunc;
    RootActionFn rootfunc;
    MenuActionFn menufunc;
    char *exec;
    char *param;
    unsigned int type, detail, mod, nmod;
    bool replay;
    struct timeval delay;
    list<int> *delay_breaks;
};

typedef struct {
    WaColor border_color;
    WaTexture texture;
    unsigned int border_width;
} DockholderStyle;

struct _DockStyle {
    int x;
    int y;
    int geometry;
    int direction;
    int stacking;
    unsigned int gridspace;
    list<Regex *> order;
    list<int> order_type;
    bool centered;
    bool inworkspace;
    DockholderStyle style;
};

struct _ButtonStyle {
    int x, id, cb, autoplace;
    bool fg;
    WaTexture t_focused, t_unfocused, t_pressed;
    WaTexture t_focused2, t_unfocused2, t_pressed2;
    WaColor c_focused, c_unfocused, c_pressed;
    WaColor c_focused2, c_unfocused2, c_pressed2;
    Pixmap p_focused, p_unfocused, p_pressed;
    Pixmap p_focused2, p_unfocused2, p_pressed2;
    GC g_focused, g_unfocused, g_pressed;
    GC g_focused2, g_unfocused2, g_pressed2;
};

enum {
    LeftJustify,
    RightJustify,
    CenterJustify
};

enum {
    VerticalDock,
    HorizontalDock,
    AlwaysOnTop,
    AlwaysAtBottom,
    NormalStacking
};

enum {
    NameMatchType,
    ClassMatchType,
    TitleMatchType
};

class ResourceHandler {
public:
    ResourceHandler(Waimea *, struct waoptions *);
    virtual ~ResourceHandler(void);

    void LoadConfig(Waimea *);
    void LoadConfig(WaScreen *);
    void LoadStyle(WaScreen *);
    void LoadMenus(WaScreen *);
    void LoadActions(WaScreen *);
    WaMenu *ParseMenu(WaMenu *, FILE *, WaScreen *);

    char *rc_file, *style_file, *menu_file, *action_file;
    bool rc_forced, style_forced, action_forced, menu_forced;
    int linenr;
    
private:
    void ReadActions(char *, list<Define *> *, list<StrComp *> *,
                     list<WaAction *> *, WaScreen *);
    void ReadDatabaseColor(char *, char *, WaColor *, unsigned long,
                           WaImageControl *);
    void ReadDatabaseTexture(char *, char *, WaTexture *, unsigned long,
                             WaImageControl *);
    void ReadDatabaseFont(char *, char *, WaFont *, WaFont *);
    void ParseAction(const char *, list<StrComp *> *, list<WaAction *> *,
                     WaScreen *);

    Waimea *waimea;
    Display *display;
    XrmDatabase database;
    char *homedir;
    list<StrComp *> wacts;
    list<StrComp *> racts;
    list<StrComp *> macts;
    list<StrComp *> types;
    list<StrComp *> bdetails;
    list<StrComp *> mods;
};

#define WindowFuncMask (1L << 0)
#define RootFuncMask   (1L << 1)
#define MenuFuncMask   (1L << 2)

class Define {
public:
    inline Define(char *n, char *v) {
        char *__m_wastrdup_tmp;
        name = __m_wastrdup(n); value = __m_wastrdup(v);
    }
    inline ~Define(void) { delete [] name; delete [] value; }
    
    char *name;
    char *value;
};

class WaActionExtList {
public:
    inline WaActionExtList(char *n, char *c, char *t) {
        name = new Regex(n);
        cl = new Regex(c);
        title = new Regex(t);
    }
    inline ~WaActionExtList(void) {
        delete name;
        delete cl;
        delete title;
        ACTLISTCLEAR(list);
    }
    
    Regex *name;
    Regex *cl;
    Regex *title;
    list<WaAction *> list;
};

class StrComp {
public:
    StrComp(char *, unsigned long); 
    StrComp(char *, WwActionFn);
    StrComp(char *, RootActionFn);
    StrComp(char *, MenuActionFn);
    
    bool Comp(char *);

    char *str;
    unsigned long value;
    int type;
    WwActionFn winfunc;
    RootActionFn rootfunc;
    MenuActionFn menufunc;
};

char *strtrim(char *);
char *strwithin(char *, char, char, bool = false);
char *environment_expansion(char *s);
char *param_eval(char *, char *, WaScreen *);

#endif // __Resources_hh
