/**
 * @file   Event.cc
 * @author David Reveman <c99drn@cs.umu.se>
 * @date   11-May-2001 11:48:03
 *
 * @brief Implementation of EventHandler class  
 *
 * Eventloop function and functions for handling XEvents.
 *
 * Copyright (C) David Reveman. All rights reserved.
 *
 */

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

extern "C" {
#include <X11/Xatom.h>

#ifdef    HAVE_STDIO_H
#  include <stdio.h>
#endif // HAVE_STDIO_H    
}

#include "Event.hh"

/**
 * @fn    EventHandler(Waimea *wa)
 * @brief Constructor for EventHandler class
 *
 * Sets waimea and rh pointers. Creates menu move return mask, window
 * move/resize return mask and empty return mask sets.
 *
 * @param wa Pointer to waimea object
 */
EventHandler::EventHandler(Waimea *wa) {
    waimea = wa;
    rh = waimea->rh;
    focused = last_click_win = (Window) 0;
    move_resize = EndMoveResizeType;
    last_button = 0;

    empty_return_mask = new set<int>;
    
    moveresize_return_mask = new set<int>;
    moveresize_return_mask->insert(MotionNotify);
    moveresize_return_mask->insert(ButtonPress);
    moveresize_return_mask->insert(ButtonRelease);
    moveresize_return_mask->insert(KeyPress);
    moveresize_return_mask->insert(KeyRelease);
    moveresize_return_mask->insert(MapRequest);
    moveresize_return_mask->insert(UnmapNotify);
    moveresize_return_mask->insert(DestroyNotify);
    moveresize_return_mask->insert(EnterNotify);
    moveresize_return_mask->insert(LeaveNotify);
    moveresize_return_mask->insert(ConfigureRequest);

    menu_viewport_move_return_mask = new set<int>;
    menu_viewport_move_return_mask->insert(MotionNotify);
    menu_viewport_move_return_mask->insert(ButtonPress);
    menu_viewport_move_return_mask->insert(ButtonRelease);
    menu_viewport_move_return_mask->insert(KeyPress);
    menu_viewport_move_return_mask->insert(KeyRelease);
    menu_viewport_move_return_mask->insert(MapRequest);
    menu_viewport_move_return_mask->insert(EnterNotify);
    menu_viewport_move_return_mask->insert(LeaveNotify);
}

/**
 * @fn    ~EventHandler(void)
 * @brief Destructor for EventHandler class
 *
 * Deletes return mask lists
 */
EventHandler::~EventHandler(void) {
    MAPPTRCLEAR(empty_return_mask);
    MAPPTRCLEAR(moveresize_return_mask);
    MAPPTRCLEAR(menu_viewport_move_return_mask);
}

/**
 * @fn    EventLoop(set<int> *return_mask, XEvent *event)
 * @brief Eventloop
 *
 * Infinite loop waiting for an event to occur. This function can be called
 * from move and resize functions the return_mask set is then used for
 * deciding if an event should be processed as normal or returned to the
 * function caller.
 *
 * @param return_mask set to use as return_mask
 * @param event Pointer to allocated event structure
 */
void EventHandler::EventLoop(set<int> *return_mask, XEvent *event) {    
    for (;;) {
        XNextEvent(waimea->display, event);
        
        if (return_mask->find(event->type) != return_mask->end()) return;
        
        HandleEvent(event);
    }
}

/**
 * @fn    HandleEvent(XEvent *event);
 * @brief Eventloop
 *
 * Executes a matching function for an event. If what to do for an
 * event is controlled by an action list we set etype, edetail and emod
 * variables and call the EvAct() function.
 *
 * @param event Pointer to allocated event structure
 */
void EventHandler::HandleEvent(XEvent *event) {
    Window w;
    int i, rx, ry;
    struct timeval click_time;
    
    EventDetail *ed = new EventDetail;

    switch (event->type) {
        case ConfigureRequest:
            EvConfigureRequest(&event->xconfigurerequest); break;
        case Expose:
            if (event->xexpose.count == 0) {
                while (XCheckTypedWindowEvent(waimea->display,
                                              event->xexpose.window,
                                              Expose, event));
                EvExpose(&event->xexpose);
            }
            break;
        case PropertyNotify:
            EvProperty(&event->xproperty); break;
        case UnmapNotify:
            if(event->xunmap.event != event->xunmap.window) return;
        case DestroyNotify:
        case ReparentNotify:
            EvUnmapDestroy(event); break;
        case FocusOut:
        case FocusIn:
            EvFocus(&event->xfocus); break;
        case LeaveNotify:
        case EnterNotify:
            if (event->xcrossing.mode == NotifyGrab) break;
            ed->type = event->type;
            ed->mod = event->xcrossing.state;
            ed->detail = 0;
            EvAct(event, event->xcrossing.window, ed);
            break;
        case KeyPress:
        case KeyRelease:
            ed->type = event->type;
            ed->mod = event->xkey.state;
            ed->detail = event->xkey.keycode;
            EvAct(event, event->xkey.window, ed);
            break;
        case ButtonPress:
            ed->type = ButtonPress;
            if (last_button == event->xbutton.button &&
                last_click_win == event->xbutton.window) {
                gettimeofday(&click_time, NULL);
                if (click_time.tv_sec <= last_click.tv_sec + 1) {
                    if (click_time.tv_sec == last_click.tv_sec &&
                        (unsigned long) 
                        (click_time.tv_usec - last_click.tv_usec) <
                        waimea->double_click * 1000) {
                        ed->type = DoubleClick;
                        last_click_win = (Window) 0;
                    }
                    else if ((1000000 - last_click.tv_usec) +
                             (unsigned long) click_time.tv_usec <
                             waimea->double_click * 1000) {
                        ed->type = DoubleClick;
                        last_click_win = (Window) 0;
                    }
                    else {
                        last_click_win = event->xbutton.window;
                        last_click.tv_sec = click_time.tv_sec;
                        last_click.tv_usec = click_time.tv_usec;
                    }
                }
                else {
                    last_click_win = event->xbutton.window;
                    last_click.tv_sec = click_time.tv_sec;
                    last_click.tv_usec = click_time.tv_usec;
                }
            }
            else {
                last_click_win = event->xbutton.window;
                gettimeofday(&last_click, NULL);
            }
            last_button = event->xbutton.button;
        case ButtonRelease:
            if (event->type == ButtonRelease) ed->type = ButtonRelease;
            ed->mod = event->xbutton.state;
            ed->detail = event->xbutton.button;
            EvAct(event, event->xbutton.window, ed);
            break;
        case ColormapNotify:
            EvColormap(&event->xcolormap); break;
        case MapRequest:
            EvMapRequest(&event->xmaprequest);
            ed->type = event->type;
            XQueryPointer(waimea->display, event->xmaprequest.parent,
                          &w, &w, &rx, &ry, &i, &i, &(ed->mod));
            ed->detail = 0;
            event->xbutton.x_root = rx;
            event->xbutton.y_root = ry;
            EvAct(event, event->xmaprequest.window, ed);
            break;
        case ClientMessage:
            EvClientMessage(event, ed);
            break;
            
#ifdef SHAPE
        default:                
            if (event->type == waimea->shape_event) {
                WaWindow *ww = (WaWindow *)
                    waimea->FindWin(event->xany.window, WindowType);
                if (ww && waimea->shape)
                    ww->Shape();
            }
#endif // SHAPE
            
    }
    delete ed;
}

/**
 * @fn    EvProperty(XPropertyEvent *e)
 * @brief PropertyEvent handler
 *
 * We receive a property event when a window want us to update some window
 * info. Unless the state of the property event is PropertyDelete, we try 
 * to find the WaWindow managing the the window who sent the event. If a
 * WaWindow was found, we update the stuff indicated by the event. If the 
 * name should be updated we also redraw the label foreground for the
 * WaWindow. If atom is _NET_WM_STRUT we update the strut list and workarea.
 *
 * @param e	The PropertyEvent
 */
void EventHandler::EvProperty(XPropertyEvent *e) {
    WaWindow *ww;

    if (e->state == PropertyDelete) {
        if (e->atom == waimea->net->net_wm_strut) {
            if ((ww = (WaWindow *) waimea->FindWin(e->window, WindowType))) {
                list<WMstrut *>::iterator s_it =
                    ww->wascreen->strut_list.begin();
                for (; s_it != ww->wascreen->strut_list.end(); ++s_it) {
                    if ((*s_it)->window == e->window) {
                        ww->wascreen->strut_list.remove(*s_it);
                        free(*s_it);
                        ww->wascreen->UpdateWorkarea();
                    }
                }
            }
        }
    } else if (e->atom == waimea->net->net_wm_strut) {
        if ((ww = (WaWindow *) waimea->FindWin(e->window, WindowType)))
            waimea->net->GetWmStrut(ww);
    } else if (e->atom == XA_WM_NAME) {
        if ((ww = (WaWindow *) waimea->FindWin(e->window, WindowType))) {
            waimea->net->GetXaName(ww);
            if (ww->wascreen->config.db) {
                ww->title->Render();
                ww->label->Render();
            } else
                ww->label->Draw();
        }
    }
#ifdef RENDER
    else if (e->atom == waimea->net->xrootpmap_id) {
        if (WaScreen *ws = (WaScreen *) waimea->FindWin(e->window, RootType)) {
            waimea->net->GetXRootPMapId(ws);
            ws->ic->setXRootPMapId((ws->xrootpmap_id)? true: false);
            
            list<DockappHandler *>::iterator dock_it = ws->docks.begin();
            for (; dock_it != ws->docks.end(); ++dock_it)
                if ((*dock_it)->dockapp_list->size()) (*dock_it)->Render();
            list<WaWindow *>::iterator win_it = ws->wawindow_list.begin();
            for (; win_it != ws->wawindow_list.end(); ++win_it) {
                if ((*win_it)->title_w) (*win_it)->DrawTitlebar();
                if ((*win_it)->handle_w) (*win_it)->DrawHandlebar();
            }
            list<WaMenu *>::iterator menu_it = ws->wamenu_list.begin();
            for (; menu_it != ws->wamenu_list.end(); ++menu_it) {
                if ((*menu_it)->mapped) (*menu_it)->Render();
            }
        }
    }
#endif // RENDER
    
}

/**
 * @fn    EvExpose(XExposeEvent *e)
 * @brief ExposeEvent handler
 *
 * We receive an expose event when a windows foreground has been exposed
 * for some change. If the event is from one of our windows with
 * foreground, we redraw the foreground for this window.  
 * 
 * @param e	The ExposeEvent
 */
void EventHandler::EvExpose(XExposeEvent *e) {
    if (WindowObject *wo = waimea->FindWin(e->window, LabelType | ButtonType |
                                           MenuTitleType | MenuItemType |
                                           MenuSubType | MenuCBItemType))
        switch (wo->type) {
            case LabelType:
                if (! ((WaChildWindow *) wo)->wa->wascreen->config.db)
                    ((WaChildWindow *) wo)->Draw();
                break;
            case ButtonType:
                ((WaChildWindow *) wo)->Draw(); break;
            case MenuTitleType:
            case MenuItemType:
            case MenuSubType:
            case MenuCBItemType:
                if (! ((WaMenuItem *) wo)->db)
                    ((WaMenuItem *) wo)->Draw();
        }
}

/**
 * @fn    EvFocus(XFocusChangeEvent *e)
 * @brief FocusChangeEvent handler
 *
 * We receive a focus change event when a window gets the keyboard focus.
 * If a WaWindow is managing the window pointed to by the event then we change
 * the WaWindows decorations to represent a focused window, and we change the
 * before focused WaWindows decorations to represent an unfocused window.
 *
 * @param e	The FocusChangeEvent
 */
void EventHandler::EvFocus(XFocusChangeEvent *e) {
    WaWindow *ww = NULL, *ww2;

    if (e->type == FocusIn && e->window != focused) {
        ww = (WaWindow *) waimea->FindWin(e->window, WindowType);
        if ((ww2 = (WaWindow *) waimea->FindWin(focused, WindowType))) {
            ww2->actionlist =
                ww2->GetActionList(&ww2->wascreen->config.ext_pwinacts);
            if (! ww2->actionlist)
                ww2->actionlist = &ww2->wascreen->config.pwinacts;
            ww2->UpdateGrabs();
            ww2->UnFocusWin();
        }
        if (ww) {
            ww->actionlist =
                ww->GetActionList(&ww->wascreen->config.ext_awinacts);
            if (! ww->actionlist)
                ww->actionlist = &ww->wascreen->config.awinacts;
            ww->UpdateGrabs();
            ww->FocusWin();
            ww->net->SetActiveWindow(ww->wascreen, ww);
        }
        focused = e->window;
    }
    if (WaScreen *ws = (WaScreen *) waimea->FindWin(e->window, RootType))
        waimea->net->SetActiveWindow(ws, NULL);
}

/**
 * @fn    EvConfigureRequest(XConfigureRequestEvent *e)
 * @brief ConfigureRequestEvent handler
 *
 * When we receive this event a window wants to be reconfigured (raised,
 * lowered, moved, resized). We try find a matching WaWindow managing the
 * window, if found we update that WaWindow with the values from the configure
 * request event. If we don't found a WaWindow managing the window who wants
 * to be reconfigured, we just update that window with the values from the
 * configure request event.
 *
 * @param e	The ConfigureRequestEvent
 */
void EventHandler::EvConfigureRequest(XConfigureRequestEvent *e) {
    WindowObject *wo;
    WaWindow *ww;
    Dockapp *da;
    XWindowChanges wc;
    int mask;

    wc.x = e->x;
    wc.y = e->y;
    wc.width = e->width;
    wc.height = e->height;
    wc.sibling = e->above;
    wc.stack_mode = e->detail;
    wc.border_width = e->border_width;

    if ((wo = waimea->FindWin(e->window, WindowType | DockAppType))) {
        if (wo->type == WindowType) {
            ww = (WaWindow *) wo;
            waimea->net->GetWMNormalHints(ww);
            if (ww->ign_config_req) return;
            ww->Gravitate(RemoveGravity);
            if (e->value_mask & CWX) ww->attrib.x = e->x;
            if (e->value_mask & CWY) ww->attrib.y = e->y;
            if (e->value_mask & CWWidth) ww->attrib.width = e->width;
            if (e->value_mask & CWHeight) ww->attrib.height = e->height;
            ww->Gravitate(ApplyGravity);
            ww->RedrawWindow();
            wc.sibling = e->above;
            wc.stack_mode = e->detail;
            wc.border_width = 0;
            mask = (e->value_mask & CWSibling)? CWSibling: 0;
            mask |= (e->value_mask & CWStackMode)? CWStackMode: 0;
            XConfigureWindow(ww->display, ww->frame->id, mask, &wc);
            if (e->value_mask & CWStackMode) {
                ww->wascreen->WaRaiseWindow((Window) 0);
            }
            ww->net->SetVirtualPos(ww);
            return;
        }
        else if (wo->type == DockAppType) {
            da = (Dockapp *) wo;
            XGrabServer(e->display);
            if (e->value_mask & CWWidth) da->width = e->width; 
            if (e->value_mask & CWHeight) da->height = e->height; 
            if (validateclient(da->id))
                XConfigureWindow(e->display, da->id, e->value_mask, &wc);
            XUngrabServer(e->display);
            da->dh->Update();
        }
    }
    XGrabServer(e->display);
    if (validateclient(e->window))
        XConfigureWindow(e->display, e->window, e->value_mask, &wc);
    XUngrabServer(e->display);
}

/**
 * @fn    EvColormap(XColormapEvent *e)
 * @brief ColormapEvent handler
 *
 * A window wants to install a new colormap, so we do it.
 *
 * @param e	The ColormapEvent
 */
void EventHandler::EvColormap(XColormapEvent *e) {
    XInstallColormap(e->display, e->colormap);
}

/**
 * @fn    EvMapRequest(XMapRequestEvent *e)
 * @brief MapRequestEvent handler
 *
 * We receive this event then a window wants to be mapped-> If the window 
 * isn't in our window hash_map already it's a new window and we create 
 * a WaWindow for it. If the window already is managed we just set its
 * state to NormalState.
 *
 * @param e	The MapRequestEvent
 */
void EventHandler::EvMapRequest(XMapRequestEvent *e) {
    XWindowAttributes attr;
    XWMHints *wm_hints;

    if (WaWindow *ww = (WaWindow *) waimea->FindWin(e->window, WindowType)) {
        ww->net->SetState(ww, NormalState);
    }
    else if (WaScreen *ws =
             (WaScreen *) waimea->FindWin(e->parent, RootType)) {
        wm_hints = XAllocWMHints();
        XGetWindowAttributes(e->display, e->window, &attr);
        if (! attr.override_redirect) {
            if ((wm_hints = XGetWMHints(e->display, e->window)) &&
                (wm_hints->flags & StateHint) &&
                (wm_hints->initial_state == WithdrawnState)) {
                ws->AddDockapp(e->window);
            } else {
                new WaWindow(e->window, ws);
                ws->net->SetClientList(ws);
                ws->net->SetClientListStacking(ws);
            }
        }
        XFree(wm_hints);
    }
}

/**
 * @fn    EvUnmapDestroy(XEvent *e)
 * @brief UnmapEvent, DestroyEvent and ReparentEvent handler
 *
 * We receive this event then a window has been unmapped, destroyed or 
 * reparented. If we can find a WaWindow for this window then the delete that
 * WaWindow. If we couldn't find a WaWindow we check if the windows is a 
 * dockapp window and if it is, we update the dockapp handler holding the 
 * dockapp.
 *
 * @param e	The XEvent
 */
void EventHandler::EvUnmapDestroy(XEvent *e) {
    DockappHandler *dh;
    WindowObject *wo;

    if ((wo = waimea->FindWin((e->type == UnmapNotify)?
                             e->xunmap.window:
                             (e->type == DestroyNotify)?
                             e->xdestroywindow.window:
                             e->xreparent.window,
                             WindowType | DockAppType))) {
        if (wo->type == WindowType) {
            if (e->type == DestroyNotify)
                ((WaWindow *) wo)->deleted = true;
            delete ((WaWindow *) wo);
        }
        else if (wo->type == DockAppType) {
            if (e->type == DestroyNotify)
                ((Dockapp *) wo)->deleted = true;
            dh = ((Dockapp *) wo)->dh;
            delete ((Dockapp *) wo);
            dh->Update();
        }
    }
}

/**
 * @fn    EvClientMessage(XEvent *e, EventDetail *ed)
 * @brief ClientMessageEvent handler
 *
 * This function handles all client message events received.
 *
 * @param e	The XEvent
 * @param ed Structure containing event details
 */
void EventHandler::EvClientMessage(XEvent *e, EventDetail *ed) {
    Window w;
    int i, rx, ry;
    WaWindow *ww;
    
    if (e->xclient.message_type == waimea->net->net_active_window) {
        if ((ww = (WaWindow *) waimea->FindWin(e->xclient.window,
                                               WindowType))) {
            ww->Focus(true);
            ww->Raise(NULL, NULL);
        }
    }
    else if (e->xclient.message_type == waimea->net->net_wm_name) {
        if ((ww = (WaWindow *) waimea->FindWin(e->xclient.window,
                                               WindowType))) {
            waimea->net->GetNetName(ww);
            if (ww->wascreen->config.db) {
                ww->title->Render();
                ww->label->Render();
            } else
                ww->label->Draw();
        }
    }
    else if (e->xclient.message_type == waimea->net->net_state) {
        if ((ww = (WaWindow *) waimea->FindWin(e->xclient.window,
                                               WindowType))) {
            if ((unsigned int) e->xclient.data.l[0] == 0xffffffff) {
                ww->desktop_mask = (1L << 16) - 1;
                ww->Show();
                ww->net->SetDesktop(ww);
                ww->net->SetDesktopMask(ww);
            }
            else if ((unsigned int) e->xclient.data.l[0] <
                ww->wascreen->config.desktops) {
                ww->desktop_mask |=
                    (1L << (unsigned int) e->xclient.data.l[0]);
                if (ww->desktop_mask &
                    (1L << ww->wascreen->current_desktop->number))
                    ww->Show();
                ww->net->SetDesktop(ww);
                ww->net->SetDesktopMask(ww);
            }
        }
    }
    else if (e->xclient.message_type == waimea->net->net_state) {
        if ((ww = (WaWindow *) waimea->FindWin(e->xclient.window,
                                               WindowType))) {
            bool max_done = false;
            for (int i = 1; i < 3; i++) {
                if ((unsigned long) e->xclient.data.l[i] ==
                    waimea->net->net_state_sticky) {
                    switch (e->xclient.data.l[0]) {
                        case _NET_WM_STATE_REMOVE:
                            ww->UnSticky(NULL, NULL); break;
                        case _NET_WM_STATE_ADD:
                            ww->Sticky(NULL, NULL); break;
                        case _NET_WM_STATE_TOGGLE:
                            ww->ToggleSticky(NULL, NULL); break;
                    }
                } else if ((unsigned long) e->xclient.data.l[i] ==
                           waimea->net->net_state_shaded) {
                    switch (e->xclient.data.l[0]) {
                        case _NET_WM_STATE_REMOVE:
                            ww->UnShade(NULL, NULL); break;
                        case _NET_WM_STATE_ADD:
                            ww->Shade(NULL, NULL); break;
                        case _NET_WM_STATE_TOGGLE:
                            ww->ToggleShade(NULL, NULL); break;
                    }
                } else if ((unsigned long) e->xclient.data.l[i] ==
                           waimea->net->net_maximized_vert ||
                           (unsigned long) e->xclient.data.l[i] ==
                           waimea->net->net_maximized_horz) {
                    if (max_done) break;
                    max_done = true;
                    switch (e->xclient.data.l[0]) {
                        case _NET_WM_STATE_REMOVE:
                            ww->UnMaximize(NULL, NULL); break;
                        case _NET_WM_STATE_ADD:
                            ww->Maximize(NULL, NULL); break;
                        case _NET_WM_STATE_TOGGLE:
                            ww->ToggleMaximize(NULL, NULL); break;
                    }
                } else if ((unsigned long) e->xclient.data.l[i] ==
                           waimea->net->net_state_aot) {
                    switch (e->xclient.data.l[0]) {
                        case _NET_WM_STATE_REMOVE:
                            ww->AlwaysontopOff(NULL, NULL); break;
                        case _NET_WM_STATE_ADD:
                            ww->AlwaysontopOn(NULL, NULL); break;
                        case _NET_WM_STATE_TOGGLE:
                            ww->AlwaysontopToggle(NULL, NULL); break;
                    }
                } else if ((unsigned long) e->xclient.data.l[i] ==
                           waimea->net->net_state_aab) {
                    switch (e->xclient.data.l[0]) {
                        case _NET_WM_STATE_REMOVE:
                            ww->AlwaysatbottomOff(NULL, NULL); break;
                        case _NET_WM_STATE_ADD:
                            ww->AlwaysatbottomOn(NULL, NULL); break;
                        case _NET_WM_STATE_TOGGLE:
                            ww->AlwaysatbottomToggle(NULL, NULL); break;
                    }
                } else if ((unsigned long) e->xclient.data.l[i] ==
                           waimea->net->net_state_decor) {
                    switch (e->xclient.data.l[0]) {
                        case _NET_WM_STATE_REMOVE:
                            ww->DecorAllOff(NULL, NULL); break;
                        case _NET_WM_STATE_ADD:
                            ww->DecorAllOn(NULL, NULL); break;
                        case _NET_WM_STATE_TOGGLE:
                            if (ww->flags.all) ww->DecorAllOff(NULL, NULL);
                            else ww->DecorAllOn(NULL, NULL);
                            break;
                    } 
                } else if ((unsigned long) e->xclient.data.l[i] ==
                           waimea->net->net_state_decortitle) {
                    switch (e->xclient.data.l[0]) {
                        case _NET_WM_STATE_REMOVE:
                            ww->DecorTitleOff(NULL, NULL); break;
                        case _NET_WM_STATE_ADD:
                            ww->DecorTitleOn(NULL, NULL); break;
                        case _NET_WM_STATE_TOGGLE:
                            ww->DecorTitleToggle(NULL, NULL); break;
                    }
                } else if ((unsigned long) e->xclient.data.l[i] ==
                           waimea->net->net_state_decorhandle) {
                    switch (e->xclient.data.l[0]) {
                        case _NET_WM_STATE_REMOVE:
                            ww->DecorHandleOff(NULL, NULL); break;
                        case _NET_WM_STATE_ADD:
                            ww->DecorHandleOn(NULL, NULL); break;
                        case _NET_WM_STATE_TOGGLE:
                            ww->DecorHandleToggle(NULL, NULL); break;
                    }
                } else if ((unsigned long) e->xclient.data.l[i] ==
                           waimea->net->net_state_decorborder) {
                    switch (e->xclient.data.l[0]) {
                        case _NET_WM_STATE_REMOVE:
                            ww->DecorBorderOff(NULL, NULL); break;
                        case _NET_WM_STATE_ADD:
                            ww->DecorBorderOn(NULL, NULL); break;
                        case _NET_WM_STATE_TOGGLE:
                            ww->DecorBorderToggle(NULL, NULL); break;
                    }
                }
            }
        }
    }
    else if (e->xclient.message_type == waimea->net->xa_xdndenter ||
        e->xclient.message_type == waimea->net->xa_xdndleave) {
        if (e->xclient.message_type == waimea->net->xa_xdndenter) {
            e->type = EnterNotify;
            ed->type = EnterNotify;
        } else {
            e->type = LeaveNotify;
            ed->type = LeaveNotify;
        }

        if (WaScreen *ws = (WaScreen *) waimea->FindWin(e->xclient.window,
                                                        RootType)) {
            XQueryPointer(ws->display, ws->id, &w, &w, &rx, &ry, &i, &i,
                          &(ed->mod));
        } else {
            rx = 0;
            ry = 0;
        }
        ed->detail = 0;
        e->xcrossing.x_root = rx;
        e->xcrossing.y_root = ry;
        
        EvAct(e, e->xclient.window, ed);
    }
    else if (e->xclient.message_type == waimea->net->net_desktop_viewport) {
        if (WaScreen *ws = (WaScreen *) waimea->FindWin(e->xclient.window,
                                                        RootType))
            ws->MoveViewportTo(e->xclient.data.l[0], e->xclient.data.l[1]);
    }
    else if (e->xclient.message_type == waimea->net->net_close_window) {
        if ((ww = (WaWindow *) waimea->FindWin(e->xclient.window, WindowType)))
            ww->Close(NULL, NULL);
    }
    else if (e->xclient.message_type == waimea->net->net_current_desktop) {
        if (WaScreen *ws = (WaScreen *) waimea->FindWin(e->xclient.window,
                                                        RootType))
            ws->GoToDesktop(e->xclient.data.l[0]);
    }
    else if (e->xclient.message_type == waimea->net->net_restart) {
        restart(NULL);
    }
    else if (e->xclient.message_type == waimea->net->net_shutdown) {
        quit(EXIT_SUCCESS);
    }
}

/**
 * @fn    EvAct(XEvent *e, Window win)
 * @brief Event action handler
 *
 * The eventloop will call this function whenever a event that could
 * be controlled by the action lists occur. This function finds the
 * WindowObject for the window, gets the action list for that type of
 * WindowObject and calls the WindowObjects EvAct function.
 *
 * @param e	The Event
 * @param win The window we should use in the WindowObject search
 * @param ed Structure containing event details
 */
void EventHandler::EvAct(XEvent *e, Window win, EventDetail *ed) {
    WindowObject *wo;
    WaWindow *wa;

    map<Window, WindowObject *>::iterator it;
    if ((it = waimea->window_table.find(win)) !=
        waimea->window_table.end()) {
        wo = (*it).second;

        waimea->timer->ValidateInterrupts(e);
        
        switch (wo->type) {
            case WindowType:
                wa = (WaWindow *) wo;
                wa->EvAct(e, ed, wo->actionlist, wo->type);
                break;
            case FrameType:
            case TitleType:
            case LabelType:
            case HandleType:
            case LGripType:
            case RGripType:
                wa = ((WaChildWindow *) wo)->wa;
                wa->EvAct(e, ed, wo->actionlist, wo->type);
                break;
            case ButtonType: {
                wa = ((WaChildWindow *) wo)->wa;
                wa->EvAct(e, ed, wo->actionlist, wo->type);
                if (ed->type == ButtonPress)
                    wa->ButtonPressed((WaChildWindow *) wo);
            } break;
            case MenuTitleType:
            case MenuItemType:
            case MenuCBItemType:
            case MenuSubType:
                ((WaMenuItem *) wo)->EvAct(e, ed, wo->actionlist);
                break;
            case WEdgeType:
            case EEdgeType:
            case NEdgeType:
            case SEdgeType:
                (((ScreenEdge *) wo)->wa)->EvAct(e, ed, wo->actionlist);
                break;
            case RootType:
                ((WaScreen *) wo)->EvAct(e, ed, wo->actionlist);
                break;
        }    
    }
}

/**
 * @fn    eventmatch(WaAction *act, EventDetail *ed)
 * @brief Event to action matcher
 *
 * Checks if action type, detail and modifiers are correct.
 *
 * @param act Action to use for matching
 * @param ed Structure containing event details
 *
 * @return True if match, otherwise false
 */
Bool eventmatch(WaAction *act, EventDetail *ed) {
    int i;
    
    if (ed->type != act->type) return false;
    if ((act->detail && ed->detail) ? (act->detail == ed->detail): true) {
        for (i = 0; i <= 12; i++)
            if (act->mod & (1 << i))
                if (! (ed->mod & (1 << i)))
                    return false;
        if (act->mod & MoveResizeMask)
            if (! (ed->mod & MoveResizeMask))
                return false;
        for (i = 0; i <= 12; i++)
            if (act->nmod & (1 << i))
                if (ed->mod & (1 << i))
                    return false;
        if (act->nmod & MoveResizeMask)
            if (ed->mod & MoveResizeMask)
                return false;
        return true;
    }
    return false;
}
