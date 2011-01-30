/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdirectfbscreen.h"
#include "qdirectfbpaintdevice.h"
#include "qdirectfbpaintengine.h"

#ifndef QT_NO_QWS_DIRECTFB

QT_BEGIN_NAMESPACE

QGixPaintDevice::QGixPaintDevice(QGixScreen *scr)
    : QCustomRasterPaintDevice(0), dfbSurface(0), screen(scr),
      bpl(-1), lockFlgs(PLATFORM_LOCK_FLAGS(0)), mem(0), engine(0), imageFormat(QImage::Format_Invalid)
{
}

QGixPaintDevice::~QGixPaintDevice()
{
    if (QGixScreen::instance()) {
        unlockSurface();

        if (dfbSurface) {
            screen->releaseDFBSurface(dfbSurface);
        }
    }
    delete engine;
}

PLATFORM_SURFACE *QGixPaintDevice::gixSurface() const
{
    return dfbSurface;
}

bool QGixPaintDevice::lockSurface(PLATFORM_LOCK_FLAGS lockFlags)
{
    if (lockFlgs && (lockFlags & ~lockFlgs))
        unlockSurface();
    if (!mem) {
        Q_ASSERT(dfbSurface);
        PLATFORM_SURFACE *surface = dfbSurface;
        Q_ASSERT(surface);
        mem = QGixScreen::lockSurface(surface, lockFlags, &bpl);
        lockFlgs = lockFlags;
        Q_ASSERT(mem);
        Q_ASSERT(bpl > 0);
        const QSize s = size();
        lockedImage = QImage(mem, s.width(), s.height(), bpl,
                             QGixScreen::pixelFormatToQt(dfbSurface->PixelFormat));
        return true;
    }

    return false;
}

void QGixPaintDevice::unlockSurface()
{
    if (QGixScreen::instance() && lockFlgs) {

        PLATFORM_SURFACE *surface = dfbSurface;
        if (surface) {
            //surface->Unlock(surface);
            lockFlgs = static_cast<PLATFORM_LOCK_FLAGS>(0);
            mem = 0;
        }
    }
}

void *QGixPaintDevice::memory() const
{
    return mem;
}

QImage::Format QGixPaintDevice::format() const
{
    return imageFormat;
}

int QGixPaintDevice::bytesPerLine() const
{
    Q_ASSERT(!mem || bpl != -1);
    return bpl;
}

QSize QGixPaintDevice::size() const
{
    int w, h;
    //dfbSurface->GetSize(dfbSurface, &w, &h);
	w = dfbSurface->width;
	h = dfbSurface->height;
    return QSize(w, h);
}

int QGixPaintDevice::metric(QPaintDevice::PaintDeviceMetric metric) const
{
    if (!dfbSurface)
        return 0;

    switch (metric) {
    case QPaintDevice::PdmWidth:
    case QPaintDevice::PdmHeight:
        return (metric == PdmWidth ? size().width() : size().height());
    case QPaintDevice::PdmWidthMM:
        return (size().width() * 1000) / dotsPerMeterX();
    case QPaintDevice::PdmHeightMM:
        return (size().height() * 1000) / dotsPerMeterY();
    case QPaintDevice::PdmPhysicalDpiX:
    case QPaintDevice::PdmDpiX:
        return (dotsPerMeterX() * 254) / 10000; // 0.0254 meters-per-inch
    case QPaintDevice::PdmPhysicalDpiY:
    case QPaintDevice::PdmDpiY:
        return (dotsPerMeterY() * 254) / 10000; // 0.0254 meters-per-inch
    case QPaintDevice::PdmDepth:
        return QGixScreen::depth(imageFormat);
    case QPaintDevice::PdmNumColors: {
        if (!lockedImage.isNull())
            return lockedImage.colorCount();
        unsigned int numColors = 0;

		qWarning("query colors\n");
#if 0 //FIXME DPP
        int result;
        IGixPalette *palette = 0;

        result = dfbSurface->GetPalette(dfbSurface, &palette);
        if ((result != DFB_OK) || !palette)
            return 0;

        result = palette->GetSize(palette, &numColors);
        palette->Release(palette);
        if (result != DFB_OK)
            return 0;
#endif
        return numColors;
    }
    default:
        qCritical("QGixPaintDevice::metric(): Unhandled metric!");
        return 0;
    }
}

QPaintEngine *QGixPaintDevice::paintEngine() const
{
    return engine;
}



QT_END_NAMESPACE

#endif // QT_NO_QWS_DIRECTFB
