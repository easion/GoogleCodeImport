/* BEGIN_COMMON_COPYRIGHT_HEADER
 *
 * Razor - a lightweight, Qt based, desktop toolset
 * http://razor-qt.org
 *
 * Copyright: 2010-2011 Razor team
 * Authors:
 *   Petr Vanek <petr@scribus.info>
 *
 * This program or library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#include "helloworld.h"
#include <QtDebug>
#include <QGraphicsScene>
#include <QInputDialog>


EXPORT_RAZOR_DESKTOP_WIDGET_PLUGIN_CPP(HelloWorld)


HelloWorld::HelloWorld(QGraphicsScene * scene, const QString & configId, RazorSettings * config)
    : DesktopWidgetPlugin(scene, configId, config),
      QTextEdit()
{
    setText("Lorem Ipsum");
}

HelloWorld::~HelloWorld()
{
}

    
QString HelloWorld::info()
{
    return QObject::tr("Display simple text. A debugging/sample widget.");
}

QString HelloWorld::instanceInfo()
{
    return tr("Hello World widget:") + " " + m_configId;
}

void HelloWorld::setSizeAndPosition(const QPointF & position, const QSizeF & size)
{
    qDebug() << "Moving to" << position;
    move(position.x(), position.y());
    resize(size.width(), size.height());
}

void HelloWorld::configure()
{
    return;
}

void HelloWorld::save()
{
    m_config->beginGroup(m_configId);
    m_config->setValue("plugin", "hellowidget");
    m_config->setValue("x", pos().x());
    m_config->setValue("y", pos().y());
    m_config->setValue("w", width());
    m_config->setValue("h", height());
    m_config->endGroup();
}

