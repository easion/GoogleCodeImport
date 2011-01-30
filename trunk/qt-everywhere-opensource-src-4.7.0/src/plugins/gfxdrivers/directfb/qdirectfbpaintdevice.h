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

#ifndef QDIRECTFBPAINTDEVICE_H
#define QDIRECTFBPAINTDEVICE_H

#include <private/qpaintengine_raster_p.h>
#include "qdirectfbscreen.h"

#ifndef QT_NO_QWS_DIRECTFB

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

// Inherited by both window surface and pixmap
class QGixPaintEngine;
class QGixPaintDevice : public QCustomRasterPaintDevice
{
public:
    ~QGixPaintDevice();

    virtual PLATFORM_SURFACE *gixSurface() const;

    bool lockSurface(PLATFORM_LOCK_FLAGS lockFlags);
    void unlockSurface();

    // Reimplemented from QCustomRasterPaintDevice:
    void *memory() const;
    QImage::Format format() const;
    int bytesPerLine() const;
    QSize size() const;
    int metric(QPaintDevice::PaintDeviceMetric metric) const;
    PLATFORM_LOCK_FLAGS lockFlags() const { return lockFlgs; }
    QPaintEngine *paintEngine() const;
protected:
    QGixPaintDevice(QGixScreen *scr);
    inline int dotsPerMeterX() const
    {
        return (screen->deviceWidth() * 1000) / screen->physicalWidth();
    }
    inline int dotsPerMeterY() const
    {
        return (screen->deviceHeight() * 1000) / screen->physicalHeight();
    }

    PLATFORM_SURFACE *dfbSurface;

    QImage lockedImage;
    QGixScreen *screen;
    int bpl;
    PLATFORM_LOCK_FLAGS lockFlgs;
    uchar *mem;
    QGixPaintEngine *engine;
    QImage::Format imageFormat;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QT_NO_QWS_DIRECTFB
#endif //QDIRECTFBPAINTDEVICE_H
