/** -*- Mode: C++ -*-
 *
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
class StrComp;
typedef struct _WaAction WaAction;
typedef struct _DockStyle DockStyle;

#include "Waimea.hh"
#include "WaWindow.hh"
#include "WaScreen.hh"
#include "WaImage.hh"
#include "WaMenu.hh"

struct _WaAction {
    WwActionFn winfunc;
    RootActionFn rootfunc;
    MenuActionFn menufunc;
    char *exec;
    unsigned long param;
    unsigned int type, detail, mod, nmod;
};

struct _DockStyle {
    int x;
    int y;
    int geometry;
    int direction;
    int stacking;
    unsigned int gridspace;
    list<char *> *order;
    bool centered;
};

#define LeftJustify   1
#define RightJustify  2
#define CenterJustify 3

class ResourceHandler {
public:
    ResourceHandler(Waimea *, struct waoptions *);
    virtual ~ResourceHandler(void);
    
    void LoadConfig(void);
    void LoadStyle(WaScreen *);
    void LoadMenus(void);
    void LoadActions(Waimea *);

    char *rc_file;
    char *style_file;
    char *menu_file;
    char *action_file;
    unsigned int virtual_x;
    unsigned int virtual_y;
    int colors_per_channel;
    long unsigned int cache_max;
    Bool image_dither, rc_forced, style_forced, action_forced, menu_forced;
    
    list<WaAction *> *frameacts, *winacts, *titleacts, *labelacts, *handleacts,
        *cbacts, *ibacts, *mbacts, *rgacts, *lgacts, *rootacts, *weacts,
        *eeacts, *neacts, *seacts, *mtacts, *miacts, *msacts;

    list<DockStyle *> *dockstyles;
    
private:
    void ReadDatabaseColor(char *, char *, WaColor *, unsigned long,
                           WaImageControl *);
    void ReadDatabaseTexture(char *, char *, WaTexture *, unsigned long,
                             WaImageControl *, WaScreen *);
    void ReadDatabaseFont(char *, char *, char **, char *);
    void ReadDatabaseActions(char *, char *, list<StrComp *> *,
                             list<WaAction *> *);
    void ParseAction(const char *, list<StrComp *> *, list<WaAction *> *);
    void ParseMenu(WaMenu *, FILE *);

    Waimea *waimea;
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
