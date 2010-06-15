/*
  QtCurve KWin window decoration
  Copyright (C) 2007 - 2010 Craig Drummond <craig.p.drummond@googlemail.com>
*/

//////////////////////////////////////////////////////////////////////////////
// Taken from: oxygenshadowcache.cpp
// handles caching of TileSet objects to draw shadows
// -------------------
//
// Copyright (c) 2009 Hugo Pereira Da Costa <hugo.pereira@free.fr>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//////////////////////////////////////////////////////////////////////////////

#include <kdeversion.h>
#if KDE_IS_VERSION(4, 3, 0)

#include "qtcurveshadowcache.h"
#include "qtcurveclient.h"
#include "qtcurvehandler.h"
#include "qtcurve.h"

#include <cassert>
#include <KColorUtils>
#include <KColorScheme>
#include <QtGui/QPainter>

namespace KWinQtCurve
{

static bool lowThreshold(const QColor &color)
{
    QColor darker = KColorScheme::shade(color, KColorScheme::MidShade, 0.5);
    return KColorUtils::luma(darker) > KColorUtils::luma(color);
}

static QColor backgroundTopColor(const QColor &color)
{

    if(lowThreshold(color)) return KColorScheme::shade(color, KColorScheme::MidlightShade, 0.0);
    qreal my = KColorUtils::luma(KColorScheme::shade(color, KColorScheme::LightShade, 0.0));
    qreal by = KColorUtils::luma(color);
    return KColorUtils::shade(color, (my - by) * 0.9/*_bgcontrast*/);

}

static QColor backgroundBottomColor(const QColor &color)
{
    QColor midColor = KColorScheme::shade(color, KColorScheme::MidShade, 0.0);
    if(lowThreshold(color)) return midColor;

    qreal by = KColorUtils::luma(color);
    qreal my = KColorUtils::luma(midColor);
    return KColorUtils::shade(color, (my - by) * 0.9/*_bgcontrast*/ * 0.85);
}

static QColor calcLightColor(const QColor &color)
{
    return KColorScheme::shade(color, KColorScheme::LightShade, 0.7/*_contrast*/);
}

QtCurveShadowCache::QtCurveShadowCache()
                  : activeShadowConfiguration_(QtCurveShadowConfiguration(QPalette::Active))
                  , inactiveShadowConfiguration_(QtCurveShadowConfiguration(QPalette::Inactive))
{
    shadowCache_.setMaxCost(1<<6);
}

bool QtCurveShadowCache::shadowConfigurationChanged(const QtCurveShadowConfiguration &other) const
{
    const QtCurveShadowConfiguration &local = (other.colorGroup() == QPalette::Active)
                ? activeShadowConfiguration_:inactiveShadowConfiguration_;
    return !(local == other);
}

void QtCurveShadowCache::setShadowConfiguration(const QtCurveShadowConfiguration &other)
{
    QtCurveShadowConfiguration &local = (other.colorGroup() == QPalette::Active)
            ? activeShadowConfiguration_:inactiveShadowConfiguration_;
    local = other;

    reset();
}

TileSet * QtCurveShadowCache::tileSet(const QtCurveClient *client, bool roundAllCorners)
{
    Key key(client);
    int hash(key.hash());

    if(shadowCache_.contains(hash))
        return shadowCache_.object(hash);

    qreal   size(shadowSize());
    TileSet *tileSet = new TileSet(shadowPixmap(client, key.active, roundAllCorners), size, size, 1, 1);

    shadowCache_.insert(hash, tileSet);
    return tileSet;
}

QPixmap QtCurveShadowCache::shadowPixmap(const QtCurveClient *client, bool active, bool roundAllCorners) const
{
    Key      key(client);
    QPalette palette(client->widget()->palette());
    QColor   color(palette.color(client->widget()->backgroundRole()));

    return simpleShadowPixmap(color, active, roundAllCorners);
}

QPixmap QtCurveShadowCache::simpleShadowPixmap(const QColor &color, bool active, bool roundAllCorners) const
{
    static const qreal fixedSize = 25.5;

    const QtCurveShadowConfiguration &shadowConfiguration(active ? activeShadowConfiguration_ : inactiveShadowConfiguration_);

    // offsets are scaled with the shadow size
    // so that the ratio Top-shadow/Bottom-shadow is kept constant when shadow size is changed
    qreal   size(shadowSize()),
            shadowSize(shadowConfiguration.shadowSize()),
            hoffset((((qreal)shadowConfiguration.horizontalOffset())/100.0)*shadowSize/fixedSize),
            voffset((((qreal)shadowConfiguration.verticalOffset())/100.0)*shadowSize/fixedSize);
    QPixmap shadow(size*2, size*2);

    shadow.fill(Qt::transparent);

    QPainter p(&shadow);

    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);

    if(shadowSize)
    {

//       if(active)
//       {
        {
            // inner (shark) gradient
#ifdef NEW_SHADOWS
            const qreal gradientSize = qMin( shadowSize,(shadowSize+fixedSize)/2 );
            const qreal hoffset = shadowConfiguration.horizontalOffset()*gradientSize/fixedSize;
            const qreal voffset = shadowConfiguration.verticalOffset()*gradientSize/fixedSize;

            QRadialGradient rg = QRadialGradient( size+12.0*hoffset, size+12.0*voffset, gradientSize );
            rg.setColorAt(1, Qt::transparent );

            // gaussian shadow is used
            int nPoints( (10*gradientSize)/fixedSize );
            Gaussian f( 0.85, 0.17 );
            QColor c = shadowConfiguration.innerColor();
            for( int i = 0; i < nPoints; i++ )
            {
                qreal x = qreal(i)/nPoints;
                c.setAlphaF( f(x) );
                rg.setColorAt( x, c );

            }
#else
            int             nPoints = 7;
            qreal           x[7] = {0, 0.05, 0.1, 0.15, 0.2, 0.3, 0.4 };
            int             values[7] = {203, 200, 175, 105, 45, 2, 0 };
            QRadialGradient rg = QRadialGradient(size+12.0*hoffset, size+12.0*voffset, shadowSize);
            QColor          c = shadowConfiguration.innerColor();

            for(int i = 0; i<nPoints; i++)
            {
                c.setAlpha(values[i]);
                rg.setColorAt(x[i], c);
            }
#endif

            p.setBrush(rg);
            if(roundAllCorners)
                p.drawRect(shadow.rect());
            else
                renderGradient(p, shadow.rect(), rg);
        }

        {
            // outer (spread) gradient
#ifdef NEW_SHADOWS
            const qreal gradientSize = shadowSize;
            const qreal hoffset = shadowConfiguration.horizontalOffset()*gradientSize/fixedSize;
            const qreal voffset = shadowConfiguration.verticalOffset()*gradientSize/fixedSize;

            QRadialGradient rg = QRadialGradient( size+12.0*hoffset, size+12.0*voffset, gradientSize );
            rg.setColorAt(1, Qt::transparent );

            // gaussian shadow is used
            int nPoints( (10*gradientSize)/fixedSize );
            Gaussian f( 0.46, 0.34 );
            QColor c = shadowConfiguration.outerColor();
            for( int i = 0; i < nPoints; i++ )
            {
                qreal x = qreal(i)/nPoints;
                c.setAlphaF( f(x) );
                rg.setColorAt( x, c );

            }
#else
            int             nPoints = 7;
            qreal           x[7] = {0, 0.15, 0.3, 0.45, 0.65, 0.75, 1 };
            int             values[7] = {120, 95, 50, 20, 10, 5, 0 };
            QRadialGradient rg = QRadialGradient(size+12.0*hoffset, size+12.0*voffset, shadowSize);
            QColor          c = shadowConfiguration.outerColor();

            for(int i = 0; i<nPoints; i++)
            {
                c.setAlpha(values[i]);
                rg.setColorAt(x[i], c);
            }
#endif
            p.setBrush(rg);
            p.drawRect(shadow.rect());
        }

//       } else {
// 
//         {
//           // inner (sharp gradient)
//           int nPoints = 5;
//           qreal values[5] = { 1, 0.32, 0.22, 0.03, 0 };
//           qreal x[5] = { 0, 4.5, 5.0, 5.5, 6.5 };
//           QRadialGradient rg = QRadialGradient(size+hoffset, size+voffset, shadowSize);
//           QColor c = shadowConfiguration.innerColor();
//           for(int i = 0; i<nPoints; i++)
//           { c.setAlphaF(values[i]); rg.setColorAt(x[i]/fixedSize, c); }
// 
//           renderGradient(p, shadow.rect(), rg/*, hasBorder*/);
// 
//         }
// 
//         {
// 
//           // mid gradient
//           int nPoints = 7;
//           qreal values[7] = {0.55, 0.25, 0.20, 0.1, 0.06, 0.015, 0 };
//           qreal x[7] = {0, 4.5, 5.5, 7.5, 8.5, 11.5, 14.5 };
//           QRadialGradient rg = QRadialGradient(size+10.0*hoffset, size+10.0*voffset, shadowSize);
//           QColor c = shadowConfiguration.midColor();
//           for(int i = 0; i<nPoints; i++)
//           { c.setAlphaF(values[i]); rg.setColorAt(x[i]/fixedSize, c); }
// 
//           p.setBrush(rg);
//           p.drawRect(shadow.rect());
// 
//         }
// 
//         {
// 
//           // outer (spread) gradient
//           int nPoints = 9;
//           qreal values[9] = { 0.17, 0.12, 0.11, 0.075, 0.06, 0.035, 0.025, 0.01, 0 };
//           qreal x[9] = {0, 4.5, 6.6, 8.5, 11.5, 14.5, 17.5, 21.5, 25.5 };
//           QRadialGradient rg = QRadialGradient(size+20.0*hoffset, size+20.0*voffset, shadowSize);
//           QColor c = shadowConfiguration.outerColor();
//           for(int i = 0; i<nPoints; i++)
//           { c.setAlphaF(values[i]); rg.setColorAt(x[i]/fixedSize, c); }
// 
//           p.setBrush(rg);
//           p.drawRect(shadow.rect());
// 
//         }
// 
//       }
    }

    // draw the corner of the window - actually all 4 corners as one circle
    // this is all fixedSize. Does not scale with shadow size
    QLinearGradient lg = QLinearGradient(0.0, size-4.5, 0.0, size+4.5);
    lg.setColorAt(0.0, calcLightColor(backgroundTopColor(color)));
    lg.setColorAt(0.51, backgroundBottomColor(color));
    lg.setColorAt(1.0, backgroundBottomColor(color));

    p.setBrush(lg);
    p.drawEllipse(QRectF(size-4, size-4, 8, 8));
    p.end();
    return shadow;
}

void QtCurveShadowCache::renderGradient(QPainter &p, const QRectF &rect, const QRadialGradient &rg) const
{
    qreal          size(rect.width()/2.0),
                   hoffset(rg.center().x() - size),
                   voffset(rg.center().y() - size),
                   radius(rg.radius());
    QGradientStops stops(rg.stops());

    // draw ellipse for the upper rect
    {
        QRectF rect(hoffset, voffset, 2*size-hoffset, size);
        p.setBrush(rg);
        p.drawRect(rect);
    }

    // draw square gradients for the lower rect
    {
        // vertical lines
        QRectF          rect(hoffset, size+voffset, 2*size-hoffset, 4);
        QLinearGradient lg(hoffset, 0.0, 2*size+hoffset, 0.0);
        
        for(int i = 0; i<stops.size(); i++)
        {
            QColor c(stops[i].second);
            qreal  xx(stops[i].first*radius);

            lg.setColorAt((size-xx)/(2.0*size), c);
            lg.setColorAt((size+xx)/(2.0*size), c);
        }

        p.setBrush(lg);
        p.drawRect(rect);
    }

    {
        // horizontal line
        QRectF          rect(size-4+hoffset, size+voffset, 8, size);
        QLinearGradient lg = QLinearGradient(0, voffset, 0, 2*size+voffset);

        for(int i = 0; i<stops.size(); i++)
        {
            QColor c(stops[i].second);
            qreal  xx(stops[i].first*radius);

            lg.setColorAt((size+xx)/(2.0*size), c);
        }

        p.setBrush(lg);
        p.drawRect(rect);
    }

    {
      // bottom-left corner
        QRectF          rect(hoffset, size+4+voffset, size-4, size);
        QRadialGradient rg = QRadialGradient(size+hoffset-4, size+4+voffset, radius);

        for(int i = 0; i<stops.size(); i++)
        {
            QColor c(stops[i].second);
            qreal  xx(stops[i].first -4.0/rg.radius());

            if(xx<0 && i < stops.size()-1)
            {
                qreal x1(stops[i+1].first -4.0/rg.radius());
                c = KColorUtils::mix(c, stops[i+1].second, -xx/(x1-xx));
                xx = 0;
            }

            rg.setColorAt(xx, c);
        }

        p.setBrush(rg);
        p.drawRect(rect);
    }

    {
      // bottom-right corner
        QRectF          rect(size+4+hoffset, size+4+voffset, size-4, size);
        QRadialGradient rg = QRadialGradient(size+hoffset+4, size+4+voffset, radius);

        for(int i = 0; i<stops.size(); i++)
        {
            QColor c(stops[i].second);
            qreal  xx(stops[i].first -4.0/rg.radius());

            if(xx<0 && i < stops.size()-1)
            {
                qreal x1(stops[i+1].first -4.0/rg.radius());
                c = KColorUtils::mix(c, stops[i+1].second, -xx/(x1-xx));
                xx = 0;
            }

            rg.setColorAt(xx, c);
        }

        p.setBrush(rg);
        p.drawRect(rect);
    }
}

QtCurveShadowCache::Key::Key(const QtCurveClient *client)
                       : active(client->isActive())
                       , isShade(client->isShade())
{
}

}

#endif