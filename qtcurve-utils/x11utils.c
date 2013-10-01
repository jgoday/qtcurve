/***************************************************************************
 *   Copyright (C) 2013~2013 by Yichao Yu                                  *
 *   yyc1992@gmail.com                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#include "x11utils.h"
#include "kdex11shadow_p.h"

#include <xcb/xcb_image.h>
#include <X11/Xlib-xcb.h>

static xcb_connection_t *qtc_xcb_conn = NULL;
static int qtc_default_screen_no = 0;
static xcb_window_t qtc_root_window = {0};
static xcb_screen_t *qtc_default_screen = NULL;
QTC_EXPORT xcb_atom_t qtc_x11_atoms[_QTC_X11_ATOM_NUMBER];

static char wm_cm_s_atom_name[100] = "_NET_WM_CM_S";

static const char *const qtc_x11_atom_names[_QTC_X11_ATOM_NUMBER] = {
    "WM_CLASS",
    "_NET_WM_MOVERESIZE",
    wm_cm_s_atom_name,

    "_KDE_NET_WM_SKIP_SHADOW",
    "_KDE_NET_WM_FORCE_SHADOW",
    "_KDE_NET_WM_SHADOW",
    "_KDE_NET_WM_BLUR_BEHIND_REGION",

    "_QTCURVE_MENUBAR_SIZE_",
    "_QTCURVE_STATUSBAR_",
    "_QTCURVE_TITLEBAR_SIZE_",
    "_QTCURVE_ACTIVE_WINDOW_",
    "_QTCURVE_TOGGLE_MENUBAR_",
    "_QTCURVE_TOGGLE_STATUSBAR_",
    "_QTCURVE_OPACITY_",
    "_QTCURVE_BGND_",
};

QTC_EXPORT xcb_window_t
qtc_x11_root_window()
{
    return qtc_root_window;
}

QTC_EXPORT int
qtc_x11_default_screen_no()
{
    return qtc_default_screen_no;
}

QTC_EXPORT xcb_screen_t*
qtc_x11_default_screen()
{
    return qtc_default_screen;
}

static xcb_screen_t*
screen_of_display(xcb_connection_t *c, int screen)
{
    xcb_screen_iterator_t iter;

    iter = xcb_setup_roots_iterator(xcb_get_setup(c));
    for (;iter.rem;--screen, xcb_screen_next(&iter)) {
        if (screen == 0) {
            return iter.data;
        }
    }

    return NULL;
}

QTC_EXPORT xcb_screen_t*
qtc_x11_get_screen(int screen_no)
{
    if (screen_no == -1 || screen_no == qtc_default_screen_no)
        return qtc_default_screen;
    return screen_of_display(qtc_xcb_conn, screen_no);
}

QTC_EXPORT void
qtc_x11_init_xcb(xcb_connection_t *conn, int screen_no)
{
    if (qtc_unlikely(qtc_xcb_conn) || !conn)
        return;
    if (screen_no < 0)
        screen_no = 0;
    qtc_xcb_conn = conn;
    qtc_default_screen_no = screen_no;
    qtc_default_screen = screen_of_display(conn, screen_no);
    if (qtc_default_screen) {
        qtc_root_window = qtc_default_screen->root;
    }
    const size_t base_len = strlen("_NET_WM_CM_S");
    sprintf(wm_cm_s_atom_name + base_len, "%d", screen_no);
    qtc_x11_get_atoms(_QTC_X11_ATOM_NUMBER, qtc_x11_atoms,
                      qtc_x11_atom_names, true);
    qtc_kde_x11_shadow_init();
}

QTC_EXPORT void
qtc_x11_init_xlib(Display *disp)
{
    if (qtc_unlikely(qtc_xcb_conn) || !disp)
        return;
    qtc_x11_init_xcb(XGetXCBConnection(disp), DefaultScreen(disp));
}

QTC_EXPORT xcb_connection_t*
qtc_x11_get_conn()
{
    return qtc_xcb_conn;
}

QTC_EXPORT void
qtc_x11_flush()
{
    xcb_flush(qtc_xcb_conn);
}

QTC_EXPORT uint32_t
qtc_x11_generate_id()
{
    return xcb_generate_id(qtc_xcb_conn);
}

QTC_EXPORT void
qtc_x11_get_atoms(size_t n, xcb_atom_t *atoms,
                  const char *const names[], boolean create)
{
    xcb_connection_t *conn = qtc_xcb_conn;
    xcb_intern_atom_cookie_t cookies[n];
    for (size_t i = 0;i < n;i++) {
        cookies[i] = xcb_intern_atom(conn, !create,
                                     strlen(names[i]), names[i]);
    }
    memset(atoms, 0, sizeof(xcb_atom_t) * n);
    for (size_t i = 0;i < n;i++) {
        xcb_intern_atom_reply_t *r =
            xcb_intern_atom_reply(conn, cookies[i], 0);
        if (r) {
            atoms[i] = r->atom;
            free(r);
        }
    }
}

QTC_EXPORT void
qtc_x11_set_wmclass(xcb_window_t win, const char *wmclass, size_t len)
{
    qtc_x11_call_void(change_property, XCB_PROP_MODE_REPLACE, win,
                      qtc_x11_atoms[QTC_X11_ATOM_WM_CLASS], XCB_ATOM_STRING,
                      8, len, wmclass);
}