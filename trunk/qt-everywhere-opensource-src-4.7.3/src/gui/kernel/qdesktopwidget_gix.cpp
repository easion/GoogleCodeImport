/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
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


#include "qdesktopwidget.h"



#include <stdio.h>
#include <gi/gi.h>

QDesktopWidget::QDesktopWidget()
    : QWidget(0, Qt::Desktop)
{
	//fprintf(stderr, "Unimplemented: QDesktopWidget::QDesktopWidget\n");
}

QDesktopWidget::~QDesktopWidget()
{
	//fprintf(stderr, "Unimplemented: QDesktopWidget::~QDesktopWidget\n");
}

void
QDesktopWidget::resizeEvent(QResizeEvent*)
{
	fprintf(stderr, "Unimplemented: QDesktopWidget::resizeEvent\n");
}

const QRect QDesktopWidget::availableGeometry(int screen) const
{
	int workarea[4];
	int rv;
	QRect workArea;

	rv = gi_wm_get_workarea(workarea);
	if (!rv)
	{
		workArea = QRect(workarea[0], workarea[1], workarea[2], workarea[3]);
	}
	else{
		workArea = screenGeometry(0);
	}
	
	return workArea;//QRect(0,0,info.scr_width,info.scr_height);
}

const QRect QDesktopWidget::screenGeometry(int screen) const
{	
	gi_screen_info_t info;

	gi_get_screen_info(&info);	
	return QRect(0,0,info.scr_width,info.scr_height);
}

int QDesktopWidget::screenNumber(const QWidget *widget) const
{
	Q_UNUSED(widget);
	//fprintf(stderr, "Reimplemented: QDesktopWidget::screenNumber(widget) \n");
	return 0;
}

int QDesktopWidget::screenNumber(const QPoint &point) const
{
	Q_UNUSED(point);
	//fprintf(stderr, "Reimplemented: QDesktopWidget::screenNumber\n");
	return 0;
}

bool QDesktopWidget::isVirtualDesktop() const
{
	//fprintf(stderr, "Unimplemented: QDesktopWidget::isVirtualDesktop\n");
	return true;
}

int QDesktopWidget::primaryScreen() const
{
	//fprintf(stderr, "Unimplemented: QDesktopWidget::primaryScreen\n");
	return 0;
}

int QDesktopWidget::numScreens() const
{
	//fprintf(stderr, "Unimplemented: QDesktopWidget::numScreens\n");
	return 1;
}

QWidget *QDesktopWidget::screen(int /* screen */)
{
	//fprintf(stderr, "Unimplemented: QDesktopWidget::screen\n");
	// It seems that a Qt::WType_Desktop cannot be moved?
	return this;
}
