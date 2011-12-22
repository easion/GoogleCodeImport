/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Copyright (C) 2011 www.hanxuantech.com. The Gix parts. 
** Written by Easion <easion@hanxuantech.com> 
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qregion.h"
#include "qpainterpath.h"
#include "qpolygon.h"
#include "qbuffer.h"
#include "qdatastream.h"
#include "qvariant.h"
#include "qvarlengtharray.h"

#include <gi/gi.h>
#include <gi/property.h>
//#include <gi/region.h>
#include <limits.h>

QT_BEGIN_INCLUDE_NAMESPACE
QT_BEGIN_NAMESPACE

extern "C" gi_region_ptr_t XCreateRegion(void);
extern "C" int XUnionRectWithRegion(gi_cliprect_t *rect, gi_region_ptr_t source, gi_region_ptr_t dest);

QRegion::QRegionData QRegion::shared_empty = {Q_BASIC_ATOMIC_INITIALIZER(1), 0, 0, 0};

void QRegion::updateX11Region() const
{
    d->rgn = XCreateRegion();
    if (!d->qt_rgn)
        return;

    int n = d->qt_rgn->numRects;
    const QRect *rect = (n == 1 ? &d->qt_rgn->extents : d->qt_rgn->rects.constData());
    while (n--) {
        gi_cliprect_t r;
        r.x = qMax(SHRT_MIN, rect->x());
        r.y = qMax(SHRT_MIN, rect->y());
        r.w =  qMin((int)USHRT_MAX, rect->width());
        r.h =  qMin((int)USHRT_MAX, rect->height());
        XUnionRectWithRegion(&r, d->rgn, d->rgn);
        ++rect;
    }
}

void *QRegion::clipRectangles(int &num) const
{
    if (!d->xrectangles && !(d == &shared_empty || d->qt_rgn->numRects == 0)) {
        gi_boxrec_t *r = static_cast<gi_boxrec_t*>(malloc(d->qt_rgn->numRects * sizeof(gi_boxrec_t)));
        d->xrectangles = r;
        int n = d->qt_rgn->numRects;
        const QRect *rect = (n == 1 ? &d->qt_rgn->extents : d->qt_rgn->rects.constData());
        while (n--) {
            r->x1 = qMax(SHRT_MIN, rect->x());
            r->y1 = qMax(SHRT_MIN, rect->y());
            r->x2 = r->x1 + qMin((int)USHRT_MAX, rect->width());
            r->y2 = r->y1 + qMin((int)USHRT_MAX, rect->height());
            ++r;
            ++rect;
        }
    }
    if (d == &shared_empty || d->qt_rgn->numRects == 0)
        num = 0;
    else
        num = d->qt_rgn->numRects;
    return d->xrectangles;
}

QT_END_NAMESPACE
QT_END_INCLUDE_NAMESPACE
