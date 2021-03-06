/*****************************************************************************
 *   Copyright 2003 - 2010 Craig Drummond <craig.p.drummond@gmail.com>       *
 *   Copyright 2013 - 2015 Yichao Yu <yyc1992@gmail.com>                     *
 *                                                                           *
 *   This program is free software; you can redistribute it and/or modify    *
 *   it under the terms of the GNU Lesser General Public License as          *
 *   published by the Free Software Foundation; either version 2.1 of the    *
 *   License, or (at your option) version 3, or any later version accepted   *
 *   by the membership of KDE e.V. (or its successor approved by the         *
 *   membership of KDE e.V.), which shall act as a proxy defined in          *
 *   Section 6 of version 3 of the license.                                  *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       *
 *   Lesser General Public License for more details.                         *
 *                                                                           *
 *   You should have received a copy of the GNU Lesser General Public        *
 *   License along with this library. If not,                                *
 *   see <http://www.gnu.org/licenses/>.                                     *
 *****************************************************************************/

#include "widgetmap.h"

#include <qtcurve-utils/gtkprops.h>

#include <unordered_map>

namespace QtCurve {
namespace WidgetMap {

typedef std::unordered_map<GtkWidget*, GObjWeakRef> WidgetMap;
static WidgetMap widget_map[2];

template<typename Id>
static inline bool
getMapHacked(const GtkWidgetProps &props, Id &id)
{
    return props->widgetMapHacked & (id ? (1 << 1) : (1 << 0));
}

template<typename Id>
static inline void
setMapHacked(const GtkWidgetProps &props, Id &id)
{
    props->widgetMapHacked |= id ? (1 << 1) : (1 << 0);
}

static GtkWidget*
lookupHash(GtkWidget *hash, GtkWidget *value, int map)
{
    auto it = widget_map[map].find(hash);
    GtkWidget *rv = (it != widget_map[map].end() ?
                     (*it).second.get<GtkWidget>() : nullptr);
    if (!rv && value) {
        widget_map[map].emplace(std::piecewise_construct,
                                std::forward_as_tuple(hash),
                                std::forward_as_tuple(value));
        rv = value;
    }
    return rv;
}

static void
removeHash(GtkWidget *hash)
{
    for (int i = 0;i < 2;++i) {
        widget_map[i].erase(hash);
    }
}

static void
cleanup(GtkWidget *widget)
{
    GtkWidgetProps props(widget);
    if (props->widgetMapHacked) {
        props->widgetMapDestroy.disconn();
        props->widgetMapUnrealize.disconn();
        props->widgetMapStyleSet.disconn();
        props->widgetMapHacked = 0;
        removeHash(widget);
    }
}

static gboolean
styleSet(GtkWidget *widget, GtkStyle*, void*)
{
    cleanup(widget);
    return false;
}

static gboolean
destroy(GtkWidget *widget, GdkEvent*, void*)
{
    cleanup(widget);
    return false;
}

void
setup(GtkWidget *from, GtkWidget *to, int map)
{
    GtkWidgetProps fromProps(from);
    if (from && to && !getMapHacked(fromProps, map)) {
        if (!fromProps->widgetMapHacked) {
            fromProps->widgetMapDestroy.conn("destroy-event", destroy);
            fromProps->widgetMapUnrealize.conn("unrealize", destroy);
            fromProps->widgetMapStyleSet.conn("style-set", styleSet);
        }
        setMapHacked(fromProps, map);
        lookupHash(from, to, map);
    }
}

GtkWidget*
getWidget(GtkWidget *widget, int map)
{
    GtkWidgetProps props(widget);
    return (widget && getMapHacked(props, map) ?
            lookupHash(widget, nullptr, map) : nullptr);
}

}
}
