/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
**
** Copyright (C) 2011 www.hanxuantech.com. The Gix parts. 
** Written by Easion <easion@hanxuantech.com> 
**
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
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qfontengine_p.h"
#include "qtextengine_p.h"

#include "qfontengine_gix_p.h"

QT_BEGIN_NAMESPACE

#ifndef QT_NO_FREETYPE

// ------------------------------------------------------------------
// PM FT engine
// ------------------------------------------------------------------

QFontEngineGix::QFontEngineGix(const QFontDef &fd, const FaceId &faceId,
                                 bool antialias, HintStyle hintStyle,
                                 bool autoHint, SubpixelAntialiasingType subPixel,
                                 int lcdFilter, bool useEmbeddedBitmap)
    : QFontEngineFT(fd)
{
    subpixelType = subPixel;

    default_hint_style = (QFontEngineFT::HintStyle)hintStyle;

#if defined(FT_LOAD_FORCE_AUTOHINT)
    if (autoHint)
        default_load_flags |= FT_LOAD_FORCE_AUTOHINT;
#endif

    lcdFilterType = lcdFilter;
    embeddedbitmap = useEmbeddedBitmap;

    GlyphFormat defaultFormat = Format_None;
    if (subpixelType != QFontEngineFT::Subpixel_None)
        defaultFormat = Format_A32;
    else if (antialias)
        defaultFormat = Format_A8;
    else
        defaultFormat = Format_Mono;

    init(faceId, antialias, defaultFormat);
}

QFontEngineGix::~QFontEngineGix()
{
    freeGlyphSets();
}

#endif // QT_NO_FREETYPE

QT_END_NAMESPACE
