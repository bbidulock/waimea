/**
 * @file   Screen.cc
 * @author David Reveman <c99drn@cs.umu.se>
 * @date   25-Jul-2001 23:26:22
 *
 * @brief Implementation of WaScreen and ScreenEdge classes
 *
 * A WaScreen object handles one X server screen. A ScreenEdge is a
 * transperant window placed at the edge of the screen, good to use for
 * virtual screen scrolling.
 *
 * Copyright (C) David Reveman. All rights reserved.
 *
 */

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

extern "C" {
#include <X11/cursorfont.h>

#ifdef    HAVE_STDIO_H
#  include <stdio.h>
#endif // HAVE_STDIO_H

#ifdef    HAVE_UNISTD_H
#  include <sys/types.h>
#  include <sys/wait.h>
#  include <unistd.h>
#endif // HAVE_UNISTD_H

#ifdef    STDC_HEADERS
#  include <stdlib.h>
#endif // STDC_HEADERS

#ifdef    HAVE_SIGNAL_H
#  include <signal.h>
#endif // HAVE_SIGNAL_H
}

#include <iostream>
using std::cerr;
using std::cout;
using std::endl;

#include "Screen.hh"

/**
 * @fn    WaScreen(Display *d, int scrn_number, Waimea *wa)
 * @brief Constructor for WaScreen class
 *
 * Sets root window input mask. Creates new image control object and reads
 * style file. Then we create fonts, colors and renders common images.
 * Last thing we do in this functions is to create WaWindows for all windows
 * that should be managed.
 *
 * @param d The display
 * @param scrn_number Screen to manage
 * @param wa Waimea object
 */
WaScreen::WaScreen(Display *d, int scrn_number, Waimea *wa) :
    WindowObject(0, RootType) {
    Window ro, pa, *children;
    int eventmask, i;
    unsigned int nchild;
    XSetWindowAttributes attrib_set;
    char *__m_wastrdup_tmp;
    
    display = d;
    screen_number = scrn_number;
    id = RootWindow(display, screen_number);
    visual = DefaultVisual(display, screen_number);
    colormap = DefaultColormap(display, screen_number);
    screen_depth = DefaultDepth(display, screen_number);
    width = DisplayWidth(display, screen_number);
    height = DisplayHeight(display, screen_number);
    
    waimea = wa;
    net = waimea->net;
    rh = wa->rh;
    focus = true;
    shutdown = false;

    default_font.xft = false;
    default_font.font = __m_wastrdup("fixed");

    XSync(display, false);
    if (! (pdisplay = XOpenDisplay(wa->options->display))) {
        ERROR << "can't open display: " << wa->options->display << endl;
        exit(1);
    }    
    
#ifdef PIXMAP
    imlib_context = imlib_context_new();
    imlib_context_push(imlib_context);
    imlib_context_set_display(pdisplay);
    imlib_context_set_drawable(RootWindow(pdisplay, screen_number));
    imlib_context_set_colormap(DefaultColormap(pdisplay, screen_number));
    imlib_context_set_visual(DefaultVisual(pdisplay, screen_number));
    imlib_context_set_anti_alias(1);
    imlib_context_pop();
#endif // PIXMAP
    
    eventmask = SubstructureRedirectMask | StructureNotifyMask |
        PropertyChangeMask | ColormapChangeMask | KeyPressMask |
        KeyReleaseMask | ButtonPressMask | ButtonReleaseMask |
        EnterWindowMask | LeaveWindowMask | FocusChangeMask;

    sprintf(displaystring, "DISPLAY=%s", DisplayString(display));
    sprintf(displaystring + strlen(displaystring) - 1, "%d", screen_number);

    XSetErrorHandler((XErrorHandler) wmrunningerror);
    XSelectInput(display, id, eventmask);
    XSync(display, false);
    XSync(pdisplay, false);
    XSetErrorHandler((XErrorHandler) xerrorhandler);
    if (waimea->wmerr) {
        cerr << "waimea: warning: another window manager is running on " <<
            displaystring + 8 << endl;
        return;
    }
	
    v_x = v_y = 0;

#ifdef RENDER
    int event_basep, error_basep;
    render_extension =
      XRenderQueryExtension(pdisplay, &event_basep, &error_basep);
#endif // RENDER

    rh->LoadConfig(this);
    
    current_desktop = new Desktop(0, width, height);
    desktop_list.push_back(current_desktop);
    net->SetWorkarea(this);
    
    for (unsigned int i = 1; config.desktops > i; i++)
        desktop_list.push_back(new Desktop(i, width, height));

    waimea->window_table.insert(make_pair(id, this));
    
    attrib_set.override_redirect = true;
    wm_check = XCreateWindow(display, id, 0, 0, 1, 1, 0,
                             CopyFromParent, InputOnly, CopyFromParent,
                             CWOverrideRedirect, &attrib_set);
    net->SetSupportedWMCheck(this, wm_check);
    net->SetSupported(this);
        
    rh->LoadMenus(this);

    ic = new WaImageControl(pdisplay, this, config.image_dither,
                            config.colors_per_channel, config.cache_max);
    ic->installRootColormap();

    rh->LoadStyle(this);
    rh->LoadActions(this);

    CreateFonts();
    CreateColors();
    RenderCommonImages();
    XDefineCursor(display, id, waimea->session_cursor);
    
    v_xmax = (config.virtual_x - 1) * width;
    v_ymax = (config.virtual_y - 1) * height;
    west = new ScreenEdge(this, 0, 0, 2, height, WEdgeType);
    west->SetActionlist(&config.weacts);
    east = new ScreenEdge(this, width - 2, 0, 2, height, EEdgeType);
    east->SetActionlist(&config.eeacts);
    north = new ScreenEdge(this, 0, 0, width, 2, NEdgeType);
    north->SetActionlist(&config.neacts);
    south = new ScreenEdge(this, 0, height - 2, width, 2, SEdgeType);
    south->SetActionlist(&config.seacts);
    
    net->SetDesktopGeometry(this);
    net->SetNumberOfDesktops(this);
    net->GetCurrentDesktop(this);
    net->SetCurrentDesktop(this);
    net->GetDesktopViewPort(this);
    net->SetDesktopViewPort(this);

#ifdef RENDER
    if (render_extension) {
      net->GetXRootPMapId(this);
      ic->setXRootPMapId((xrootpmap_id)? true: false);
    }
#endif // RENDER

    list<DockStyle *>::iterator dit = wstyle.dockstyles.begin();
    for (; dit != wstyle.dockstyles.end(); ++dit) {
        docks.push_back(new DockappHandler(this, *dit));
    }
    
    window_menu = new WindowMenu();
    wamenu_list.push_back(window_menu);
    
    list<WaMenu *>::iterator mit = wamenu_list.begin();
    for (; mit != wamenu_list.end(); ++mit)
    	(*mit)->Build(this);
    
    XWindowAttributes attr;
    XQueryTree(display, id, &ro, &pa, &children, &nchild);
    for (i = 0; i < (int) nchild; ++i) {
        bool status = false;
        XGrabServer(display);
        if (validateclient(id)) {
            XGetWindowAttributes(display, children[i], &attr);
            status = true;
        }
        XUngrabServer(display);
        if (status && (! attr.override_redirect) &&
            (attr.map_state == IsViewable)) {
            XWMHints *wm_hints = NULL;
            XGrabServer(display);
            if (validateclient(id)) {
                wm_hints = XGetWMHints(display, children[i]);
            }
            XUngrabServer(display);
            if ((wm_hints) && (wm_hints->flags & StateHint) &&
                (wm_hints->initial_state == WithdrawnState)) {
                AddDockapp(children[i]);
            }
            else if ((waimea->window_table.find(children[i]))
                     == waimea->window_table.end()) {
                WaWindow *newwin = new WaWindow(children[i], this);
                if (waimea->FindWin(children[i], WindowType))
                    newwin->net->SetState(newwin, NormalState);
            }
            if (wm_hints) XFree(wm_hints);
        }
    }
    XFree(children);
    net->GetClientListStacking(this);
    net->SetClientList(this);
    net->SetClientListStacking(this);
    net->GetActiveWindow(this);

    actionlist = &config.rootacts;

    delete [] config.style_file;
    delete [] config.action_file;
}

/**
 * @fn    ~WaScreen(void)
 * @brief Destructor for WaScreen class
 *
 * Deletes all created colors and fonts.
 */
WaScreen::~WaScreen(void) {
    shutdown = true;
    XSelectInput(display, id, NoEventMask);
    net->DeleteSupported(this);
    XDestroyWindow(display, wm_check);

    LISTDEL(docks);

    WaWindow **delstack = new WaWindow*[wawindow_list.size()];
    int stackp = 0;

    list<WaWindow *>::reverse_iterator rwit = wawindow_list.rbegin();
    for (; rwit != wawindow_list.rend(); rwit++)
        if ((*rwit)->flags.forcedatbottom)
            delstack[stackp++] = ((WaWindow *) *rwit);
    list<WaWindow *>::iterator wit = wawindow_list_stacking_aab.begin();
    for (; wit != wawindow_list_stacking_aab.end(); wit++)
        delstack[stackp++] = *wit;
    list<WindowObject *>::reverse_iterator rwoit = wa_list_stacking.rbegin();
    for (; rwoit != wa_list_stacking.rend(); rwoit++)
        if ((*rwoit)->type == WindowType)
            delstack[stackp++] = ((WaWindow *) *rwoit);
    rwit = wawindow_list_stacking_aot.rbegin();
    for (; rwit != wawindow_list_stacking_aot.rend(); rwit++)
        delstack[stackp++] = ((WaWindow *) *rwit);

    for (int i = 0; i < stackp; i++)
        delete delstack[i];

    delete [] delstack;
    
    LISTCLEAR(wawindow_list);
    LISTCLEAR(wa_list_stacking);
    LISTCLEAR(wawindow_list_stacking_aab);
    LISTCLEAR(wawindow_list_stacking_aot);
    LISTCLEAR(wawindow_list_map_order);
    LISTCLEAR(always_on_top_list);
    LISTCLEAR(always_at_bottom_list);

    LISTDEL(strut_list);
    
    list<ButtonStyle *>::iterator bit = wstyle.buttonstyles.begin();
    for (; bit != wstyle.buttonstyles.end(); ++bit) {
        if ((*bit)->fg) {            
            XFreeGC(display, (*bit)->g_focused);
            XFreeGC(display, (*bit)->g_unfocused);
            XFreeGC(display, (*bit)->g_pressed);
        }
    }

#ifdef PIXMAP
    imlib_context_free(imlib_context);
#endif // PIXMAP
    
    while (! wstyle.dockstyles.empty()) {
        while (! wstyle.dockstyles.back()->order.empty()) {
            delete wstyle.dockstyles.back()->order.back();
            wstyle.dockstyles.back()->order.pop_back();
            wstyle.dockstyles.back()->order_type.pop_back();
        }
        delete wstyle.dockstyles.back();
        wstyle.dockstyles.pop_back();
    }
    
    ACTLISTCLEAR(config.frameacts);
    ACTLISTCLEAR(config.awinacts);
    ACTLISTCLEAR(config.pwinacts);
    ACTLISTCLEAR(config.titleacts);
    ACTLISTCLEAR(config.labelacts);
    ACTLISTCLEAR(config.handleacts);
    ACTLISTCLEAR(config.rgacts);
    ACTLISTCLEAR(config.lgacts);
    ACTLISTCLEAR(config.rootacts);
    ACTLISTCLEAR(config.weacts);
    ACTLISTCLEAR(config.eeacts);
    ACTLISTCLEAR(config.neacts);
    ACTLISTCLEAR(config.seacts);
    ACTLISTCLEAR(config.mtacts);
    ACTLISTCLEAR(config.miacts);
    ACTLISTCLEAR(config.msacts);
    ACTLISTCLEAR(config.mcbacts);
    int i;
    for (i = 0; i < wstyle.b_num; i++) {
        ACTLISTPTRCLEAR(config.bacts[i]);
        delete config.bacts[i];
    }
    delete [] config.bacts;

    LISTDEL(config.ext_frameacts);
    LISTDEL(config.ext_awinacts);
    LISTDEL(config.ext_pwinacts);
    LISTDEL(config.ext_titleacts);
    LISTDEL(config.ext_labelacts);
    LISTDEL(config.ext_handleacts);
    LISTDEL(config.ext_rgacts);
    LISTDEL(config.ext_lgacts);
    for (i = 0; i < wstyle.b_num; i++) {
        LISTPTRDEL(config.ext_bacts[i]);
        delete config.ext_bacts[i];
    }
    delete [] config.ext_bacts;

    delete west;
    delete east;
    delete north;
    delete south;
    delete ic;

    delete [] config.menu_file;
    delete [] mstyle.bullet;
    delete [] mstyle.checkbox_true;
    delete [] mstyle.checkbox_false;

    LISTDEL(wstyle.buttonstyles);    
    
    XSync(display, false);
    XSync(pdisplay, false);
    XCloseDisplay(pdisplay);
    waimea->window_table.erase(id);
}

/**
 * @fn    WaRaiseWindow(Window win)
 * @brief Raise window
 *
 * Raises a window in the display stack keeping alwaysontop windows, always
 * on top. To update the stacking order so that alwaysontop windows are on top
 * of all other windows we call this function with zero as win argument. 
 *
 * @param win Window to Raise, or 0 for alwaysontop windows update
 */
void WaScreen::WaRaiseWindow(Window win) {
    int i;
    bool in_list = false;
    
    if (always_on_top_list.size() || wawindow_list_stacking_aot.size() ||
        wamenu_list_stacking_aot.size()) {
        Window *stack = new Window[always_on_top_list.size() +
                                  wawindow_list_stacking_aot.size() +
                                  wamenu_list_stacking_aot.size() +
                                  ((win)? 1: 0)];
        
        list<Window>::iterator it = always_on_top_list.begin();
        for (i = 0; it != always_on_top_list.end(); ++it) {
            if (*it == win) in_list = true;
            stack[i++] = *it;
        }
        list<WaMenu *>::iterator mit = wamenu_list_stacking_aot.begin();
        for (; mit != wamenu_list_stacking_aot.end(); ++mit) {
            if ((*mit)->frame == win) in_list = true;
            stack[i++] = (*mit)->frame;
        }
        list<WaWindow *>::iterator wit = wawindow_list_stacking_aot.begin();
        for (; wit != wawindow_list_stacking_aot.end(); ++wit) {
            if ((*wit)->frame->id == win) in_list = true;
            stack[i++] = (*wit)->frame->id;
        }
        if (win && ! in_list) stack[i++] = win;
        
        XRaiseWindow(display, stack[0]);
        XRestackWindows(display, stack, i);
        
        delete [] stack;
    } else
        if (win) {
            XGrabServer(display);
            if (validateclient(win))
                XRaiseWindow(display, win);
            XUngrabServer(display);
        }
}

/**
 * @fn    WaLowerWindow(Window win)
 * @brief Lower window
 *
 * Lowers a window in the display stack keeping alwaysatbottom windows, always
 * at bottom.
 *
 * @param win Window to Lower, or 0 for alwaysatbottom windows update
 */
void WaScreen::WaLowerWindow(Window win) {
    int i;
    bool end = false;
    list<WaMenu *>::iterator mit;
    list<WaWindow *>::iterator wit;
    list<WindowObject *>::iterator woit;
    
    Window *stack = new Window[always_on_top_list.size() +
                              wawindow_list_stacking_aot.size() +
                              wamenu_list_stacking_aot.size() +
                              wa_list_stacking.size() +
                              wawindow_list_stacking_aab.size() +
                              wamenu_list_stacking_aab.size() +
                              always_at_bottom_list.size()];
        
    list<Window>::iterator it = always_on_top_list.begin();
    for (i = 0; it != always_on_top_list.end(); ++it) {
        if (*it == win) { end = true; break; } 
        stack[i++] = *it;
    }
    if (! end) {
        mit = wamenu_list_stacking_aot.begin();
        for (; mit != wamenu_list_stacking_aot.end(); ++mit) {
            if ((*mit)->frame == win) { end = true; break; }
            stack[i++] = (*mit)->frame;
        }
    }
    if (! end) {
        wit = wawindow_list_stacking_aot.begin();
        for (; wit != wawindow_list_stacking_aot.end(); ++wit) {
            if ((*wit)->frame->id == win) { end = true; break; }
            stack[i++] = (*wit)->frame->id;
        }
    }
    if (! end) {
        woit = wa_list_stacking.begin();
        for (; woit != wa_list_stacking.end(); ++woit) {
            if ((*woit)->type == WindowType) {
                if (((WaWindow *) (*woit))->frame->id == win) {
                    end = true; break;
                }
                stack[i++] = ((WaWindow *) (*woit))->frame->id;
            }
            else if ((*woit)->type == MenuType) {
                if (((WaMenu *) (*woit))->frame == win) {
                    end = true; break;
                }
                stack[i++] = ((WaMenu *) (*woit))->frame;
            }
        }
    }
    if (! end) {
        wit = wawindow_list_stacking_aab.begin();
        for (; wit != wawindow_list_stacking_aab.end(); ++wit) {
            if ((*wit)->frame->id == win) { end = true; break; }
            stack[i++] = (*wit)->frame->id;
        }
    }
    if (! end) {
        mit = wamenu_list_stacking_aab.begin();
        for (; mit != wamenu_list_stacking_aab.end(); ++mit) {
            if ((*mit)->frame == win) { end = true; break; }
            stack[i++] = (*mit)->frame;
        }
    }
    if (! end) {
        it = always_at_bottom_list.begin();
        for (; it != always_at_bottom_list.end(); ++it) {
            if (*it == win) { end = true; break; }
            stack[i++] = *it;
        }
    }
    XRaiseWindow(display, stack[0]);
    XRestackWindows(display, stack, i);
        
    delete [] stack;
}

/**
 * @fn    UpdateCheckboxes(int type)
 * @brief Updates menu checkboxes
 *
 * Redraws all checkbox menu items to make sure they are synchronized with
 * their given flag.
 *
 * @param type Type of checkboxes to update
 */
void WaScreen::UpdateCheckboxes(int type) {
    list<WaMenuItem *>::iterator miit;

    if (! waimea->eh) return;
    
    list<WaMenu *>::iterator mit = wamenu_list.begin();
    for (; mit != wamenu_list.end(); ++mit) {
        (*mit)->cb_db_upd = false;
        miit = (*mit)->item_list.begin();
        for (; miit != (*mit)->item_list.end(); ++miit) {
            if ((*miit)->cb == type && (*miit)->menu->mapped)
                (*miit)->Render();
        }
        if ((*mit)->cb_db_upd && config.db)
            (*mit)->Render();
    }
}

/**
 * @fn    GetMenuNamed(char *menu)
 * @brief Find a menu
 *
 * Searches through menu list after a menu named as 'menu' parameter.
 *
 * @param menu menu name to use for search
 *
 * @return Pointer to menu object if a menu was found, if no menu was found
 *         NULL is returned
 */
WaMenu *WaScreen::GetMenuNamed(char *menu) {
    WaMenu *dmenu;
    int i;

    if (! menu) return NULL;
   
    list<WaMenu *>::iterator menu_it = wamenu_list.begin();
    for (; menu_it != wamenu_list.end(); ++menu_it)
        if (! strcmp((*menu_it)->name, menu))
            return *menu_it;
            
    for (i = 0; menu[i] != '\0' && menu[i] != '!'; i++);
    if (menu[i] == '!' && menu[i + 1] != '\0') {
        dmenu = CreateDynamicMenu(menu);
        return dmenu;
    }
    
    WARNING << "`" << menu << "' unknown menu" << endl;
    return NULL;
} 

/**
 * @fn    CreateDynamicMenu(char *name)
 * @brief Creates a dynamic menu
 *
 * Executes command line and parses standard out as a menu file.
 *
 * @param name Name of dynamic menu to create
 *
 * @return Created menu, NULL if error occured inmenu parsing
 */
WaMenu *WaScreen::CreateDynamicMenu(char *name) {
    char *tmp_argv[128];
    int m_pipe[2];
    WaMenu *dmenu;
    int pid, status, i;
    char *allocname = NULL;
    char *__m_wastrdup_tmp;

    for (i = 0; name[i] != '\0' && name[i] != '!'; i++);
    if (name[i] == '!' && name[i + 1] != '\0') {
        allocname = __m_wastrdup(&name[i + 1]);
        commandline_to_argv(allocname, tmp_argv);
    } else 
        return NULL;

    if (pipe(m_pipe) < 0) {
        WARNING;
        perror("pipe");
    }
    else {
        struct sigaction action;
        
        action.sa_handler = SIG_DFL;
        action.sa_mask = sigset_t();
        action.sa_flags = 0;
        sigaction(SIGCHLD, &action, NULL);
        pid = fork();
        if (pid == 0) {
            dup2(m_pipe[1], STDOUT_FILENO);
            close(m_pipe[0]);
            close(m_pipe[1]);
            putenv(waimea->pathenv);
            if (execvp(*tmp_argv, tmp_argv) < 0)
                WARNING << *tmp_argv << ": command not found" << endl;
            close(STDOUT_FILENO);
            exit(127);
        }
        close(m_pipe[1]);
        rh->linenr = 0;
        delete [] config.menu_file;
        config.menu_file = new char[strlen(*tmp_argv) + 8];
        sprintf(config.menu_file, "%s:STDOUT", *tmp_argv);
        dmenu = new WaMenu(name);
        dmenu->dynamic = dmenu->dynamic_root = true;
        FILE *fd = fdopen(m_pipe[0], "r");
        dmenu = rh->ParseMenu(dmenu, fd, this);
        fclose(fd);
        if (waitpid(pid, &status, 0) == -1) {
            WARNING; 
            perror("waitpid");
        }
        action.sa_handler = signalhandler;
        action.sa_flags = SA_NOCLDSTOP | SA_NODEFER;
        sigaction(SIGCHLD, &action, NULL);
        if (dmenu != NULL) {
            dmenu->Build(this);
            if (allocname) delete [] allocname;
            
            return dmenu;
        }
    }
    if (allocname) delete [] allocname;
    return NULL;
}

/**
 * @fn    CreateFonts(void)
 * @brief Open fonts
 *
 * Opens all fonts and sets frame height.
 */
void WaScreen::CreateFonts(void) {
    bool set_mih;
    
    if (! mstyle.item_height) set_mih = true;
    else set_mih = false;

    if (default_font.Open(display, screen_number, NULL) == -1) {
        ERROR << "failed loading default font" << endl;
        exit(1);
    }
    
    int height;
    height = wstyle.wa_font.Open(display, screen_number, &default_font);
    if (! wstyle.title_height) wstyle.title_height = height + 4;

    int tmp_sx = wstyle.wa_font_u.shodow_off_x;
    int tmp_sy = wstyle.wa_font_u.shodow_off_y;
    memcpy(&wstyle.wa_font_u, &wstyle.wa_font, sizeof(wstyle.wa_font));
    wstyle.wa_font_u.shodow_off_x = tmp_sx;
    wstyle.wa_font_u.shodow_off_y = tmp_sy;
    
    height = mstyle.wa_f_font.Open(display, screen_number, &default_font);
    if (set_mih) mstyle.item_height = height + 2;

    tmp_sx = mstyle.wa_fh_font.shodow_off_x;
    tmp_sy = mstyle.wa_fh_font.shodow_off_y;
    memcpy(&mstyle.wa_fh_font, &mstyle.wa_f_font, sizeof(mstyle.wa_f_font));
    mstyle.wa_fh_font.shodow_off_x = tmp_sx;
    mstyle.wa_fh_font.shodow_off_y = tmp_sy;

    height = mstyle.wa_b_font.Open(display, screen_number, &default_font);
    if (set_mih && mstyle.item_height < (unsigned int) (height + 2))
        mstyle.item_height = height + 2;

    memcpy(&mstyle.wa_bh_font, &mstyle.wa_b_font, sizeof(mstyle.wa_b_font));
    mstyle.wa_bh_font.shodow_off_x = tmp_sx;
    mstyle.wa_bh_font.shodow_off_y = tmp_sy;

    height = mstyle.wa_ct_font.Open(display, screen_number, &default_font);
    if (set_mih && mstyle.item_height < (unsigned int) (height + 2))
        mstyle.item_height = height + 2;

    memcpy(&mstyle.wa_cth_font, &mstyle.wa_ct_font, sizeof(mstyle.wa_ct_font));
    mstyle.wa_cth_font.shodow_off_x = tmp_sx;
    mstyle.wa_cth_font.shodow_off_y = tmp_sy;

    height = mstyle.wa_cf_font.Open(display, screen_number, &default_font);
    if (set_mih && mstyle.item_height < (unsigned int) (height + 2))
        mstyle.item_height = height + 2;

    memcpy(&mstyle.wa_cfh_font, &mstyle.wa_cf_font, sizeof(mstyle.wa_cf_font));
    mstyle.wa_cfh_font.shodow_off_x = tmp_sx;
    mstyle.wa_cfh_font.shodow_off_y = tmp_sy;

    height = mstyle.wa_t_font.Open(display, screen_number, &default_font);
    if (! mstyle.title_height) mstyle.title_height = height + 2;
    
    if (wstyle.title_height < 10) mstyle.title_height = 10;
    if (mstyle.title_height < 4) mstyle.title_height = 4;
    if (mstyle.item_height < 4) mstyle.item_height = 4;
    
    wstyle.y_pos = (wstyle.title_height / 2 - 2) +
        wstyle.wa_font.diff / 2 + wstyle.wa_font.diff % 2;
    mstyle.f_y_pos = (mstyle.item_height / 2) +
        mstyle.wa_f_font.diff / 2 + mstyle.wa_f_font.diff % 2;
    mstyle.t_y_pos = (mstyle.title_height / 2) +
        mstyle.wa_t_font.diff / 2 + mstyle.wa_t_font.diff % 2;
    mstyle.b_y_pos = (mstyle.item_height / 2) +
        mstyle.wa_b_font.diff / 2 + mstyle.wa_b_font.diff % 2;
    mstyle.ct_y_pos = (mstyle.item_height / 2) +
        mstyle.wa_ct_font.diff / 2 + mstyle.wa_ct_font.diff % 2;
    mstyle.cf_y_pos = (mstyle.item_height / 2) +
        mstyle.wa_cf_font.diff / 2 + mstyle.wa_cf_font.diff % 2;
}

/**
 * @fn    CreateColors(void)
 * @brief Creates all colors
 *
 * Creates all color GCs.
 */
void WaScreen::CreateColors(void) {
    XGCValues gcv;
    
    list<ButtonStyle *>::iterator bit = wstyle.buttonstyles.begin();
    for (; bit != wstyle.buttonstyles.end(); ++bit) {
        if ((*bit)->fg) {
            gcv.foreground = (*bit)->c_focused.getPixel();
            (*bit)->g_focused = XCreateGC(display, id, GCForeground, &gcv);
            gcv.foreground = (*bit)->c_unfocused.getPixel();
            (*bit)->g_unfocused = XCreateGC(display, id, GCForeground, &gcv);
            gcv.foreground = (*bit)->c_pressed.getPixel();
            (*bit)->g_pressed = XCreateGC(display, id, GCForeground, &gcv);
            gcv.foreground = (*bit)->c_focused2.getPixel();
            (*bit)->g_focused2 = XCreateGC(display, id, GCForeground, &gcv);
            gcv.foreground = (*bit)->c_unfocused2.getPixel();
            (*bit)->g_unfocused2 = XCreateGC(display, id, GCForeground, &gcv);
            gcv.foreground = (*bit)->c_pressed2.getPixel();
            (*bit)->g_pressed2 = XCreateGC(display, id, GCForeground, &gcv);
        }
    }
    wstyle.wa_font.AllocColor(display, id, &wstyle.l_text_focus,
                              &wstyle.l_text_focus_s);
    wstyle.wa_font_u.AllocColor(display, id, &wstyle.l_text_unfocus,
                                &wstyle.l_text_unfocus_s);

    mstyle.wa_t_font.AllocColor(display, id, &mstyle.t_text,
                                &mstyle.t_text_s);
    
    mstyle.wa_f_font.AllocColor(display, id, &mstyle.f_text,
                                &mstyle.f_text_s);
    mstyle.wa_fh_font.AllocColor(display, id, &mstyle.f_hilite_text,
                                 &mstyle.f_hilite_text_s);

    mstyle.wa_b_font.AllocColor(display, id, &mstyle.f_text,
                                &mstyle.f_text_s);
    mstyle.wa_bh_font.AllocColor(display, id, &mstyle.f_hilite_text,
                                 &mstyle.f_hilite_text_s);

    mstyle.wa_ct_font.AllocColor(display, id, &mstyle.f_text,
                                 &mstyle.f_text_s);
    mstyle.wa_cth_font.AllocColor(display, id, &mstyle.f_hilite_text,
                                  &mstyle.f_hilite_text_s);

    mstyle.wa_cf_font.AllocColor(display, id, &mstyle.f_text,
                                 &mstyle.f_text_s);
    mstyle.wa_cfh_font.AllocColor(display, id, &mstyle.f_hilite_text,
                                  &mstyle.f_hilite_text_s);
}

/**
 * @fn    RenderCommonImages(void)
 * @brief Render common images
 *
 * Render images which are common for all windows.
 */
void WaScreen::RenderCommonImages(void) {
    WaTexture *texture;
    list<ButtonStyle *>::iterator bit = wstyle.buttonstyles.begin();
    for (; bit != wstyle.buttonstyles.end(); ++bit) {
        texture = &(*bit)->t_focused;
        if (texture->getTexture() == (WaImage_Flat | WaImage_Solid)) {
            (*bit)->p_focused = None;
            (*bit)->c_focused = texture->getColor()->getPixel();
        } else
            (*bit)->p_focused = ic->renderImage(wstyle.title_height - 4,
                                                wstyle.title_height - 4,
                                                texture);

        texture = &(*bit)->t_unfocused;
        if (texture->getTexture() == (WaImage_Flat | WaImage_Solid)) {
            (*bit)->p_unfocused = None;
            (*bit)->c_unfocused = texture->getColor()->getPixel();
        } else
            (*bit)->p_unfocused = ic->renderImage(wstyle.title_height - 4,
                                                  wstyle.title_height - 4,
                                                  texture);

        texture = &(*bit)->t_pressed;
        if (texture->getTexture() == (WaImage_Flat | WaImage_Solid)) {
            (*bit)->p_pressed = None;
            (*bit)->c_pressed = texture->getColor()->getPixel();
        } else
            (*bit)->p_pressed = ic->renderImage(wstyle.title_height - 4,
                                                wstyle.title_height - 4,
                                                texture);
        
        texture = &(*bit)->t_focused2;
        if (texture->getTexture() == (WaImage_Flat | WaImage_Solid)) {
            (*bit)->p_focused2 = None;
            (*bit)->c_focused2 = texture->getColor()->getPixel();
        } else
            (*bit)->p_focused2 = ic->renderImage(wstyle.title_height - 4,
                                                 wstyle.title_height - 4,
                                                 texture);

        texture = &(*bit)->t_unfocused2;
        if (texture->getTexture() == (WaImage_Flat | WaImage_Solid)) {
            (*bit)->p_unfocused2 = None;
            (*bit)->c_unfocused2 = texture->getColor()->getPixel();
        } else
            (*bit)->p_unfocused2 = ic->renderImage(wstyle.title_height - 4,
                                                   wstyle.title_height - 4,
                                                   texture);

        texture = &(*bit)->t_pressed2;
        if (texture->getTexture() == (WaImage_Flat | WaImage_Solid)) {
            (*bit)->p_pressed2 = None;
            (*bit)->c_pressed2 = texture->getColor()->getPixel();
        } else
            (*bit)->p_pressed2 = ic->renderImage(wstyle.title_height - 4,
                                                 wstyle.title_height - 4,
                                                 texture);
    }
    
    texture = &wstyle.g_focus;
    if (texture->getTexture() == (WaImage_Flat | WaImage_Solid)) {
        fgrip = None;
        fgrip_pixel = texture->getColor()->getPixel();
    } else
        fgrip = ic->renderImage(25, wstyle.handle_width, texture);

    texture = &wstyle.g_unfocus;
    if (texture->getTexture() == (WaImage_Flat | WaImage_Solid)) {
        ugrip = None;
        ugrip_pixel = texture->getColor()->getPixel();
    } else
        ugrip = ic->renderImage(25, wstyle.handle_width, texture);
}

/**
 * @fn    UpdateWorkarea(void)
 * @brief Update workarea
 *
 * Updates workarea, all maximized windows are maximizied to the new
 * workarea.
 */
void WaScreen::UpdateWorkarea(void) {
    int old_x = current_desktop->workarea.x,
        old_y = current_desktop->workarea.y,
        old_width = current_desktop->workarea.width,
        old_height = current_desktop->workarea.height;
    
    current_desktop->workarea.x = current_desktop->workarea.y = 0;
    current_desktop->workarea.width = width;
    current_desktop->workarea.height = height;
    list<WMstrut *>::iterator it = strut_list.begin();
    for (; it != strut_list.end(); ++it) {
        WindowObject *wo = waimea->FindWin((*it)->window,
                                           WindowType | DockHandlerType);
        if (wo && wo->type == WindowType) {
            if (! (((WaWindow *) wo)->desktop_mask &
                   (1L << current_desktop->number)))
                break;
        } else if (wo && wo->type == DockHandlerType) {
            if (! (((DockappHandler *) wo)->style->desktop_mask &
                   (1L << current_desktop->number)))
                break;
        }
            
        if ((*it)->left > current_desktop->workarea.x)
            current_desktop->workarea.x = (*it)->left;
        if ((*it)->top > current_desktop->workarea.y)
            current_desktop->workarea.y = (*it)->top;
        if ((width - (*it)->right) < current_desktop->workarea.width)
            current_desktop->workarea.width = width - (*it)->right;
        if ((height - (*it)->bottom) < current_desktop->workarea.height)
            current_desktop->workarea.height = height - (*it)->bottom;
    }
    current_desktop->workarea.width = current_desktop->workarea.width -
        current_desktop->workarea.x;
    current_desktop->workarea.height = current_desktop->workarea.height -
        current_desktop->workarea.y;
    
    int res_x, res_y, res_w, res_h;
    if (old_x != current_desktop->workarea.x ||
        old_y != current_desktop->workarea.y ||
        old_width != current_desktop->workarea.width ||
        old_height != current_desktop->workarea.height) {
        net->SetWorkarea(this);
        
        list<WaWindow *>::iterator wa_it = wawindow_list.begin();
        for (; wa_it != wawindow_list.end(); ++wa_it) {
            if (! ((*wa_it)->desktop_mask &
                   (1L << current_desktop->number))) break;
            if ((*wa_it)->flags.max) {
                (*wa_it)->flags.max = false;
                res_x = (*wa_it)->restore_max.x;
                res_y = (*wa_it)->restore_max.y;
                res_w = (*wa_it)->restore_max.width;
                res_h = (*wa_it)->restore_max.height;
                (*wa_it)->_Maximize((*wa_it)->restore_max.misc0,
                                    (*wa_it)->restore_max.misc1);
                (*wa_it)->restore_max.x = res_x;
                (*wa_it)->restore_max.y = res_y;
                (*wa_it)->restore_max.width = res_w;
                (*wa_it)->restore_max.height = res_h;
            }
        }
    }
}

//  int WaScreen::WSwidth(int x) {
    
//  #ifdef XINERAMA
//      for (int i = 0; i < xinerama_info_num; ++i) {
//          xinerama_info[i].x_org
//              xinerama_info[i].y_org,
//              xinerama_info[i].width
//              xinerama_info[i].height));
        
//  #endif // XINERAMA
    
//      return current_desktop->workarea.width;
//  }

//  int WaScreen::WSx(int x) {
//      current_desktop->workarea.x;
//  }

//  int WaScreen::WSheight(int y) {
//      current_desktop->workarea.height;
//  }

//  int WaScreen::WSy(int y) {
//      current_desktop->workarea.y;
//  }

/**
 * @fn    MoveViewportTo(int x, int y)
 * @brief Move viewport to position
 *
 * Moves the virtual viewport to position (x, y). This is done by moving all
 * windows relative to the viewport change.
 *
 * @param x New x viewport
 * @param y New y viewport
 */
void WaScreen::MoveViewportTo(int x, int y) {
    if (x > v_xmax) x = v_xmax;
    else if (x < 0) x = 0;
    if (y > v_ymax) y = v_ymax;
    else if (y < 0) y = 0;
    
    int x_move = - (x - v_x);
    int y_move = - (y - v_y);
    v_x = x;
    v_y = y;

    list<WaWindow *>::iterator it = wawindow_list.begin();
    for (; it != wawindow_list.end(); ++it) {
        if (! (*it)->flags.sticky) {
            int old_x = (*it)->attrib.x;
            int old_y = (*it)->attrib.y;
            (*it)->attrib.x = (*it)->attrib.x + x_move;
            (*it)->attrib.y = (*it)->attrib.y + y_move;
            
            if ((((*it)->attrib.x + (*it)->attrib.width) > 0 &&
                 (*it)->attrib.x < width) && 
                (((*it)->attrib.y + (*it)->attrib.height) > 0 &&
                 (*it)->attrib.y < height))
                (*it)->RedrawWindow(true);
            else {
                if (((old_x + (*it)->attrib.width) > 0 && old_x < width) && 
                    ((old_y + (*it)->attrib.height) > 0 && old_y < height))
                    (*it)->RedrawWindow();
                else {
                    (*it)->dontsend = true;
                    (*it)->RedrawWindow();
                    (*it)->dontsend = false;
                    net->SetVirtualPos(*it);
                }
            }
        }
    }
    list<WaMenu *>::iterator it2 = wamenu_list.begin();
    for (; it2 != wamenu_list.end(); ++it2) {
        if ((*it2)->mapped && (! (*it2)->root_menu))
            (*it2)->Move(x_move, y_move);
    }
    net->SetDesktopViewPort(this);
}

/**
 * @fn    MoveViewport(int direction)
 * @brief Move viewport one screen in specified direction
 *
 * Moves viewport one screen in direction specified by direction parameter.
 * Direction can be one of WestDirection, EastDirection, NorthDirection and
 * SouthDirection.
 *
 * @param direction Direction to move viewport
 */
void WaScreen::MoveViewport(int direction) {
    int vd;
    
    switch (direction) {
        case WestDirection:
            if (v_x > 0) {
                if ((v_x - width) < 0) vd = v_x;
                else vd = width;
                XWarpPointer(display, None, None, 0, 0, 0, 0, vd - 6, 0);
                MoveViewportTo(v_x - vd, v_y);
            }
            break;
        case EastDirection:
            if (v_x < v_xmax) {
                if ((v_x + width) > v_xmax) vd = v_xmax - v_x;
                else vd = width;
                XWarpPointer(display, None, None, 0, 0, 0, 0, 6 - vd, 0);
                MoveViewportTo(v_x + vd, v_y);
            }
            break;
        case NorthDirection:
            if (v_y > 0) {
                if ((v_y - height) < 0) vd = v_y;
                else vd = height;
                XWarpPointer(display, None, None, 0, 0, 0, 0, 0, vd - 6);
                MoveViewportTo(v_x, v_y - vd);
            }
            break;
        case SouthDirection:
            if (v_y < v_ymax) {
                if ((v_y + height) > v_ymax) vd = v_ymax - v_y;
                else vd = height;
                XWarpPointer(display, None, None, 0, 0, 0, 0, 0, 6 - vd);
                MoveViewportTo(v_x, v_y + vd);
            }
    }
}

/** 
 * @fn    ViewportFixedMove(XEvent *, WaAction *ac)
 * @brief Fixed viewport move
 *
 * Parses the action parameter as an X offset string. The X and Y
 * offset values defines the fixed pixel position to move the
 * viewport to.
 *
 * @param ac WaAction object
 */ 
void WaScreen::ViewportFixedMove(XEvent *, WaAction *ac) {
    int x, y, mask; 
    unsigned int w = 0, h = 0;
                            
    if (! ac->param) return;
   
    mask = XParseGeometry(ac->param, &x, &y, &w, &h);
    if (mask & XNegative) x = v_xmax + x;
    if (mask & YNegative) y = v_ymax + y;
    MoveViewportTo(x, y); 
}

/**
 * @fn    ViewportRelativeMove(XEvent *, WaAction *ac)
 * @brief Relative viewport move
 *
 * Parses the action parameter as an X offset string. The X and Y
 * offset values defines the number of pixels to move the viewport.
 *
 * @param ac WaAction object
 */
void WaScreen::ViewportRelativeMove(XEvent *, WaAction *ac) {
    int x, y, mask;
    unsigned int w = 0, h = 0;
                  
    if (! ac->param) return;
   
    mask = XParseGeometry(ac->param, &x, &y, &w, &h);
    MoveViewportTo(v_x + x, v_y + y);
}

/**
 * @fn    ViewportMove(XEvent *e, WaAction *)
 * @brief Move viewport after mouse movement
 *
 * Moves viewport after mouse motion events.
 *
 * @param e XEvent causing function call
 */
void WaScreen::ViewportMove(XEvent *e, WaAction *) {
    XEvent event;
    int px, py, i;
    list<XEvent *> *maprequest_list;
    Window w;
    unsigned int ui;
    list<WaWindow *>::iterator it;

    if (waimea->eh->move_resize != EndMoveResizeType) return;
    waimea->eh->move_resize = MoveOpaqueType;
    
    XQueryPointer(display, id, &w, &w, &px, &py, &i, &i, &ui);
    
    maprequest_list = new list<XEvent *>;
    XGrabPointer(display, id, true, ButtonReleaseMask | ButtonPressMask |
                 PointerMotionMask | EnterWindowMask | LeaveWindowMask,
                 GrabModeAsync, GrabModeAsync, None, waimea->move_cursor,
                 CurrentTime);
    XGrabKeyboard(display, id, true, GrabModeAsync, GrabModeAsync,
                  CurrentTime);
    for (;;) {
        it = wawindow_list.begin();
        for (; it != wawindow_list.end(); ++it) {
            (*it)->dontsend = true;
        }
        waimea->eh->EventLoop(waimea->eh->menu_viewport_move_return_mask,
                              &event);
        switch (event.type) {
            case MotionNotify: {
                while (XCheckTypedWindowEvent(display, event.xmotion.window,
                                              MotionNotify, &event));
                int x = v_x - (event.xmotion.x_root - px);
                int y = v_y - (event.xmotion.y_root - py);
                
                if (x > v_xmax) x = v_xmax;
                else if (x < 0) x = 0;
                if (y > v_ymax) y = v_ymax;
                else if (y < 0) y = 0;
                
                int x_move = - (x - v_x);
                int y_move = - (y - v_y);
                v_x = x;
                v_y = y;

                list<WaWindow *>::iterator it = wawindow_list.begin();
                for (; it != wawindow_list.end(); ++it) {
                    if (! (*it)->flags.sticky) {
                        int old_x = (*it)->attrib.x;
                        int old_y = (*it)->attrib.y;
                        (*it)->attrib.x = (*it)->attrib.x + x_move;
                        (*it)->attrib.y = (*it)->attrib.y + y_move;
            
                        if ((((*it)->attrib.x + (*it)->attrib.width) > 0 &&
                             (*it)->attrib.x < width) && 
                            (((*it)->attrib.y + (*it)->attrib.height) > 0 &&
                             (*it)->attrib.y < height))
                            (*it)->RedrawWindow();
                        else {
                            if (((old_x + (*it)->attrib.width) > 0 &&
                                 old_x < width) && 
                                ((old_y + (*it)->attrib.height) > 0 &&
                                 old_y < height))
                                (*it)->RedrawWindow();
                        }
                    }
                }
                list<WaMenu *>::iterator it2 = wamenu_list.begin();
                for (; it2 != wamenu_list.end(); ++it2) {
                    if ((*it2)->mapped && (! (*it2)->root_menu))
                        (*it2)->Move(x_move, y_move

#ifdef RENDER
                                     , !config.lazy_trans
#endif // RENDER

                                     );
                }
                px = event.xmotion.x_root;
                py = event.xmotion.y_root;
            } break;
            case LeaveNotify:
            case EnterNotify:
                break;
            case MapRequest:
                maprequest_list->push_front(&event); break;
            case ButtonPress:
            case ButtonRelease:
                event.xbutton.window = id;
            case KeyPress:
            case KeyRelease:
                if (event.type == KeyPress || event.type == KeyRelease)
                    event.xkey.window = id;
                waimea->eh->HandleEvent(&event);
                if (waimea->eh->move_resize != EndMoveResizeType) break;
                while (! maprequest_list->empty()) {
                    XPutBackEvent(display, maprequest_list->front());
                    maprequest_list->pop_front();
                }
                delete maprequest_list;
                it = wawindow_list.begin();
                for (; it != wawindow_list.end(); ++it) {
                    (*it)->dontsend = false;
                    net->SetVirtualPos(*it);
                    if ((((*it)->attrib.x + (*it)->attrib.width) > 0 &&
                         (*it)->attrib.x < width) && 
                        (((*it)->attrib.y + (*it)->attrib.height) > 0 &&
                         (*it)->attrib.y < height)) {
                        
#ifdef RENDER
                        if (config.lazy_trans) {
                            (*it)->render_if_opacity = true;
                            (*it)->DrawTitlebar();
                            (*it)->DrawHandlebar();
                            (*it)->render_if_opacity = false;
                        }
#endif // RENDER

                        (*it)->SendConfig();
                    }
                }

#ifdef RENDER
                if (config.lazy_trans) {
                    list<WaMenu *>::iterator it2 = wamenu_list.begin();
                    for (; it2 != wamenu_list.end(); ++it2) {
                        if ((*it2)->mapped && (! (*it2)->root_menu))
                            (*it2)->Move(0, 0, true);
                    }
                }
#endif // RENDER

                net->SetDesktopViewPort(this);
                XUngrabKeyboard(display, CurrentTime);
                XUngrabPointer(display, CurrentTime);
                return;
        }
    }
}

/**
 * @fn    EndMoveResize(XEvent *e, WaAction *)
 * @brief Ends move
 *
 * Ends viewport moving process.
 */
void WaScreen::EndMoveResize(XEvent *, WaAction *) {
    waimea->eh->move_resize = EndMoveResizeType;
}

/**
 * @fn    Focus(XEvent *, WaAction *)
 * @brief Set input focus to root image
 *
 * Sets the keyboard input focus to the WaScreens root window.
 */
void WaScreen::Focus(XEvent *, WaAction *) {
    focus = true;
    XSetInputFocus(display, id, RevertToPointerRoot, CurrentTime);
}

/**
 * @fn    MenuMap(XEvent *e, WaAction *ac)
 * @brief Maps a menu
 *
 * Maps a menu at the current mouse position.
 *
 * @param ac WaAction object
 * @param bool True if we should focus first item in menu
 */
void WaScreen::MenuMap(XEvent *, WaAction *ac, bool focus) {
    Window w;
    int i, x, y;
    unsigned int ui;
    WaMenu *menu = GetMenuNamed(ac->param);

    if (! menu) return;
    if (waimea->eh->move_resize != EndMoveResizeType) return;

    if (XQueryPointer(display, id, &w, &w, &x, &y, &i, &i, &ui)) {
        if (menu->tasksw) menu->Build(this);
        menu->rf = this;
        menu->ftype = MenuRFuncMask;
        if ((y + menu->height + mstyle.border_width * 2) >
            (unsigned int) (current_desktop->workarea.height +
                            current_desktop->workarea.y))
           y -= (menu->height + mstyle.border_width * 2);
        if ((x + menu->width + mstyle.border_width * 2) >
            (unsigned int) (current_desktop->workarea.width +
                            current_desktop->workarea.x))
            x -= (menu->width + mstyle.border_width * 2);
        menu->Map(x, y);
        if (focus) menu->FocusFirst();
    }
}

/**
 * @fn    MenuRemap(XEvent *e, WaAction *ac)
 * @brief Maps a menu
 *
 * Remaps a menu at the current mouse position.
 *
 * @param ac WaAction object
 * @param bool True if we should focus first item in menu
 */
void WaScreen::MenuRemap(XEvent *, WaAction *ac, bool focus) {
    Window w;
    int i, x, y;
    unsigned int ui;
    WaMenu *menu = GetMenuNamed(ac->param);

    if (! menu) return;
    if (menu->dynamic && menu->mapped) {
        menu->Unmap(menu->has_focus);
        if (! (menu = CreateDynamicMenu(ac->param))) return;
    }
    if (waimea->eh->move_resize != EndMoveResizeType) return;
    
    if (XQueryPointer(display, id, &w, &w, &x, &y, &i, &i, &ui)) {
        if (menu->tasksw) menu->Build(this);
        menu->rf = this;
        menu->ftype = MenuRFuncMask;
        if ((y + menu->height + mstyle.border_width * 2) >
            (unsigned int) (current_desktop->workarea.height +
                            current_desktop->workarea.y))
           y -= (menu->height + mstyle.border_width * 2);
        if ((x + menu->width + mstyle.border_width * 2) >
            (unsigned int) (current_desktop->workarea.width +
                            current_desktop->workarea.x))
            x -= (menu->width + mstyle.border_width * 2);
        menu->ignore = true;
        menu->ReMap(x, y);
        menu->ignore = false;
        if (focus) menu->FocusFirst();
    }
}

/**
 * @fn    MenuUnmap(XEvent *, WaAction *ac, bool focus)
 * @brief Unmaps menu
 *
 * Unmaps a menu and its linked submenus.
 *
 * @param ac WaAction object
 * @param bool True if we should focus root item
 */
void WaScreen::MenuUnmap(XEvent *, WaAction *ac, bool focus) {
    WaMenu *menu = GetMenuNamed(ac->param);

    if (! menu) return;
    if (waimea->eh->move_resize != EndMoveResizeType) return;
    
    menu->Unmap(focus);
    menu->UnmapSubmenus(focus);
}

/**
 * @fn    Restart(XEvent *, WaAction *)
 * @brief Restarts window manager
 *
 * Restarts window manager by deleting all objects and executing the program
 * file again.
 *
 * @param ac WaAction object
 */
void WaScreen::Restart(XEvent *, WaAction *ac) {
    if (ac->param)
        restart(ac->param);
    else
        restart(NULL);
}

/**
 * @fn    Exit(XEvent *, WaAction *)
 * @brief Shutdowns window manager
 *
 * Shutdowns window manager. Returning successful status.
 */
void WaScreen::Exit(XEvent *, WaAction *) {
    quit(EXIT_SUCCESS);
}

/**
 * @fn    TaskSwitcher(XEvent *, WaAction *)
 * @brief Maps task switcher menu
 *
 * Maps task switcher menu at middle of screen and sets input focus to
 * first window in list.
 */
void WaScreen::TaskSwitcher(XEvent *, WaAction *) {
    if (waimea->eh->move_resize != EndMoveResizeType) return;
    window_menu->Build(this);
    window_menu->ReMap(width / 2 - window_menu->width / 2,
                       height / 2 - window_menu->height / 2);
    window_menu->FocusFirst();
}

/**
 * @fn    PreviousTask(XEvent *e, WaAction *ac)
 * @brief Switches to previous task
 *
 * Switches to the previous focused window.
 *
 * @param e X event that have occurred
 * @param ac WaAction object
 */
void WaScreen::PreviousTask(XEvent *e, WaAction *ac) {
    if (waimea->eh->move_resize != EndMoveResizeType) return;
    list<WaWindow *>::iterator it = wawindow_list.begin();
    it++;
    (*it)->Raise(e, ac);
    (*it)->FocusVis(e, ac);
}

/**
 * @fn    NextTask(XEvent *e, WaAction *ac)
 * @brief Switches to next task
 *
 * Switches to the window that haven't had focus for longest time.
 *
 * @param e X event that have occurred
 * @param ac WaAction object
 */
void WaScreen::NextTask(XEvent *e, WaAction *ac) {
    if (waimea->eh->move_resize != EndMoveResizeType) return;
    wawindow_list.back()->Raise(e, ac);
    wawindow_list.back()->FocusVis(e, ac);
}

/**
 * @fn    PointerFixedWarp(XEvent *e, WaAction *ac)
 * @brief Warps pointer to fixed position
 *
 * Parses the action parameter as an X offset string. The X and Y
 * offset values defines the fixed pixel position to warp the 
 * pointer to.
 *
 * @param ac WaAction object
 */
void WaScreen::PointerFixedWarp(XEvent *, WaAction *ac) {
    int x, y, mask, i, o_x, o_y;
    unsigned int ui, w, h;
    Window dw;
    
    mask = XParseGeometry(ac->param, &x, &y, &w, &h);
    if (mask & XNegative) x = width + x;
    if (mask & YNegative) y = height + y;
    XQueryPointer(display, id, &dw, &dw, &o_x, &o_y, &i, &i, &ui);
    x = x - o_x;
    y = y - o_y;
    XWarpPointer(display, None, None, 0, 0, 0, 0, x, y);
}

/** 
 * @fn    PointerRelativeWarp(XEvent *e, WaAction *ac)
 * @brief Relative pointer warp
 *
 * Parses the action parameter as an X offset string. The X and Y
 * offset values defines the number of pixels to warp the pointer.
 * 
 * @param ac WaAction object
 */
void WaScreen::PointerRelativeWarp(XEvent *, WaAction *ac) {
    int x, y, mask;
    unsigned int w, h;
                    
    mask = XParseGeometry(ac->param, &x, &y, &w, &h);
    XWarpPointer(display, None, None, 0, 0, 0, 0, x, y);
}

/** 
 * @fn    GoToDesktop(unsigned int number)
 * @brief Go to desktop
 *
 * Sets current desktop to the one specified by number parameter.
 * 
 * @param number Desktop number to go to
 */
void WaScreen::GoToDesktop(unsigned int number) {
    list<Desktop *>::iterator dit = desktop_list.begin();
    for (; dit != desktop_list.end(); dit++)
        if ((unsigned int) (*dit)->number == number) break;
    
    if (dit != desktop_list.end() && *dit != current_desktop) {
        Window oldf = waimea->eh->focused;
        XSetInputFocus(display, id, RevertToPointerRoot, CurrentTime);
        (*dit)->workarea.x = current_desktop->workarea.x;
        (*dit)->workarea.y = current_desktop->workarea.y;
        (*dit)->workarea.width = current_desktop->workarea.width;
        (*dit)->workarea.height = current_desktop->workarea.height;
        current_desktop = (*dit);

        list<WaWindow *>::iterator it = wawindow_list.begin();
        for (; it != wawindow_list.end(); ++it) {
            if ((*it)->desktop_mask & (1L << current_desktop->number)) {
                (*it)->Show();
                net->SetDesktop(*it);
            }
            else
                (*it)->Hide();
        }

        WaWindow *ww = (WaWindow *) waimea->FindWin(oldf, WindowType);
        if (ww) {
            if (ww->desktop_mask & (1L << current_desktop->number)) {
                ww->Focus(false);
            }
        }
        
        list<DockappHandler *>::iterator dock_it = docks.begin();
        for (; dock_it != docks.end(); ++dock_it) {
            if ((*dock_it)->style->desktop_mask &
                (1L << current_desktop->number)) {
                if ((*dock_it)->hidden) {
                    XMapWindow(display, (*dock_it)->id);
                    (*dock_it)->hidden = false;
                    (*dock_it)->Render();
                }
            } else if (! (*dock_it)->hidden) {
                XUnmapWindow(display, (*dock_it)->id);
                (*dock_it)->hidden = true;
            }
        }
        UpdateWorkarea();
        net->SetCurrentDesktop(this);
    } else
        if (dit == desktop_list.end())
            WARNING << "bad desktop id `" << number << "', desktop " <<
                number << " doesn't exist" << endl;
}

/** 
 * @fn    GoToDesktop(XEvent *, WaAction *ac)
 * @brief Go to desktop
 *
 * Sets current desktop to the one specified by action parameter.
 * 
 * @param ac WaAction object
 */
void WaScreen::GoToDesktop(XEvent *, WaAction *ac) {
    if (ac->param) GoToDesktop((unsigned int) atoi(ac->param));
}

/** 
 * @fn    NextDesktop(XEvent *, WaAction *)
 * @brief Go to next desktop
 *
 * Sets current desktop to desktop with ID number current ID + 1. If
 * current ID + 1 doesn't exist then current desktop is set to desktop with
 * ID 0.
 * 
 * @param ac WaAction object
 */
void WaScreen::NextDesktop(XEvent *, WaAction *) {
    if (current_desktop->number + 1 == config.desktops)
        GoToDesktop(0);
    else
        GoToDesktop(current_desktop->number + 1);
}

/** 
 * @fn    PreviousDesktop(XEvent *, WaAction *)
 * @brief Go to previous desktop
 *
 * Sets current desktop to desktop with ID number current ID - 1. If
 * current ID - 1 is negative then current desktop is set to desktop with
 * highest desktop number.
 * 
 * @param ac WaAction object
 */
void WaScreen::PreviousDesktop(XEvent *, WaAction *) {
    if (current_desktop->number == 0)
        GoToDesktop(config.desktops - 1);
    else
        GoToDesktop(current_desktop->number - 1);
}

/**
 * @fn    EvAct(XEvent *e, EventDetail *ed, list<WaAction *> *acts)
 * @brief Calls WaScreen function
 *
 * Tries to match an occurred X event with the actions in an action list.
 * If we have a match then we execute that action.
 *
 * @param e X event that have occurred
 * @param ed Event details
 * @param acts List with actions to match event with
 */
void WaScreen::EvAct(XEvent *e, EventDetail *ed, list<WaAction *> *acts) {
    if (waimea->eh->move_resize != EndMoveResizeType)
        ed->mod |= MoveResizeMask;
    list<WaAction *>::iterator it = acts->begin();
    for (; it != acts->end(); ++it) {
        if (eventmatch(*it, ed)) {
            if ((*it)->delay.tv_sec || (*it)->delay.tv_usec) {
                Interrupt *i = new Interrupt(*it, e, id);
                waimea->timer->AddInterrupt(i);
            }
            else {
                if ((*it)->exec)
                    waexec((*it)->exec, displaystring);
                else
                    ((*this).*((*it)->rootfunc))(e, *it);
            }
        }
    }
}

/**
 * @fn    AddDockapp(Window window)
 * @brief Add dockapp to dockapp system
 *
 * Inserts the dockapp into the right dockapp holder.
 *
 * @param window Window ID of dockapp window
 */
void WaScreen::AddDockapp(Window window) {
    Dockapp *da;
    XClassHint *c_hint = XAllocClassHint();
    int have_hints = XGetClassHint(display, window, c_hint);
    char *title;
    if (! XFetchName(display, window, &title))
        title = NULL;

    list<Regex *>::iterator reg_it;
    list<int>::iterator regt_it;
    list<DockappHandler *>::iterator dock_it = docks.begin();
    for (; dock_it != docks.end(); ++dock_it) {
        if (have_hints) {
            reg_it = (*dock_it)->style->order.begin();
            regt_it = (*dock_it)->style->order_type.begin();
            for (; reg_it != (*dock_it)->style->order.end();
                 ++reg_it, ++regt_it) {
                if ((*regt_it == NameMatchType) &&
                    ((*reg_it)->Match(c_hint->res_name))) {
                    da = new Dockapp(window, *dock_it);
                    da->c_hint = c_hint;
                    da->title = title;
                    (*dock_it)->Update();
                    return;
                }
            }
            reg_it = (*dock_it)->style->order.begin();
            regt_it = (*dock_it)->style->order_type.begin();
            for (; reg_it != (*dock_it)->style->order.end();
                 ++reg_it, ++regt_it) {
                if (have_hints && (*regt_it == ClassMatchType) &&
                    ((*reg_it)->Match(c_hint->res_class))) {
                    da = new Dockapp(window, *dock_it);
                    da->c_hint = c_hint;
                    da->title = title;
                    (*dock_it)->Update();
                    return;
                }
            }
        }
        if (title) {
            reg_it = (*dock_it)->style->order.begin();
            regt_it = (*dock_it)->style->order_type.begin();
            for (; reg_it != (*dock_it)->style->order.end();
                 ++reg_it, ++regt_it) {
                if ((*regt_it == TitleMatchType) &&
                    ((*reg_it)->Match(title))) {
                    da = new Dockapp(window, *dock_it);
                    da->c_hint = c_hint;
                    da->title = title;
                    (*dock_it)->Update();
                    return;
                }
            }
        }
    }
    DockappHandler *lastd = docks.back();
    da = new Dockapp(window, lastd);
    da->c_hint = NULL;
    da->title = NULL;
    lastd->Update();
    if (have_hints) {
        XFree(c_hint->res_name);
        XFree(c_hint->res_class);
    }
    XFree(c_hint);
}


/**
 * @fn    ScreenEdge(WaScreen *wascrn, int x, int y, int width, int height,
 *                   int type) : WindowObject(0, type)
 * @brief Constructor for ScreenEdge class
 *
 * Creates an always on top screen edge window.
 *
 * @param wascrn WaScreen Object
 * @param x X position
 * @param y Y position
 * @param width Width of ScreenEdge window
 * @param height Height of ScreenEdge window
 * @param type Type of WindowObject
 */
ScreenEdge::ScreenEdge(WaScreen *wascrn, int x, int y, int width, int height,
                       int type) : WindowObject(0, type) {
    XSetWindowAttributes attrib_set;
    
    wa = wascrn;
    attrib_set.override_redirect = true;
    attrib_set.event_mask = EnterWindowMask | LeaveWindowMask |
        ButtonPressMask | ButtonReleaseMask;
    
    id = XCreateWindow(wa->display, wa->id, x, y, width, height, 0,
                       CopyFromParent, InputOnly, CopyFromParent,
                       CWOverrideRedirect | CWEventMask, &attrib_set);

    wa->waimea->net->wXDNDMakeAwareness(id);
}

/**
 * @fn    ScreenEdge::SetActionlist(list<WaAction *> *list)
 * @brief Sets actionlist
 *
 * Sets screenedge actionlist and if list is other than empty screenedge
 * window is mapped.
 *
 * @param list Actionlist to set
 */
void ScreenEdge::SetActionlist(list<WaAction *> *list) {
    actionlist = list;
    if (! actionlist->empty()) {
        XMapWindow(wa->display, id);
        wa->always_on_top_list.push_back(id);
        wa->WaRaiseWindow(0);
        wa->waimea->window_table.insert(make_pair(id, this));
    }
}

/**
 * @fn    ~ScreenEdge(void)
 * @brief Destructor for ScreenEdge class
 *
 * Destroys ScreenEdge window
 */
ScreenEdge::~ScreenEdge(void) {
    if (! actionlist->empty()) {
        wa->always_on_top_list.remove(id);
        wa->waimea->window_table.erase(id);
    }
    XDestroyWindow(wa->display, id);
}
