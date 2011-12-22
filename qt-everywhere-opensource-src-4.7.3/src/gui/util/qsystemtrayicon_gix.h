/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Copyright (C) 2011 www.hanxuantech.com. The Gix parts. 
** Written by Easion <easion@hanxuantech.com> 
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSYSTRAYICONGIX_H
#define QSYSTRAYICONGIX_H

#include <QtCore/qcoreapplication.h>
#include <QtCore/qobject.h>
#include <qwidget.h>
#include "qsystemtrayicon.h"
#include <gi/gi.h>


QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)



class QSystemTrayIconSys : QWidget
{
public:
    QSystemTrayIconSys(QSystemTrayIcon *object);
    ~QSystemTrayIconSys();
#if 0
    bool pfWinEvent( gi_msg_t *m, long *result );
#else
	static bool sysTrayTracker(void *message, long *result);
	static QSystemTrayIconSys* findwindow(gi_window_id_t window);
	static QList<QSystemTrayIconSys *> trayIcons;
    static QCoreApplication::EventFilter oldEventFilter;
	bool process_event(gi_msg_t *m);
	gi_window_id_t proxyid(){return proxyWindow;}
#endif
    void UpdateIcon();
    void UpdateTooltip();
    gi_image_t *hIcon;
	gi_window_id_t proxyWindow;
     
    QSystemTrayIcon *q;

	QRect	shelfRect;

	inline bool deliverToolTipEvent(QEvent *e)
    { return QWidget::event(e); }


private:	
	void 	InstallIcon(void);

    int maxTipLength;
		
	bool 	ignoreNextMouseRelease;
	
	//BMessageRunner *pulse;
	
    friend class QSystemTrayIconPrivate;
};    




QT_END_NAMESPACE

QT_END_HEADER

#endif //QSYSTRAYICONGIX_H 
