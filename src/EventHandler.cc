/** -*- Mode: C++ -*-
 *
 * @file   EventHandler.cc
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

#include "EventHandler.hh"

#include <stdio.h>
#include <X11/Xatom.h>

/**
 * @fn    EventHandler(Waimea *wa)
 * @brief Constructor for EventHandler class
 *
 * Sets waimea and rh pointers. Creates menu move return mask, window
 * move/resize return mask and empty return mask hash_sets.
 */
EventHandler::EventHandler(Waimea *wa) {
    waimea = wa;
    rh = waimea->rh;
    focused = last_click_win = (Window) 0;
    move_resize = EndMoveResizeType;

    empty_return_mask = new hash_set<int>;
    
    moveresize_return_mask = new hash_set<int>;
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

    menu_viewport_move_return_mask = new hash_set<int>;
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
    HASHDEL(empty_return_mask);
    HASHDEL(moveresize_return_mask);
    HASHDEL(menu_viewport_move_return_mask);
}

/**
 * @fn    EventLoop(hash_set<int> *return_mask, XEvent *event)
 * @brief Eventloop
 *
 * Infinite loop waiting for an event to occur. Executes a matching function
 * for an event then it occurs. If what to do when an event occurs is
 * controlled by a action list we set etype, edetail and emod variables and
 * jump into EvAct() function. This function can be called from move and resize
 * functions the return_mask hash_set is then used for deciding if an event
 * should be processed as normal or returned to the function caller.
 *
 * @param return_mask hash_set to use as return_mask
 * @param event Pointer to allocated event structure
 */
void EventHandler::EventLoop(hash_set<int> *return_mask, XEvent *event) {    
    for (;;) {
        XNextEvent(waimea->display, event);
        
        if (return_mask->find(event->type) != return_mask->end()) return;

        HandleEvent(event);
    }
}

void EventHandler::HandleEvent(XEvent *event) {
    Window w;
    int i, rx, ry;
    struct timeb click_time;
    
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
        case DestroyNotify:
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
            if (last_click_win == event->xbutton.window) {
                ftime(&click_time);
                if (click_time.time <= last_click.time + 1) {
                    if (click_time.time == last_click.time &&
                        (unsigned int)
                        (click_time.millitm - last_click.millitm) <
                        waimea->rh->double_click) {
                        ed->type = DoubleClick;
                        last_click_win = (Window) 0;
                    }
                    else if ((1000 - last_click.millitm) +
                             (unsigned int) click_time.millitm <
                             waimea->rh->double_click) {
                        ed->type = DoubleClick;
                            last_click_win = (Window) 0;
                    }
                    else {
                        last_click_win = event->xbutton.window;
                        last_click.time = click_time.time;
                        last_click.millitm = click_time.millitm;
                    }
                }
                else {
                    last_click_win = event->xbutton.window;
                    last_click.time = click_time.time;
                    last_click.millitm = click_time.millitm;
                }
            }
            else {
                last_click_win = event->xbutton.window;
                ftime(&last_click);
            }
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
            XQueryPointer(waimea->wascreen->display,
                          waimea->wascreen->id, &w, &w, &rx, &ry, &i, &i,
                          &(ed->mod));
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
            if (event->type == waimea->wascreen->shape_event) {
                hash_map<Window, WindowObject *>::iterator it;
                if ((it = waimea->window_table->find(event->xany.window)) !=
                    waimea->window_table->end()) {
                    if (((*it).second)->type == WindowType)
                        if (((WaWindow *) (*it).second)->wascreen->shape)
                            ((WaWindow *) (*it).second)->Shape();
                }
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
    hash_map<Window, WindowObject *>::iterator it;
    char *tmp_name;

    if (e->state == PropertyDelete) {
        if (e->atom == waimea->wascreen->net->net_wm_strut) {
            list<WMstrut *>::iterator s_it =
                waimea->wascreen->strut_list->begin();
            for (; s_it != waimea->wascreen->strut_list->end(); ++s_it) {
                if ((*s_it)->window == e->window) {
                    waimea->wascreen->strut_list->remove(*s_it);
                    free(*s_it);
                    waimea->wascreen->UpdateWorkarea();
                }
            }
        }
    } else if (e->atom == waimea->wascreen->net->net_wm_strut) {
        if ((it = waimea->window_table->find(e->window)) !=
            waimea->window_table->end()) {
            if (((*it).second)->type == WindowType) {
                waimea->wascreen->net->GetWmStrut((WaWindow *) (*it).second);
            }
        }
    } else if (e->atom == XA_WM_NAME) {
        if ((it = waimea->window_table->find(e->window)) !=
            waimea->window_table->end()) {
            if (((*it).second)->type == WindowType) {
                ww = (WaWindow *) (*it).second;
                XGrabServer(e->display);
                if (validateclient(ww->id)) {
                    if (XFetchName(ww->display, ww->id, &tmp_name)) {
                        delete [] ww->name;
                        ww->name = wastrdup(tmp_name);
                        if (ww->title_w) ww->DrawLabelFg();
                        XFree(tmp_name);
                    }
                }
                XUngrabServer(e->display);
            }
        }
    }
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
    hash_map<Window, WindowObject *>::iterator it;
    if ((it = waimea->window_table->find(e->window)) !=
        waimea->window_table->end()) {
        switch (((*it).second)->type) {
            case LabelType:
                (((WaChildWindow *) (*it).second)->wa)->DrawLabelFg(); break;
            case CButtonType:
                (((WaChildWindow *) (*it).second)->wa)->DrawCloseButtonFg();
                break;
            case IButtonType:
                (((WaChildWindow *) (*it).second)->wa)->DrawIconifyButtonFg();
                break;
            case MButtonType:
                (((WaChildWindow *) (*it).second)->wa)->DrawMaxButtonFg();
                break;
            case MenuTitleType:
            case MenuItemType:
            case MenuSubType:
            case MenuCBItemType:
                ((WaMenuItem *) (*it).second)->DrawFg();
        }
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
    WaWindow *ww = NULL;

    hash_map<Window, WindowObject *>::iterator it;
    if (e->type == FocusIn && e->window != focused) {
        if ((it = waimea->window_table->find(e->window)) !=
            waimea->window_table->end()) {
            if (((*it).second)->type == WindowType)
                ww = (WaWindow *) ((*it).second);
            if ((it = waimea->window_table->find(focused)) !=
                waimea->window_table->end())
                if (((*it).second)->type == WindowType) {
                    ((WaWindow *) (*it).second)->UnFocusWin();
                    ((WaWindow *) (*it).second)->UpdateGrabs();
                }
            if (ww) {
                ww->FocusWin();
                ww->UpdateGrabs();
                ww->net->SetActiveWindow(ww->wascreen, ww);
            }
            focused = e->window;
        }
        if (e->window == waimea->wascreen->id)
            waimea->net->SetActiveWindow(waimea->wascreen, NULL);
    }
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
    
    hash_map<Window, WindowObject *>::iterator it;
    if ((it = waimea->window_table->find(e->window)) !=
        waimea->window_table->end()) {
        if (((*it).second)->type == WindowType) {
            ww = (WaWindow *) (*it).second;
            waimea->net->GetWMNormalHints(ww);
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
                waimea->WaRaiseWindow((Window) 0);
                waimea->WaLowerWindow((Window) 0);
            }
            ww->net->SetVirtualPos(ww);
            return;
        }
        else if (((*it).second)->type == DockAppType) {
            da = (Dockapp *) (*it).second;
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
    if (e->value_mask & CWStackMode) waimea->WaRaiseWindow((Window) 0);
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
    
    hash_map<Window, WindowObject *>::iterator it;
    if ((it = waimea->window_table->find(e->window)) !=
        waimea->window_table->end()) {
        if (((*it).second)->type == WindowType) {
            ((WaWindow *) (*it).second)->net->SetState(
                ((WaWindow *) (*it).second), NormalState);
        }
    } 
    else {
        wm_hints = XAllocWMHints();
        XGetWindowAttributes(e->display, e->window, &attr);
        if (! attr.override_redirect) {
            if ((wm_hints = XGetWMHints(e->display, e->window)) &&
                (wm_hints->flags & StateHint) &&
                (wm_hints->initial_state == WithdrawnState)) {
                waimea->wascreen->AddDockapp(e->window);
            }
            else new WaWindow(e->window, waimea->wascreen);
        }
        XFree(wm_hints);
    }
}

/**
 * @fn    EvUnmapDestroy(XEvent *e)
 * @brief UnmapEvent and DestroyEvent handler
 *
 * We receive this event then a window has been unmapped or destroyed->
 * If we can find a WaWindow for this window then the delete that WaWindow.
 * If we couldn't find a WaWindow we check if the windows is a dockapp window
 * and if it is, we update the dockapp handler holding the dockapp.
 *
 * @param e	The UnmapEvent
 */
void EventHandler::EvUnmapDestroy(XEvent *e) {
    DockappHandler *dh;
    
    hash_map<Window, WindowObject *>::iterator it;
    if ((it = waimea->window_table->find((e->type == UnmapNotify)?
                                          e->xunmap.window:
                                          e->xdestroywindow.window))
        != waimea->window_table->end()) {
        if (((*it).second)->type == WindowType) {
            if (e->type == DestroyNotify)
                ((WaWindow *) (*it).second)->deleted = true;
            delete ((WaWindow *) (*it).second);
            if (! waimea->wawindow_list->empty())
                waimea->wawindow_list->front()->Focus(false);
        }
        else if (((*it).second)->type == DockAppType) {
            if (e->type == DestroyNotify)
                ((Dockapp *) (*it).second)->deleted = true;
            dh = ((Dockapp *) (*it).second)->dh;
            delete ((Dockapp *) (*it).second);
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
    
    if (e->xclient.message_type == waimea->net->xa_xdndenter ||
        e->xclient.message_type == waimea->net->xa_xdndleave) {
        if (e->xclient.message_type == waimea->net->xa_xdndenter) {
            e->type = EnterNotify;
            ed->type = EnterNotify;
        } else {
            e->type = LeaveNotify;
            ed->type = LeaveNotify;
        }
        XQueryPointer(waimea->wascreen->display,
                      waimea->wascreen->id, &w, &w, &rx, &ry, &i, &i,
                      &(ed->mod));
        ed->detail = 0;
        e->xcrossing.x_root = rx;
        e->xcrossing.y_root = ry;
        
        EvAct(e, e->xclient.window, ed);
    }
    else if (e->xclient.message_type == waimea->net->net_desktop_viewport) {
        waimea->wascreen->MoveViewportTo(e->xclient.data.l[0],
                                         e->xclient.data.l[1]);
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

    hash_map<Window, WindowObject *>::iterator it;
    if ((it = waimea->window_table->find(win)) !=
        waimea->window_table->end()) {
        wo = (*it).second;
        
        switch (wo->type) {
            
            case FrameType:
                wa = ((WaChildWindow *) wo)->wa;
                if (wa->frameacts) wa->EvAct(e, ed, wa->frameacts, wo->type);
                else wa->EvAct(e, ed, rh->frameacts, wo->type);
                break;
            case WindowType:
                wa = (WaWindow *) wo;
                if (wa->has_focus) {
                    if (wa->awinacts) wa->EvAct(e, ed, wa->awinacts, wo->type);
                    else wa->EvAct(e, ed, rh->awinacts, wo->type);
                }
                else {
                    if (wa->pwinacts) wa->EvAct(e, ed, wa->pwinacts, wo->type);
                    else wa->EvAct(e, ed, rh->pwinacts, wo->type);
                } 
                break;
            case TitleType:
                wa = ((WaChildWindow *) wo)->wa;
                if (wa->titleacts) wa->EvAct(e, ed, wa->titleacts, wo->type);
                else wa->EvAct(e, ed, rh->titleacts, wo->type);
                break;
            case LabelType:
                wa = ((WaChildWindow *) wo)->wa;
                if (wa->labelacts) wa->EvAct(e, ed, wa->labelacts, wo->type);
                else wa->EvAct(e, ed, rh->labelacts, wo->type);
                break;
            case HandleType:
                wa = ((WaChildWindow *) wo)->wa;
                if (wa->handleacts) wa->EvAct(e, ed, wa->handleacts, wo->type);
                else wa->EvAct(e, ed, rh->handleacts, wo->type);
                break;
            case CButtonType:
                wa = ((WaChildWindow *) wo)->wa;
                if (wa->cbacts) wa->EvAct(e, ed, wa->cbacts, wo->type);
                else wa->EvAct(e, ed, rh->cbacts, wo->type);
                break;
            case IButtonType:
                wa = ((WaChildWindow *) wo)->wa;
                if (wa->ibacts) wa->EvAct(e, ed, wa->ibacts, wo->type);
                else wa->EvAct(e, ed, rh->ibacts, wo->type);
                break;
            case MButtonType:
                wa = ((WaChildWindow *) wo)->wa;
                if (wa->mbacts) wa->EvAct(e, ed, wa->mbacts, wo->type);
                else wa->EvAct(e, ed, rh->mbacts, wo->type);
                break;
            case LGripType:
                wa = ((WaChildWindow *) wo)->wa;
                if (wa->lgacts) wa->EvAct(e, ed, wa->lgacts, wo->type);
                else wa->EvAct(e, ed, rh->lgacts, wo->type);
                break;
            case RGripType:
                wa = ((WaChildWindow *) wo)->wa;
                if (wa->rgacts) wa->EvAct(e, ed, wa->rgacts, wo->type);
                else wa->EvAct(e, ed, rh->rgacts, wo->type);
                break;
            case MenuTitleType:
                ((WaMenuItem *) wo)->EvAct(e, ed, rh->mtacts); break;
            case MenuItemType:
                ((WaMenuItem *) wo)->EvAct(e, ed, rh->miacts); break;
            case MenuCBItemType:
                ((WaMenuItem *) wo)->EvAct(e, ed, rh->mcbacts); break;
            case MenuSubType:
                ((WaMenuItem *) wo)->EvAct(e, ed, rh->msacts); break;
            case WEdgeType:
                (((ScreenEdge *) wo)->wa)->EvAct(e, ed, rh->weacts); break;
            case EEdgeType:
                (((ScreenEdge *) wo)->wa)->EvAct(e, ed, rh->eeacts); break;
            case NEdgeType:
                (((ScreenEdge *) wo)->wa)->EvAct(e, ed, rh->neacts); break;
            case SEdgeType:
                (((ScreenEdge *) wo)->wa)->EvAct(e, ed, rh->seacts); break;
            case RootType:
                ((WaScreen *) wo)->EvAct(e, ed, rh->rootacts); break;
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
