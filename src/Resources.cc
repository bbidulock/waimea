/**
 * @file   Resources.cc
 * @author David Reveman <david@waimea.org>
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

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

extern "C" {
#include <X11/Xlib.h>

#ifdef    HAVE_STDIO_H
#  include <stdio.h>
#endif // HAVE_STDIO_H

#ifdef    STDC_HEADERS
#  include <stdlib.h>
#  include <string.h>
#endif // STDC_HEADERS

#ifdef    HAVE_CTYPE_H
#  include <ctype.h>
#endif // HAVE_CTYPE_H

#ifdef    HAVE_LIBGEN_H
#  include <libgen.h>
#else
inline char *basename(char *name) {
    int i = strlen(name);
    for (; i >= 0 && name[i] != '/'; i--);
    if (name[i] == '/') i++;
    return &name[i];
}
#endif // HAVE_LIBGEN_H

#ifdef    PIXMAP
#  include <Imlib2.h>
#endif // PIXMAP
}

#include <iostream>
using std::cerr;
using std::cout;
using std::endl;

#include "Resources.hh"

    
/**
 * @fn    ResourceHandler(void)
 * @brief Constructor for ResourceHandler class
 *
 * Sets config file variables. Creates lists with function pointers and
 * lists for actions.
 */
ResourceHandler::ResourceHandler(Waimea *wa, struct waoptions *options) {
    char *__m_wastrdup_tmp;

    waimea = wa;
    display = waimea->display;
    
    homedir = getenv("HOME");

    style_file = __m_wastrdup((char *) DEFAULTSTYLE);
    action_file = __m_wastrdup((char *) DEFAULTACTION);
    menu_file = __m_wastrdup((char *) DEFAULTMENU);
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

    wacts.push_back(new StrComp("raise", &WaWindow::Raise));
    wacts.push_back(new StrComp("lower", &WaWindow::Lower));
    wacts.push_back(new StrComp("focus", &WaWindow::Focus));
    wacts.push_back(new StrComp("startmove", &WaWindow::Move));
    wacts.push_back(new StrComp("startresizeright", &WaWindow::ResizeRight));
    wacts.push_back(new StrComp("startresizeleft", &WaWindow::ResizeLeft));
    wacts.push_back(new StrComp("startopaquemove", &WaWindow::MoveOpaque));
    wacts.push_back(new StrComp("startopaqueresizeright",
                                 &WaWindow::ResizeRightOpaque));
    wacts.push_back(new StrComp("startopaqueresizeleft",
                                 &WaWindow::ResizeLeftOpaque));
    wacts.push_back(new StrComp("endmoveresize", &WaWindow::EndMoveResize));
    wacts.push_back(new StrComp("close", &WaWindow::Close));
    wacts.push_back(new StrComp("kill", &WaWindow::Kill));
    wacts.push_back(new StrComp("closekill", &WaWindow::CloseKill));    
    wacts.push_back(new StrComp("menumap", &WaWindow::MenuMap));
    wacts.push_back(new StrComp("menuremap", &WaWindow::MenuRemap));
    wacts.push_back(new StrComp("menumapfocused",
                                 &WaWindow::MenuMapFocused));
    wacts.push_back(new StrComp("menuremapfocused",
                                 &WaWindow::MenuRemapFocused));
    wacts.push_back(new StrComp("menuunmap", &WaWindow::MenuUnmap));
    wacts.push_back(new StrComp("menuunmapfocused",
                                 &WaWindow::MenuUnmapFocus));
    wacts.push_back(new StrComp("shade", &WaWindow::Shade));
    wacts.push_back(new StrComp("unshade", &WaWindow::UnShade));
    wacts.push_back(new StrComp("toggleshade", &WaWindow::ToggleShade));
    wacts.push_back(new StrComp("maximize", &WaWindow::Maximize));
    wacts.push_back(new StrComp("unmaximize", &WaWindow::UnMaximize));
    wacts.push_back(new StrComp("togglemaximize", &WaWindow::ToggleMaximize));
    wacts.push_back(new StrComp("minimize", &WaWindow::Minimize));
    wacts.push_back(new StrComp("unminimize", &WaWindow::UnMinimize));
    wacts.push_back(new StrComp("toggleminimize", &WaWindow::ToggleMinimize));
    wacts.push_back(new StrComp("fullscreenon", &WaWindow::FullscreenOn));
    wacts.push_back(new StrComp("fullscreenoff", &WaWindow::FullscreenOff));
    wacts.push_back(new StrComp("fullscreentoggle",
                                &WaWindow::FullscreenToggle));
    wacts.push_back(new StrComp("sticky", &WaWindow::Sticky));
    wacts.push_back(new StrComp("unsticky", &WaWindow::UnSticky));
    wacts.push_back(new StrComp("togglesticky", &WaWindow::ToggleSticky));
    wacts.push_back(new StrComp("viewportleft", &WaWindow::MoveViewportLeft));
    wacts.push_back(new StrComp("viewportright",
                                 &WaWindow::MoveViewportRight));
    wacts.push_back(new StrComp("viewportup", &WaWindow::MoveViewportUp));
    wacts.push_back(new StrComp("viewportdown", &WaWindow::MoveViewportDown));
    wacts.push_back(new StrComp("viewportrelativemove", 
                                 &WaWindow::ViewportRelativeMove));
    wacts.push_back(new StrComp("viewportfixedmove", 
                                 &WaWindow::ViewportFixedMove));
    wacts.push_back(new StrComp("startviewportmove",
                                 &WaWindow::ViewportMove));
    wacts.push_back(new StrComp("taskswitcher", &WaWindow::TaskSwitcher));
    wacts.push_back(new StrComp("previoustask", &WaWindow::PreviousTask));
    wacts.push_back(new StrComp("nexttask", &WaWindow::NextTask));
    wacts.push_back(new StrComp("raisefocus", &WaWindow::RaiseFocus));
    wacts.push_back(new StrComp("decortitleon", &WaWindow::DecorTitleOn));
    wacts.push_back(new StrComp("decorhandleon", &WaWindow::DecorHandleOn));
    wacts.push_back(new StrComp("decorborderon", &WaWindow::DecorBorderOn));
    wacts.push_back(new StrComp("decorallon", &WaWindow::DecorAllOn));
    wacts.push_back(new StrComp("decortitleoff", &WaWindow::DecorTitleOff));
    wacts.push_back(new StrComp("decorhandleoff", &WaWindow::DecorHandleOff));
    wacts.push_back(new StrComp("decorborderoff", &WaWindow::DecorBorderOff));
    wacts.push_back(new StrComp("decoralloff", &WaWindow::DecorAllOff));
    wacts.push_back(new StrComp("decortitletoggle",
                                 &WaWindow::DecorTitleToggle));
    wacts.push_back(new StrComp("decorhandletoggle",
                                 &WaWindow::DecorHandleToggle));
    wacts.push_back(new StrComp("decorbordertoggle",
                                 &WaWindow::DecorBorderToggle));
    wacts.push_back(new StrComp("alwaysontopon",
                                 &WaWindow::AlwaysontopOn));
    wacts.push_back(new StrComp("alwaysatbottomon",
                                 &WaWindow::AlwaysatbottomOn));
    wacts.push_back(new StrComp("alwaysontopoff",
                                 &WaWindow::AlwaysontopOff));
    wacts.push_back(new StrComp("alwaysatbottomoff",
                                 &WaWindow::AlwaysatbottomOff));
    wacts.push_back(new StrComp("alwaysontoptoggle",
                                 &WaWindow::AlwaysontopToggle));
    wacts.push_back(new StrComp("alwaysatbottomtoggle",
                                 &WaWindow::AlwaysatbottomToggle));
    wacts.push_back(new StrComp("acceptconfigrequeston",
                                 &WaWindow::AcceptConfigRequestOn));
    wacts.push_back(new StrComp("acceptconfigrequestoff",
                                 &WaWindow::AcceptConfigRequestOff));
    wacts.push_back(new StrComp("acceptconfigrequesttoggle",
                                 &WaWindow::AcceptConfigRequestToggle));
    wacts.push_back(new StrComp("pointerrelativewarp",
                                 &WaWindow::PointerRelativeWarp));
    wacts.push_back(new StrComp("pointerfixedwarp",
                                 &WaWindow::PointerFixedWarp));
    wacts.push_back(new StrComp("moveresize", &WaWindow::MoveResize));
    wacts.push_back(new StrComp("moveresizevirtual",
                                 &WaWindow::MoveResizeVirtual));
    wacts.push_back(new StrComp("movetopointer",
                                 &WaWindow::MoveWindowToPointer));
    wacts.push_back(new StrComp("movetosmartplace",
                                 &WaWindow::MoveWindowToSmartPlace));
    wacts.push_back(new StrComp("movetosmartplaceifuninitialized",
                                 &WaWindow::moveToSmartPlaceIfUninitialized));
    wacts.push_back(new StrComp("gotodesktop", &WaWindow::GoToDesktop));
    wacts.push_back(new StrComp("nextdesktop", &WaWindow::NextDesktop));
    wacts.push_back(new StrComp("previousdesktop",
                                &WaWindow::PreviousDesktop));
    wacts.push_back(new StrComp("desktopmask", &WaWindow::DesktopMask));
    wacts.push_back(new StrComp("joindesktop", &WaWindow::JoinDesktop));
    wacts.push_back(new StrComp("partdesktop", &WaWindow::PartDesktop));
    wacts.push_back(new StrComp("partcurrentdesktop",
                                &WaWindow::PartCurrentDesktop));
    wacts.push_back(new StrComp("joinalldesktops",
                                &WaWindow::JoinAllDesktops));
    wacts.push_back(new StrComp("partalldesktopsexceptcurrent",
                                &WaWindow::PartAllDesktopsExceptCurrent));
    wacts.push_back(new StrComp("partcurrentjoindesktop",
                                &WaWindow::PartCurrentJoinDesktop));
    wacts.push_back(new StrComp("mergewithwindow",
                                &WaWindow::CloneMergeWithWindow));
    wacts.push_back(new StrComp("vertmergewithwindow",
                                &WaWindow::VertMergeWithWindow));
    wacts.push_back(new StrComp("horizmergewithwindow",
                                &WaWindow::HorizMergeWithWindow));
    wacts.push_back(new StrComp("explode", &WaWindow::Explode));
    wacts.push_back(new StrComp("mergedtofront", &WaWindow::ToFront));
    wacts.push_back(new StrComp("unmerge", &WaWindow::UnMergeMaster));
    wacts.push_back(new StrComp("setmergemode", &WaWindow::SetMergeMode));
    wacts.push_back(new StrComp("nextmergemode", &WaWindow::NextMergeMode));
    wacts.push_back(new StrComp("prevmergemode", &WaWindow::PrevMergeMode));
    wacts.push_back(new StrComp("restart", &WaWindow::Restart));
    wacts.push_back(new StrComp("exit", &WaWindow::Exit));
    wacts.push_back(new StrComp("nop", &WaWindow::Nop));
    
    racts.push_back(new StrComp("focus", &WaScreen::Focus));
    racts.push_back(new StrComp("menumap", &WaScreen::MenuMap));
    racts.push_back(new StrComp("menuremap", &WaScreen::MenuRemap));
    racts.push_back(new StrComp("menumapfocused", &WaScreen::MenuMapFocused));
    racts.push_back(new StrComp("menuremapfocused",
                                 &WaScreen::MenuRemapFocused));
    racts.push_back(new StrComp("menuunmap", &WaScreen::MenuUnmap));
    racts.push_back(new StrComp("menuunmapfocused",
                                 &WaScreen::MenuUnmapFocus));
    racts.push_back(new StrComp("restart", &WaScreen::Restart));
    racts.push_back(new StrComp("exit", &WaScreen::Exit));
    racts.push_back(new StrComp("viewportleft", &WaScreen::MoveViewportLeft));
    racts.push_back(new StrComp("viewportright",
                                 &WaScreen::MoveViewportRight));
    racts.push_back(new StrComp("viewportup", &WaScreen::MoveViewportUp));
    racts.push_back(new StrComp("viewportdown", &WaScreen::MoveViewportDown));
    racts.push_back(new StrComp("viewportrelativemove", 
                                 &WaScreen::ViewportRelativeMove));
    racts.push_back(new StrComp("viewportfixedmove", 
                                 &WaScreen::ViewportFixedMove));
    racts.push_back(new StrComp("startviewportmove",
                                 &WaScreen::ViewportMove));
    racts.push_back(new StrComp("endmoveresize", &WaScreen::EndMoveResize));
    racts.push_back(new StrComp("taskswitcher", &WaScreen::TaskSwitcher));
    racts.push_back(new StrComp("previoustask", &WaScreen::PreviousTask));
    racts.push_back(new StrComp("nexttask", &WaScreen::NextTask));
    racts.push_back(new StrComp("pointerrelativewarp", 
                                 &WaScreen::PointerRelativeWarp));
    racts.push_back(new StrComp("pointerfixedwarp",
                                 &WaScreen::PointerFixedWarp));
    racts.push_back(new StrComp("gotodesktop", &WaScreen::GoToDesktop));
    racts.push_back(new StrComp("nextdesktop", &WaScreen::NextDesktop));
    racts.push_back(new StrComp("previousdesktop",
                                &WaScreen::PreviousDesktop));
    racts.push_back(new StrComp("nop", &WaScreen::Nop));
    
    macts.push_back(new StrComp("unlink", &WaMenuItem::UnLinkMenu));
    macts.push_back(new StrComp("mapsub", &WaMenuItem::MapSubmenu));
    macts.push_back(new StrComp("mapsubonly", &WaMenuItem::MapSubmenuOnly));
    macts.push_back(new StrComp("remapsub", &WaMenuItem::RemapSubmenu));
    macts.push_back(new StrComp("mapsubfocused",
                                 &WaMenuItem::MapSubmenuFocused));
    macts.push_back(new StrComp("mapsubfocusedonly",
                                 &WaMenuItem::MapSubmenuFocusedOnly));
    macts.push_back(new StrComp("remapsubfocused",
                                 &WaMenuItem::RemapSubmenuFocused));
    macts.push_back(new StrComp("unmap", &WaMenuItem::UnmapMenu));
    macts.push_back(new StrComp("unmapfocused", &WaMenuItem::UnmapMenuFocus));
    macts.push_back(new StrComp("unmapsubs", &WaMenuItem::UnmapSubmenus));
    macts.push_back(new StrComp("unmaptree", &WaMenuItem::UnmapTree));
    macts.push_back(new StrComp("exec", &WaMenuItem::Exec));
    macts.push_back(new StrComp("func", &WaMenuItem::Func));
    macts.push_back(new StrComp("raise", &WaMenuItem::Raise));
    macts.push_back(new StrComp("focus", &WaMenuItem::Focus));
    macts.push_back(new StrComp("lower", &WaMenuItem::Lower));
    macts.push_back(new StrComp("startmove", &WaMenuItem::Move));
    macts.push_back(new StrComp("startopaquemove", &WaMenuItem::MoveOpaque));
    macts.push_back(new StrComp("endmoveresize", &WaMenuItem::EndMoveResize));
    macts.push_back(new StrComp("viewportleft",
                                 &WaMenuItem::MoveViewportLeft));
    macts.push_back(new StrComp("viewportright",
                                 &WaMenuItem::MoveViewportRight));
    macts.push_back(new StrComp("viewportup", &WaMenuItem::MoveViewportUp));
    macts.push_back(new StrComp("viewportdown",
                                 &WaMenuItem::MoveViewportDown));
    macts.push_back(new StrComp("viewportrelativemove", 
                                 &WaMenuItem::ViewportRelativeMove));
    macts.push_back(new StrComp("viewportfixedmove", 
                                 &WaMenuItem::ViewportFixedMove));
    macts.push_back(new StrComp("startviewportmove",
                                 &WaMenuItem::ViewportMove));
    macts.push_back(new StrComp("taskswitcher", &WaMenuItem::TaskSwitcher));
    macts.push_back(new StrComp("previoustask", &WaMenuItem::PreviousTask));
    macts.push_back(new StrComp("nexttask", &WaMenuItem::NextTask));
    macts.push_back(new StrComp("nextitem", &WaMenuItem::NextItem));
    macts.push_back(new StrComp("previousitem", &WaMenuItem::PreviousItem));
    macts.push_back(new StrComp("pointerrelativewarp", 
                                 &WaMenuItem::PointerRelativeWarp));
    macts.push_back(new StrComp("pointerfixedwarp",
                                 &WaMenuItem::PointerFixedWarp));
    macts.push_back(new StrComp("menumap", &WaMenuItem::MenuMap));
    macts.push_back(new StrComp("menuremap", &WaMenuItem::MenuRemap));
    macts.push_back(new StrComp("menumapfocused",
                                 &WaMenuItem::MenuMapFocused));
    macts.push_back(new StrComp("menuremapfocused",
                                 &WaMenuItem::MenuRemapFocused));
    macts.push_back(new StrComp("menuunmap", &WaMenuItem::MenuUnmap));
    macts.push_back(new StrComp("menuunmapfocused",
                                 &WaMenuItem::MenuUnmapFocus));
    macts.push_back(new StrComp("gotodesktop", &WaMenuItem::GoToDesktop));
    macts.push_back(new StrComp("nextdesktop", &WaMenuItem::NextDesktop));
    macts.push_back(new StrComp("previousdesktop",
                                &WaMenuItem::PreviousDesktop));
    macts.push_back(new StrComp("restart", &WaMenuItem::Restart));
    macts.push_back(new StrComp("exit", &WaMenuItem::Exit));
    macts.push_back(new StrComp("nop", &WaMenuItem::Nop));
    
    types.push_back(new StrComp("keypress", KeyPress));
    types.push_back(new StrComp("keyrelease", KeyRelease));
    types.push_back(new StrComp("buttonpress", ButtonPress));
    types.push_back(new StrComp("buttonrelease", ButtonRelease));
    types.push_back(new StrComp("doubleclick", DoubleClick));
    types.push_back(new StrComp("enternotify", EnterNotify));
    types.push_back(new StrComp("leavenotify", LeaveNotify));
    types.push_back(new StrComp("maprequest", MapRequest));

    bdetails.push_back(new StrComp("anybutton", (unsigned long) 0));
    bdetails.push_back(new StrComp("button1", Button1));
    bdetails.push_back(new StrComp("button2", Button2));
    bdetails.push_back(new StrComp("button3", Button3));
    bdetails.push_back(new StrComp("button4", Button4));
    bdetails.push_back(new StrComp("button5", Button5));
    bdetails.push_back(new StrComp("button6", 6));
    bdetails.push_back(new StrComp("button7", 7));
    bdetails.push_back(new StrComp("button8", 8));
    bdetails.push_back(new StrComp("button9", 9));
    bdetails.push_back(new StrComp("button10", 10));
    bdetails.push_back(new StrComp("button11", 11));
    bdetails.push_back(new StrComp("button12", 12));
    
    mods.push_back(new StrComp("shiftmask", ShiftMask));
    mods.push_back(new StrComp("lockmask", LockMask));
    mods.push_back(new StrComp("controlmask", ControlMask));
    mods.push_back(new StrComp("mod1mask", Mod1Mask));
    mods.push_back(new StrComp("mod2mask", Mod2Mask));
    mods.push_back(new StrComp("mod3mask", Mod3Mask));
    mods.push_back(new StrComp("mod4mask", Mod4Mask));
    mods.push_back(new StrComp("mod5mask", Mod5Mask));
    mods.push_back(new StrComp("button1mask", Button1Mask));
    mods.push_back(new StrComp("button2mask", Button2Mask));
    mods.push_back(new StrComp("button3mask", Button3Mask));
    mods.push_back(new StrComp("button4mask", Button4Mask));
    mods.push_back(new StrComp("button5mask", Button5Mask));
    mods.push_back(new StrComp("moveresizemask", MoveResizeMask));

    const XModifierKeymap* const modmap = XGetModifierMapping(display);

    if (modmap && modmap->max_keypermod > 0) {
        const int mask_table[] = { 
            ShiftMask, LockMask, ControlMask, Mod1Mask,
            Mod2Mask, Mod3Mask, Mod4Mask, Mod5Mask
        };    
        const size_t size = (sizeof(mask_table) / sizeof(mask_table[0])) *
            modmap->max_keypermod;
        
        for (size_t i = 0; i < size; ++i) {
            if (! modmap->modifiermap[i]) continue;
            KeySym ksym = XKeycodeToKeysym(display, modmap->modifiermap[i], 0);
            if (ksym) {
                char *kstring = XKeysymToString(ksym);
                if (kstring) {
                    int modmask = mask_table[i / modmap->max_keypermod];
                    mods.push_back(new StrComp(kstring, modmask));
                }
            }
        }
        if (modmap) XFreeModifiermap(const_cast<XModifierKeymap*>(modmap));
    }
}

/**
 * @fn    ~ResourceHandler(void)
 * @brief Destructor for ResourceHandler class
 *
 * Deletes all action lists and all WaActions in them.
 */
ResourceHandler::~ResourceHandler(void) {
    LISTDEL(wacts);
    LISTDEL(racts);
    LISTDEL(macts);
    LISTDEL(types);
    LISTDEL(bdetails);
    LISTDEL(mods);    
    
    delete [] rc_file;
    delete [] style_file;
    delete [] action_file;
    delete [] menu_file;
}

/**
 * @fn    LoadConfig(WaScreen *waimea)
 * @brief Reads config file
 *
 * Reads all configuration resources for common for all screens from the
 * config file.
 *
 * @param waimea Pointer to Waimea object
 */
void ResourceHandler::LoadConfig(Waimea *waimea) {
    XrmValue value;
    char *value_type;
    char rc_name[30], rc_class[30];
    char *__m_wastrdup_tmp;
    
    database = (XrmDatabase) 0;
    if (! (database = XrmGetFileDatabase(rc_file))) {
        if (rc_forced) WARNING << "can't open rcfile `" << rc_file <<
                           "' for reading" << endl;
        else
            if (! (database = XrmGetFileDatabase(DEFAULTRCFILE)))
                WARNING << "can't open system default rcfile `" << 
                    DEFAULTRCFILE << "' for reading" << endl;
    }
    
    waimea->screenmask = 0;
    sprintf(rc_name, "screenMask");
    sprintf(rc_class, "ScreenMask");
    if (XrmGetResource(database, rc_name, rc_class, &value_type, &value)) {
        char *token = strtok(value.addr, ", \t");
        if (token) waimea->screenmask |= (1L << atoi(token));
        while ((token = strtok(NULL, ", \t")))
            waimea->screenmask |= (1L << atoi(token));
    } else
        waimea->screenmask = (1L << 0) | (1L << 2) | (1L << 3);
    
    char *path = getenv("PATH");
    sprintf(rc_name, "scriptDir");
    sprintf(rc_class, "ScriptDir");
    if (XrmGetResource(database, rc_name, rc_class, &value_type, &value)) {
        char *sdir = environment_expansion(__m_wastrdup(value.addr));
        waimea->pathenv = new char[strlen(path) + strlen(sdir) + 7];
        sprintf(waimea->pathenv, "PATH=%s:%s", sdir, path);
        delete [] sdir;
    }
    else { 
        waimea->pathenv =
            new char[strlen(path) + strlen(DEFAULTSCRIPTDIR) + 7];
        sprintf(waimea->pathenv, "PATH=%s:%s", DEFAULTSCRIPTDIR, path);
    }

    sprintf(rc_name, "doubleClickInterval");
    sprintf(rc_class, "DoubleClickInterval");
    if (XrmGetResource(database, rc_name, rc_class, &value_type, &value)) {
        if (sscanf(value.addr, "%lu", &waimea->double_click) != 1)
            waimea->double_click = 300;
    } else
        waimea->double_click = 300;

    if (waimea->double_click > 999) waimea->double_click = 999;

    XrmDestroyDatabase(database);
}

/**
 * @fn    LoadConfig(WaScreen *wascreen)
 * @brief Reads config file
 *
 * Reads all configuration resources for this screen from the config file.
 *
 * @param wascreen Screen to read resources for
 */
void ResourceHandler::LoadConfig(WaScreen *wascreen) {
    XrmValue value;
    char *value_type;
    char rc_name[30], rc_class[30];
    int sn = wascreen->screen_number;
    ScreenConfig *sc = &wascreen->config;
    char *__m_wastrdup_tmp;
    
    database = (XrmDatabase) 0;
    if (! (database = XrmGetFileDatabase(rc_file)))
        if (! rc_forced) database = XrmGetFileDatabase(DEFAULTRCFILE);
    
    sc->style_file = __m_wastrdup(style_file);
    if (! style_forced) {
        sprintf(rc_name, "screen%d.styleFile", sn);
        sprintf(rc_class, "Screen%d.StyleFile", sn);
        if (XrmGetResource(database, rc_name, rc_class, &value_type, &value)) {
            delete [] sc->style_file;
            sc->style_file = environment_expansion(__m_wastrdup(value.addr));
        }
    }

    sc->action_file = __m_wastrdup(action_file);
    if (! action_forced) {
        sprintf(rc_name, "screen%d.actionFile", sn);
        sprintf(rc_class, "Screen%d.ActionFile", sn);
        if (XrmGetResource(database, rc_name, rc_class, &value_type, &value)) {
            delete [] sc->action_file;
            sc->action_file = environment_expansion(__m_wastrdup(value.addr));
        }
    }

    sc->menu_file = __m_wastrdup(menu_file);
    if (! menu_forced) {
        sprintf(rc_name, "screen%d.menuFile", sn);
        sprintf(rc_class, "Screen%d.MenuFile", sn);
        if (XrmGetResource(database, rc_name, rc_class, &value_type, &value)) {
            delete [] sc->menu_file;
            sc->menu_file = environment_expansion(__m_wastrdup(value.addr));
        }
    }

    sprintf(rc_name, "screen%d.numberOfDesktops", sn);
    sprintf(rc_class, "Screen%d.NumberOfDesktops", sn);
    if (XrmGetResource(database, rc_name, rc_class, &value_type, &value)) {
        if (sscanf(value.addr, "%u", &sc->desktops) != 1) {
            sc->desktops = 1;
        } else {
            if (sc->desktops < 1) sc->desktops = 1;
            if (sc->desktops > 16) sc->desktops = 16;
        }
    } else
        sc->desktops = 1;
    
    sprintf(rc_name, "screen%d.desktopNames", sn);
    sprintf(rc_class, "Screen%d.DesktopNames", sn);
    if (XrmGetResource(database, rc_name, rc_class, &value_type, &value)) {
        wascreen->net->SetDesktopNames(wascreen, value.addr);
    }

    sprintf(rc_name, "screen%d.virtualSize", sn);
    sprintf(rc_class, "Screen%d.VirtualSize", sn);
    if (XrmGetResource(database, rc_name, rc_class, &value_type, &value)) {
        if (sscanf(value.addr, "%ux%u", &sc->virtual_x, &sc->virtual_y) != 2) {
            sc->virtual_x = sc->virtual_y = 3;
        }
    } else
        sc->virtual_x = sc->virtual_y = 3;
    if (sc->virtual_x > 20) sc->virtual_x = 20;
    if (sc->virtual_y > 20) sc->virtual_y = 20;
    if (sc->virtual_x < 1) sc->virtual_x = 1;
    if (sc->virtual_y < 1) sc->virtual_y = 1;

    sprintf(rc_name, "screen%d.doubleBufferedText", sn);
    sprintf(rc_class, "Screen%d.DoubleBufferedText", sn);
    if (XrmGetResource(database, rc_name, rc_class, &value_type, &value)) {
        if (! strncasecmp("true", value.addr, value.size))
            sc->db = true;
        else
            sc->db = false;
    } else
        sc->db = true;

#ifdef RENDER
    sprintf(rc_name, "screen%d.lazyTransparency", sn);
    sprintf(rc_class, "Screen%d.LazyTransparency", sn);
    if (XrmGetResource(database, rc_name, rc_class, &value_type, &value)) {
        if (! strncasecmp("true", value.addr, value.size))
            sc->lazy_trans = true;
        else
            sc->lazy_trans = false;
    } else
        sc->lazy_trans = true;
#endif // RENDER

    sprintf(rc_name, "screen%d.colorsPerChannel", sn);
    sprintf(rc_class, "Screen%d.ColorsPerChannel", sn);
    if (XrmGetResource(database, rc_name, rc_class, &value_type, &value)) {
        if (sscanf(value.addr, "%d", &sc->colors_per_channel) != 1) {
            sc->colors_per_channel = 4;
        } else {
            if (sc->colors_per_channel < 2) sc->colors_per_channel = 2;
            if (sc->colors_per_channel > 6) sc->colors_per_channel = 6;
        }
    } else
        sc->colors_per_channel = 4;

    sprintf(rc_name, "screen%d.cacheMax", sn);
    sprintf(rc_class, "Screen%d.CacheMax", sn);
    if (XrmGetResource(database, rc_name, rc_class, &value_type, &value)) {
        if (sscanf(value.addr, "%lu", &sc->cache_max) != 1)
            sc->cache_max = 200;
    } else
        sc->cache_max = 200;

    sprintf(rc_name, "screen%d.imageDither", sn);
    sprintf(rc_class, "screen%d.ImageDither", sn);
    if (XrmGetResource(database, rc_name, rc_class, &value_type, &value)) {
        if (! strncasecmp("true", value.addr, value.size))
            sc->image_dither = true;
        else
            sc->image_dither = false;
    } else
        sc->image_dither = true;

    sprintf(rc_name, "screen%d.menuStacking", sn);
    sprintf(rc_class, "Screen%d.MenuStacking", sn);
    if (XrmGetResource(database, rc_name, rc_class, &value_type, &value)) {
        if (! strncasecmp("AlwaysAtBottom", value.addr, value.size))
            sc->menu_stacking = AlwaysAtBottom;
        else if (! strncasecmp("AlwaysOnTop", value.addr, value.size))
            sc->menu_stacking = AlwaysOnTop;
    } else
        sc->menu_stacking = NormalStacking;

    sprintf(rc_name, "screen%d.transientAbove", sn);
    sprintf(rc_class, "Screen%d.TransientAbove", sn);
    if (XrmGetResource(database, rc_name, rc_class, &value_type, &value)) {
        if (! strncasecmp("true", value.addr, value.size))
            sc->transient_above = true;
        else
            sc->transient_above = false;
    } else
        sc->transient_above = true;

    sprintf(rc_name, "screen%d.focusRevertTo", sn);
    sprintf(rc_class, "Screen%d.focusRevertTo", sn);
    if (XrmGetResource(database, rc_name, rc_class, &value_type, &value)) {
        if (! strncasecmp("Root", value.addr, value.size))
            sc->revert_to_window = false;
        else
            sc->revert_to_window = true;
    } else
        sc->transient_above = true;
    
    unsigned int dummy;
    char *token;
    int dock_num, i;
    bool d_exists = true;
    DockStyle *dockstyle;

    for (dock_num = 0; d_exists && dock_num < 100; ++dock_num) {
        d_exists = false;
        dockstyle = new DockStyle;
    
        sprintf(rc_name, "screen%d.dock%d.geometry", sn, dock_num);
        sprintf(rc_class, "Screen%d.Dock%d.Geometry", sn, dock_num);
        if (XrmGetResource(database, rc_name, rc_class, &value_type, &value)) {
            dockstyle->geometry = XParseGeometry(value.addr, &dockstyle->x,
                                                 &dockstyle->y, &dummy,
                                                 &dummy);
            d_exists = true;
        } else
            dockstyle->geometry = XParseGeometry("-0+0", &dockstyle->x,
                                                 &dockstyle->y, &dummy,
                                                 &dummy);
        
        sprintf(rc_name, "screen%d.dock%d.order", sn, dock_num);
        sprintf(rc_class, "Screen%d.Dock%d.Order", sn, dock_num);
        if (XrmGetResource(database, rc_name, rc_class, &value_type, &value)) {
            d_exists = true;
            token = value.addr;
            while (strlen(token) > 6) {
                token = strtrim(token);
                if (token[0] == 'n' && token[1] == '/') {
                    for (i = 2; token[i] != '\0' &&
                           ! (token[i] == '/' && token[i - 1] != '\\'); i++);
                    if (token[i] == '\0') break;
                    token[i] = '\0';
                    dockstyle->order.push_back(new Regex(&token[2]));
                    dockstyle->order_type.push_back(NameMatchType);
                }
                else if (token[0] == 'c' && token[1] == '/') {
                    for (i = 2; token[i] != '\0' &&
                           ! (token[i] == '/' && token[i - 1] != '\\'); i++);
                    if (token[i] == '\0') break;
                    token[i] = '\0';
                    dockstyle->order.push_back(new Regex(&token[2]));
                    dockstyle->order_type.push_back(ClassMatchType);
                }
                else if (token[0] == 't' && token[1] == '/') {
                    for (i = 2; token[i] != '\0' &&
                           ! (token[i] == '/' && token[i - 1] != '\\'); i++);
                    if (token[i] == '\0') break;
                    token[i] = '\0';
                    dockstyle->order.push_back(new Regex(&token[2]));
                    dockstyle->order_type.push_back(TitleMatchType);
                }
                token = token + strlen(token) + 1;
            }
        }
        
        sprintf(rc_name, "screen%d.dock%d.desktopMask", sn, dock_num);
        sprintf(rc_class, "Screen%d.Dock%d.DesktopMask", sn, dock_num);
        if (XrmGetResource(database, rc_name, rc_class, &value_type,
                           &value)) {
            d_exists = true;
            if (! strncasecmp("all", value.addr, 3))
                dockstyle->desktop_mask = (1L << 16) - 1;
            else {
                dockstyle->desktop_mask = 0;
                char *token = strtok(value.addr, " \t");
                while (token) {
                    int desk = (unsigned int) atoi(token);
                    if (desk < 16) dockstyle->desktop_mask |= (1L << desk);
                    token = strtok(NULL, " \t");
                }
            }
        } else
            dockstyle->desktop_mask = (1L << 16) - 1;

        sprintf(rc_name, "screen%d.dock%d.centered", sn, dock_num);
        sprintf(rc_class, "Screen%d.Dock%d.Centered", sn, dock_num);
        if (XrmGetResource(database, rc_name, rc_class, &value_type,
                           &value)) {
            d_exists = true;
            if (! strncasecmp("true", value.addr, value.size))
                dockstyle->centered = true;
            else
                dockstyle->centered = false;
        } else
            dockstyle->centered = false;

        sprintf(rc_name, "screen%d.dock%d.inworkspace", sn, dock_num);
        sprintf(rc_class, "Screen%d.Dock%d.Inworkspace", sn, dock_num);
        if (XrmGetResource(database, rc_name, rc_class, &value_type,
                           &value)) {
            d_exists = true;
            if (! strncasecmp("true", value.addr, value.size))
                dockstyle->inworkspace = true;
            else
                dockstyle->inworkspace = false;
        } else
            dockstyle->inworkspace = false;

        sprintf(rc_name, "screen%d.dock%d.direction", sn, dock_num);
        sprintf(rc_class, "Screen%d.Dock%d.Direction", sn, dock_num);
        if (XrmGetResource(database, rc_name, rc_class, &value_type,
                           &value)) {
            d_exists = true;
            if (! strncasecmp("Horizontal", value.addr, value.size))
                dockstyle->direction = HorizontalDock;
            else
                dockstyle->direction = VerticalDock;
        } else
            dockstyle->direction = VerticalDock;

        sprintf(rc_name, "screen%d.dock%d.gridSpace", sn, dock_num);
        sprintf(rc_class, "Screen%d.Dock%d.GridSpace", sn, dock_num);
        if (XrmGetResource(database, rc_name, rc_class, &value_type,
                           &value)) {
            d_exists = true;
            if (sscanf(value.addr, "%u", &dockstyle->gridspace) != 1)
                dockstyle->gridspace = 2;
        } else
            dockstyle->gridspace = 2;
        
        if (dockstyle->gridspace > 50) dockstyle->gridspace = 50;

        sprintf(rc_name, "screen%d.dock%d.stacking", sn, dock_num);
        sprintf(rc_class, "Screen%d.Dock%d.Stacking", sn, dock_num);
        if (XrmGetResource(database, rc_name, rc_class, &value_type,
                           &value)) {
            d_exists = true;
            if (! strncasecmp("AlwaysAtBottom", value.addr, value.size))
                dockstyle->stacking = AlwaysAtBottom;
            else
                dockstyle->stacking = AlwaysOnTop;
        } else
            dockstyle->stacking = AlwaysOnTop;
        
        if (d_exists || ! dock_num)
            wascreen->wstyle.dockstyles.push_back(dockstyle);
        else
            delete dockstyle;
    }
    
    XrmDestroyDatabase(database);
} 
    
/**
 * @fn    LoadStyle(WaScreen *wascreen)
 * @brief Reads style file
 *
 * Reads a style resources from a style file.
 *
 * @param wascreen WaScreen to load style for
 */
void ResourceHandler::LoadStyle(WaScreen *wascreen) {
    XrmValue value;
    char *value_type;
    int screen = wascreen->screen_number;
    WindowStyle *wstyle = &wascreen->wstyle;
    MenuStyle   *mstyle = &wascreen->mstyle;
    WaImageControl *ic = wascreen->ic;
    char *__m_wastrdup_tmp;
    
    database = (XrmDatabase) 0;
    
    if (! (database = XrmGetFileDatabase(wascreen->config.style_file)))
        WARNING << "can't open stylefile `" << wascreen->config.style_file
                << "' for reading" << endl;
    
    int slen = strlen(wascreen->config.style_file) - 1;
    for (; slen >= 1 && wascreen->config.style_file[slen] != '/'; slen--);
    wascreen->config.style_file[slen] = '\0';

    WaFont default_font;
    
#ifdef XFT
    default_font.xft = true;
    default_font.font = "arial:pixelsize=12";
#else // !XFT
    default_font.xft = false;
    default_font.font = "fixed";
#endif // XFT

    ReadDatabaseFont("window.font", "Window.Font", &wstyle->wa_font,
                     &default_font);
    ReadDatabaseFont("menu.frame.font", "Menu.Frame.Font",
                     &mstyle->wa_f_font, &wstyle->wa_font);
    ReadDatabaseFont("menu.title.font", "Menu.Title.Font",
                     &mstyle->wa_t_font, &mstyle->wa_f_font);
    ReadDatabaseFont("menu.bullet.font", "Menu.Bullet.Font",
                     &mstyle->wa_b_font, &mstyle->wa_f_font);
    ReadDatabaseFont("menu.checkbox.true.font",
                     "Menu.Checkbox.True.Font",
                     &mstyle->wa_ct_font, &mstyle->wa_f_font);
    ReadDatabaseFont("menu.checkbox.false.font",
                     "Menu.Checkbox.False.Font",
                     &mstyle->wa_cf_font, &mstyle->wa_ct_font);
    
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
    ReadDatabaseColor("window.label.focus.textColor",
                      "Window.Label.Focus.TextColor",
                      &wstyle->l_text_focus, BlackPixel(display, screen), ic);
    ReadDatabaseColor("window.label.focus.textShadowColor",
                      "Window.Label.Focus.TextShadowColor",
                      &wstyle->l_text_focus_s,
                      BlackPixel(display, screen), ic);
    ReadDatabaseColor("window.label.unfocus.textColor",
                      "Window.Label.Unfocus.TextColor",
                      &wstyle->l_text_unfocus, WhitePixel(display, screen),
                      ic);
    ReadDatabaseColor("window.label.unfocus.textShadowColor",
                      "Window.Label.Unfocus.TextShadowColor",
                      &wstyle->l_text_unfocus_s, BlackPixel(display, screen),
                      ic);

    if (XrmGetResource(database,
                       "window.label.focus.textShadowXOffset",
                       "Window.Label.focus.TextShadowXOffset", &value_type,
                       &value)) {
        if (sscanf(value.addr, "%d", &wstyle->wa_font.shodow_off_x) != 1)
            wstyle->wa_font.shodow_off_x = 0;
    } else
        wstyle->wa_font.shodow_off_x = 0;

    if (wstyle->wa_font.shodow_off_x > 10) wstyle->wa_font.shodow_off_x = 10;
    if (wstyle->wa_font.shodow_off_x < -10) wstyle->wa_font.shodow_off_x = -10;

    if (XrmGetResource(database,
                       "window.label.focus.textShadowYOffset",
                       "Window.Label.focus.TextShadowYOffset", &value_type,
                       &value)) {
        if (sscanf(value.addr, "%d", &wstyle->wa_font.shodow_off_y) != 1)
            wstyle->wa_font.shodow_off_y = 0;
    } else
        wstyle->wa_font.shodow_off_y = 0;

    if (wstyle->wa_font.shodow_off_y > 10) wstyle->wa_font.shodow_off_y = 10;
    if (wstyle->wa_font.shodow_off_y < -10) wstyle->wa_font.shodow_off_y = -10;
    
    if (XrmGetResource(database,
                       "window.label.unfocus.textShadowXOffset",
                       "Window.Label.Unfocus.TextShadowXOffset", &value_type,
                       &value)) {
        if (sscanf(value.addr, "%d", &wstyle->wa_font_u.shodow_off_x) != 1)
            wstyle->wa_font_u.shodow_off_x = 0;
    } else
        wstyle->wa_font_u.shodow_off_x = 0;
    
    if (wstyle->wa_font_u.shodow_off_x > 10)
        wstyle->wa_font_u.shodow_off_x = 10;
    if (wstyle->wa_font_u.shodow_off_x < -10)
        wstyle->wa_font_u.shodow_off_x = -10;

    if (XrmGetResource(database,
                       "window.label.unfocus.textShadowYOffset",
                       "Window.Label.Unfocus.TextShadowYOffset", &value_type,
                       &value)) {
        if (sscanf(value.addr, "%d", &wstyle->wa_font_u.shodow_off_y) != 1)
            wstyle->wa_font_u.shodow_off_y = 0;
    } else
        wstyle->wa_font_u.shodow_off_y = 0;
    
    if (wstyle->wa_font_u.shodow_off_y > 10)
        wstyle->wa_font_u.shodow_off_y = 10;
    if (wstyle->wa_font_u.shodow_off_y < -10)
        wstyle->wa_font_u.shodow_off_y = -10;
    
    
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
                        &mstyle->back_frame, WhitePixel(display, screen), ic);
    ReadDatabaseTexture("menu.hilite", "Menu.Hilite",
                        &mstyle->hilite, WhitePixel(display, screen), ic);
    ReadDatabaseTexture("menu.title", "Menu.Title",
                        &mstyle->title, WhitePixel(display, screen), ic);

    ReadDatabaseColor("menu.frame.textColor", "Menu.Frame.TextColor",
                      &mstyle->f_text, BlackPixel(display, screen), ic);
    ReadDatabaseColor("menu.frame.textShadowColor",
                      "Menu.Frame.TextShadowColor",
                      &mstyle->f_text_s, BlackPixel(display, screen), ic);
    
    ReadDatabaseColor("menu.hilite.textColor", "Menu.Hilite.TextColor",
                      &mstyle->f_hilite_text, BlackPixel(display, screen),
                      ic);
    ReadDatabaseColor("menu.hilite.textShadowColor",
                      "Menu.Hilite.TextShadowColor",
                      &mstyle->f_hilite_text_s, BlackPixel(display, screen),
                      ic);
    
    ReadDatabaseColor("menu.title.textColor", "Menu.Title.TextColor",
                      &mstyle->t_text, BlackPixel(display, screen), ic);
    ReadDatabaseColor("menu.title.textShadowColor",
                      "Menu.Title.TextShadowColor",
                      &mstyle->t_text_s, BlackPixel(display, screen), ic);

    if (XrmGetResource(database,
                       "menu.frame.textShadowXOffset",
                       "Menu.Frame.TextShadowXOffset", &value_type,
                       &value)) {
        if (sscanf(value.addr, "%d", &mstyle->wa_f_font.shodow_off_x) != 1)
            mstyle->wa_f_font.shodow_off_x = 0;
    } else
        mstyle->wa_f_font.shodow_off_x = 0;

    if (mstyle->wa_f_font.shodow_off_x > 10)
        mstyle->wa_f_font.shodow_off_x = 10;
    if (mstyle->wa_f_font.shodow_off_x < -10)
        mstyle->wa_f_font.shodow_off_x = -10;

    if (XrmGetResource(database,
                       "menu.frame.textShadowYOffset",
                       "Menu.Frame.TextShadowYOffset", &value_type,
                       &value)) {
        if (sscanf(value.addr, "%d", &mstyle->wa_f_font.shodow_off_y) != 1)
            mstyle->wa_f_font.shodow_off_y = 0;
    } else
        mstyle->wa_f_font.shodow_off_y = 0;

    if (mstyle->wa_f_font.shodow_off_y > 10)
        mstyle->wa_f_font.shodow_off_y = 10;
    if (mstyle->wa_f_font.shodow_off_y < -10)
        mstyle->wa_f_font.shodow_off_y = -10;

        if (XrmGetResource(database,
                       "menu.hilite.textShadowXOffset",
                       "Menu.Hilite.TextShadowXOffset", &value_type,
                       &value)) {
        if (sscanf(value.addr, "%d", &mstyle->wa_fh_font.shodow_off_x) != 1)
            mstyle->wa_fh_font.shodow_off_x = 0;
    } else
        mstyle->wa_fh_font.shodow_off_x = 0;

    if (mstyle->wa_fh_font.shodow_off_x > 10)
        mstyle->wa_fh_font.shodow_off_x = 10;
    if (mstyle->wa_fh_font.shodow_off_x < -10)
        mstyle->wa_fh_font.shodow_off_x = -10;

    if (XrmGetResource(database,
                       "menu.hilite.textShadowYOffset",
                       "Menu.Hilite.TextShadowYOffset", &value_type,
                       &value)) {
        if (sscanf(value.addr, "%d", &mstyle->wa_fh_font.shodow_off_y) != 1)
            mstyle->wa_fh_font.shodow_off_y = 0;
    } else
        mstyle->wa_fh_font.shodow_off_y = 0;

    if (mstyle->wa_fh_font.shodow_off_y > 10)
        mstyle->wa_fh_font.shodow_off_y = 10;
    if (mstyle->wa_fh_font.shodow_off_y < -10)
        mstyle->wa_fh_font.shodow_off_y = -10;

    if (XrmGetResource(database,
                       "menu.title.textShadowXOffset",
                       "Menu.Title.TextShadowXOffset", &value_type,
                       &value)) {
        if (sscanf(value.addr, "%d", &mstyle->wa_t_font.shodow_off_x) != 1)
            mstyle->wa_t_font.shodow_off_x = 0;
    } else
        mstyle->wa_t_font.shodow_off_x = 0;

    if (mstyle->wa_t_font.shodow_off_x > 10)
        mstyle->wa_t_font.shodow_off_x = 10;
    if (mstyle->wa_t_font.shodow_off_x < -10)
        mstyle->wa_t_font.shodow_off_x = -10;

    if (XrmGetResource(database,
                       "menu.title.textShadowYOffset",
                       "Menu.Title.TextShadowYOffset", &value_type,
                       &value)) {
        if (sscanf(value.addr, "%d", &mstyle->wa_t_font.shodow_off_y) != 1)
            mstyle->wa_t_font.shodow_off_y = 0;
    } else
        mstyle->wa_t_font.shodow_off_y = 0;

    if (mstyle->wa_t_font.shodow_off_y > 10)
        mstyle->wa_t_font.shodow_off_y = 10;
    if (mstyle->wa_t_font.shodow_off_y < -10)
        mstyle->wa_t_font.shodow_off_y = -10;

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
            mstyle->bullet = __m_wastrdup(value.addr);
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
            mstyle->checkbox_true = __m_wastrdup(value.addr);
        } else {
            look_tmp = new char[2];
            sprintf(look_tmp, "%c", ch);
            mstyle->checkbox_true = look_tmp;
        }
    } else
        mstyle->checkbox_true = __m_wastrdup("[x]");

    if (XrmGetResource(database, "menu.checkbox.false.look",
                       "Menu.Checkbox.False.Look", &value_type, &value)) {
        if (sscanf(value.addr, "'%u'", &ch) != 1) {
            mstyle->checkbox_false = __m_wastrdup(value.addr);
        } else {
            look_tmp = new char[2];
            sprintf(look_tmp, "%c", ch);
            mstyle->checkbox_false = look_tmp;
        }
    } else
        mstyle->checkbox_false = __m_wastrdup("[ ]");

    
    ReadDatabaseColor("borderColor", "BorderColor",
                      &wstyle->border_color, BlackPixel(display, screen), ic);
    mstyle->border_color = wstyle->border_color;

    ReadDatabaseColor("outlineColor", "OutlineColor",
                      &wstyle->outline_color, WhitePixel(display, screen),
                      ic);
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
        waexec(value.addr, wascreen->displaystring);

    int num = 0;
    char rc_name[50], rc_class[50];
    list<DockStyle *>::iterator dit = wascreen->wstyle.dockstyles.begin();
    for (; dit != wascreen->wstyle.dockstyles.end(); ++dit, ++num) {
        (*dit)->style.border_color = wstyle->border_color;
        (*dit)->style.texture = wstyle->t_focus;
        (*dit)->style.border_width = wstyle->border_width;
        sprintf(rc_name, "dockappholder.dock%d.frame", num);
        sprintf(rc_class, "Dockappholder.Dock%d.frame", num);        
        if (XrmGetResource(database, rc_name, rc_class, &value_type, &value))
            ReadDatabaseTexture(rc_name, rc_class, &(*dit)->style.texture,
                                WhitePixel(display, screen), ic);
        sprintf(rc_name, "dockappholder.dock%d.borderWidth", num);
        sprintf(rc_class, "Dockappholder.Dock%d.BorderWidth", num);
        if (XrmGetResource(database, rc_name, rc_class, &value_type,
                           &value)) {
            if (sscanf(value.addr, "%u", &(*dit)->style.border_width) != 1)
                (*dit)->style.border_width = wstyle->border_width;
        }
        sprintf(rc_name, "dockappholder.dock%d.borderColor", num);
        sprintf(rc_class, "Dockappholder.Dock%d.BorderColor", num);
        if (XrmGetResource(database, rc_name, rc_class, &value_type, &value))
            ReadDatabaseColor(rc_name, rc_class, &(*dit)->style.border_color,
                              BlackPixel(display, screen), ic);
    }

    WaTexture tf_tmp, tu_tmp, tp_tmp;
    WaColor cf_tmp, cu_tmp, cp_tmp;

    ReadDatabaseTexture("window.button.focus", "Window.Button.Focus",
                        &tf_tmp, WhitePixel(display, screen), ic);
    ReadDatabaseTexture("window.button.unfocus", "Window.Button.Unfocus",
                        &tu_tmp, BlackPixel(display, screen), ic);
    ReadDatabaseTexture("window.button.pressed", "Window.Button.Pressed",
                        &tp_tmp, BlackPixel(display, screen), ic);
    
    ReadDatabaseColor("window.button.focus.picColor",
                      "Window.Button.Focus.PicColor",
                      &cf_tmp, BlackPixel(display, screen), ic);
    ReadDatabaseColor("window.button.unfocus.picColor",
                      "Window.Button.Unfocus.PicColor",
                      &cu_tmp, WhitePixel(display, screen), ic);
    ReadDatabaseColor("window.button.pressed.picColor",
                      "Window.Button.Pressed.PicColor",
                      &cp_tmp, cf_tmp.getPixel(), ic);

    list<ButtonStyle *> *buttonstyles = &wascreen->wstyle.buttonstyles;

    ButtonStyle *b = new ButtonStyle;
    b->id = 0;
    b->autoplace = WestType;
    b->cb = ShadeCBoxType;
    buttonstyles->push_back(b);
    b = new ButtonStyle;
    b->id = 1;
    b->autoplace = EastType;
    b->cb = CloseCBoxType;
    buttonstyles->push_back(b);
    b = new ButtonStyle;
    b->id = 2;
    b->autoplace = EastType;
    b->cb = MaxCBoxType;
    buttonstyles->push_back(b);
    
    list<ButtonStyle *>::iterator bit = buttonstyles->begin();
    for (; bit != buttonstyles->end(); ++bit) {
        (*bit)->fg = true;
        (*bit)->x = 0;
        (*bit)->t_focused = (*bit)->t_focused2 = tf_tmp;
        (*bit)->c_focused = (*bit)->c_focused2 = cf_tmp;
        (*bit)->t_unfocused = (*bit)->t_unfocused2 = tu_tmp;
        (*bit)->c_unfocused = (*bit)->c_unfocused2 = cu_tmp;
        (*bit)->t_pressed = (*bit)->t_pressed2 = tp_tmp;
        (*bit)->c_pressed = (*bit)->c_pressed2 = cp_tmp;
    }

    bool first = true, found = true;
    for (num = 0; found; ++num) {
        found = false;
        b = new ButtonStyle;
        b->id = num;
        b->autoplace = EastType;
        b->cb = b->x = 0;
        b->fg = true;
        b->t_focused = tf_tmp;
        b->c_focused = cf_tmp;
        b->t_unfocused = tu_tmp;
        b->c_unfocused = cu_tmp;
        b->t_pressed = tp_tmp;
        b->c_pressed = cp_tmp;
        sprintf(rc_name, "window.button%d.foreground", num);
        sprintf(rc_class, "Window.Button%d.Foreground", num);
        if (XrmGetResource(database, rc_name, rc_class, &value_type,
                           &value)) {
            if (first) { LISTPTRDEL(buttonstyles); first = false; }
            if (! strncasecmp("true", value.addr, value.size)) b->fg = true;
            else b->fg = false;
            found = true;
        }
        sprintf(rc_name, "window.button%d.autoplace", num);
        sprintf(rc_class, "Window.Button%d.Autoplace", num);
        if (XrmGetResource(database, rc_name, rc_class, &value_type,
                           &value)) {
            if (first) { LISTPTRDEL(buttonstyles); first = false; }
            if (! strncasecmp("Left", value.addr, value.size))
                b->autoplace = WestType;
            else if(! strncasecmp("False", value.addr, value.size))
                b->autoplace = 0;
            else b->autoplace = EastType;
            found = true;
        }
        sprintf(rc_name, "window.button%d.position", num);
        sprintf(rc_class, "Window.Button%d.Position", num);
        if (XrmGetResource(database, rc_name, rc_class, &value_type,
                           &value)) {
            if (first) { LISTPTRDEL(buttonstyles); first = false; }
            if (! sscanf(value.addr, "%d", &b->x)) {
                b->autoplace = EastType;
            }
            else if (b->x != 0) b->autoplace = 0;
            found = true;
        }
        sprintf(rc_name, "window.button%d.state", num);
        sprintf(rc_class, "Window.Button%d.State", num);
        if (XrmGetResource(database, rc_name, rc_class, &value_type,
                           &value)) {
            if (first) { LISTPTRDEL(buttonstyles); first = false; }
            if (! strncasecmp("SHADED", value.addr, value.size))
                b->cb = ShadeCBoxType;
            else if(! strncasecmp("MAXIMIZED", value.addr, value.size))
                b->cb = MaxCBoxType;
            else if(! strncasecmp("MINIMIZED", value.addr, value.size))
                b->cb = MinCBoxType;
            else if(! strncasecmp("STICKY", value.addr, value.size))
                b->cb = StickCBoxType;
            else if(! strncasecmp("ALWAYSONTOP", value.addr, value.size))
                b->cb = AOTCBoxType;
            else if(! strncasecmp("ALWAYSATBOTTOM", value.addr, value.size))
                b->cb = AABCBoxType;
            else if(! strncasecmp("DECORTITLE", value.addr, value.size))
                b->cb = TitleCBoxType;
            else if(! strncasecmp("DECORHANDLE", value.addr, value.size))
                b->cb = HandleCBoxType;
            else if(! strncasecmp("DECORBORDER", value.addr, value.size))
                b->cb = BorderCBoxType;
            else if(! strncasecmp("DECORALL", value.addr, value.size))
                b->cb = AllCBoxType;
            else if(! strncasecmp("FULLSCREEN", value.addr, value.size))
                b->cb = FsCBoxType;
            else if(! strncasecmp("CLOSE", value.addr, value.size))
                b->cb = CloseCBoxType;
            found = true;
        }
        sprintf(rc_name, "window.button%d.false.focus", num);
        sprintf(rc_class, "Window.Button%d.False.Focus", num);
        if (XrmGetResource(database, rc_name, rc_class, &value_type,
                           &value)) {
            if (first) { LISTPTRDEL(buttonstyles); first = false; }
            ReadDatabaseTexture(rc_name, rc_class, &b->t_focused,
                                WhitePixel(display, screen), ic);
            found = true;
        }        
        sprintf(rc_name, "window.button%d.false.focus.picColor", num);
        sprintf(rc_class, "Window.Button%d.False.Focus.PicColor", num);
        if (XrmGetResource(database, rc_name, rc_class, &value_type,
                           &value)) {
            if (first) { LISTPTRDEL(buttonstyles); first = false; }
            ReadDatabaseColor(rc_name, rc_class, &b->c_focused,
                              BlackPixel(display, screen), ic);
            found = true;
        }
        sprintf(rc_name, "window.button%d.false.unfocus", num);
        sprintf(rc_class, "Window.Button%d.False.Unfocus", num);
        if (XrmGetResource(database, rc_name, rc_class, &value_type,
                           &value)) {
            if (first) { LISTPTRDEL(buttonstyles); first = false; }
            ReadDatabaseTexture(rc_name, rc_class, &b->t_unfocused,
                                WhitePixel(display, screen), ic);
            found = true;
        }
        sprintf(rc_name, "window.button%d.false.unfocus.picColor", num);
        sprintf(rc_class, "Window.Button%d.False.Unfocus.PicColor", num);
        if (XrmGetResource(database, rc_name, rc_class, &value_type,
                           &value)) {
            if (first) { LISTPTRDEL(buttonstyles); first = false; }
            ReadDatabaseColor(rc_name, rc_class, &b->c_unfocused,
                              BlackPixel(display, screen), ic);
            found = true;
        }
        sprintf(rc_name, "window.button%d.false.pressed", num);
        sprintf(rc_class, "Window.Button%d.False.Pressed", num);
        if (XrmGetResource(database, rc_name, rc_class, &value_type,
                           &value)) {
            if (first) { LISTPTRDEL(buttonstyles); first = false; }
            ReadDatabaseTexture(rc_name, rc_class, &b->t_pressed,
                                WhitePixel(display, screen), ic);
            found = true;
        }
        sprintf(rc_name, "window.button%d.false.pressed.picColor", num);
        sprintf(rc_class, "Window.Button%d.False.Pressed.PicColor", num);
        if (XrmGetResource(database, rc_name, rc_class, &value_type,
                           &value)) {
            if (first) { LISTPTRDEL(buttonstyles); first = false; }
            ReadDatabaseColor(rc_name, rc_class, &b->c_pressed,
                              BlackPixel(display, screen), ic);
            found = true;
        }
        b->t_focused2 = b->t_focused;
        b->c_focused2 = b->c_focused;
        b->t_unfocused2 = b->t_unfocused;
        b->c_unfocused2 = b->c_unfocused;
        b->t_pressed2 = b->t_pressed;
        b->c_pressed2 = b->c_pressed;
        
        sprintf(rc_name, "window.button%d.true.focus", num);
        sprintf(rc_class, "Window.Button%d.True.Focus", num);
        if (XrmGetResource(database, rc_name, rc_class, &value_type,
                           &value)) {
            if (first) { LISTPTRDEL(buttonstyles); first = false; }
            ReadDatabaseTexture(rc_name, rc_class, &b->t_focused2,
                                WhitePixel(display, screen), ic);
            found = true;
        }        
        sprintf(rc_name, "window.button%d.true.focus.picColor", num);
        sprintf(rc_class, "Window.Button%d.True.Focus.PicColor", num);
        if (XrmGetResource(database, rc_name, rc_class, &value_type,
                           &value)) {
            if (first) { LISTPTRDEL(buttonstyles); first = false; }
            ReadDatabaseColor(rc_name, rc_class, &b->c_focused2,
                              BlackPixel(display, screen), ic);
            found = true;
        }
        sprintf(rc_name, "window.button%d.true.unfocus", num);
        sprintf(rc_class, "Window.Button%d.True.Unfocus", num);
        if (XrmGetResource(database, rc_name, rc_class, &value_type,
                           &value)) {
            if (first) { LISTPTRDEL(buttonstyles); first = false; }
            ReadDatabaseTexture(rc_name, rc_class, &b->t_unfocused2,
                                WhitePixel(display, screen), ic);
            found = true;
        }
        sprintf(rc_name, "window.button%d.true.unfocus.picColor", num);
        sprintf(rc_class, "Window.Button%d.True.Unfocus.PicColor", num);
        if (XrmGetResource(database, rc_name, rc_class, &value_type,
                           &value)) {
            if (first) { LISTPTRDEL(buttonstyles); first = false; }
            ReadDatabaseColor(rc_name, rc_class, &b->c_unfocused2,
                              BlackPixel(display, screen), ic);
            found = true;
        }
        sprintf(rc_name, "window.button%d.true.pressed", num);
        sprintf(rc_class, "Window.Button%d.True.Pressed", num);
        if (XrmGetResource(database, rc_name, rc_class, &value_type,
                           &value)) {
            if (first) { LISTPTRDEL(buttonstyles); first = false; }
            ReadDatabaseTexture(rc_name, rc_class, &b->t_pressed2,
                                WhitePixel(display, screen), ic);
            found = true;
        }
        sprintf(rc_name, "window.button%d.true.pressed.picColor", num);
        sprintf(rc_class, "Window.Button%d.True.Pressed.PicColor", num);
        if (XrmGetResource(database, rc_name, rc_class, &value_type,
                           &value)) {
            if (first) { LISTPTRDEL(buttonstyles); first = false; }
            ReadDatabaseColor(rc_name, rc_class, &b->c_pressed2,
                              BlackPixel(display, screen), ic);
            found = true;
        }
        if (found) buttonstyles->push_back(b);
        else delete b;
    }
    wstyle->b_num = buttonstyles->size();
        
    XrmDestroyDatabase(database);
}

/**
 * @fn    LoadMenus(WaScreen *wascreen)
 * @brief Reads menu file
 *
 * Creates menus by parsing the menu file.
 *
 * @param wascreen WaScreen to load menus for
 */
void ResourceHandler::LoadMenus(WaScreen *wascreen) {
    FILE *file;

    if (! (file = fopen(wascreen->config.menu_file, "r"))) {
        WARNING << "can't open menufile `" << wascreen->config.menu_file << 
            "' for reading" << endl;
        return;
    }
    while (! feof(file)) ParseMenu(NULL, file, wascreen);
    fclose(file);
}

/**
 * @fn    LoadActions(WaScreen *wascreen)
 * @brief Reads action file
 *
 * Creates action lists by parsing the action file.
 *
 * @param wascreen WaScreen to create action lists for
 */
void ResourceHandler::LoadActions(WaScreen *wascreen) {
    ScreenConfig *sc = &wascreen->config;
    FILE *file;
    int i, i2, i3;
    bool cmd;
    int ret;
    char buffer[8192];
    char buffer2[8192];
    char *str;
    WaActionExtList *ext_list;
    list<Define *> *defs = new list<Define *>;
    sc->bacts = new list<WaAction *>*[wascreen->wstyle.b_num];
    sc->ext_bacts = new list<WaActionExtList *>*[wascreen->wstyle.b_num];
    for (i = 0; i < wascreen->wstyle.b_num; i++) {
        sc->bacts[i] = new list<WaAction *>;
        sc->ext_bacts[i] = new list<WaActionExtList *>;
    }
    
    if (! (file = fopen(sc->action_file, "r"))) {
        WARNING << "can't open action file `" << sc->action_file << 
            "' for reading" << endl;
        return;
    }
    for (;;) {
        for (i = 0; (ret = fgetc(file)) != EOF &&
                 ret != '{'; i++) {
            buffer[i] = ret;
            if (buffer[i] == '#' || buffer[i] == '!') {
                i--;
                while ((ret = fgetc(file)) != EOF && ret != '\n');
            }
        }
        if (ret == EOF) {
            fclose(file);
            LISTPTRDEL(defs);
            delete defs;
            return;
        }
        else buffer[i] = ret;
        str = strtrim(buffer);
        switch (buffer[i]) {
            case '\n':
                i = 0;
                break;
            case '{':
                buffer[i] = '\0';
                cmd = false;
                for (i2 = 0; (ret = fgetc(file)) != EOF &&
                         (ret != '}' || cmd); i2++) {
                    buffer2[i2] = ret;
                    if (buffer2[i2] == '{') cmd = true;
                    if (buffer2[i2] == '}') cmd = false;
                    if (buffer2[i2] == '#' || buffer[i] == '!') {
                        i2--;
                        while ((ret = fgetc(file)) != EOF && ret != '\n');
                    }
                }
                buffer2[i2] = ret;
                if (ret == EOF) ERROR << "missing '}'" << endl;
                buffer2[i2] = '\0';
                if (! strncasecmp(str, "DEF", 3)) {
                    str = strtrim(str + 3);                   
                    defs->push_front(new Define(str, strtrim(buffer2)));
                }
                else {                  
                    str = strtrim(str);
                    if (! strcasecmp(str, "root")) {
                        ReadActions((char *) buffer2, defs, &racts,
                                    &sc->rootacts, wascreen);
                    }
                    else if (! strcasecmp(str, "westedge")) {
                        ReadActions((char *) buffer2, defs, &racts,
                                    &sc->weacts, wascreen);
                    }
                    else if (! strcasecmp(str, "eastedge")) {
                        ReadActions((char *) buffer2, defs, &racts,
                                    &sc->eeacts, wascreen);
                    }
                    else if (! strcasecmp(str, "northedge")) {
                        ReadActions((char *) buffer2, defs, &racts,
                                    &sc->neacts, wascreen);
                    }
                    else if (! strcasecmp(str, "southedge")) {
                        ReadActions((char *) buffer2, defs, &racts,
                                    &sc->seacts, wascreen);
                    }
                    else if (! strcasecmp(str, "menu.title")) {
                        ReadActions((char *) buffer2, defs, &macts,
                                    &sc->mtacts, wascreen);
                    }
                    else if (! strcasecmp(str, "menu.item")) {
                        ReadActions((char *) buffer2, defs, &macts,
                                    &sc->miacts, wascreen);
                    }
                    else if (! strcasecmp(str, "menu.sub")) {
                        ReadActions((char *) buffer2, defs, &macts,
                                    &sc->msacts, wascreen);
                    }
                    else if (! strcasecmp(str, "menu.checkbox")) {
                        ReadActions((char *) buffer2, defs, &macts,
                                    &sc->mcbacts, wascreen);
                    }
                    else {
                        ext_list = NULL;
                        if (str[0] == 'c' && str[1] == '/') {
                            for (i3 = 2; str[i3] != '\0' &&
                                     ! (str[i3] == '/' && str[i3 - 1] != '\\');
                                 i3++);
                            if (str[i3] == '\0') {
                                WARNING << "missing '/'" << endl;
                                break;
                            }
                            str[i3] = '\0';
                            ext_list = new WaActionExtList(NULL, str + 2,
                                                           NULL);
                            str = str + i3 + 1;
                            ReadActions((char *) buffer2, defs, &wacts,
                                        &ext_list->list, wascreen);
                        }
                        else if (str[0] == 'n' && str[1] == '/') {
                            for (i3 = 2; str[i3] != '\0' &&
                                     ! (str[i3] == '/' && str[i3 - 1] != '\\');
                                 i3++);
                            if (str[i3] == '\0') {
                                WARNING << "missing '/'" << endl;
                                break;
                            }
                            str[i3] = '\0';
                            ext_list = new WaActionExtList(str + 2, NULL,
                                                           NULL);
                            str = str + i3 + 1;
                            ReadActions((char *) buffer2, defs, &wacts,
                                        &ext_list->list, wascreen);
                        }
                        else if (str[0] == 't' && str[1] == '/') {
                            for (i3 = 2; str[i3] != '\0' &&
                                     ! (str[i3] == '/' && str[i3 - 1] != '\\');
                                 i3++);
                            if (str[i3] == '\0') {
                                WARNING << "missing '/'" << endl;
                                break;
                            }
                            str[i3] = '\0';
                            ext_list = new WaActionExtList(NULL, NULL,
                                                           str + 2);
                            str = str + i3 + 1;
                            ReadActions((char *) buffer2, defs, &wacts,
                                        &ext_list->list, wascreen);
                        }
                        else if (! strncasecmp(str, "window", 6)) {
                            str = str + 6;
                        }
                        else {
                            WARNING << "unknown window: " << str << endl;
                            break;
                        }
                        if (! strcasecmp(str, ".frame")) {
                            if (ext_list)
                                sc->ext_frameacts.push_back(ext_list);
                            else ReadActions((char *) buffer2, defs, &wacts,
                                             &sc->frameacts, wascreen);
                        }
                        else if (! strcasecmp(str, ".title")) {
                            if (ext_list)
                                sc->ext_titleacts.push_back(ext_list);
                            else ReadActions((char *) buffer2, defs, &wacts,
                                             &sc->titleacts, wascreen);
                        }
                        else if (! strcasecmp(str, ".label")) {
                            if (ext_list)
                                sc->ext_labelacts.push_back(ext_list);
                            else ReadActions((char *) buffer2, defs, &wacts,
                                             &sc->labelacts, wascreen);
                        }
                        else if (! strcasecmp(str, ".handle")) {
                            if (ext_list)
                                sc->ext_handleacts.push_back(ext_list);
                            else ReadActions((char *) buffer2, defs, &wacts,
                                             &sc->handleacts, wascreen);
                        }
                        else if (! strcasecmp(str, ".activeclient")) {
                            if (ext_list)
                                sc->ext_awinacts.push_back(ext_list);
                            else ReadActions((char *) buffer2, defs, &wacts,
                                             &sc->awinacts, wascreen);
                        }
                        else if (! strcasecmp(str, ".passiveclient")) {
                            if (ext_list)
                                sc->ext_pwinacts.push_back(ext_list);
                            else ReadActions((char *) buffer2, defs, &wacts,
                                             &sc->pwinacts, wascreen);
                        }
                        else if (! strcasecmp(str, ".leftgrip")) {
                            if (ext_list) sc->ext_lgacts.push_back(ext_list);
                            else ReadActions((char *) buffer2, defs, &wacts,
                                             &sc->lgacts, wascreen);
                        }
                        else if (! strcasecmp(str, ".rightgrip")) {
                            if (ext_list) sc->ext_rgacts.push_back(ext_list);
                            else ReadActions((char *) buffer2, defs, &wacts,
                                             &sc->rgacts, wascreen);
                        }
                        else if (! strncasecmp(str, ".button", 7)) {
                            int id;
                            if (strlen(str) > 7) {
                                id = atoi(str + 7);
                                if (id < 0 || id >= wascreen->wstyle.b_num)
                                    WARNING << "bad button id: " << id <<
                                        endl;
                                else {
                                    if (ext_list)
                                        sc->ext_bacts[id]->push_back(
                                            ext_list);
                                    else
                                        ReadActions((char *) buffer2, defs,
                                                    &wacts, sc->bacts[id],
                                                    wascreen);
                                }
                            }
                        }
                        else {
                            WARNING << "unknown child window: " << str << endl;
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
 *                    list<WaAction *> *insert,
 *                    WaScreen *wascreen)
 * @brief Parses a block of actions
 *
 * Parses a block of action lines. All defines are replaced with actual lines
 * and then parsed.
 *
 * @param defs List with temporary defined action lists
 * @param comp List with available actions
 * @param insert List to insert action in
 * @param wascreen WaScreen to create action list for
 */
void ResourceHandler::ReadActions(char s[8192],
                                  list<Define *> *defs,
                                  list<StrComp *> *comp,
                                  list<WaAction *> *insert,
                                  WaScreen *wascreen) {
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
            ParseAction(ts, comp, insert, wascreen);           
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
    int opacity;
    
    if (XrmGetResource(database, rname, rclass, &value_type,
                       &value)) {
        strtrim(value.addr);
        ic->parseColor(color, value.addr);
    } else {
        ic->parseColor(color);
        color->setPixel(default_pixel);
    }

    int clen = strlen(rclass) + 9, nlen = strlen(rname) + 9;
    char *oclass = new char[clen], *oname = new char[nlen];
    
    sprintf(oclass, "%s.Opacity", rclass);
    sprintf(oname,  "%s.opacity", rname);
    if (XrmGetResource(database, oname, oclass, &value_type, &value))
        opacity = atoi(value.addr);
    else
        opacity = 0;
    
    if (opacity > 100) opacity = 100;
    else if (opacity < 0) opacity = 0;

#ifdef XFT
    color->setXftOpacity(opacity);
#endif // XFT

    delete [] oclass;
    delete [] oname;
}

/**
 * @fn    ReadDatabaseTexture(char *rname, char *rclass,
 *                            WaColor *color,
 *                            unsigned long default_pixel,
 *                            WaImageControl *ic)
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

    Colormap colormap = ic->getColormap();

    if (XrmGetResource(database, rname, rclass, &value_type,
                       &value))
        ic->parseTexture(texture, value.addr);
    else
        texture->setTexture(WaImage_Solid | WaImage_Flat);

    int clen = strlen(rclass) + 32, nlen = strlen(rname) + 32;
    char *colorclass = new char[clen], *colorname = new char[nlen];

#ifdef PIXMAP
    if (texture->getTexture() & WaImage_Pixmap) {
        int clen = strlen(rclass) + 20, nlen = strlen(rname) + 20;
        char *pixmapclass = new char[clen], *pixmapname = new char[nlen];
        char pixmap_path[1024];

        imlib_context_push(ic->getWaScreen()->imlib_context);
        texture->setContext(&ic->getWaScreen()->imlib_context);
        imlib_context_set_mask(0);
        
        Imlib_Border bd;
        Imlib_Image image = NULL;

        sprintf(pixmapclass, "%s.Pixmap", rclass);
        sprintf(pixmapname,  "%s.pixmap", rname);
        if (XrmGetResource(database, pixmapname, pixmapclass, &value_type,
                           &value)) {
            if (strstr(value.addr, "/")) {
                if (! (image = imlib_load_image(value.addr)))
                    WARNING << "failed loading image `" << value.addr <<
                        "'\n";
            }
            else {
                sprintf(pixmap_path, "%s/%s",
                        ic->getWaScreen()->config.style_file, value.addr);
                if (! (image = imlib_load_image(pixmap_path))) {
                    WARNING << "failed loading image `" << value.addr <<
                        "'\n";
                }
            }
        }
        if (image) {
            texture->setPixmap(image);
            if (texture->getTexture() & WaImage_Stretch) {
                sprintf(pixmapclass, "%s.Border", rclass);
                sprintf(pixmapname,  "%s.border", rname);
                
                imlib_context_set_image(image);
                if (XrmGetResource(database, pixmapname, pixmapclass,
                                   &value_type, &value)) {
                    sscanf(value.addr, "{ %u, %u, %u, %u }",
                           (unsigned int *) &bd.left,
                           (unsigned int *) &bd.right,
                           (unsigned int *) &bd.top,
                           (unsigned int *) &bd.bottom);
                    if (bd.left > imlib_image_get_width())
                        bd.left = imlib_image_get_width();
                    if (bd.right > imlib_image_get_width())
                        bd.right = imlib_image_get_width();
                    if ((bd.left + bd.right) > imlib_image_get_width())
                        bd.right = imlib_image_get_width() - bd.left - 1;

                    if (bd.top > imlib_image_get_height())
                        bd.top = imlib_image_get_height();
                    if (bd.bottom > imlib_image_get_height())
                        bd.bottom = imlib_image_get_height();
                    if ((bd.top + bd.bottom) > imlib_image_get_height())
                        bd.bottom = imlib_image_get_width() - bd.top - 1;
                }
                else {
                    bd.left = imlib_image_get_width() / 2;
                    bd.right = imlib_image_get_width() - bd.left - 1;
                    bd.top = imlib_image_get_height() / 2;
                    bd.bottom = imlib_image_get_height() - bd.top - 1;
                }
                imlib_image_set_border(&bd);
            }
        }
        else
            texture->setTexture(WaImage_Solid | WaImage_Flat);
        
        delete [] pixmapclass;
        delete [] pixmapname;
        imlib_context_pop();
    }
#endif // PIXMAP
        
    if (texture->getTexture() & WaImage_Solid) {
        
        sprintf(colorclass, "%s.Color", rclass);
        sprintf(colorname,  "%s.color", rname);
        strtrim(colorclass);
        strtrim(colorname);
        
        ReadDatabaseColor(colorname, colorclass, texture->getColor(),
                          default_pixel, ic);

#ifdef INTERLACE
        sprintf(colorclass, "%s.ColorTo", rclass);
        sprintf(colorname,  "%s.colorTo", rname);
        strtrim(colorclass);
        strtrim(colorname);

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
                                        (texture->getColor()->getBlue() >>
                                         1));
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
                                (texture->getColor()->getGreen() >> 1)) *
                0xff;
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
        strtrim(colorclass);
        strtrim(colorname);
        
        sprintf(colortoclass, "%s.ColorTo", rclass);
        sprintf(colortoname,  "%s.colorTo", rname);
        strtrim(colortoclass);
        strtrim(colortoname);

        ReadDatabaseColor(colorname, colorclass, texture->getColor(),
                          default_pixel, ic);
        ReadDatabaseColor(colortoname, colortoclass, texture->getColorTo(),
                          default_pixel, ic);

        delete [] colortoclass;
        delete [] colortoname;
    }

#ifdef RENDER
    if (texture->getTexture() & WaImage_ParentRelative) {
        delete [] colorclass;
        delete [] colorname;
        return;
    }

    if (! ic->getWaScreen()->render_extension) {
        texture->setOpacity(0);
        delete [] colorclass;
        delete [] colorname;
        return;
    }
    
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
        xformat = XRenderFindFormat(ic->getDisplay(), PictFormatType |
                                    PictFormatDepth | PictFormatAlphaMask,
                                    &Rpf, 0);
        alphaPixmap = XCreatePixmap(ic->getDisplay(), ic->getDrawable(),
                                    1, 1, 8);
        alphaPicture = XRenderCreatePicture(ic->getDisplay(), alphaPixmap,
                                            xformat, CPRepeat, &Rpa);
        XRenderFillRectangle(ic->getDisplay(), PictOpSrc, alphaPicture, &clr,
                             0, 0, 1, 1);
        texture->setAlphaPicture(alphaPicture);
        XFreePixmap(ic->getDisplay(), alphaPixmap);
        if (texture->getTexture() == (WaImage_Solid | WaImage_Flat)) {
            Rpf.depth = ic->getDepth();
            xformat = XRenderFindFormat(ic->getDisplay(), PictFormatType |
                                        PictFormatDepth,
                                        &Rpf, 0);
            solidPixmap = XCreatePixmap(ic->getDisplay(), ic->getDrawable(),
                                        1, 1, ic->getDepth());
            solidPicture = XRenderCreatePicture(ic->getDisplay(), solidPixmap,
                                                xformat, CPRepeat, &Rpa);
            XRenderFillRectangle(ic->getDisplay(), PictOpSrc, solidPicture,
                                 texture->getColor()->getXRenderColor(),
                                 0, 0, 1, 1);
            texture->setSolidPicture(solidPicture);
            XFreePixmap(ic->getDisplay(), solidPixmap);
        }
    }
#endif // RENDER
        
    delete [] colorclass;
    delete [] colorname;
}

/**
 * @fn    ReadDatabaseFont(char *rname, char *rclass,
 *                         WaFont *font, WaFont *defaultfont)
 * @brief Reads a font
 *
 * Reads a font from resource database.
 *
 * @param rname Resource name to use
 * @param rclass Resource class name to use
 * @param font Pointer to WaFont structure
 * @param defaultfont Font to use if resource doesn't exist  
 */
void ResourceHandler::ReadDatabaseFont(char *rname, char *rclass,
                                       WaFont *font, WaFont *defaultfont) {
    XrmValue value;
    char *value_type;
    char *xft_match;
    char *f;
    char *__m_wastrdup_tmp;
    
    if (XrmGetResource(database, rname, rclass, &value_type, &value)) {        
        f = value.addr;
        font->xft = false;
        
        if ((xft_match = strchr(f, '['))) {
            xft_match[0] = '\0';
            
#ifdef XFT
            if (xft_match[1] != '\0' && xft_match[2] != '\0' &&
                xft_match[3] != '\0')
                if (strncasecmp(&xft_match[1], "XFT", 3) == 0)
                    font->xft = true;
#endif // XFT
            
        }
        font->font = __m_wastrdup(f);
        strtrim(font->font);
        if (xft_match) xft_match[0] = '[';
    } else {
        font->xft = defaultfont->xft;
        font->font = __m_wastrdup(defaultfont->font);
    }
}

/**
 * @fn    ParseAction(const char *_s, list<StrComp *> *comp,
 *                    list<WaAction *> *insert, WaScreen *wascreen)
 * @brief Parses an action line
 *
 * Parses an action line into an action object and inserts it in action list.
 *
 * @param _s Action line to parse
 * @param comp List with available actions
 * @param insert List to insert action in
 * @param wascreen WaScreen to parse action for
 */
void ResourceHandler::ParseAction(const char *_s, list<StrComp *> *comp,
                                  list<WaAction *> *insert,
                                  WaScreen *wascreen) {
    char *line, *token, *par, *tmp_par;
    int i, detail, mod;
    WaAction *act_tmp;
    KeySym keysym;
    list<StrComp *>::iterator it;
    char *__m_wastrdup_tmp;
    char *s = NULL;

    int min_key, max_key;
    XDisplayKeycodes(wascreen->display, &min_key, &max_key);
    
    act_tmp = new WaAction;
    act_tmp->replay = false;
    act_tmp->delay.tv_sec = act_tmp->delay.tv_usec = 0;
    act_tmp->delay_breaks = NULL;
    
    line = __m_wastrdup((char *) _s);
    
    detail = strchr(line, '=') ? 1: 0;
    mod    = strchr(line, '&') ? 1: 0;
    token  = strtok(line, ":");
    token  = strtrim(token);
    if (*token == '*') {
        act_tmp->replay = true;
        token++;
    }
    
    tmp_par = __m_wastrdup(token);
    par = tmp_par;
    
    act_tmp->exec = NULL;
    act_tmp->param = NULL;
    for (; *par != '(' && *par != '\0'; par++);
    if (*(par++) == '(') {
        for (i = 0; par[i] != ')'; i++)
            if (par[i] == '\0') {
                WARNING << "missing `)' in resource line `" << s << "'" 
                        << endl;
                delete act_tmp;
                delete [] line;
                if (s) delete [] s; s = NULL;
                return;
            }
        if (strlen(par)) {
            par[i] = '\0';
            act_tmp->param = param_eval(token, par, wascreen);
        }
        for (i = 0; token[i] != '('; i++);
        token[i] = '\0';
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
        if (s) delete [] s; s = NULL;
        if ((s = strwithin(token, '{', '}'))) {
            act_tmp->exec = __m_wastrdup(s);
        } else {
            WARNING << "`" << token << "' unknown action" << endl;
            delete act_tmp;
            delete [] line;
            if (s) delete [] s; s = NULL;
            return;
        }
    }

    if (! act_tmp->param || *(act_tmp->param) == '\0') {
        if (act_tmp->winfunc && (
            act_tmp->winfunc == (WwActionFn) &WaWindow::MenuMap ||
            act_tmp->winfunc == (WwActionFn) &WaWindow::MenuRemap ||
            act_tmp->winfunc == (WwActionFn) &WaWindow::MenuMapFocused ||
            act_tmp->winfunc == (WwActionFn) &WaWindow::MenuRemapFocused ||
            act_tmp->winfunc == (WwActionFn) &WaWindow::MenuUnmap ||
            act_tmp->winfunc == (WwActionFn) &WaWindow::MenuUnmapFocus ||
            act_tmp->winfunc == (WwActionFn) &WaWindow::PointerRelativeWarp ||
            act_tmp->winfunc == (WwActionFn) &WaWindow::PointerFixedWarp ||
            act_tmp->winfunc == (WwActionFn) &WaWindow::ViewportRelativeMove ||
            act_tmp->winfunc == (WwActionFn) &WaWindow::ViewportFixedMove ||
            act_tmp->winfunc == (WwActionFn) &WaWindow::GoToDesktop ||
            act_tmp->winfunc == (WwActionFn) &WaWindow::PartDesktop ||
            act_tmp->winfunc == (WwActionFn) &WaWindow::JoinDesktop ||
            act_tmp->winfunc == (WwActionFn) &WaWindow::DesktopMask ||
            act_tmp->winfunc == (WwActionFn)
            &WaWindow::PartCurrentJoinDesktop ||
            act_tmp->winfunc == (WwActionFn) &WaWindow::VertMergeWithWindow ||
            act_tmp->winfunc == (WwActionFn) &WaWindow::HorizMergeWithWindow ||
            act_tmp->winfunc == (WwActionFn)
            &WaWindow::CloneMergeWithWindow ||
            act_tmp->winfunc == (WwActionFn) &WaWindow::SetMergeMode)) {
            WARNING "`" << token << "' action must have a parameter" <<
                endl;
            delete act_tmp;
            delete [] line;
            if (s) delete [] s; s = NULL;
            return;
        }
        if (act_tmp->rootfunc && (
            act_tmp->rootfunc == (RootActionFn) &WaScreen::MenuMap ||
            act_tmp->rootfunc == (RootActionFn) &WaScreen::MenuRemap ||
            act_tmp->rootfunc == (RootActionFn) &WaScreen::MenuMapFocused ||
            act_tmp->rootfunc == (RootActionFn) &WaScreen::MenuRemapFocused ||
            act_tmp->rootfunc == (RootActionFn) &WaScreen::MenuUnmap ||
            act_tmp->rootfunc == (RootActionFn) &WaScreen::MenuUnmapFocus ||
            act_tmp->rootfunc == (RootActionFn)
            &WaScreen::PointerRelativeWarp ||
            act_tmp->rootfunc == (RootActionFn) &WaScreen::PointerFixedWarp ||
            act_tmp->rootfunc == (RootActionFn)
            &WaScreen::ViewportRelativeMove ||
            act_tmp->rootfunc == (RootActionFn) &WaScreen::ViewportFixedMove ||
            act_tmp->rootfunc == (RootActionFn) &WaScreen::GoToDesktop)) {
            WARNING "`" << token << "' action must have a parameter" <<
                endl;
            delete act_tmp;
            delete [] line;
            if (s) delete [] s; s = NULL;
            return;
        }
        if (act_tmp->menufunc && (
            act_tmp->menufunc == (MenuActionFn) &WaMenuItem::MenuMap ||
            act_tmp->menufunc == (MenuActionFn) &WaMenuItem::MenuRemap ||
            act_tmp->menufunc == (MenuActionFn) &WaMenuItem::MenuMapFocused ||
            act_tmp->menufunc == (MenuActionFn)
            &WaMenuItem::MenuRemapFocused ||
            act_tmp->menufunc == (MenuActionFn) &WaMenuItem::MenuUnmap ||
            act_tmp->menufunc == (MenuActionFn) &WaMenuItem::MenuUnmapFocus ||
            act_tmp->menufunc == (MenuActionFn)
            &WaMenuItem::PointerRelativeWarp ||
            act_tmp->menufunc == (MenuActionFn)
            &WaMenuItem::PointerFixedWarp ||
            act_tmp->menufunc == (MenuActionFn)
            &WaMenuItem::ViewportRelativeMove ||
            act_tmp->menufunc == (MenuActionFn)
            &WaMenuItem::ViewportFixedMove ||
            act_tmp->menufunc == (MenuActionFn) &WaMenuItem::GoToDesktop)) {
            WARNING "`" << token << "' action must have a parameter" <<
                endl;
            delete act_tmp;
            delete [] line;
            if (s) delete [] s; s = NULL;
            return;
        }
    }
    
    if (detail) token = strtok(NULL, "=");
    else {
        if (mod) token = strtok(NULL, "&");
        else token = strtok(NULL, "[");
    }
    if (! token) {
        WARNING << "`" << _s << "' no event type in action line" << endl;
        delete act_tmp;
        delete [] line;
        if (s) delete [] s; s = NULL;
        return;
    }
    token = strtrim(token);
    
    it = types.begin();
    for (; it != types.end(); ++it) {
        if ((*it)->Comp(token)) {
            act_tmp->type = (*it)->value;
            break;
        }
    }
    if (! *it) {
        WARNING << "`" << token << "' unknown type" << endl;
        delete act_tmp;
        delete [] line;
        if (s) delete [] s; s = NULL;
        return;
    }
    
    act_tmp->detail = 0;
    if (detail) {
        if (mod) token = strtok(NULL, "&");
        else token = strtok(NULL, "[");
            
        token = strtrim(token);
        if (act_tmp->type == KeyPress || act_tmp->type == KeyRelease) {
            if (! strcasecmp(token, "anykey"))
                act_tmp->detail = 0;
            else {
                if ((keysym = XStringToKeysym(token)) == NoSymbol) {
                    WARNING << "`" << token << "' unknown key" << endl;
                    delete act_tmp;
                    delete [] line;
                    if (s) delete [] s; s = NULL;
                    return;
                } else {
                    act_tmp->detail = XKeysymToKeycode(display, keysym);
                    if (act_tmp->detail < (unsigned int) min_key ||
                        act_tmp->detail > (unsigned int) max_key) {
                        WARNING << "`" << token << "' bad keycode" << endl;
                        delete act_tmp;
                        delete [] line;
                        if (s) delete [] s; s = NULL;
                        return;
                    }   
                }
            }
        } else if (act_tmp->type == ButtonPress ||
                   act_tmp->type == ButtonRelease ||
                   act_tmp->type == DoubleClick) {
            it = bdetails.begin();
            for (; it != bdetails.end(); ++it) {
                if ((*it)->Comp(token)) {
                    act_tmp->detail = (*it)->value;
                    break;
                }
            }
            if (! *it) {
                WARNING << "`" << token << "' unknown detail" << endl;
                delete act_tmp;
                delete [] line;
                if (s) delete [] s; s = NULL;
                return;
            }
        }        
    }

    bool negative;
    act_tmp->mod = act_tmp->nmod = 0;
    if (mod) {
        token = strtok(NULL, "[");
        for (token = strtok(token, "&"); token; token = strtok(NULL, "&")) {
            token = strtrim(token);
            negative = false;
            if (*token == '!') {
                negative = true;
                token = strtrim(token + 1);
            }
            for (it = mods.begin(); it != mods.end(); ++it) {
                if ((*it)->Comp(token)) {
                    if (negative)
                        act_tmp->nmod |= (*it)->value;
                    else
                        act_tmp->mod |= (*it)->value;
                    break;
                }
            }
            if (! *it) {
                WARNING << "`" << token << "' unknown modifier " <<
                    "or bad modifier key" << endl;
                delete act_tmp;
                delete [] line;
                if (s) delete [] s; s = NULL;
                return;
            }
        }
    }
    if ((token = strtok(NULL, "]"))) {
        int msdelay = 0;
        act_tmp->delay_breaks = new list<int>;
        if ((token = strtok(token, ":"))) {
            token = strtrim(token);
            msdelay = atoi(token);
            act_tmp->delay.tv_usec = (msdelay % 1000) * 1000;
            act_tmp->delay.tv_sec = msdelay / 1000;
            act_tmp->delay_breaks = new list<int>;
            while ((token = strtok(NULL, "|"))) {
                token = strtrim(token);
                it = types.begin();
                for (; it != types.end(); ++it) {
                    if ((*it)->Comp(token)) {
                        act_tmp->delay_breaks->push_back((*it)->value);
                        break;
                    }
                }
                if (! *it) {
                    WARNING << "`" << token <<
                        "' unknown break event type" << endl;
                }
            }
        }
    }
    delete [] line;
    insert->push_back(act_tmp);
    if (s) delete [] s; s = NULL;
}

/**
 * @fn    ParseMenu(WaMenu *menu, FILE *file, WaScreen *wascreen)
 * @brief Parses a menu file
 *
 * Parses a menu section of the menu file and creates a menu object for the
 * menu. If a [start] or [begin] statement is found when parsing a menu, we
 * make a recursive function call to this function. This makes it possible to
 * to define a submenu within a the menu itself.
 *
 * @param menu Menu to add items to
 * @param file File descriptor for menu file
 *
 * @return Pointer to new menu if menu was sucessfully parsed,
 *         otherwise NULL
 */
WaMenu *ResourceHandler::ParseMenu(WaMenu *menu, FILE *file,
                                   WaScreen *wascreen) {
    char *s = NULL, line[8192], *line1 = NULL, *line2 = NULL,
        *par = NULL, *tmp_par = NULL;
    WaMenuItem *m;
    int i, type, cb;
    WaMenu *tmp_menu;
    list<StrComp *>::iterator it;
    char *__m_wastrdup_tmp;

    while (fgets(line, 8192, file)) {
        linenr++;
        for (i = 0; line[i] == ' ' || line[i] == '\t'; i++);
        if (line[i] == '\n') continue;
        if (line[i] == '#') continue;
        if (line[i] == '!') continue;

        cb = 0;
        
        if (s) delete [] s; s = NULL;
        if (! (s = strwithin(line, '[', ']'))) {
            WARNING << "(" << basename(menu_file) << ":" << linenr << "):" <<
                " missing tag" << endl;
            continue;
        }
        if (! strcasecmp(s, "include")) {
            FILE *include_file;
            char *tmp_mf;
            int tmp_linenr;

            if (s) delete [] s; s = NULL;
            if ((s = strwithin(line, '(', ')', true))) {
                if (! (include_file = fopen(s, "r"))) { 
                    WARNING << "can't open menufile `" << s <<
                        "' for reading" << endl;
                    continue;
                }
                tmp_mf = menu_file;
                tmp_linenr = linenr;
                menu_file = s;
                while (! feof(include_file))
                    ParseMenu(menu, include_file, wascreen);
                menu_file = tmp_mf;
                linenr = tmp_linenr;
                fclose(include_file);                
            } else {
                WARNING << "(" << basename(menu_file) << ":" << linenr <<
                    "): missing menufile name" << endl;
            }            
            continue;
        }
        if ((strcasecmp(s, "start") && strcasecmp(s, "begin"))) {
            if (menu == NULL) {
                WARNING << "(" << basename(menu_file) << ":" << linenr << 
                    "): bad tag, expected [start], [begin] or [include]" <<
                    endl;
                continue;
            }
        }
        if (! strcasecmp(s, "start")) {
            if (s) delete [] s; s = NULL;
            if ((s = strwithin(line, '(', ')', true))) {
                tmp_menu = new WaMenu(s);
                if (menu) {
                    if (menu->dynamic) {
                        tmp_menu->dynamic = true;
                        if (ParseMenu(tmp_menu, file, wascreen))
                            tmp_menu->Build(wascreen);
                    } else
                        ParseMenu(tmp_menu, file, wascreen);
                }
                else menu = tmp_menu;
            } else
                WARNING << "(" << basename(menu_file) << ":" << linenr << 
                    "): missing menu name" << endl;
            continue;
        }
        else if ((! strcasecmp(s, "submenu")) || (! strcasecmp(s, "begin"))) {
            if (s) delete [] s; s = NULL;
            if ((s = strwithin(line, '(', ')', true))) {
                if (menu) {
                    m = new WaMenuItem(s);
                    m->type = MenuSubType;
                    m->func_mask |= MenuSubMask;
                    m->func_mask1 |= MenuSubMask;
                    m->sub = m->sub1 = __m_wastrdup(s);
                    menu->AddItem(m);
                }
                tmp_menu = new WaMenu(s);
                m = new WaMenuItem(s);
                m->type = MenuTitleType;
                tmp_menu->AddItem(m);
                if (menu) {
                    if (menu->dynamic) {
                        tmp_menu->dynamic = true;
                        if (ParseMenu(tmp_menu, file, wascreen))
                            tmp_menu->Build(wascreen);
                    } else
                        ParseMenu(tmp_menu, file, wascreen);
                }
                else menu = tmp_menu;
            } else
                WARNING << "(" << basename(menu_file) << ":" << linenr << 
                    "): missing menu name" << endl;
            continue;
        }
        else if (! strcasecmp(s, "restart")) {
            if (s) delete [] s; s = NULL;
            if ((s = strwithin(line, '(', ')', true)))
                m = new WaMenuItem(s);
            else                
                m = new WaMenuItem("");

            if (s) delete [] s; s = NULL;
            if ((s = strwithin(line, '{', '}')))
                m->param1 = m->param = __m_wastrdup(s);
            m->type = MenuItemType;            
            m->func_mask = MenuRFuncMask | MenuWFuncMask | MenuMFuncMask;
            m->rfunc = &WaScreen::Restart;
            m->wfunc = &WaWindow::Restart;
            m->mfunc = &WaMenuItem::Restart;
            
            menu->AddItem(m);
            continue;
        }
        else if (! strcasecmp(s, "exit")) {
            if (s) delete [] s; s = NULL;
            if ((s = strwithin(line, '(', ')', true)))
                m = new WaMenuItem(s);
            else
                m = new WaMenuItem("");
            m->type = MenuItemType;
            m->func_mask = MenuRFuncMask | MenuWFuncMask | MenuMFuncMask;
            m->rfunc = &WaScreen::Exit;
            m->wfunc = &WaWindow::Exit;
            m->mfunc = &WaMenuItem::Exit;
            
            menu->AddItem(m);
            continue;
        }
        else if (! strcasecmp(s, "exec")) {
            if (s) delete [] s; s = NULL;
            if ((s = strwithin(line, '(', ')', true)))
                m = new WaMenuItem(s);
            else
                m = new WaMenuItem("");
            m->type = MenuItemType;
            if (s) delete [] s; s = NULL;
            if ((s = strwithin(line, '{', '}'))) {
                if (*s != '\0') {
                    m->exec = m->exec1 = __m_wastrdup(s);
                    m->func_mask |= MenuExecMask;
                    m->func_mask1 |= MenuExecMask;
                }
            }
            menu->AddItem(m);
            continue;
        }
        else if (! strcasecmp(s, "nop")) {
            if (s) delete [] s; s = NULL;
            if ((s = strwithin(line, '(', ')', true)))
                m = new WaMenuItem(s);
            else
                m = new WaMenuItem("");
            m->type = MenuItemType;
            menu->AddItem(m);
            continue;
        }
        else if (! strcasecmp(s, "end")) {
            if (menu->item_list.empty()) {
                WARNING << "no elements in menu `" << menu->name <<
                    "'" << endl;
                delete menu;
                if (s) delete [] s; s = NULL;
                return NULL;
            }
            wascreen->wamenu_list.push_back(menu);
            if (s) delete [] s; s = NULL;
            return menu;
        }
        else if (! strncasecmp(s, "checkbox", 8)) {
            if (! strcasecmp(s + 9, "MAXIMIZED")) {
                type = MenuCBItemType;
                cb = MaxCBoxType;
            }
            else if (! strcasecmp(s + 9, "MINIMIZED")) {
                type = MenuCBItemType;
                cb = MinCBoxType;
            }
            else if (! strcasecmp(s + 9, "SHADED")) {
                type = MenuCBItemType;
                cb = ShadeCBoxType;
            }
            else if (! strcasecmp(s + 9, "STICKY")) {
                type = MenuCBItemType;
                cb = StickCBoxType;
            }
            else if (! strcasecmp(s + 9, "DECORTITLE")) {
                type = MenuCBItemType;
                cb = TitleCBoxType;
            }
            else if (! strcasecmp(s + 9, "DECORHANDLE")) {
                type = MenuCBItemType;
                cb = HandleCBoxType;
            }
            else if (! strcasecmp(s + 9, "DECORBORDER")) {
                type = MenuCBItemType;
                cb = BorderCBoxType;
            }
            else if (! strcasecmp(s + 9, "DECORALL")) {
                type = MenuCBItemType;
                cb = AllCBoxType;
            }
            else if (! strcasecmp(s + 9, "ALWAYSONTOP")) {
                type = MenuCBItemType;
                cb = AOTCBoxType;
            }
            else if (! strcasecmp(s + 9, "ALWAYSATBOTTOM")) {
                type = MenuCBItemType;
                cb = AABCBoxType;
            }
            else if (! strcasecmp(s + 9, "FULLSCREEN")) {
                type = MenuCBItemType;
                cb = FsCBoxType;
            }
            else {
                WARNING << "(" << basename(menu_file) << ":" << linenr << 
                    "): '"<< s + 9 << "' unknown checkbox" << endl;
                continue;
            }
            for (i = 0; strncasecmp(&line[i], "@TRUE", 5) &&
                     line[i + 5] != '\0'; i++);
            if (line[i + 5] == '\0') {
                WARNING << "(" << basename(menu_file) << ":" << linenr << 
                    "): No '@TRUE' linepart for checkbox item" << endl;
                continue;
            }
            line2 = &line[i + 5];
            for (i = 0; strncasecmp(&line[i], "@FALSE", 6) &&
                     line[i + 6] != '\0'; i++);
            if (line[i + 6] == '\0') {
                WARNING << "(" << basename(menu_file) << ":" << linenr << 
                    "): No '@FALSE' linepart for checkbox item" << endl;
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
            type = MenuTitleType;
        }
        else if (! strcasecmp(s, "item")) {
            type = MenuItemType;
        }
        else if (! strcasecmp(s, "sub")) {
            type = MenuSubType;
        }
        else {
            WARNING << "(" << basename(menu_file) << ":" << linenr << 
                "): bad tag [" << s << "]" << endl;
            continue;
        }
        if (! cb) line1 = line;
        if (s) delete [] s; s = NULL;
        if (! (s = strwithin(line1, '(', ')', true)))
            m = new WaMenuItem("");
        else
            m = new WaMenuItem(s);
        m->label1 = m->label;
        m->type = type;
        m->cb = cb;
        if (s) delete [] s; s = NULL;
        if ((s = strwithin(line1, '{', '}'))) {
            if (*s != '\0') {
                m->exec = m->exec1 = __m_wastrdup(s);
                m->func_mask |= MenuExecMask;
                m->func_mask1 |= MenuExecMask;
            }
        }
        if (s) delete [] s; s = NULL;
        if ((s = strwithin(line1, '<', '>'))) {
            m->sub = m->sub1 = __m_wastrdup(s);
            m->func_mask |= MenuSubMask;
            m->func_mask1 |= MenuSubMask;
        }
        if (s) delete [] s; s = NULL;
        if ((s = strwithin(line1, '"', '"'))) {
            tmp_par = par = __m_wastrdup(s);
            for (i = 0; *par != '(' && *par != '\0'; par++, i++);
            if (*(par++) == '(') {
                s[i] = '\0';
                for (i = 0; par[i] != ')' && par[i] != '\0'; i++);
                if (par[i] == '\0') {
                    WARNING << "(" << basename(menu_file) << ":" << linenr << 
                        "): missing ')'" << endl;
                    delete [] tmp_par;
                    continue;
                }
                if (strlen(par)) {
                    par[i] = '\0';
                    m->param1 = m->param = param_eval(s, par, wascreen);
                    delete [] tmp_par;
                }
            }
            else
                delete [] tmp_par;

            it = wacts.begin();
            for (; it != wacts.end(); ++it) {
                if ((*it)->Comp(s)) {
                    m->wfunc = (*it)->winfunc;
                    m->wfunc1 = (*it)->winfunc;
                    m->func_mask |= MenuWFuncMask;
                    m->func_mask1 |= MenuWFuncMask;
                    break;
                }
            }
            it = racts.begin();
            for (; it != racts.end(); ++it) {
                if ((*it)->Comp(s)) {
                    m->rfunc = (*it)->rootfunc;
                    m->rfunc1 = (*it)->rootfunc;
                    m->func_mask |= MenuRFuncMask;
                    m->func_mask1 |= MenuRFuncMask;
                    break;
                }
            }
            it = macts.begin();
            for (; it != macts.end(); ++it) {
                if ((*it)->Comp(s)) {
                    m->mfunc = (*it)->menufunc;
                    m->mfunc1 = (*it)->menufunc;
                    m->func_mask |= MenuMFuncMask;
                    m->func_mask1 |= MenuMFuncMask;
                    break;
                }
            }
            if (! (m->wfunc || m->rfunc || m->mfunc)) {
                WARNING << "(" << basename(menu_file) << ":" << linenr << 
                    "): function `" << s << "' not available" << endl;
                continue;
            }
        }
        
        if (cb) {
            if (s) delete [] s; s = NULL;
            if (! (s = strwithin(line2, '(', ')', true)))
                m->label2 = __m_wastrdup("");
            else
                m->label2 = __m_wastrdup(s);
            if (s) delete [] s; s = NULL;
            if ((s = strwithin(line2, '{', '}'))) {
                if (*s != '\0') {
                    m->exec2 = __m_wastrdup(s);
                    m->func_mask2 |= MenuExecMask;
                }
            }
            if (s) delete [] s; s = NULL;
            if ((s = strwithin(line2, '<', '>'))) {
                m->sub2 = __m_wastrdup(s);
                m->func_mask2 |= MenuSubMask;
            }
            if (s) delete [] s; s = NULL;
            if ((s = strwithin(line2, '"', '"'))) {
                tmp_par = par = __m_wastrdup(s);
                for (i = 0; *par != '(' && *par != '\0'; par++, i++);
                if (*(par++) == '(') {
                    s[i] = '\0';
                    for (i = 0; par[i] != ')' && par[i] != '\0'; i++);
                    if (par[i] == '\0') {
                        WARNING << "(" << basename(menu_file) << ":" << 
                            linenr << "): missing ')'" << endl;
                        delete [] tmp_par;
                        continue;
                    }
                    if (strlen(par)) {
                        par[i] = '\0';
                        m->param2 = param_eval(s, par, wascreen);
                        delete [] tmp_par;
                    }
                }
                else
                    delete [] tmp_par;
                
                it = wacts.begin();
                for (; it != wacts.end(); ++it) {
                    if ((*it)->Comp(s)) {
                        m->wfunc2 = (*it)->winfunc;
                        m->func_mask2 |= MenuWFuncMask;
                        break;
                    }
                }
                it = racts.begin();
                for (; it != racts.end(); ++it) {
                    if ((*it)->Comp(s)) {
                        m->rfunc2 = (*it)->rootfunc;
                        m->func_mask2 |= MenuRFuncMask;
                        break;
                    }
                }
                it = macts.begin();
                for (; it != macts.end(); ++it) {
                    if ((*it)->Comp(s)) {
                        m->mfunc2 = (*it)->menufunc;
                        m->func_mask2 |= MenuMFuncMask;
                        break;
                    }
                }
                if (! (m->wfunc2 || m->rfunc2 || m->mfunc2)) {
                    WARNING << "(" << basename(menu_file) << ":" << linenr <<
                        "): function `" << s << "' not available" << endl;
                    continue;
                }
            }
        }
        menu->AddItem(m);
    }
    if (menu) {
        if (menu->item_list.empty()) {
            WARNING << "no elements in menu `" << menu->name <<
                "'" << endl;
            delete menu;
            if (s) delete [] s; s = NULL;
            return NULL;
        }
        wascreen->wamenu_list.push_back(menu);
        if (s) delete [] s; s = NULL;
        return menu;
    }
    if (s) delete [] s; s = NULL;
    return NULL;
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
 * @fn    strwithin(char *s, char c1, char c2, bool eval_env)
 * @brief Return string between to characters
 *
 * Duplicates and returns the string between c1 and c2 if c1 and c2 was
 * found. Possible environment variables are evaluated if eval_env
 * parameter is true. All special character sequences are replaced
 * with real characters.
 *
 * @param s String to search for c1 and c2 in
 * @param c1 Starting character
 * @param c2 Ending character
 * @param eval_env True if environment variables should be evaluated
 *
 * @return String within c1 and c2
 */
char *strwithin(char *s, char c1, char c2, bool eval_env) {
    int i, n;
    char *str;
    char *__m_wastrdup_tmp;
    
    for (i = 0;; i++) {
        if (s[i] == '\0') break;
        if (s[i] == c1 && (i == 0 || s[i - 1] != '\\')) break;
    }
    if (s[i] == '\0') return NULL;
    
    for (n = i + 1;; n++) {
        if (s[n] == '\0') break;
        if (s[n] == c2 && s[n - 1] != '\\') break;
    }
    if (s[n] == '\0') return NULL; 
    
    s[n] = '\0';
    str = __m_wastrdup(s + i + 1);
    s[n] = c2;
    
    if (eval_env) {
        str = environment_expansion(str);
    }
    for (i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\\' && 
            (str[i + 1] == '$' ||
             str[i + 1] == '\\' ||
             str[i + 1] == '"' ||
             str[i + 1] == '(' ||
             str[i + 1] == ')' ||
             str[i + 1] == '[' ||
             str[i + 1] == ']' ||
             str[i + 1] == '{' ||
             str[i + 1] == '}' ||
             str[i + 1] == '<' ||
             str[i + 1] == '>')) {
            for (n = 1; str[i + n] != '\0'; n++)
                str[i + n - 1] = str[i + n];
            str[i + n - 1] = '\0';
        }
    }
    return str;
}

/**
 * @fn    environment_expansion(char *s)
 * @brief Expand '~' and environment variables
 *
 * Replaces all '~' characters with HOME environment variable and all
 * $ENVIRONMENT_VARIABLE with environment variable value.
 *
 * @param s	String to expand
 * @return Pointer to expanded string
 */
char *environment_expansion(char *s) {
    char *tmp, *env, *env_name;
    int i, tmp_char;
    
    for (i = 0; s[i] != '\0'; i++) {
        switch (s[i]) {
            case '\\':
                if (s[i + 1] != '\0') i++;
                break;
            case '$':
                if (IS_ENV_CHAR(s[i + 1])) {
                    s[i] = '\0';
                    env_name = &s[++i];
                    for (; IS_ENV_CHAR(s[i]); i++);
                    tmp_char = s[i];
                    s[i] = '\0';
                    if ((env = getenv(env_name)) == NULL) env = "";
                    s[i] = tmp_char;
                    tmp = new char[strlen(s) + strlen(env) +
                                   strlen(&s[i]) + 1];
                    sprintf(tmp, "%s%s%s", s, env, &s[i]);
                    i = strlen(s) + strlen(env);
                    delete [] s;
                    s = tmp;
                }
                break;
            case '~': 
                s[i] = '\0';
                if ((env = getenv("HOME")) == NULL) env = "~";
                tmp = new char[strlen(s) + strlen(env) +
                               strlen(&s[i + 1]) + 1];
                sprintf(tmp, "%s%s%s", s, env, &s[i + 1]);
                i = strlen(s) + strlen(env);
                delete [] s;
                s = tmp;
                break;
        }
    }
    return s;
}

/**
 * @fn    param_eval(char *action, char *param, WaScreen *wascreen)
 * @brief Evaluate parameter string
 *
 * Replaces special parameter characters and returns new parameter string.
 *
 * @param action Action name for parameter
 * @param param Parameter string
 * @param wascreen WaScreen object pointer
 *
 * @return New parameter
 */
char *param_eval(char *action, char *param, WaScreen *wascreen) {
    char *tmp, *p;
    int i;
    char *__m_wastrdup_tmp;

    if (! param) return param;
    
    p = __m_wastrdup(param);
    if ((! strncasecmp(action, "viewport", 8)) ||
        (! strncasecmp(action, "moveresize", 10))) {
        for (i = 0; p[i] != '\0'; i++) {
            if (p[i] == 'W' || p[i] == 'w') {
                tmp = new char[strlen(p) + 5];
                p[i] = '\0';
                sprintf(tmp, "%s%d%s", p, wascreen->width, &p[i + 1]);
                delete [] p;
                p = tmp;
            }
            else if (p[i] == 'H' || p[i] == 'h') {
                tmp = new char[strlen(p) + 5];
                p[i] = '\0';
                sprintf(tmp, "%s%d%s", p, wascreen->height, &p[i + 1]);
                delete [] p;
                p = tmp;
            }
        }
    }
    return p;
}
