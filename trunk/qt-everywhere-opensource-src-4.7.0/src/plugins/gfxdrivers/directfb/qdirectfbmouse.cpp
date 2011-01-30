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

#include "qdirectfbmouse.h"

#ifndef QT_NO_QWS_DIRECTFB

#include "qdirectfbscreen.h"
#include <qsocketnotifier.h>

#include <gi/gi.h>
#include <gi/property.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

QT_BEGIN_NAMESPACE

class QGixMouseHandlerPrivate : public QObject
{
    Q_OBJECT
public:
    QGixMouseHandlerPrivate(QGixMouseHandler *h);
    ~QGixMouseHandlerPrivate();

    void setEnabled(bool on);
private:
    QGixMouseHandler *handler;
    //IGixEventBuffer *eventBuffer;

    QSocketNotifier *mouseNotifier;

    QPoint prevPoint;
    Qt::MouseButtons prevbuttons;

    gi_msg_t event;
    uint bytesRead;

private Q_SLOTS:
    void readMouseData();
};

QGixMouseHandlerPrivate::QGixMouseHandlerPrivate(QGixMouseHandler *h)
    : handler(h)
{
    int result;

    QScreen *screen = QScreen::instance();
    if (!screen) {
        qCritical("QGixMouseHandler: no screen instance found");
        return;
    }

    /*IGix *fb = QGixScreen::instance()->dfb();
    if (!fb) {
        qCritical("QGixMouseHandler: Gix not initialized");
        return;
    }
	*/

    //layer = QGixScreen::instance()->dfbDisplayLayer(); 


    /*DFBInputDeviceCapabilities caps;
    caps = DICAPS_BUTTONS | DICAPS_AXES;
    result = fb->CreateInputEventBuffer(fb, caps, DFB_TRUE, &eventBuffer);
    if (result != DFB_OK) {
        PLATFORM_ERROR("QGixMouseHandler: "
                      "Unable to create input event buffer", result);
        return;
    }*/

    int fd;
	fd = gi_get_connection_fd();
    //result = eventBuffer->CreateFileDescriptor(eventBuffer, &fd);
    if (fd < 0) {
        PLATFORM_ERROR("QGixMouseHandler: "
                      "Unable to create file descriptor");
        return;
    }

    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    // Gix seems to assume that the mouse always starts centered
    prevPoint = QPoint(screen->deviceWidth() / 2, screen->deviceHeight() / 2);
    prevbuttons = Qt::NoButton;
    memset(&event, 0, sizeof(event));
    bytesRead = 0;

    mouseNotifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
    connect(mouseNotifier, SIGNAL(activated(int)),this, SLOT(readMouseData()));
    setEnabled(true);
}

QGixMouseHandlerPrivate::~QGixMouseHandlerPrivate()
{
    //if (eventBuffer)
     //   eventBuffer->Release(eventBuffer);
}

void QGixMouseHandlerPrivate::setEnabled(bool on)
{
    if (mouseNotifier->isEnabled() != on) {
        mouseNotifier->setEnabled(on);
    }
}

void QGixMouseHandlerPrivate::readMouseData()
{
    if (!QScreen::instance())
        return;

    for (;;) {
        // GetEvent returns DFB_UNSUPPORTED after CreateFileDescriptor().
        // This seems stupid and I really hope it's a bug which will be fixed.

        // int ret = eventBuffer->GetEvent(eventBuffer, &event);

        char *buf = reinterpret_cast<char*>(&event);
        int ret = ::read(mouseNotifier->socket(),
                         buf + bytesRead, sizeof(gi_msg_t) - bytesRead);
        if (ret == -1) {
            if (errno == EINTR)
                continue;
            if (errno == EAGAIN)
                return;
            qWarning("QGixMouseHandlerPrivate::readMouseData(): %s",
                     strerror(errno));
            return;
        }

        Q_ASSERT(ret >= 0);
        bytesRead += ret;
        if (bytesRead < sizeof(gi_msg_t))
            break;
        bytesRead = 0;

#if 0 //FIXME DPP
        Q_ASSERT(event.clazz == DFEC_INPUT);
        const DFBInputEvent input = event.input;
        int x = prevPoint.x();
        int y = prevPoint.y();
        int wheel = 0;

        if (input.type == DIET_AXISMOTION) {
#if defined(QT_NO_DIRECTFB_LAYER) || defined(QT_DIRECTFB_WINDOW_AS_CURSOR) //delete
            if (input.flags & DIEF_AXISABS) {
                switch (input.axis) {
                case DIAI_X: x = input.axisabs; break;
                case DIAI_Y: y = input.axisabs; break;
                default:
                    qWarning("QGixMouseHandlerPrivate::readMouseData: "
                             "unknown axis (absolute) %d", input.axis);
                    break;
                }
            } else if (input.flags & DIEF_AXISREL) {
                switch (input.axis) {
                case DIAI_X: x += input.axisrel; break;
                case DIAI_Y: y += input.axisrel; break;
                case DIAI_Z: wheel = -120 * input.axisrel; break;
                default:
                    qWarning("QGixMouseHandlerPrivate::readMouseData: "
                             "unknown axis (releative) %d", input.axis);
                }
            }
#else
            if (input.axis == DIAI_X || input.axis == DIAI_Y) {
                int result = layer->GetCursorPosition(layer, &x, &y);
                if (result != DFB_OK) {
                    PLATFORM_ERROR("QGixMouseHandler::readMouseData",
                                  result);
                }
            } else if (input.axis == DIAI_Z) {
                Q_ASSERT(input.flags & DIEF_AXISREL);
                wheel = input.axisrel;
                wheel *= -120;
            }
#endif
        }

        Qt::MouseButtons buttons = Qt::NoButton;
        if (input.flags & DIEF_BUTTONS) {
            if (input.buttons & DIBM_LEFT)
                buttons |= Qt::LeftButton;
            if (input.buttons & DIBM_MIDDLE)
                buttons |= Qt::MidButton;
            if (input.buttons & DIBM_RIGHT)
                buttons |= Qt::RightButton;
        }

        QPoint p = QPoint(x, y);
        handler->limitToScreen(p);

        if (p == prevPoint && wheel == 0 && buttons == prevbuttons)
            continue;

        prevPoint = p;
        prevbuttons = buttons;

        handler->mouseChanged(p, buttons, wheel);
#endif
    }
}

QGixMouseHandler::QGixMouseHandler(const QString &driver,
                                             const QString &device)
    : QWSMouseHandler(driver, device)
{
    d = new QGixMouseHandlerPrivate(this);
}

QGixMouseHandler::~QGixMouseHandler()
{
    delete d;
}

void QGixMouseHandler::suspend()
{
    //d->setEnabled(false);
}

void QGixMouseHandler::resume()
{
    //d->setEnabled(true);
}

QT_END_NAMESPACE
#include "qdirectfbmouse.moc"
#endif // QT_NO_QWS_DIRECTFB


