/**
 * @file   ResourceHandler.hh
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

#ifndef __ResourceHandler_hh
#define __ResourceHandler_hh

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xresource.h>

class ResourceHandler;
class Define;
class WaActionExtList;
class StrComp;

typedef struct _WaAction WaAction;
typedef struct _DockStyle DockStyle;
typedef struct _ButtonStyle ButtonStyle;

#include "Waimea.hh"
#include "WaWindow.hh"
#include "WaScreen.hh"
#include "WaImage.hh"
#include "WaMenu.hh"

#define ACTLISTCLEAR(list) \
    while (! list->empty()) { \
        if (list->back()->exec) \
            delete [] list->back()->exec; \
        if (list->back()->param) \
            delete [] list->back()->param; \
        delete list->back(); \
        list->pop_back(); \
    } \
    delete list;

#define ACTLISTCLEAR2(list) \
    while (! list.empty()) { \
        if (list.back()->exec) \
            delete [] list.back()->exec; \
        if (list.back()->param) \
            delete [] list.back()->param; \
        delete list.back(); \
        list.pop_back(); \
    }

#define BSCLEAR(list) \
    while (! list->empty()) { \
        delete list->back(); \
        list-> pop_back(); \
    }

struct _WaAction {
    WwActionFn winfunc;
    RootActionFn rootfunc;
    MenuActionFn menufunc;
    char *exec;
    char *param;
    unsigned int type, detail, mod, nmod;
    bool replay;
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
    list<char *> *order;
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

class ResourceHandler {
public:
    ResourceHandler(Waimea *, struct waoptions *);
    virtual ~ResourceHandler(void);
    
    void LoadConfig(void);
    void LoadStyle(WaScreen *);
    void LoadMenus(void);
    void LoadActions(void);
    void ReadActions(char *, list<Define *> *, list<StrComp *> *,
                     list<WaAction *> *);

    char *rc_file;
    char *style_file;
    char *menu_file;
    char *action_file;
    unsigned int virtual_x;
    unsigned int virtual_y;
    int colors_per_channel, menu_stacking;
    long unsigned int cache_max, double_click;
    bool image_dither, rc_forced, style_forced, action_forced, menu_forced;
    
    list<WaAction *> *frameacts, *awinacts, *pwinacts, *titleacts, *labelacts,
        *handleacts, *rgacts, *lgacts, *rootacts, *weacts, *eeacts, *neacts,
        *seacts, *mtacts, *miacts, *msacts, *mcbacts;
    list<WaAction *> **bacts;

    list<WaActionExtList *> ext_frameacts, ext_awinacts, ext_pwinacts,
        ext_titleacts, ext_labelacts, ext_handleacts, ext_rgacts, ext_lgacts;
    list<WaActionExtList *> **ext_bacts;

    list<DockStyle *> *dockstyles;
    list<ButtonStyle *> *buttonstyles;
    
private:
    void ReadDatabaseColor(char *, char *, WaColor *, unsigned long,
                           WaImageControl *);
    void ReadDatabaseTexture(char *, char *, WaTexture *, unsigned long,
                             WaImageControl *);
    void ReadDatabaseFont(char *, char *, char **, char *);
    void ParseAction(const char *, list<StrComp *> *, list<WaAction *> *);
    void ParseMenu(WaMenu *, FILE *);

    Waimea *waimea;
    WaScreen *wascreen;
    Display *display;
    XrmDatabase database;
    char *homedir;
    int linenr;
    list<StrComp *> *wacts;
    list<StrComp *> *racts;
    list<StrComp *> *macts;
    list<StrComp *> *types;
    list<StrComp *> *bdetails;
    list<StrComp *> *mods;
};

#define WindowFuncMask (1L << 0)
#define RootFuncMask   (1L << 1)
#define MenuFuncMask   (1L << 2)

class Define {
public:
    inline Define(char *n, char *v) { name = n; value = v; }
    inline ~Define(void) { delete name; delete value; }
    
    char *name;
    char *value;
};

class WaActionExtList {
public:
    inline WaActionExtList(char *n, char *c, char *t) {
        name = n;
        cl = c;
        title = t;
    }
    inline ~WaActionExtList(void) {
        if (name) delete name;
        if (cl) delete cl;
        ACTLISTCLEAR2(list);
    }
    
    char *name;
    char *cl;
    char *title;
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
char *strwithin(char *, char, char);

#endif // __ResourceHandler_hh
