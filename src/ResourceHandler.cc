/**
 * @file   ResourceHandler.cc
 * @author David Reveman <c99drn@cs.umu.se>
 * @date   18-Jul-2001 00:31:22
 *
 * @brief Implementation of ResourceHandler and StrComp classes
 *
 * ResourceHandler class is used for reading window manager settings.
 * Most settings are retrieved from X resource files. StrComp class
 * is used for comparing strings to objects.
 *
 * Copyright (C) David Reveman. All rights reserved.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>

#ifdef PIXMAP
#include <Imlib2.h>
#endif // PIXMAP

#include "ResourceHandler.hh"

/**
 * @fn    ResourceHandler(void)
 * @brief Constructor for ResourceHandler class
 *
 * Sets config file variables. Creates lists with function pointers and
 * lists for actions.
 */
ResourceHandler::ResourceHandler(Waimea *wa, struct waoptions *options) {
    waimea = wa;
    display = waimea->display;
    
    homedir = getenv("HOME");

    style_file = wastrdup((char *) DEFAULTSTYLE);
    action_file = wastrdup((char *) DEFAULTACTION);
    menu_file = wastrdup((char *) DEFAULTMENU);
    rc_forced = style_forced = action_forced = menu_forced = false;
    if (options->rcfile) {
        rc_file = options->rcfile;
        rc_forced = true;
    } else {
        rc_file = new char[strlen(homedir) + strlen("/.waimearc") + 1];
        sprintf(rc_file, "%s/.waimearc", homedir);
    }
    if (options->stylefile) {
        style_file = options->stylefile;
        style_forced = true;
    }
    if (options->actionfile) {
        action_file = options->actionfile;
        action_forced = true;
    }
    if (options->menufile) {
        menu_file = options->menufile;
        menu_forced = true;
    }

    wacts = new list<StrComp *>;
    wacts->push_back(new StrComp("raise", &WaWindow::Raise));
    wacts->push_back(new StrComp("lower", &WaWindow::Lower));
    wacts->push_back(new StrComp("focus", &WaWindow::Focus));
    wacts->push_back(new StrComp("startmove", &WaWindow::Move));
    wacts->push_back(new StrComp("startresizeright", &WaWindow::ResizeRight));
    wacts->push_back(new StrComp("startresizeleft", &WaWindow::ResizeLeft));
    wacts->push_back(new StrComp("startopaquemove", &WaWindow::MoveOpaque));
    wacts->push_back(new StrComp("startopaqueresizeright",
                             &WaWindow::ResizeRightOpaque));
    wacts->push_back(new StrComp("startopaqueresizeleft",
                              &WaWindow::ResizeLeftOpaque));
    wacts->push_back(new StrComp("endmoveresize", &WaWindow::EndMoveResize));
    wacts->push_back(new StrComp("close", &WaWindow::Close));
    wacts->push_back(new StrComp("kill", &WaWindow::Kill));
    wacts->push_back(new StrComp("closekill", &WaWindow::CloseKill));    
    wacts->push_back(new StrComp("menumap", &WaWindow::MenuMap));
    wacts->push_back(new StrComp("menuremap", &WaWindow::MenuRemap));
    wacts->push_back(new StrComp("menumapfocused",
                                 &WaWindow::MenuMapFocused));
    wacts->push_back(new StrComp("menuremapfocused",
                                 &WaWindow::MenuRemapFocused));
    wacts->push_back(new StrComp("menuunmap", &WaWindow::MenuUnmap));
    wacts->push_back(new StrComp("menuunmapfocused",
                                 &WaWindow::MenuUnmapFocus));
    wacts->push_back(new StrComp("shade", &WaWindow::Shade));
    wacts->push_back(new StrComp("unshade", &WaWindow::UnShade));
    wacts->push_back(new StrComp("toggleshade", &WaWindow::ToggleShade));
    wacts->push_back(new StrComp("maximize", &WaWindow::Maximize));
    wacts->push_back(new StrComp("unmaximize", &WaWindow::UnMaximize));
    wacts->push_back(new StrComp("togglemaximize", &WaWindow::ToggleMaximize));
    wacts->push_back(new StrComp("sticky", &WaWindow::Sticky));
    wacts->push_back(new StrComp("unsticky", &WaWindow::UnSticky));
    wacts->push_back(new StrComp("togglesticky", &WaWindow::ToggleSticky));
    wacts->push_back(new StrComp("viewportleft", &WaWindow::MoveViewportLeft));
    wacts->push_back(new StrComp("viewportright",
                                 &WaWindow::MoveViewportRight));
    wacts->push_back(new StrComp("viewportup", &WaWindow::MoveViewportUp));
    wacts->push_back(new StrComp("viewportdown", &WaWindow::MoveViewportDown));
    wacts->push_back(new StrComp("scrollviewportleft",
                                 &WaWindow::ScrollViewportLeft));
    wacts->push_back(new StrComp("scrollviewportright",
                                 &WaWindow::ScrollViewportRight));
    wacts->push_back(new StrComp("scrollviewportup",
                                 &WaWindow::ScrollViewportUp));
    wacts->push_back(new StrComp("scrollviewportdown",
                                 &WaWindow::ScrollViewportDown));
    wacts->push_back(new StrComp("viewportleftnowarp",
                                 &WaWindow::MoveViewportLeftNoWarp));
    wacts->push_back(new StrComp("viewportrightnowarp",
                                 &WaWindow::MoveViewportRightNoWarp));
    wacts->push_back(new StrComp("viewportupnowarp",
                                 &WaWindow::MoveViewportUpNoWarp));
    wacts->push_back(new StrComp("viewportdownnowarp",
                                 &WaWindow::MoveViewportDownNoWarp));
    wacts->push_back(new StrComp("scrollviewportleftnowarp",
                                 &WaWindow::ScrollViewportLeftNoWarp));
    wacts->push_back(new StrComp("scrollviewportrightnowarp",
                                 &WaWindow::ScrollViewportRightNoWarp));
    wacts->push_back(new StrComp("scrollviewportupnowarp",
                                 &WaWindow::ScrollViewportUpNoWarp));
    wacts->push_back(new StrComp("scrollviewportdownnowarp",
                                 &WaWindow::ScrollViewportDownNoWarp));
    wacts->push_back(new StrComp("startviewportmove",
                                 &WaWindow::ViewportMove));
    wacts->push_back(new StrComp("taskswitcher", &WaWindow::TaskSwitcher));
    wacts->push_back(new StrComp("previoustask", &WaWindow::PreviousTask));
    wacts->push_back(new StrComp("nexttask", &WaWindow::NextTask));
    wacts->push_back(new StrComp("raisefocus", &WaWindow::RaiseFocus));
    wacts->push_back(new StrComp("decortitleon", &WaWindow::DecorTitleOn));
    wacts->push_back(new StrComp("decorhandleon", &WaWindow::DecorHandleOn));
    wacts->push_back(new StrComp("decorborderon", &WaWindow::DecorBorderOn));
    wacts->push_back(new StrComp("decorallon", &WaWindow::DecorAllOn));
    wacts->push_back(new StrComp("decortitleoff", &WaWindow::DecorTitleOff));
    wacts->push_back(new StrComp("decorhandleoff", &WaWindow::DecorHandleOff));
    wacts->push_back(new StrComp("decorborderoff", &WaWindow::DecorBorderOff));
    wacts->push_back(new StrComp("decoralloff", &WaWindow::DecorAllOff));
    wacts->push_back(new StrComp("decortitletoggle",
                                 &WaWindow::DecorTitleToggle));
    wacts->push_back(new StrComp("decorhandletoggle",
                                 &WaWindow::DecorHandleToggle));
    wacts->push_back(new StrComp("decorbordertoggle",
                                 &WaWindow::DecorBorderToggle));
    wacts->push_back(new StrComp("alwaysontopon",
                                 &WaWindow::AlwaysontopOn));
    wacts->push_back(new StrComp("alwaysatbottomon",
                                 &WaWindow::AlwaysatbottomOn));
    wacts->push_back(new StrComp("alwaysontopoff",
                                 &WaWindow::AlwaysontopOff));
    wacts->push_back(new StrComp("alwaysatbottomoff",
                                 &WaWindow::AlwaysatbottomOff));
    wacts->push_back(new StrComp("alwaysontoptoggle",
                                 &WaWindow::AlwaysontopToggle));
    wacts->push_back(new StrComp("alwaysatbottomtoggle",
                                 &WaWindow::AlwaysatbottomToggle));
    wacts->push_back(new StrComp("acceptconfigrequeston",
                                 &WaWindow::AcceptConfigRequestOn));
    wacts->push_back(new StrComp("acceptconfigrequestoff",
                                 &WaWindow::AcceptConfigRequestOff));
    wacts->push_back(new StrComp("acceptconfigrequesttoggle",
                                 &WaWindow::AcceptConfigRequestToggle));
    wacts->push_back(new StrComp("pointerwarp", &WaWindow::PointerWarp));
    wacts->push_back(new StrComp("moveresize", &WaWindow::MoveResize));
    wacts->push_back(new StrComp("moveresizevirtual",
                                 &WaWindow::MoveResizeVirtual));
    wacts->push_back(new StrComp("movetopointer",
                                 &WaWindow::MoveWindowToPointer));
    wacts->push_back(new StrComp("restart", &WaWindow::Restart));
    wacts->push_back(new StrComp("exit", &WaWindow::Exit));
    wacts->push_back(new StrComp("nop", &WaWindow::Nop));
    
    racts = new list<StrComp *>;
    racts->push_back(new StrComp("focus", &WaScreen::Focus));
    racts->push_back(new StrComp("menumap", &WaScreen::MenuMap));
    racts->push_back(new StrComp("menuremap", &WaScreen::MenuRemap));
    racts->push_back(new StrComp("menumapfocused", &WaScreen::MenuMapFocused));
    racts->push_back(new StrComp("menuremapfocused",
                                 &WaScreen::MenuRemapFocused));
    racts->push_back(new StrComp("menuunmap", &WaScreen::MenuUnmap));
    racts->push_back(new StrComp("menuunmapfocused",
                                 &WaScreen::MenuUnmapFocus));
    racts->push_back(new StrComp("restart", &WaScreen::Restart));
    racts->push_back(new StrComp("exit", &WaScreen::Exit));
    racts->push_back(new StrComp("viewportleft", &WaScreen::MoveViewportLeft));
    racts->push_back(new StrComp("viewportright",
                                 &WaScreen::MoveViewportRight));
    racts->push_back(new StrComp("viewportup", &WaScreen::MoveViewportUp));
    racts->push_back(new StrComp("viewportdown", &WaScreen::MoveViewportDown));
    racts->push_back(new StrComp("scrollviewportleft",
                                 &WaScreen::ScrollViewportLeft));
    racts->push_back(new StrComp("scrollviewportright",
                                 &WaScreen::ScrollViewportRight));
    racts->push_back(new StrComp("scrollviewportup",
                                 &WaScreen::ScrollViewportUp));
    racts->push_back(new StrComp("scrollviewportdown",
                                 &WaScreen::ScrollViewportDown));
    racts->push_back(new StrComp("startviewportmove",
                                 &WaScreen::ViewportMove));
    racts->push_back(new StrComp("endmoveresize", &WaScreen::EndMoveResize));
    racts->push_back(new StrComp("viewportleftnowarp",
                                 &WaScreen::MoveViewportLeftNoWarp));
    racts->push_back(new StrComp("viewportrightnowarp",
                                 &WaScreen::MoveViewportRightNoWarp));
    racts->push_back(new StrComp("viewportupnowarp",
                                 &WaScreen::MoveViewportUpNoWarp));
    racts->push_back(new StrComp("viewportdownnowarp",
                                 &WaScreen::MoveViewportDownNoWarp));
    racts->push_back(new StrComp("scrollviewportleftnowarp",
                                 &WaScreen::ScrollViewportLeftNoWarp));
    racts->push_back(new StrComp("scrollviewportrightnowarp",
                                 &WaScreen::ScrollViewportRightNoWarp));
    racts->push_back(new StrComp("scrollviewportupnowarp",
                                 &WaScreen::ScrollViewportUpNoWarp));
    racts->push_back(new StrComp("scrollviewportdownnowarp",
                                 &WaScreen::ScrollViewportDownNoWarp));
    racts->push_back(new StrComp("taskswitcher", &WaScreen::TaskSwitcher));
    racts->push_back(new StrComp("previoustask", &WaScreen::PreviousTask));
    racts->push_back(new StrComp("nexttask", &WaScreen::NextTask));
    racts->push_back(new StrComp("pointerwarp", &WaScreen::PointerWarp));
    racts->push_back(new StrComp("nop", &WaScreen::Nop));
    
    macts = new list<StrComp *>;
    macts->push_back(new StrComp("unlink", &WaMenuItem::UnLinkMenu));
    macts->push_back(new StrComp("mapsub", &WaMenuItem::MapSubmenu));
    macts->push_back(new StrComp("remapsub", &WaMenuItem::RemapSubmenu));
    macts->push_back(new StrComp("mapsubfocused",
                                 &WaMenuItem::MapSubmenuFocused));
    macts->push_back(new StrComp("remapsubfocused",
                                 &WaMenuItem::RemapSubmenuFocused));
    macts->push_back(new StrComp("unmap", &WaMenuItem::UnmapMenu));
    macts->push_back(new StrComp("unmapfocused", &WaMenuItem::UnmapMenuFocus));
    macts->push_back(new StrComp("unmapsubs", &WaMenuItem::UnmapSubmenus));
    macts->push_back(new StrComp("unmaptree", &WaMenuItem::UnmapTree));
    macts->push_back(new StrComp("exec", &WaMenuItem::Exec));
    macts->push_back(new StrComp("func", &WaMenuItem::Func));
    macts->push_back(new StrComp("raise", &WaMenuItem::Raise));
    macts->push_back(new StrComp("focus", &WaMenuItem::Focus));
    macts->push_back(new StrComp("lower", &WaMenuItem::Lower));
    macts->push_back(new StrComp("startmove", &WaMenuItem::Move));
    macts->push_back(new StrComp("startopaquemove", &WaMenuItem::MoveOpaque));
    macts->push_back(new StrComp("endmoveresize", &WaMenuItem::EndMoveResize));
    macts->push_back(new StrComp("viewportleft",
                                 &WaMenuItem::MoveViewportLeft));
    macts->push_back(new StrComp("viewportright",
                                 &WaMenuItem::MoveViewportRight));
    macts->push_back(new StrComp("viewportup", &WaMenuItem::MoveViewportUp));
    macts->push_back(new StrComp("viewportdown",
                                 &WaMenuItem::MoveViewportDown));
    macts->push_back(new StrComp("scrollviewportleft",
                                 &WaMenuItem::ScrollViewportLeft));
    macts->push_back(new StrComp("scrollviewportright",
                                 &WaMenuItem::ScrollViewportRight));
    macts->push_back(new StrComp("scrollviewportup",
                                 &WaMenuItem::ScrollViewportUp));
    macts->push_back(new StrComp("scrollviewportdown",
                                 &WaMenuItem::ScrollViewportDown));
    macts->push_back(new StrComp("viewportleftnowarp",
                                 &WaMenuItem::MoveViewportLeftNoWarp));
    macts->push_back(new StrComp("viewportrightnowarp",
                                 &WaMenuItem::MoveViewportRightNoWarp));
    macts->push_back(new StrComp("viewportupnowarp",
                                 &WaMenuItem::MoveViewportUpNoWarp));
    macts->push_back(new StrComp("viewportdownnowarp",
                                 &WaMenuItem::MoveViewportDownNoWarp));
    macts->push_back(new StrComp("scrollviewportleftnowarp",
                                 &WaMenuItem::ScrollViewportLeftNoWarp));
    macts->push_back(new StrComp("scrollviewportrightnowarp",
                                 &WaMenuItem::ScrollViewportRightNoWarp));
    macts->push_back(new StrComp("scrollviewportupnowarp",
                                 &WaMenuItem::ScrollViewportUpNoWarp));
    macts->push_back(new StrComp("scrollviewportdownnowarp",
                                 &WaMenuItem::ScrollViewportDownNoWarp));
    macts->push_back(new StrComp("startviewportmove",
                                 &WaMenuItem::ViewportMove));
    macts->push_back(new StrComp("taskswitcher", &WaMenuItem::TaskSwitcher));
    macts->push_back(new StrComp("previoustask", &WaMenuItem::PreviousTask));
    macts->push_back(new StrComp("nexttask", &WaMenuItem::NextTask));
    macts->push_back(new StrComp("nextitem", &WaMenuItem::NextItem));
    macts->push_back(new StrComp("previousitem", &WaMenuItem::PreviousItem));
    macts->push_back(new StrComp("pointerwarp", &WaMenuItem::PointerWarp));
    macts->push_back(new StrComp("restart", &WaMenuItem::Restart));
    macts->push_back(new StrComp("exit", &WaMenuItem::Exit));
    macts->push_back(new StrComp("nop", &WaMenuItem::Nop));
    
    types = new list<StrComp *>;
    types->push_back(new StrComp("keypress", KeyPress));
    types->push_back(new StrComp("keyrelease", KeyRelease));
    types->push_back(new StrComp("buttonpress", ButtonPress));
    types->push_back(new StrComp("buttonrelease", ButtonRelease));
    types->push_back(new StrComp("doubleclick", DoubleClick));
    types->push_back(new StrComp("enternotify", EnterNotify));
    types->push_back(new StrComp("leavenotify", LeaveNotify));
    types->push_back(new StrComp("maprequest", MapRequest));

    bdetails = new list<StrComp *>;
    bdetails->push_back(new StrComp("anybutton", (unsigned long) 0));
    bdetails->push_back(new StrComp("button1", Button1));
    bdetails->push_back(new StrComp("button2", Button2));
    bdetails->push_back(new StrComp("button3", Button3));
    bdetails->push_back(new StrComp("button4", Button4));
    bdetails->push_back(new StrComp("button5", Button5));
    bdetails->push_back(new StrComp("button6", 6));
    bdetails->push_back(new StrComp("button7", 7));
    
    mods = new list<StrComp *>;
    mods->push_back(new StrComp("shiftmask", ShiftMask));
    mods->push_back(new StrComp("lockmask", LockMask));
    mods->push_back(new StrComp("controlmask", ControlMask));
    mods->push_back(new StrComp("mod1mask", Mod1Mask));
    mods->push_back(new StrComp("mod2mask", Mod2Mask));
    mods->push_back(new StrComp("mod3mask", Mod3Mask));
    mods->push_back(new StrComp("mod4mask", Mod4Mask));
    mods->push_back(new StrComp("mod5mask", Mod5Mask));
    mods->push_back(new StrComp("button1mask", Button1Mask));
    mods->push_back(new StrComp("button2mask", Button2Mask));
    mods->push_back(new StrComp("button3mask", Button3Mask));
    mods->push_back(new StrComp("button4mask", Button4Mask));
    mods->push_back(new StrComp("button5mask", Button5Mask));
    mods->push_back(new StrComp("moveresizemask", MoveResizeMask));

    frameacts  = new list<WaAction *>;
    awinacts   = new list<WaAction *>;
    pwinacts   = new list<WaAction *>;
    titleacts  = new list<WaAction *>;
    labelacts  = new list<WaAction *>;
    handleacts = new list<WaAction *>;
    cbacts     = new list<WaAction *>;
    ibacts     = new list<WaAction *>;
    mbacts     = new list<WaAction *>;
    rgacts     = new list<WaAction *>;
    lgacts     = new list<WaAction *>;
    rootacts   = new list<WaAction *>;
    weacts     = new list<WaAction *>;
    eeacts     = new list<WaAction *>;
    neacts     = new list<WaAction *>;
    seacts     = new list<WaAction *>;
    mtacts     = new list<WaAction *>;
    miacts     = new list<WaAction *>;
    msacts     = new list<WaAction *>;
    mcbacts    = new list<WaAction *>;

    dockstyles = new list<DockStyle *>;
}

/**
 * @fn    ~ResourceHandler(void)
 * @brief Destructor for ResourceHandler class
 *
 * Deletes all action lists and all WaActions in them.
 */
ResourceHandler::~ResourceHandler(void) {
    while (! dockstyles->empty()) {
        while (! dockstyles->back()->order->empty()) {
            delete [] dockstyles->back()->order->back();
            dockstyles->back()->order->pop_back();
        }
        delete dockstyles->back()->order;
        delete dockstyles->back();
        dockstyles->pop_back();
    }
    delete dockstyles;
    
    ACTLISTCLEAR(frameacts);
    ACTLISTCLEAR(awinacts);
    ACTLISTCLEAR(pwinacts);
    ACTLISTCLEAR(titleacts);
    ACTLISTCLEAR(labelacts);
    ACTLISTCLEAR(handleacts);
    ACTLISTCLEAR(cbacts);
    ACTLISTCLEAR(mbacts);
    ACTLISTCLEAR(ibacts);
    ACTLISTCLEAR(rgacts);
    ACTLISTCLEAR(lgacts);
    ACTLISTCLEAR(rootacts);
    ACTLISTCLEAR(weacts);
    ACTLISTCLEAR(eeacts);
    ACTLISTCLEAR(neacts);
    ACTLISTCLEAR(seacts);
    ACTLISTCLEAR(mtacts);
    ACTLISTCLEAR(miacts);
    ACTLISTCLEAR(msacts);
    ACTLISTCLEAR(mcbacts);

    LISTCLEAR3(ext_frameacts);
    LISTCLEAR3(ext_awinacts);
    LISTCLEAR3(ext_pwinacts);
    LISTCLEAR3(ext_titleacts);
    LISTCLEAR3(ext_labelacts);
    LISTCLEAR3(ext_handleacts);
    LISTCLEAR3(ext_cbacts);
    LISTCLEAR3(ext_mbacts);
    LISTCLEAR3(ext_ibacts);
    LISTCLEAR3(ext_rgacts);
    LISTCLEAR3(ext_lgacts);
    
    delete [] rc_file;
    delete [] style_file;
    delete [] action_file;
    delete [] menu_file;
}

/**
 * @fn    LoadConfig(void)
 * @brief Reads config file
 *
 * Reads all configuration resources from config file.
 */
void ResourceHandler::LoadConfig(void) {
    XrmValue value;
    char *value_type;
    
    database = (XrmDatabase) 0;
    if (! (database = XrmGetFileDatabase(rc_file)))
        if (rc_forced) WARNING << "can't open rcfile \"" << rc_file <<
                           "\" for reading" << endl;

    if (! style_forced)
        if (XrmGetResource(database, "styleFile", "StyleFile",
                           &value_type, &value)) {
            delete [] style_file;
            style_file = wastrdup(value.addr);
        }

    if (! action_forced)
        if (XrmGetResource(database, "actionFile", "ActionFile",
                           &value_type, &value)) {
            delete [] action_file;
            action_file = wastrdup(value.addr);
        }

    if (! menu_forced)
        if (XrmGetResource(database, "menuFile", "MenuFile",
                           &value_type, &value)) {
            delete [] menu_file;
            menu_file = wastrdup(value.addr);
        }
    
    if (XrmGetResource(database, "virtualSize", "ViriualSize",
                     &value_type, &value)) {
        if (sscanf(value.addr, "%dx%d", &virtual_x, &virtual_y) != 2) {
            virtual_x = virtual_y = 3;
        }
    } else
        virtual_x = virtual_y = 3;
    if (virtual_x > 20) virtual_x = 20;
    if (virtual_y > 20) virtual_y = 20;
    if (virtual_x < 1) virtual_x = 1;
    if (virtual_y < 1) virtual_y = 1;
    
    if (XrmGetResource(database, "colorsPerChannel",
                       "ColorsPerChannel", &value_type, &value)) {
        if (sscanf(value.addr, "%d", &colors_per_channel) != 1) {
            colors_per_channel = 4;
        } else {
            if (colors_per_channel < 2) colors_per_channel = 2;
            if (colors_per_channel > 6) colors_per_channel = 6;
        }
    } else
        colors_per_channel = 4;
    
    if (XrmGetResource(database, "cacheMax", "CacheMax",
                     &value_type, &value)) {
        if (sscanf(value.addr, "%lu", &cache_max) != 1)
            cache_max = 200;
    } else
        cache_max = 200;

    if (XrmGetResource(database, "imageDither", "ImageDither",
                       &value_type, &value)) {
        if (! strncasecmp("true", value.addr, value.size))
            image_dither = true;
        else
            image_dither = false;
    } else
        image_dither = true;
    
    if (XrmGetResource(database, "doubleClickInterval",
                       "DoubleClickInterval", &value_type, &value)) {
        if (sscanf(value.addr, "%lu", &double_click) != 1)
            double_click = 300;
    } else
        double_click = 300;

    if (double_click > 999) double_click = 999;

    if (XrmGetResource(database, "menuStacking", "MenuStacking",
                       &value_type, &value)) {
        if (! strncasecmp("AlwaysAtBottom", value.addr, value.size))
            menu_stacking = AlwaysAtBottom;
        else if (! strncasecmp("AlwaysOnTop", value.addr, value.size))
            menu_stacking = AlwaysOnTop;
    } else
        menu_stacking = NormalStacking;    

    unsigned int dummy;
    char *token;
    int dock_num, i;
    bool d_exists = true, have_u = false;
    DockStyle *dockstyle;
    char rc_name[20], rc_class[20];

    for (dock_num = 0; d_exists && dock_num < 100; ++dock_num) {
        d_exists = false;
        dockstyle = new DockStyle;
    
        sprintf(rc_name, "dock%d.geometry", dock_num);
        sprintf(rc_class, "Dock%d.Geometry", dock_num);
        if (XrmGetResource(database, rc_name, rc_class, &value_type, &value)) {
            dockstyle->geometry = XParseGeometry(value.addr, &dockstyle->x,
                                                &dockstyle->y, &dummy, &dummy);
            d_exists = true;
        }
        
        dockstyle->order = new list<char *>;
        sprintf(rc_name, "dock%d.order", dock_num);
        sprintf(rc_class, "Dock%d.Order", dock_num);
        if (XrmGetResource(database, rc_name, rc_class, &value_type, &value)) {
            d_exists = true;
            token = value.addr;
            while (strlen(token) > 6) {
                token = strtrim(token);
                if (! strncasecmp("name", token, 4)) {
                    for (i = 0; token[i] != '\0' && token[i] != ']'; i++);
                    if (token[i] == '\0') break;
                    token[i] = '\0';
                    token[3] = 'N';
                    token[4] = '_';
                    dockstyle->order->push_back(wastrdup(&token[3]));
                    token = token + strlen(token) + 1;
                }
                else if (! strncasecmp("class", token, 5)) {
                    for (i = 0; token[i] != '\0' && token[i] != ']'; i++);
                    if (token[i] == '\0') break;
                    token[i] = '\0';
                    token[4] = 'N';
                    token[5] = '_';
                    dockstyle->order->push_back(wastrdup(&token[4]));
                    token = token + strlen(token) + 1;
                }
                else if (! strncasecmp("unknown", token, 7) && !have_u) {
                    have_u = true;
                    dockstyle->order->push_back(wastrdup("U"));
                    token = token + 7;
                }
            }
        }

        sprintf(rc_name, "dock%d.centered", dock_num);
        sprintf(rc_class, "Dock%d.Centered", dock_num);
        if (XrmGetResource(database, rc_name, rc_class, &value_type, &value)) {
            d_exists = true;
            if (! strncasecmp("true", value.addr, value.size))
                dockstyle->centered = true;
            else
                dockstyle->centered = false;
        } else
            dockstyle->centered = false;

        sprintf(rc_name, "dock%d.inworkspace", dock_num);
        sprintf(rc_class, "Dock%d.Inworkspace", dock_num);
        if (XrmGetResource(database, rc_name, rc_class, &value_type, &value)) {
            d_exists = true;
            if (! strncasecmp("true", value.addr, value.size))
                dockstyle->inworkspace = true;
            else
                dockstyle->inworkspace = false;
        } else
            dockstyle->inworkspace = false;

        sprintf(rc_name, "dock%d.direction", dock_num);
        sprintf(rc_class, "Dock%d.Direction", dock_num);
        if (XrmGetResource(database, rc_name, rc_class, &value_type, &value)) {
            d_exists = true;
            if (! strncasecmp("Horizontal", value.addr, value.size))
                dockstyle->direction = HorizontalDock;
            else
                dockstyle->direction = VerticalDock;
        } else
            dockstyle->direction = VerticalDock;

        sprintf(rc_name, "dock%d.gridSpace", dock_num);
        sprintf(rc_class, "Dock%d.GridSpace", dock_num);
        if (XrmGetResource(database, rc_name, rc_class, &value_type, &value)) {
            d_exists = true;
            if (sscanf(value.addr, "%u", &dockstyle->gridspace) != 1)
                dockstyle->gridspace = 2;
        } else
            dockstyle->gridspace = 2;
        
        if (dockstyle->gridspace > 50) dockstyle->gridspace = 50;

        sprintf(rc_name, "dock%d.stacking", dock_num);
        sprintf(rc_class, "Dock%d.Stacking", dock_num);
        if (XrmGetResource(database, rc_name, rc_class, &value_type, &value)) {
            d_exists = true;
            if (! strncasecmp("AlwaysAtBottom", value.addr, value.size))
                dockstyle->stacking = AlwaysAtBottom;
            else
                dockstyle->stacking = AlwaysOnTop;
        } else
            dockstyle->stacking = AlwaysOnTop;
        
        if (d_exists || ! dock_num)
            dockstyles->push_back(dockstyle);
        else {
            delete dockstyle->order;
            delete dockstyle;
        }
    }
    if (! have_u) dockstyles->back()->order->push_back(wastrdup("U"));
    
    XrmDestroyDatabase(database);
} 
    
/**
 * @fn    LoadStyle(WaScreen *scrn)
 * @brief Reads style file
 *
 * Reads a style resources from a style file.
 *
 * @param scrn WaScreen to load style for
 */
void ResourceHandler::LoadStyle(WaScreen *scrn) {
    XrmValue value;
    char *value_type;
    int screen = scrn->screen_number;
    WindowStyle *wstyle = &scrn->wstyle;
    MenuStyle   *mstyle = &scrn->mstyle;
    WaImageControl *ic = scrn->ic;
    wascreen = scrn;
    
    database = (XrmDatabase) 0;
    
    if (! (database = XrmGetFileDatabase(style_file))) {
        ERROR << "can't open stylefile\"" << style_file << "\" for reading" <<
            endl; exit(1);
    }
    int slen = strlen(style_file) - 1;
    for (; slen >= 1 && style_file[slen] != '/'; slen--);
    style_file[slen] = '\0';

    ReadDatabaseFont("window.font", "Window.Font", &wstyle->fontname, "fixed");
    ReadDatabaseFont("menu.frame.font", "Menu.Frame.Font",
                     &mstyle->f_fontname, wstyle->fontname);
    ReadDatabaseFont("menu.title.font", "Menu.Title.Font",
                     &mstyle->t_fontname, mstyle->f_fontname);
    ReadDatabaseFont("menu.bullet.font", "Menu.Bullet.Font",
                     &mstyle->b_fontname, mstyle->f_fontname);
    ReadDatabaseFont("menu.checkbox.true.font",
                     "Menu.Checkbox.True.Font",
                     &mstyle->ct_fontname, mstyle->f_fontname);
    ReadDatabaseFont("menu.checkbox.false.font",
                     "Menu.Checkbox.False.Font",
                     &mstyle->cf_fontname, mstyle->ct_fontname);
    
#ifdef XFT
    if (XrmGetResource(database, "window.xftfontsize",
                       "Window.XftFontSize", &value_type, &value)) {
        if (! (wstyle->xftsize = strtod(value.addr, 0))) {
            wstyle->xftsize = 0.0;
        } else {
            if (wstyle->xftsize < 2.0) wstyle->xftsize = 2.0;
            if (wstyle->xftsize > 100.0) wstyle->xftsize = 100.0;
        }
    } else
        wstyle->xftsize = 0.0;
    
    if (XrmGetResource(database, "menu.frame.xftfontsize",
                       "Menu.Frame.XftFontSize", &value_type, &value)) {
        if (! (mstyle->f_xftsize = strtod(value.addr, 0))) {
            mstyle->f_xftsize = wstyle->xftsize;
        } else {
            if (mstyle->f_xftsize < 2.0) mstyle->f_xftsize = 2.0;
            if (mstyle->f_xftsize > 100.0) mstyle->f_xftsize = 100.0;
        }
    } else
        mstyle->f_xftsize = wstyle->xftsize;

    if (XrmGetResource(database, "menu.title.xftfontsize",
                       "Menu.Title.XftFontSize", &value_type, &value)) {
        if (! (mstyle->t_xftsize = strtod(value.addr, 0))) {
            mstyle->t_xftsize = mstyle->f_xftsize;
        } else {
            if (mstyle->t_xftsize < 2.0) mstyle->t_xftsize = 2.0;
            if (mstyle->t_xftsize > 100.0) mstyle->t_xftsize = 100.0;
        }
    } else
        mstyle->t_xftsize = mstyle->f_xftsize;

    if (XrmGetResource(database, "menu.bullet.xftfontsize",
                       "Menu.Bullet.XftFontSize", &value_type, &value)) {
        if (! (mstyle->b_xftsize = strtod(value.addr, 0))) {
            mstyle->b_xftsize = mstyle->f_xftsize;
        } else {
            if (mstyle->b_xftsize < 2.0) mstyle->b_xftsize = 2.0;
            if (mstyle->b_xftsize > 100.0) mstyle->b_xftsize = 100.0;
        }
    } else
        mstyle->b_xftsize = mstyle->f_xftsize;

    if (XrmGetResource(database, "menu.checkbox.true.xftfontsize",
                       "Menu.Checkbox.True.XftFontSize", &value_type,
                       &value)) {
        if (! (mstyle->ct_xftsize = strtod(value.addr, 0))) {
            mstyle->ct_xftsize = mstyle->ct_xftsize;
        } else {
            if (mstyle->ct_xftsize < 2.0) mstyle->ct_xftsize = 2.0;
            if (mstyle->ct_xftsize > 100.0) mstyle->ct_xftsize = 100.0;
        }
    } else
        mstyle->ct_xftsize = mstyle->f_xftsize;

    if (XrmGetResource(database, "menu.checkbox.false.xftfontsize",
                       "Menu.Checkbox.False.XftFontSize", &value_type,
                       &value)) {
        if (! (mstyle->cf_xftsize = strtod(value.addr, 0))) {
            mstyle->cf_xftsize = mstyle->cf_xftsize;
        } else {
            if (mstyle->cf_xftsize < 2.0) mstyle->cf_xftsize = 2.0;
            if (mstyle->cf_xftsize > 100.0) mstyle->cf_xftsize = 100.0;
        }
    } else
        mstyle->cf_xftsize = mstyle->ct_xftsize;
    
    ReadDatabaseFont("window.xftfont", "Window.xftFont",
                     &wstyle->xftfontname, "arial");
    ReadDatabaseFont("menu.frame.xftfont", "Menu.Frame.xftFont",
                     &mstyle->f_xftfontname, wstyle->xftfontname);
    ReadDatabaseFont("menu.title.xftfont", "Menu.Title.xftFont",
                     &mstyle->t_xftfontname, mstyle->f_xftfontname);
    ReadDatabaseFont("menu.bullet.xftfont", "Menu.Bullet.xftFont",
                     &mstyle->b_xftfontname, mstyle->f_xftfontname);
    ReadDatabaseFont("menu.checkbox.true.xftfont",
                     "Menu.Checkbox.True.xftFont",
                     &mstyle->ct_xftfontname, mstyle->f_xftfontname);
    ReadDatabaseFont("menu.checkbox.false.xftfont",
                     "Menu.Checkbox.False.xftFont",
                     &mstyle->cf_xftfontname, mstyle->f_xftfontname);

#endif // XFT
    ReadDatabaseTexture("window.title.focus", "Window.Title.Focus",
                        &wstyle->t_focus, WhitePixel(display, screen), ic);
    ReadDatabaseTexture("window.title.unfocus", "Window.Title.Unfocus",
                        &wstyle->t_unfocus, BlackPixel(display, screen), ic);
    ReadDatabaseTexture("window.label.focus", "Window.Label.Focus",
                        &wstyle->l_focus, WhitePixel(display, screen), ic);
    ReadDatabaseTexture("window.label.unfocus", "Window.Label.Unfocus",
                        &wstyle->l_unfocus, BlackPixel(display, screen), ic);
    ReadDatabaseTexture("window.handle.focus", "Window.Handle.Focus",
                        &wstyle->h_focus, WhitePixel(display, screen), ic);
    ReadDatabaseTexture("window.handle.unfocus", "Window.Handle.Unfocus",
                        &wstyle->h_unfocus, BlackPixel(display, screen), ic);
    ReadDatabaseTexture("window.grip.focus", "Window.Grip.Focus",
                        &wstyle->g_focus, WhitePixel(display, screen), ic);
    ReadDatabaseTexture("window.grip.unfocus", "Window.Grip.Unfocus",
                        &wstyle->g_unfocus, BlackPixel(display, screen), ic);
    ReadDatabaseTexture("window.button.focus", "Window.Button.Focus",
                        &wstyle->b_focus, WhitePixel(display, screen), ic);
    ReadDatabaseTexture("window.button.unfocus", "Window.Button.Unfocus",
                        &wstyle->b_unfocus, BlackPixel(display, screen), ic);
    ReadDatabaseTexture("window.button.pressed", "Window.Button.Pressed",
                        &wstyle->b_pressed, BlackPixel(display, screen), ic);
    ReadDatabaseColor("window.label.focus.textColor",
                      "Window.Label.Focus.TextColor",
                      &wstyle->l_text_focus, BlackPixel(display, screen), ic);
    ReadDatabaseColor("window.label.unfocus.textColor",
                      "Window.Label.Unfocus.TextColor",
                      &wstyle->l_text_unfocus, WhitePixel(display, screen),
                      ic);
    ReadDatabaseColor("window.button.focus.picColor",
                      "Window.Button.Focus.PicColor",
                      &wstyle->b_pic_focus, BlackPixel(display, screen), ic);
    ReadDatabaseColor("window.button.unfocus.picColor",
                      "Window.Button.Unfocus.PicColor",
                      &wstyle->b_pic_unfocus, WhitePixel(display, screen),
                      ic);
    ReadDatabaseColor("window.button.hilite.picColor",
                      "Window.Button.Hilite.PicColor",
                      &wstyle->b_pic_hilite, wstyle->b_pic_focus.getPixel(),
                      ic);
    
    if (XrmGetResource(database, "window.justify", "Window.Justify",
                       &value_type, &value)) {
        if (strstr(value.addr, "right") || strstr(value.addr, "Right"))
            wstyle->justify = RightJustify;
        else if (strstr(value.addr, "center") || strstr(value.addr, "Center"))
            wstyle->justify = CenterJustify;
        else
            wstyle->justify = LeftJustify;
    } else
        wstyle->justify = LeftJustify;

    ReadDatabaseTexture("menu.frame", "Menu.Frame",
                        &mstyle->back_frame, BlackPixel(display, screen), ic);
    ReadDatabaseTexture("menu.hilite", "Menu.Hilite",
                        &mstyle->hilite, WhitePixel(display, screen), ic);
    ReadDatabaseTexture("menu.title", "Menu.Title",
                        &mstyle->title, WhitePixel(display, screen), ic);

    ReadDatabaseColor("menu.frame.textColor", "Menu.Frame.TextColor",
                      &mstyle->f_text, WhitePixel(display, screen), ic);
    ReadDatabaseColor("menu.hilite.textColor", "Menu.Hilite.TextColor",
                      &mstyle->f_hilite_text, BlackPixel(display, screen), ic);
    ReadDatabaseColor("menu.title.textColor", "Menu.Title.TextColor",
                      &mstyle->t_text, BlackPixel(display, screen), ic);

    if (XrmGetResource(database, "menu.justify", "Menu.Justify",
                       &value_type, &value)) {
        if (strstr(value.addr, "right") || strstr(value.addr, "Right")) {
            mstyle->f_justify = RightJustify;
            mstyle->t_justify = RightJustify;
        }
        else if (strstr(value.addr, "center") ||
                 strstr(value.addr, "Center")) {
            mstyle->f_justify = CenterJustify;
            mstyle->t_justify = CenterJustify;
        }
        else {
            mstyle->f_justify = LeftJustify;
            mstyle->t_justify = LeftJustify;
        }
    } else {
        mstyle->f_justify = LeftJustify;
        mstyle->t_justify = LeftJustify;
    }
    
    if (XrmGetResource(database, "menu.frame.justify", "Menu.Frame.Justify",
                       &value_type, &value)) {
        if (strstr(value.addr, "right") || strstr(value.addr, "Right"))
            mstyle->f_justify = RightJustify;
        else if (strstr(value.addr, "center") || strstr(value.addr, "Center"))
            mstyle->f_justify = CenterJustify;
        else
            mstyle->f_justify = LeftJustify;
    }
    
    if (XrmGetResource(database, "menu.title.justify", "Menu.Title.Justify",
                       &value_type, &value)) {
        if (strstr(value.addr, "right") || strstr(value.addr, "Right"))
            mstyle->t_justify = RightJustify;
        else if (strstr(value.addr, "center") || strstr(value.addr, "Center"))
            mstyle->t_justify = CenterJustify;
        else
            mstyle->t_justify = LeftJustify;
    }

    char *look_tmp;
    unsigned int ch;
    if (XrmGetResource(database, "menu.bullet.look", "Menu.Bullet.Look",
                       &value_type, &value)) {
        if (sscanf(value.addr, "'%u'", &ch) != 1) {
            mstyle->bullet = wastrdup(value.addr);
        } else {
            look_tmp = new char[2];
            sprintf(look_tmp, "%c", ch);
            mstyle->bullet = look_tmp;
        }
    } else {
        look_tmp = new char[2];
        sprintf(look_tmp, ">");
        mstyle->bullet = look_tmp;
    }

    if (XrmGetResource(database, "menu.checkbox.true.look",
                       "Menu.Checkbox.True.Look", &value_type, &value)) {
        if (sscanf(value.addr, "'%u'", &ch) != 1) {
            mstyle->checkbox_true = wastrdup(value.addr);
        } else {
            look_tmp = new char[2];
            sprintf(look_tmp, "%c", ch);
            mstyle->checkbox_true = look_tmp;
        }
    } else
        mstyle->checkbox_true = wastrdup("[x]");

    if (XrmGetResource(database, "menu.checkbox.false.look",
                       "Menu.Checkbox.False.Look", &value_type, &value)) {
        if (sscanf(value.addr, "'%u'", &ch) != 1) {
            mstyle->checkbox_false = wastrdup(value.addr);
        } else {
            look_tmp = new char[2];
            sprintf(look_tmp, "%c", ch);
            mstyle->checkbox_false = look_tmp;
        }
    } else
        mstyle->checkbox_false = wastrdup("[ ]");

    
    ReadDatabaseColor("borderColor", "BorderColor",
                      &wstyle->border_color, BlackPixel(display, screen), ic);
    mstyle->border_color = wstyle->border_color;

    ReadDatabaseColor("outlineColor", "OutlineColor",
                      &wstyle->outline_color, WhitePixel(display, screen), ic);
    mstyle->border_color = wstyle->border_color;
    
    if (XrmGetResource(database, "handleWidth", "HandleWidth", &value_type,
                       &value)) {
        if (sscanf(value.addr, "%u", &wstyle->handle_width) != 1 ||
            wstyle->handle_width > 6)
            wstyle->handle_width = 6;
    } else
        wstyle->handle_width = 6;
    
    if (XrmGetResource(database, "borderWidth", "BorderWidth", &value_type,
                       &value)) {
        if (sscanf(value.addr, "%u", &wstyle->border_width) != 1)
            wstyle->border_width = 1;
    } else
        wstyle->border_width = 1;

    mstyle->border_width = wstyle->border_width;

    if (XrmGetResource(database, "menu.borderWidth", "Menu.BorderWidth",
                       &value_type, &value))
        sscanf(value.addr, "%u", &mstyle->border_width);
    
    if (XrmGetResource(database, "window.title.height", "window.title.height",
                       &value_type, &value)) {
        if (sscanf(value.addr, "%u", &wstyle->title_height) != 1)
            wstyle->title_height = 0;
        else if (wstyle->title_height > 50)
            wstyle->title_height = 50;   
    } else
        wstyle->title_height = 0;
    
    if (XrmGetResource(database, "menu.title.height", "menu.title.height",
                       &value_type, &value)) {
        if (sscanf(value.addr, "%u", &mstyle->title_height) != 1)
            mstyle->title_height = 0;
        else if (mstyle->title_height > 50)
            mstyle->title_height = 50;   
    } else
        mstyle->title_height = 0;
    
    if (XrmGetResource(database, "menu.item.height", "menu.item.height",
                       &value_type, &value)) {
        if (sscanf(value.addr, "%u", &mstyle->item_height) != 1)
            mstyle->item_height = mstyle->title_height;
        else if (mstyle->item_height > 50)
            mstyle->item_height = 50;
    } else
        mstyle->item_height = mstyle->title_height;
    
    if (XrmGetResource(database, "rootCommand", "RootCommand",
                       &value_type, &value))
        waexec(value.addr, scrn->displaystring);

    int dock_num = 0;
    char rc_name[35], rc_class[35];
    list<DockStyle *>::iterator dit = dockstyles->begin();
    for (; dit != dockstyles->end(); ++dit, ++dock_num) {
        (*dit)->style.border_color = wstyle->border_color;
        (*dit)->style.texture = wstyle->t_focus;
        (*dit)->style.border_width = wstyle->border_width;
        sprintf(rc_name, "dockappholder.dock%d.frame", dock_num);
        sprintf(rc_class, "Dockappholder.Dock%d.frame", dock_num);        
        if (XrmGetResource(database, rc_name, rc_class, &value_type, &value))
            ReadDatabaseTexture(rc_name, rc_class, &(*dit)->style.texture,
                                WhitePixel(display, screen), ic);
        sprintf(rc_name, "dockappholder.dock%d.borderWidth", dock_num);
        sprintf(rc_class, "Dockappholder.Dock%d.BorderWidth", dock_num);
        if (XrmGetResource(database, rc_name, rc_class, &value_type, &value)) {
            if (sscanf(value.addr, "%u", &(*dit)->style.border_width) != 1)
                (*dit)->style.border_width = wstyle->border_width;
        }
        sprintf(rc_name, "dockappholder.dock%d.borderColor", dock_num);
        sprintf(rc_class, "Dockappholder.Dock%d.BorderColor", dock_num);
        if (XrmGetResource(database, rc_name, rc_class, &value_type, &value));
            ReadDatabaseColor(rc_name, rc_class, &(*dit)->style.border_color,
                              BlackPixel(display, screen), ic);
    }
    XrmDestroyDatabase(database);
}

/**
 * @fn    LoadMenus(void)
 * @brief Reads menu file
 *
 * Creates menus by parsing the menu file.
 */
void ResourceHandler::LoadMenus(void) {
    FILE *file;
    char *s, line[1024];
    int i;
    WaMenu *menu;
    WaMenuItem *m;

    linenr = 0;
    if (! (file = fopen(menu_file, "r"))) {
        WARNING << "can't open menufile \"" << menu_file << 
		"\" for reading" << endl;
        return;
    }
    while (fgets(line, 1024, file)) {
        linenr++;
        for (i = 0; line[i] == ' ' || line[i] == '\t'; i++);
        if (line[i] == '\n') continue;
        if (line[i] == '#') continue;
        if (line[i] == '!') continue;
        
        if ((s = strwithin(line, '[', ']'))) {
            if (! strcasecmp("start", s)) {
                delete [] s;
                if ((s = strwithin(line, '(', ')')))
                    ParseMenu(new WaMenu(s), file);
                else
                    WARNING << "failed to find menu name at line " <<
                        linenr << endl;
            }
            else if (! strcasecmp("begin", s)) {
                delete [] s;
                if ((s = strwithin(line, '(', ')'))) {
                    menu = new WaMenu(s);
                    m = new WaMenuItem(s);
                    m->type = MenuTitleType;
                    menu->AddItem(m);
                    ParseMenu(menu, file);
                } else {
                    delete [] s;
                    WARNING << "failed to find menu name at line " <<
                        linenr << endl;
                }
            } else {
                delete [] s;
                WARNING << "missing [start] or [begin] statement at line " <<
                    linenr << endl;
            }
        }
        else {
            delete [] s;
            WARNING << "missing [start] or [begin] statement at line " <<
                linenr << endl;
        }
    }
    fclose(file);
}

/**
 * @fn    LoadActions(void)
 * @brief Reads action file
 *
 * Creates action lists by parsing the action file.
 */
void ResourceHandler::LoadActions(void) {
    FILE *file;
    int i, i2, i3;
    bool cmd;
    char *buffer = new char[8192];
    char *buffer2 = new char[8192];
    char tmp_ch;
    char *str;
    WaActionExtList *ext_list;
    list<Define *> *defs = new list<Define *>;
    
    if (! (file = fopen(action_file, "r"))) {
        WARNING << "can't open action file \"" << action_file << 
            "\" for reading" << endl;
        exit(1);
    }
    for (;;) {
        for (i = 0; (buffer[i] = fgetc(file)) != EOF &&
                 buffer[i] != '{'; i++) {
            if (buffer[i] == '#' || buffer[i] == '!') {
                i--;
                while ((tmp_ch = fgetc(file)) != EOF && tmp_ch != '\n');
            }
        }
        if (buffer[i] == EOF) {
            fclose(file);
            LISTCLEAR(defs);
            LISTCLEAR(wacts);
            LISTCLEAR(racts);
            LISTCLEAR(macts);
            LISTCLEAR(types);
            LISTCLEAR(bdetails);
            LISTCLEAR(mods);
            delete buffer;
            delete buffer2;
            return;
        }
        str = strtrim(buffer);
        switch (buffer[i]) {
            case '\n':
                i = 0;
                break;
            case '{':
                buffer[i] = '\0';
                cmd = false;
                for (i2 = 0; (buffer2[i2] = fgetc(file)) != EOF &&
                         (buffer2[i2] != '}' || cmd); i2++) {
                    if (buffer2[i2] == '{') cmd = true;
                    if (buffer2[i2] == '}') cmd = false;
                    if (buffer2[i2] == '#' || buffer[i] == '!') {
                        i2--;
                        while ((tmp_ch = fgetc(file)) != EOF &&
                               tmp_ch != '\n');
                    }
                }
                if (buffer2[i2] == EOF) ERROR << "missing '}'" << endl;
                buffer2[i2] = '\0';               
                if (! strncasecmp(str, "DEF", 3)) {
                    str = strtrim(str + 3);                   
                    defs->push_front(new Define(wastrdup(str),
                                                wastrdup(strtrim(buffer2))));
                }
                else {                  
                    str = strtrim(str);
                    if (! strcasecmp(str, "root")) {
                        ReadActions(buffer2, defs, racts, rootacts);
                    }
                    else if (! strcasecmp(str, "westedge")) {
                        ReadActions(buffer2, defs, racts, weacts);
                    }
                    else if (! strcasecmp(str, "eastedge")) {
                        ReadActions(buffer2, defs, racts, eeacts);
                    }
                    else if (! strcasecmp(str, "northedge")) {
                        ReadActions(buffer2, defs, racts, neacts);
                    }
                    else if (! strcasecmp(str, "southedge")) {
                        ReadActions(buffer2, defs, racts, seacts);
                    }
                    else if (! strcasecmp(str, "menu.title")) {
                        ReadActions(buffer2, defs, macts, mtacts);
                    }
                    else if (! strcasecmp(str, "menu.item")) {
                        ReadActions(buffer2, defs, macts, miacts);
                    }
                    else if (! strcasecmp(str, "menu.sub")) {
                        ReadActions(buffer2, defs, macts, msacts);
                    }
                    else if (! strcasecmp(str, "menu.checkbox")) {
                        ReadActions(buffer2, defs, macts, mcbacts);
                    }
                    else {
                        ext_list = NULL;
                        if (! strncasecmp(str, "class", 5)) {
                            for (i3 = 5; str[i3] != ']' && str[i3] != '\0';
                                 i3++);
                            if (str[i3] == '\0') {
                                WARNING << "missing ']'" << endl;
                                break;
                            }
                            str[i3] = '\0';
                            ext_list = new WaActionExtList(NULL,
                                                           wastrdup(str + 6),
                                                           NULL);
                            str = str + i3 + 1;
                            ReadActions(buffer2, defs, wacts, &ext_list->list);
                        }
                        else if (! strncasecmp(str, "name", 4)) {
                            for (i3 = 4; str[i3] != ']' && str[i3] != '\0';
                                 i3++);
                            if (str[i3] == '\0') {
                                WARNING << "missing ']'" << endl;
                                break;
                            }
                            str[i3] = '\0';
                            ext_list = new WaActionExtList(wastrdup(str + 5),
                                                           NULL,
                                                           NULL);
                            str = str + i3 + 1;
                            ReadActions(buffer2, defs, wacts, &ext_list->list);
                        }
                        else if (! strncasecmp(str, "title", 5)) {
                            for (i3 = 5; str[i3] != ']' && str[i3] != '\0';
                                 i3++);
                            if (str[i3] == '\0') {
                                WARNING << "missing ']'" << endl;
                                break;
                            }
                            str[i3] = '\0';
                            ext_list = new WaActionExtList(NULL,
                                                           NULL,
                                                           wastrdup(str + 6));
                            str = str + i3 + 1;
                            ReadActions(buffer2, defs, wacts, &ext_list->list);
                        }
                        else if (! strncasecmp(str, "window", 6)) {
                            str = str + 6;
                        }
                        else {
                            WARNING << "unknown window type" << endl;
                            break;
                        }
                        if (! strcasecmp(str, ".frame")) {
                            if (ext_list) ext_frameacts.push_back(ext_list);
                            else ReadActions(buffer2, defs, wacts, frameacts);
                        }
                        else if (! strcasecmp(str, ".title")) {
                            if (ext_list) ext_titleacts.push_back(ext_list);
                            else ReadActions(buffer2, defs, wacts, titleacts);
                        }
                        else if (! strcasecmp(str, ".label")) {
                            if (ext_list) ext_labelacts.push_back(ext_list);
                            else ReadActions(buffer2, defs, wacts, labelacts);
                        }
                        else if (! strcasecmp(str, ".handle")) {
                            if (ext_list) ext_handleacts.push_back(ext_list);
                            else ReadActions(buffer2, defs, wacts, handleacts);
                        }
                        else if (! strcasecmp(str, ".activeclient")) {
                            if (ext_list) ext_awinacts.push_back(ext_list);
                            else ReadActions(buffer2, defs, wacts, awinacts);
                        }
                        else if (! strcasecmp(str, ".passiveclient")) {
                            if (ext_list) ext_pwinacts.push_back(ext_list);
                            else ReadActions(buffer2, defs, wacts, pwinacts);
                        }
                        else if (! strcasecmp(str, ".closebutton")) {
                            if (ext_list) ext_cbacts.push_back(ext_list);
                            else ReadActions(buffer2, defs, wacts, cbacts);
                        }
                        else if (! strcasecmp(str, ".iconifybutton")) {
                            if (ext_list) ext_ibacts.push_back(ext_list);
                            else ReadActions(buffer2, defs, wacts, ibacts);
                        }
                        else if (! strcasecmp(str, ".maximizebutton")) {
                            if (ext_list) ext_mbacts.push_back(ext_list);
                            else ReadActions(buffer2, defs, wacts, mbacts);
                        }
                        else if (! strcasecmp(str, ".leftgrip")) {
                            if (ext_list) ext_lgacts.push_back(ext_list);
                            else ReadActions(buffer2, defs, wacts, lgacts);
                        }
                        else if (! strcasecmp(str, ".rightgrip")) {
                            if (ext_list) ext_rgacts.push_back(ext_list);
                            else ReadActions(buffer2, defs, wacts, rgacts);
                        }
                        else {
                            WARNING << "unknown window" << endl;
                            break;
                        }
                    }
                }
                break;
        }
    }
}

/**
 * @fn    ReadActions(char *s,
 *                    list<Define *> *defs,
 *                    list<StrComp *> *comp,
 *                    list<WaAction *> *insert)
 * @brief Parses a block of actions
 *
 * Parses a block of action lines. All defines are replaced with actual lines
 * and then parsed.
 *
 * @param comp List with available actions
 * @param insert List to insert action in
 */
void ResourceHandler::ReadActions(char *s,
                                  list<Define *> *defs,
                                  list<StrComp *> *comp,
                                  list<WaAction *> *insert) {
    bool match, ret = false;
    char tmp[8192];
    char *ts;
    int i;
    list<Define *>::iterator it;
    for (;;) {       
        for (i = 0; s[i] != ',' && s[i] != '\0'; i++);       
        if (s[i] == '\0') {
            s[i + 1] = '\0';
            ret = true;
        }
        else s[i] = '\0';
        ts = strtrim(s);
        if (strlen(ts) == 0) {
            s = s + i + 1;           
            if (ret) return;
            continue;
        }
        match = false;
        for (it = defs->begin(); it != defs->end(); ++it) {
            if (! strcasecmp(ts, (*it)->name)) {
                if (! ret) {
                    s[i] = ',';
                    sprintf(tmp, "%s", s + i);                  
                }
                else tmp[0] = '\0'; 
                sprintf(s, "%s%s", (*it)->value, tmp);              
                ret = false;
                match = true;
                break;
            }
        }
        if (! match) {
            ParseAction(ts, comp, insert);           
            s = s + i + 1;
        }
        if (ret) return;
    }
}

/**
 * @fn    ReadDatabaseColor(char *rname, char *rclass,
 *                          WaColor *color,
 *                          unsigned long default_pixel,
 *                          WaImageControl *ic)
 * @brief Reads a color
 *
 * Reads a color from resource database.
 *
 * @param rname Resource name to use
 * @param rclass Resource class name to use
 * @param default_pixel Pixel value to use if resource doesn't exist
 * @param ic WaImageControl to use for parsing color
 */
void ResourceHandler::ReadDatabaseColor(char *rname, char *rclass,
                                        WaColor *color,
                                        unsigned long default_pixel,
                                        WaImageControl *ic) {
    XrmValue value;
    char *value_type;
    
    if (XrmGetResource(database, rname, rclass, &value_type,
                       &value)) {
        ic->parseColor(color, value.addr);
    } else {
        ic->parseColor(color);
        color->setPixel(default_pixel);
    }
    
#ifdef XFT
    color->createXftColor(display, wascreen->id, wascreen->colormap);
#endif // XFT
    
}

/**
 * @fn    ReadDatabaseTexture(char *rname, char *rclass,
 *                            WaColor *color,
 *                            unsigned long default_pixel,
 *                            WaImageControl *ic, WaScreen *scrn)
 * @brief Reads a texture
 *
 * Reads a texture from resource database.
 *
 * @param rname Resource name to use
 * @param rclass Resource class name to use
 * @param default_pixel Pixel value to use if resource doesn't exist
 * @param ic WaImageControl to use for parsing color
 */
void ResourceHandler::ReadDatabaseTexture(char *rname, char *rclass,
                                          WaTexture *texture,
                                          unsigned long default_pixel,
                                          WaImageControl *ic) {
    
    XrmValue value;
    char *value_type;

    Colormap colormap = wascreen->colormap;

    if (XrmGetResource(database, rname, rclass, &value_type,
                       &value))
        ic->parseTexture(texture, value.addr);
    else
        texture->setTexture(WaImage_Solid | WaImage_Flat);

    int clen = strlen(rclass) + 32, nlen = strlen(rname) + 32;
    char *colorclass = new char[clen], *colorname = new char[nlen];

#ifdef PIXMAP
    if (texture->getTexture() & WaImage_Pixmap) {
        int clen = strlen(rclass) + 10, nlen = strlen(rname) + 10;
        char *pixmapclass = new char[clen], *pixmapname = new char[nlen];
        char pixmap_path[1024];
        
        Imlib_Border bd;
        Imlib_Image image = NULL;

        sprintf(pixmapclass, "%s.Pixmap", rclass);
        sprintf(pixmapname,  "%s.pixmap", rname);
        if (XrmGetResource(database, pixmapname, pixmapclass, &value_type,
                           &value)) {
            if (strstr(value.addr, "/")) {
                if (! (image = imlib_load_image(value.addr)))
                WARNING << "failed loading image \"" << value.addr << "\"\n";
            }
            else {
                sprintf(pixmap_path, "%s/%s", style_file, value.addr);
                if (! (image = imlib_load_image(pixmap_path))) {
                    WARNING << "failed loading image \"" << value.addr <<
                        "\"\n";
                }
            }
        }
        if (image) {
            texture->setPixmap(image);
            if (texture->getTexture() & WaImage_Stretch) {
                imlib_context_set_image(image);
                bd.left = imlib_image_get_width() / 2;
                bd.right = imlib_image_get_width() - bd.left - 1;
                bd.top = imlib_image_get_height() / 2;
                bd.bottom = imlib_image_get_height() - bd.top - 1;
                imlib_image_set_border(&bd);
            }
        }
        else
            texture->setTexture(WaImage_Solid | WaImage_Flat);
        
        delete [] pixmapclass;
        delete [] pixmapname;
    }
#endif // PIXMAP
        
    if (texture->getTexture() & WaImage_Solid) {
        
        sprintf(colorclass, "%s.Color", rclass);
        sprintf(colorname,  "%s.color", rname);
        
        ReadDatabaseColor(colorname, colorclass, texture->getColor(),
                          default_pixel, ic);

#ifdef INTERLACE
        sprintf(colorclass, "%s.ColorTo", rclass);
        sprintf(colorname,  "%s.colorTo", rname);

        ReadDatabaseColor(colorname, colorclass, texture->getColorTo(),
                          default_pixel, ic);
#endif // INTERLACE        
        
        if (texture->getColor()->isAllocated() &&
            (!(texture->getTexture() & WaImage_Flat))) {
            XColor xcol;
        
            xcol.red = (unsigned int) (texture->getColor()->getRed() +
                                       (texture->getColor()->getRed() >> 1));
            if (xcol.red >= 0xff) xcol.red = 0xffff;
            else xcol.red *= 0xff;
            xcol.green = (unsigned int) (texture->getColor()->getGreen() +
                                         (texture->getColor()->getGreen() >>
                                          1));
            if (xcol.green >= 0xff) xcol.green = 0xffff;
            else xcol.green *= 0xff;
            xcol.blue = (unsigned int) (texture->getColor()->getBlue() +
                                        (texture->getColor()->getBlue() >> 1));
            if (xcol.blue >= 0xff) xcol.blue = 0xffff;
            else xcol.blue *= 0xff;

            if (! XAllocColor(display, colormap, &xcol))
                xcol.pixel = 0;
        
            texture->getHiColor()->setPixel(xcol.pixel);
        
            xcol.red =
                (unsigned int) ((texture->getColor()->getRed() >> 2) +
                                (texture->getColor()->getRed() >> 1)) * 0xff;
            xcol.green =
                (unsigned int) ((texture->getColor()->getGreen() >> 2) +
                                (texture->getColor()->getGreen() >> 1)) * 0xff;
            xcol.blue =
                (unsigned int) ((texture->getColor()->getBlue() >> 2) +
                                (texture->getColor()->getBlue() >> 1)) * 0xff;
        
            if (! XAllocColor(display, colormap, &xcol))
                xcol.pixel = 0;
        
            texture->getLoColor()->setPixel(xcol.pixel);
        }
    } else if (texture->getTexture() & WaImage_Gradient) {
        int clen = strlen(rclass) + 10, nlen = strlen(rname) + 10;
        char *colortoclass = new char[clen], *colortoname = new char[nlen];
        
        sprintf(colorclass, "%s.Color", rclass);
        sprintf(colorname,  "%s.color", rname);
        
        sprintf(colortoclass, "%s.ColorTo", rclass);
        sprintf(colortoname,  "%s.colorTo", rname);

        ReadDatabaseColor(colorname, colorclass, texture->getColor(),
                          default_pixel, ic);
        ReadDatabaseColor(colortoname, colortoclass, texture->getColorTo(),
                          default_pixel, ic);

        delete [] colortoclass;
        delete [] colortoname;
    }

#ifdef XFT
    int opacity;
    XRenderPictFormat *xformat;
    XRenderPictFormat Rpf;
    XRenderPictureAttributes Rpa;
    XRenderColor clr;
    Pixmap alphaPixmap, solidPixmap;
    Picture alphaPicture, solidPicture;

    sprintf(colorclass, "%s.Opacity", rclass);
    sprintf(colorname,  "%s.opacity", rname);

    if (XrmGetResource(database, colorname, colorclass, &value_type, 
                       &value))
        opacity = atoi(value.addr);
    else
        opacity = 0;

    opacity = (opacity * 255) / 100;
    if (opacity > 255) opacity = 255;
    else if (opacity < 0) opacity = 0;

    texture->setOpacity(opacity);

    if (opacity > 0 && opacity < 255) {
        clr.alpha = ((unsigned short) (255 * opacity) << 8);
        Rpf.type  = PictTypeDirect;
        Rpf.depth = 8;
        Rpf.direct.alphaMask = 0xff;
        Rpa.repeat = True;
        xformat = XRenderFindFormat(wascreen->display, PictFormatType |
                                    PictFormatDepth | PictFormatAlphaMask,
                                    &Rpf, 0);
        alphaPixmap = XCreatePixmap(wascreen->display, wascreen->id, 1, 1, 8);
        alphaPicture = XRenderCreatePicture(wascreen->display, alphaPixmap,
                                            xformat, CPRepeat, &Rpa);
        XRenderFillRectangle(wascreen->display, PictOpSrc, alphaPicture, &clr,
                             0, 0, 1, 1);
        texture->setAlphaPicture(alphaPicture);
        XFreePixmap(wascreen->display, alphaPixmap);
        if (texture->getTexture() == (WaImage_Solid | WaImage_Flat)) {
            Rpf.depth = wascreen->screen_depth;
            xformat = XRenderFindFormat(wascreen->display, PictFormatType |
                                        PictFormatDepth,
                                        &Rpf, 0);
            solidPixmap = XCreatePixmap(wascreen->display, wascreen->id, 1, 1,
                                        wascreen->screen_depth);
            solidPicture = XRenderCreatePicture(wascreen->display, solidPixmap,
                                                xformat, CPRepeat, &Rpa);
            XRenderFillRectangle(wascreen->display, PictOpSrc, solidPicture,
                                 &texture->getColor()->getXftColor()->color,
                                 0, 0, 1, 1);
            texture->setSolidPicture(solidPicture);
            XFreePixmap(wascreen->display, solidPixmap);
        }
    }
#endif // XFT
        
    delete [] colorclass;
    delete [] colorname;
}

/**
 * @fn    ReadDatabaseFont(char *rname, char *rclass,
 *                         char **fontname, char *defaultfont)
 * @brief Reads a font
 *
 * Reads a font from resource database.
 *
 * @param rname Resource name to use
 * @param rclass Resource class name to use
 * @param fontname For storing font name
 * @param defaultfont Font to use if resource doesn't exist  
 */
void ResourceHandler::ReadDatabaseFont(char *rname, char *rclass,
                                       char **fontname, char *defaultfont) {
    XrmValue value;
    char *value_type;
    
    if (XrmGetResource(database, rname, rclass, &value_type, &value)) {
        *fontname = wastrdup(value.addr);
    } else
        *fontname = wastrdup(defaultfont);
}

/**
 * @fn    ParseAction(const char *s, list<StrComp *> *comp,
 *                    list<WaAction *> *insert)
 * @brief Parses an action line
 *
 * Parses an action line into an action object and inserts it in action list.
 *
 * @param s Action line to parse
 * @param comp List with available actions
 * @param insert List to insert action in
 */
void ResourceHandler::ParseAction(const char *s, list<StrComp *> *comp,
                                  list<WaAction *> *insert) {
    char *line, *token, *par, *tmp_par;
    int i, detail, mod;
    WaAction *act_tmp;
    KeySym keysym;
    list<StrComp *>::iterator it;
    
    act_tmp = new WaAction;
    act_tmp->replay = false;
    
    line = wastrdup((char *) s);
    
    detail = strchr(line, '=') ? 1: 0;
    mod    = strchr(line, '&') ? 1: 0;
    token  = strtok(line, ":");
    token  = strtrim(token);
    if (*token == '*') {
        act_tmp->replay = true;
        token++;
    }
    
    tmp_par = wastrdup(token);
    par = tmp_par;
    
    act_tmp->exec = NULL;
    act_tmp->param = NULL;
    for (; *par != '(' && *par != '\0'; par++);
    if (*(par++) == '(') {
        for (i = 0; par[i] != ')'; i++)
            if (par[i] == '\0') {
                WARNING << "missing \")\" in resource line \"" << s << "\"" 
                        << endl;
                delete act_tmp;
                delete [] line;
                return;
            }
        if (*par == '\0' || *par == ')') {
            if (! strncasecmp(token, "menu", 4)) {
                WARNING "\"" << token << "\" action must have a menu as" <<
                    " parameter" << endl;
                delete act_tmp;
                delete [] line;
                return;
            }
        }
        if (strlen(par)) {
            par[i] = '\0';
            act_tmp->param = wastrdup(par);
        }
        for (i = 0; token[i] != '('; i++);
        token[i] = '\0';
    }
    else if (! strncasecmp(token, "menu", 4)) {
        WARNING "\"" << token << "\" action must have a menu as" <<
            " parameter" << endl;
        delete act_tmp;
        delete [] line;
        return;
    }
    delete [] tmp_par;
    
    it = comp->begin();
    for (; it != comp->end(); ++it) {
        if ((*it)->Comp(token)) {
            if ((*it)->type & WindowFuncMask)
                act_tmp->winfunc = (*it)->winfunc;
            if ((*it)->type & RootFuncMask)
                act_tmp->rootfunc = (*it)->rootfunc;
            if ((*it)->type & MenuFuncMask)
                act_tmp->menufunc = (*it)->menufunc;
            break;
        }
    }
    if (! *it) {
        if ((s = strwithin(token, '{', '}'))) {
            act_tmp->exec = (char *) s;
        } else {
            WARNING << "\"" << token << "\" unknown action" << endl;
            delete act_tmp;
            delete [] line;
            return;
        }
    }
    
    if (detail) token = strtok(NULL, "=");
    else {
        if (mod) token = strtok(NULL, "&");
        else {
            for (; *token != ' ' && *token != ':'; token++);
            for (; *token == '\0'; token++);
        }
    }
    token = strtrim(token);
    
    it = types->begin();
    for (; it != types->end(); ++it) {
        if ((*it)->Comp(token)) {
            act_tmp->type = (*it)->value;
            break;
        }
    }
    if (! *it) {
        WARNING << "\"" << token << "\" unknown type" << endl;
        delete act_tmp;
        delete [] line;
        return;
    }
    
    act_tmp->detail = 0;
    if (detail) {
        if (mod) token = strtok(NULL, "&");
        else {
            token += strlen(token) + 1;
            for (; *token == '\0'; token++);
        }
        token = strtrim(token);
        if (act_tmp->type == KeyPress || act_tmp->type == KeyRelease) {
            if (! strcasecmp(token, "anykey"))
                act_tmp->detail = 0;
            else {
                if ((keysym = XStringToKeysym(token)) == NoSymbol) {
                    WARNING << "\"" << token << "\" unknown key" << endl;
                    delete act_tmp;
                    delete [] line;
                    return;
                } else
                    act_tmp->detail = XKeysymToKeycode(display, keysym);
            }
        } else if (act_tmp->type == ButtonPress ||
                   act_tmp->type == ButtonRelease ||
                   act_tmp->type == DoubleClick) {
            it = bdetails->begin();
            for (; it != bdetails->end(); ++it) {
                if ((*it)->Comp(token)) {
                    act_tmp->detail = (*it)->value;
                    break;
                }
            }
            if (! *it) {
                WARNING << "\"" << token << "\" unknown detail" << endl;
                delete act_tmp;
                delete [] line;
                return;
            }
        }        
    }

    bool negative;
    act_tmp->mod = act_tmp->nmod = 0;
    if (mod) {
        token += strlen(token) + 1;
        for (; *token == '\0'; token++);
        for (token = strtok(token, "&"); token; token = strtok(NULL, "&")) {
            token = strtrim(token);
            negative = false;
            if (*token == '!') {
                negative = true;
                token = strtrim(token + 1);
            }
            for (it = mods->begin(); it != mods->end(); ++it) {
                if ((*it)->Comp(token)) {
                    if (negative)
                        act_tmp->nmod |= (*it)->value;
                    else
                        act_tmp->mod |= (*it)->value;
                    break;
                }
            }
            if (! *it) {
                WARNING << "\"" << token << "\" unknown modifier" << endl;
                delete act_tmp;
                delete [] line;
                return;
            }
        }
    }    
    delete [] line;
    insert->push_back(act_tmp);
}

/**
 * @fn    ParseMenu(WaMenu *menu, FILE *file)
 * @brief Parses a menu
 *
 * Parses a menu section of the menu file and creates a menu object for the
 * menu. If a [start] or [begin] statement is found when parsing a menu, we
 * make a recursive function call to this function. This makes it possible to
 * to define a submenu within a the menu itself.
 *
 * @param menu Menu to add items to
 * @param file File descriptor for menu file
 */
void ResourceHandler::ParseMenu(WaMenu *menu, FILE *file) {
    char *s, line[8192], *line1 = NULL, *line2 = NULL,
        *par = NULL, *tmp_par = NULL;
    WaMenuItem *m;
    int i, type, cb;
    WaMenu *tmp_menu;
    list<StrComp *>::iterator it;

    WARNING << "newmenu: " << menu->name << endl;
    
    while (fgets(line, 8192, file)) {
        linenr++;
        for (i = 0; line[i] == ' ' || line[i] == '\t'; i++);
        if (line[i] == '\n') continue;
        if (line[i] == '#') continue;
        if (line[i] == '!') continue;

        cb = 0;
        
        if (! (s = strwithin(line, '[', ']'))) {
            WARNING << "failed to find menu item type at line " << 
                linenr << endl;
            continue;
        }
        if (! strcasecmp(s, "start")) {
            delete [] s;
            if ((s = strwithin(line, '(', ')'))) {                
                tmp_menu = new WaMenu(wastrdup(s));                
                ParseMenu(tmp_menu, file);
            } else
                WARNING << "failed to find menu name at line " <<
                    linenr << endl;
            continue;
        }
        else if ((! strcasecmp(s, "submenu")) || (! strcasecmp(s, "begin"))) {
            delete [] s;
            if ((s = strwithin(line, '(', ')'))) {
                m = new WaMenuItem(s);
                m->type = MenuSubType;
                m->func_mask |= MenuSubMask;
                m->func_mask1 |= MenuSubMask;
                m->sub = m->sub1 = wastrdup(s);
                menu->AddItem(m);
                tmp_menu = new WaMenu(wastrdup(s));
                m = new WaMenuItem(wastrdup(s));
                m->type = MenuTitleType;
                tmp_menu->AddItem(m);
                ParseMenu(tmp_menu, file);
            } else
                WARNING << "failed to find menu name at line " <<
                    linenr << endl;
            continue;
        }
        else if (! strcasecmp(s, "restart")) {
            delete [] s;
            if ((s = strwithin(line, '(', ')')))
                m = new WaMenuItem(s);
            else
                m = new WaMenuItem(wastrdup(""));
            m->type = MenuItemType;
            m->func_mask = MenuRFuncMask;
            m->rfunc = &WaScreen::Restart;
            menu->AddItem(m);
            continue;
        }
        else if (! strcasecmp(s, "exit")) {
            delete [] s;
            if ((s = strwithin(line, '(', ')')))
                m = new WaMenuItem(s);
            else
                m = new WaMenuItem(wastrdup(""));
            m->type = MenuItemType;
            m->func_mask = MenuRFuncMask;
            m->rfunc = &WaScreen::Exit;
            menu->AddItem(m);
            continue;
        }
        else if (! strcasecmp(s, "exec")) {
            delete [] s;
            if ((s = strwithin(line, '(', ')')))
                m = new WaMenuItem(s);
            else
                m = new WaMenuItem(wastrdup(""));
            m->type = MenuItemType;
            if ((s = strwithin(line, '{', '}'))) {
                if (*s != '\0') {
                    m->exec = s;
                    m->exec1 = s;
                    m->func_mask |= MenuExecMask;
                    m->func_mask1 |= MenuExecMask;
                }
                else
                    delete [] s;
            }
            menu->AddItem(m);
            continue;
        }
        else if (! strcasecmp(s, "nop")) {
            delete [] s;
            if ((s = strwithin(line, '(', ')')))
                m = new WaMenuItem(s);
            else
                m = new WaMenuItem(wastrdup(""));
            m->type = MenuItemType;
            menu->AddItem(m);
            continue;
        }
        else if (! strcasecmp(s, "end")) {
            delete [] s;
            if (menu->item_list->empty()) {
                WARNING << "no elements in menu \"" << menu->name <<
                    "\"" << endl;
                delete menu;
                return;
            }
            waimea->wamenu_list->push_back(menu);
            return;
        }
        else if (! strncasecmp(s, "checkbox", 8)) {
            if (! strcasecmp(s + 9, "MAXIMIZED")) {
                delete [] s;
                type = MenuCBItemType;
                cb = MaxCBoxType;
            }
            else if (! strcasecmp(s + 9, "SHADED")) {
                delete [] s;
                type = MenuCBItemType;
                cb = ShadeCBoxType;
            }
            else if (! strcasecmp(s + 9, "STICKY")) {
                delete [] s;
                type = MenuCBItemType;
                cb = StickCBoxType;
            }
            else if (! strcasecmp(s + 9, "DECORTITLE")) {
                delete [] s;
                type = MenuCBItemType;
                cb = TitleCBoxType;
            }
            else if (! strcasecmp(s + 9, "DECORHANDLE")) {
                delete [] s;
                type = MenuCBItemType;
                cb = HandleCBoxType;
            }
            else if (! strcasecmp(s + 9, "DECORBORDER")) {
                delete [] s;
                type = MenuCBItemType;
                cb = BorderCBoxType;
            }
            else if (! strcasecmp(s + 9, "DECORALL")) {
                delete [] s;
                type = MenuCBItemType;
                cb = AllCBoxType;
            }
            else if (! strcasecmp(s + 9, "ALWAYSONTOP")) {
                delete [] s;
                type = MenuCBItemType;
                cb = AOTCBoxType;
            }
            else if (! strcasecmp(s + 9, "ALWAYSATBOTTOM")) {
                delete [] s;
                type = MenuCBItemType;
                cb = AABCBoxType;
            }
            else {
                WARNING << "at line " << linenr << ": '"<< s + 9 << "'" <<
                    " unknown checkbox" << endl;
                delete [] s;
                continue;
            }
            for (i = 0; strncasecmp(&line[i], "@TRUE", 5) &&
                     line[i + 5] != '\0'; i++);
            if (line[i + 5] == '\0') {
                WARNING << "at line " << linenr << ": No '@TRUE' linepart" <<
                    " for checkbox item" << endl;
                continue;
            }
            line2 = &line[i + 5];
            for (i = 0; strncasecmp(&line[i], "@FALSE", 6) &&
                     line[i + 6] != '\0'; i++);
            if (line[i + 6] == '\0') {
                WARNING << "at line " << linenr << ": No '@FALSE' linepart" <<
                    " for checkbox item" << endl;
                continue;
            }
            line1 = &line[i + 6];
            for (i = 0; strncasecmp(&line1[i], "@TRUE", 5) &&
                     line1[i + 5] != '\0'; i++);
            if (line1[i + 5] != '\0') line1[i] = '\0';
            for (i = 0; strncasecmp(&line2[i], "@FALSE", 6) &&
                     line2[i + 6] != '\0'; i++);
            if (line2[i + 6] != '\0') line2[i] = '\0';
        }
        else if (! strcasecmp(s, "title")) {
            delete [] s;
            type = MenuTitleType;
        }
        else if (! strcasecmp(s, "item")) {
            delete [] s;
            type = MenuItemType;
        }
        else if (! strcasecmp(s, "sub")) {
            delete [] s;
            type = MenuSubType;
        }
        else {
            delete [] s;
            WARNING << "at line " << linenr << ": [" << s << "]" <<
                " invalid statement" << endl;
            continue;
        }
        if (! cb) line1 = line;
        if (! (s = strwithin(line1, '(', ')')))
            s = wastrdup("");
        m = new WaMenuItem(s);
        m->label1 = m->label;
        m->type = type;
        m->cb = cb;
        if ((s = strwithin(line1, '{', '}'))) {
            if (*s != '\0') {
                m->exec = s;
                m->exec1 = s;
                m->func_mask |= MenuExecMask;
                m->func_mask1 |= MenuExecMask;
            }
        }
        if ((s = strwithin(line1, '<', '>'))) {
            m->sub = s;
            m->sub1 = s;
            m->func_mask |= MenuSubMask;
            m->func_mask1 |= MenuSubMask;
        }
        if ((s = strwithin(line1, '"', '"'))) {
            tmp_par = par = wastrdup(s);
            for (i = 0; *par != '(' && *par != '\0'; par++, i++);
            if (*(par++) == '(') {
                s[i] = '\0';
                for (i = 0; par[i] != ')' && par[i] != '\0'; i++);
                if (par[i] == '\0') {
                    WARNING << "missing \")\" at line " << linenr << endl;
                    delete [] tmp_par;
                    continue;
                }
                if (strlen(par)) {
                    par[i] = '\0';
                    m->param1 = m->param = wastrdup(par);
                    delete [] tmp_par;
                }
            }
            else
                delete [] tmp_par;
            
            it = wacts->begin();
            for (; it != wacts->end(); ++it) {
                if ((*it)->Comp(s)) {
                    m->wfunc = (*it)->winfunc;
                    m->wfunc1 = (*it)->winfunc;
                    m->func_mask |= MenuWFuncMask;
                    m->func_mask1 |= MenuWFuncMask;
                    break;
                }
            }
            it = racts->begin();
            for (; it != racts->end(); ++it) {
                if ((*it)->Comp(s)) {
                    m->rfunc = (*it)->rootfunc;
                    m->rfunc1 = (*it)->rootfunc;
                    m->func_mask |= MenuRFuncMask;
                    m->func_mask1 |= MenuRFuncMask;
                    break;
                }
            }
            it = macts->begin();
            for (; it != macts->end(); ++it) {
                if ((*it)->Comp(s)) {
                    m->mfunc = (*it)->menufunc;
                    m->mfunc1 = (*it)->menufunc;
                    m->func_mask |= MenuMFuncMask;
                    m->func_mask1 |= MenuMFuncMask;
                    break;
                }
            }
            delete [] s;
            if (! (m->wfunc || m->rfunc || m->mfunc)) {
                WARNING << "at line " << linenr << ": function \"" << s <<
                    "\" not available" << endl;
                continue;
            }
        }
        
        if (cb) {
            if (! (s = strwithin(line2, '(', ')')))
                s = wastrdup("");
            m->label2 = s;
            if ((s = strwithin(line2, '{', '}'))) {
                if (*s != '\0') {
                    m->exec2 = s;
                    m->func_mask2 |= MenuExecMask;
                }
            }
            if ((s = strwithin(line2, '<', '>'))) {
                m->sub2 = s;
                m->func_mask2 |= MenuSubMask;
            }
            if ((s = strwithin(line2, '"', '"'))) {
                tmp_par = par = wastrdup(s);
                for (i = 0; *par != '(' && *par != '\0'; par++, i++);
                if (*(par++) == '(') {
                    s[i] = '\0';
                    for (i = 0; par[i] != ')' && par[i] != '\0'; i++);
                    if (par[i] == '\0') {
                        WARNING << "missing \")\" at line " << linenr << endl;
                        delete [] tmp_par;
                        continue;
                    }
                    if (strlen(par)) {
                        par[i] = '\0';
                        m->param2 = wastrdup(par);
                        delete [] tmp_par;
                    }
                }
                else
                    delete [] tmp_par;
                
                it = wacts->begin();
                for (; it != wacts->end(); ++it) {
                    if ((*it)->Comp(s)) {
                        delete [] s;
                        m->wfunc2 = (*it)->winfunc;
                        m->func_mask2 |= MenuWFuncMask;
                        break;
                    }
                }
                it = racts->begin();
                for (; it != racts->end(); ++it) {
                    if ((*it)->Comp(s)) {
                        delete [] s;
                        m->rfunc2 = (*it)->rootfunc;
                        m->func_mask2 |= MenuRFuncMask;
                        break;
                    }
                }
                it = macts->begin();
                for (; it != macts->end(); ++it) {
                    if ((*it)->Comp(s)) {
                        delete [] s;
                        m->mfunc2 = (*it)->menufunc;
                        m->func_mask2 |= MenuMFuncMask;
                        break;
                    }
                }
                if (! (m->wfunc2 || m->rfunc2 || m->mfunc2)) {
                    WARNING << "at line " << linenr << ": function \"" << s <<
                        "\" not available" << endl;
                    delete [] s;
                    continue;
                }
            }
        }
        menu->AddItem(m);
    }
    WARNING << "at line " << linenr << ": missing [end] statement" << endl;
}

/**
 * @fn    StrComp(char *s, ???)
 * @brief Constructor for StrComp class
 *
 * Creates a string comparer object. There is one constructor for each type
 * of object you can compare a string to. Objects are: int, WwActionFn,
 * RootActionFn, MenuActionFn.
 *
 * @param s String that match object
 * @param ??? Object that match string
 */
StrComp::StrComp(char *s, unsigned long v) { str = s; value = v; type = 0; }
StrComp::StrComp(char *s, WwActionFn a) {
   str = s; winfunc = a; type = WindowFuncMask; }
StrComp::StrComp(char *s, RootActionFn ra) {
   str = s; rootfunc = ra; type = RootFuncMask; }
StrComp::StrComp(char *s, MenuActionFn ma) {
   str = s; menufunc = ma; type = MenuFuncMask; }

/**
 * @fn    Comp(char *s)
 * @brief Tries to match object with string
 *
 * Tries to match string s with object string, if they match we return true
 * otherwise false.
 *
 * @param s String we want to try matching with
 *
 * @return True if match, otherwise false
 */
bool StrComp::Comp(char *s) {
    if (! strcasecmp(s, str))
        return true;    
    return false;
}

/**
 * @fn    strtrim(char *s)
 * @brief Trims a string
 *
 * Removes leading and trailing spaces, tabs and newlines.
 *
 * @param s String we want to trim
 *
 * @return Trimmed string
 */
char *strtrim(char *s) {
    for (; *s != '\0' && *s == ' ' || *s == '\t' || *s == '\n'; s++);
    if (*s == '\0') return s;
    while (s[strlen(s) - 1] == ' ' ||
           s[strlen(s) - 1] == '\t' ||
           s[strlen(s) - 1] == '\n')
        s[strlen(s) - 1] = '\0';
    return s;
}

/**
 * @fn    strwithin(char *s, char c1, char c2)
 * @brief Return string between to characters
 *
 * Duplicates and returns the string between c1 and c2 if c1 and c2 was
 * found. All occurenses of $STRING$ is replaced by environment variable
 * STRING's value, if environment variable doesn't exist $STRING$ will be
 * replaced by an empty string. $$ will be converted to a single $.
 * %c1 an %c2 will be replaced with c1 and c2. %% is replaced with %.
 *
 * @param s String to search for c1 and c2 in
 * @param c1 Starting character
 * @param c2 Ending character
 *
 * @return String within c1 and c2
 */
char *strwithin(char *s, char c1, char c2) {
    int i, n;
    char *str, *tmp, *env, *env_name;
    
    for (i = 0;; i++) {
        if (s[i] == '\0') break;
        if (s[i] == c1 && (i == 0 || s[i - 1] != '%')) break;
    }
    if (s[i] == '\0') return NULL;
    
    for (n = i + 1;; n++) {
        if (s[n] == '\0') break;
        if (s[n] == c2 && s[n - 1] != '%') break;
    }
    if (s[n] == '\0') return NULL; 
    s[n] = '\0';

    str = wastrdup(s + i + 1);
    
    s[n] = c2;
    for (i = 0; str[i] != '\0'; i++) {
        if (str[i] == '%') {
	    for (n = 1; str[i + n] != '\0'; n++)
	        str[i + n - 1] = str[i + n];
	    str[i + n - 1] = '\0';
        }
    }
    for (i = 0;;) {
        for (; str[i] != '$' && str[i] != '\0'; i++);
        if (str[i] == '\0')
            return str;
        else {
            str[i] = '\0';
            env_name = &str[++i];
            for (; str[i] != '$' && str[i] != '\0'; i++);
            if (str[i] == '\0') {
                WARNING << "ending \"$\" not found" << endl;
                delete [] str;
                return NULL;
            }
            else {
                str[i] = '\0';
                if (strlen(env_name)) {
                    if ((env = getenv(env_name)) == NULL)
                        env = "";
                } else
                    env = "$";
                tmp = new char[strlen(str) + strlen(env) +
                              strlen(&str[i + 1]) + 1];
                sprintf(tmp, "%s%s%s", str, env, &str[i + 1]);
                i = strlen(str) + 1;
                delete [] str;
                str = tmp;
            }
        }
    }
}
