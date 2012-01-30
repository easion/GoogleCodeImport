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

// WARNINGS: order of the includes is important here
#include "desktopbackgrounddialog.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QLibrary>
#include <QMenu>
#include <QGraphicsProxyWidget>
#include <QtDebug>
#include <QGraphicsTextItem>
#include <QMessageBox>
#include <QUuid>

#include "razorworkspace.h"
#include "workspacemanager.h"
#include "desktopwidgetplugin.h"
#include "arrangeitem.h"
#include <razorqt/razorsettings.h>
#include <razorqt/powermanager.h>
#include <razorqt/screensaver.h>
#include <qtxdg/xdgmenuwidget.h>
#include <qtxdg/xdgmenu.h>
#include <razorqt/razoraboutdlg.h>
#include <razorqt/addplugindialog/addplugindialog.h>


RazorWorkSpace::RazorWorkSpace(RazorSettings * config, int screen, QWidget* parent)
    : QGraphicsView(parent),
      m_config(config),
      m_screen(screen),
      m_mode(ModeNormal)
{
    qDebug() << "RazorWorkSpace::RazorWorkSpace";
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnBottomHint);
    setAttribute(Qt::WA_X11NetWmWindowTypeDesktop);
    setFrameShape(QFrame::NoFrame);
    
    m_power = new PowerManager(this);
    m_screenSaver = new ScreenSaver(this);
    
    m_config->beginGroup("razor");
    m_menuFile = m_config->value("menu_file", "").toString();
    m_config->endGroup();
    if (m_menuFile.isEmpty())
        m_menuFile = XdgMenu::getMenuFileName();
    qDebug() << "File Name:" << m_menuFile;

    // this is mandatory for virtualized (virtualbox) installations. Dunno why.
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    connect(QApplication::desktop(), SIGNAL(workAreaResized(int)),
            this, SLOT(workspaceResized(int)));
    
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);
    workspaceResized(screen);
    
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::TextAntialiasing);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setRenderHint(QPainter::HighQualityAntialiasing);
    
    setDragMode(QGraphicsView::RubberBandDrag);
    
    setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    
    setCacheMode(QGraphicsView::CacheBackground);
    
    m_actArrangeWidgets = new QAction(tr("Edit Desktop..."), this);
    m_actArrangeWidgets->setCheckable(true);
    connect(m_actArrangeWidgets, SIGNAL(toggled(bool)),
            this, SLOT(arrangeWidgets(bool)));
            
    m_actAddNewPlugin = new QAction(tr("Add New Desktop Widget..."), this);
    connect(m_actAddNewPlugin, SIGNAL(triggered()),
            this, SLOT(showAddPluginDialog()));

    m_actRemovePlugin = new QAction(tr("Remove Plugin..."), this);
    connect(m_actRemovePlugin, SIGNAL(triggered()),
            this, SLOT(removePlugin()));
            
    m_actConfigurePlugin = new QAction(tr("Configure Plugin..."), this);
    connect(m_actConfigurePlugin, SIGNAL(triggered()),
            this, SLOT(configurePlugin()));
            
    m_actSetbackground = new QAction(tr("Set Desktop Background..."), this);
    connect(m_actSetbackground, SIGNAL(triggered()),
            this, SLOT(setDesktopBackground()));
            
    m_actAbout = new QAction(tr("About Razor..."), this);
    connect(m_actAbout, SIGNAL(triggered()), this, SLOT(about()));
}

void RazorWorkSpace::setConfig(const WorkspaceConfig & bg)
{
    QBrush background;
    switch (bg.wallpaperType)
    {
        case (RazorWorkSpaceManager::BackgroundPixmap):
        {
            QPixmap pm(bg.wallpaper);
            if (pm.isNull())
                // TODO/FIXME: defaults
                qDebug() << "Pixmap is null" << bg.wallpaper;

            pm = pm.scaled(size(), bg.keepAspectRatio ? Qt::KeepAspectRatio : Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            background.setTexture(pm);
            break;
        }
        default:
            background.setColor(QColor(bg.wallpaper));
            background.setStyle(Qt::SolidPattern);
    }
    setBackgroundBrush(background);
    
    // load plugins
    
    foreach (QString configId, bg.plugins)
    {
        qDebug() << "RazorWorkSpace::setConfig() Plugin conf id" << configId << m_config;
        
        m_config->beginGroup(configId);
        QString libName(m_config->value("plugin", "").toString());
        // TODO/FIXME: now it's readable. Move it to QVariant in settings later with GUI
        //QPointF position(s->value("position", QPointF(10, 10)).toPointF());
        //QSizeF size(s->value("size", QSizeF(100, 100)).toSizeF());
        qreal x = m_config->value("x", 10.0).toReal();
        qreal y = m_config->value("y", 10.0).toReal();
        qreal w = m_config->value("w", 10.0).toReal();
        qreal h = m_config->value("h", 10.0).toReal();
        QPointF position(x, y);
        QSizeF size(w, h);
        m_config->endGroup();
        
        //qDebug() << libName << position;

        RazorPluginInfo pluginInfo;
        pluginInfo.load(QString("%1/%2.desktop").arg(PLUGIN_DESKTOP_FILES_DIR, libName));

        if (!pluginInfo.isValid() || pluginInfo.serviceType() != "RazorDesktop/Plugin")
        {
            qWarning() << "RazorWorkSpace::setConfig() Plugin" << libName << "not found";
            continue;
        }

        QLibrary * lib = pluginInfo.loadLibrary(DESKTOP_PLUGIN_DIR);
        if (!lib)
        {
            qWarning() << "RazorWorkSpace::setConfig() Library" << libName << "is not loaded";
            continue;
        }

        QGraphicsItem * item = loadPlugin(lib, configId);
        DesktopWidgetPlugin * plugin = getPluginFromItem(item);
        if (plugin)
            plugin->setSizeAndPosition(position, size);
    }
}

void RazorWorkSpace::saveConfig()
{
    QStringList plugins;
    
    QList<QGraphicsItem*> items = m_scene->items();
    foreach (QGraphicsItem * item, items)
    {
        DesktopWidgetPlugin * plug = getPluginFromItem(item);
        // here it *must* be a plugin
        Q_ASSERT(plug);
        plugins.append(plug->configId());
    }
    m_config->beginGroup("razor");
    m_config->beginWriteArray("desktops");
    m_config->setArrayIndex(m_screen);
    m_config->setValue("plugins", plugins);
    m_config->endArray();
    m_config->endGroup();
}

QGraphicsItem * RazorWorkSpace::loadPlugin(QLibrary * lib, const QString & configId)
{
    Q_ASSERT(lib);
    DesktopWidgetInitFunction initFunc = (DesktopWidgetInitFunction) lib->resolve("init");
    if (!initFunc)
    {
        qDebug() << lib->errorString();
        delete lib;
    }
    else
    {
        DesktopWidgetPlugin * plugin = initFunc(m_scene, configId, m_config);
        Q_ASSERT(plugin);

        qDebug() << "    * Plugin loaded.";
        qDebug() << plugin->info();

        QGraphicsItem * item = dynamic_cast<QGraphicsItem*>(plugin);
        QWidget * w = dynamic_cast<QWidget*>(plugin);
        if (w)
        {
            qDebug() << "adding widget";
            return m_scene->addWidget(w);
        }
        else if (item)
        {
            qDebug() << "adding item";
            m_scene->addItem(item);
            return item;
        }
    }
    return 0;
}

void RazorWorkSpace::workspaceResized(int screen)
{
    if (screen != m_screen)
        return;

    QRect geometry(QApplication::desktop()->screenGeometry(screen)); 
    move(geometry.x(), geometry.y());
    resize(geometry.width(), geometry.height());
    // do not use geometry.x(), geometry.y() - scene starts always from 0,0
    // the view is moved to geometry x, y instead
    m_scene->setSceneRect(0, 0, geometry.width(), geometry.height());
}

void RazorWorkSpace::mouseReleaseEvent(QMouseEvent* _ev)
{
    DesktopWidgetPlugin * plug = getPluginFromItem(m_scene->itemAt(_ev->posF()));
    if (plug && plug->blockGlobalMenu())
    {
        //qDebug() << "RazorWorkSpace::mouseReleaseEvent" << "menu blocked by plugin";
        QGraphicsView::mouseReleaseEvent(_ev);
        return;
    }
    
    // "context" menu
    if (_ev->button() == Qt::RightButton)
    {
        QMenu * context = 0;
        if (m_mode == ModeNormal)
        {
            // TODO/FIXME: cache it?
            XdgMenu xdgMenu;
            xdgMenu.setEnvironments("X-RAZOR");
            if (xdgMenu.read(m_menuFile))
            {
                context = new XdgMenuWidget(xdgMenu, "Context Menu", this);
            }
            else
            {
                QMessageBox::warning(this, "Parse error", xdgMenu.errorString());
                context = new QMenu("Context Menu", this);
            }

            context->addSeparator();
            context->addAction(m_actArrangeWidgets);
            context->addAction(m_actSetbackground);
            context->addAction(m_actAbout);
            context->addSeparator();
            context->addActions(m_power->availableActions());
            context->addSeparator();
            context->addActions(m_screenSaver->availableActions());
        }
        else
        {
            context = new QMenu("Context Menu");
            context->addAction(m_actArrangeWidgets);
            context->addAction(m_actAddNewPlugin);
            ArrangeItem * curr = dynamic_cast<ArrangeItem*>(m_scene->itemAt(_ev->posF()));
            if (curr && curr->editable())
            {
                context->addAction(m_actRemovePlugin);
                m_actRemovePlugin->setData(_ev->posF());
                context->addAction(m_actConfigurePlugin);
                m_actConfigurePlugin->setData(_ev->posF());
            }
        }

        context->exec(QCursor::pos());
        delete context;
    }

    QGraphicsView::mouseReleaseEvent(_ev);
}

void RazorWorkSpace::wheelEvent(QWheelEvent * e)
{
    //! \todo TODO/FIXME: wheel event is disabled because openbox handles it on its owns. Check it for different WMs... maybe we should consume XLib event instead...s
#if 0
    int max = xfitMan().getNumDesktop() - 1;
    int delta = e->delta() > 0 ? 1 : -1;
    int current = xfitMan().getActiveDesktop() + delta;
    
    if (current > max)
        current = 0;
    else if (current < 0)
        current = max;

    xfitMan().setActiveDesktop(current);
#else
    QGraphicsView::wheelEvent(e);
#endif
}

void RazorWorkSpace::arrangeWidgets(bool start)
{
    if (start)
    {
        // enter the "edit" mode
        m_mode = ModeArrange;
        QList<QGraphicsItem*> items = m_scene->items();
        m_arrangeRoot = new ArrangeItem(0, 0, tr("Razor Desktop Edit Mode"),
                                        m_scene->sceneRect(),
                                        false);
        m_scene->addItem(m_arrangeRoot);
        
        foreach (QGraphicsItem * item, items)
        {
            DesktopWidgetPlugin * plug = getPluginFromItem(item);
            // here it *must* be a plugin
            Q_ASSERT(plug);
            QRectF br = item->sceneBoundingRect();
            ArrangeItem * i = new ArrangeItem(item, plug, plug->instanceInfo(), br, true, m_arrangeRoot);
            m_arrangeList.append(i);
        }
    }
    else
    {
        // save new positions
        foreach (ArrangeItem * i, m_arrangeList)
        {
            i->plugin()->setSizeAndPosition(i->pos(), i->boundingRect().size());
            i->plugin()->save();
        }
        // back to normal
        m_mode = ModeNormal;
        m_scene->removeItem(m_arrangeRoot);
        delete m_arrangeRoot;
        m_arrangeRoot = 0;
        m_arrangeList.clear();
        
        saveConfig();
    }
}

void RazorWorkSpace::showAddPluginDialog()
{
    AddPluginDialog* dlg = findChild<AddPluginDialog*>();

    if (!dlg)
    {
        dlg = new AddPluginDialog(PLUGIN_DESKTOP_FILES_DIR, "RazorDesktop/Plugin", "*", this);
        dlg->move(geometry().center() - dlg->geometry().center());
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        connect(dlg, SIGNAL(pluginSelected(const RazorPluginInfo&)), this, SLOT(addPlugin(const RazorPluginInfo&)));
    }

    dlg->exec();
}

void RazorWorkSpace::addPlugin(const RazorPluginInfo &pluginInfo)
{
    qDebug() << "addPlugin" << pluginInfo;
    QLibrary * lib = pluginInfo.loadLibrary(DESKTOP_PLUGIN_DIR);
    if (!lib)
        return;

    QGraphicsItem * item = loadPlugin(lib, QString("%1_%2").arg(pluginInfo.id()).arg(QUuid::createUuid().toString()));
    DesktopWidgetPlugin * plugin = getPluginFromItem(item);

    // "clever" positioning
    QSizeF size(100, 100);
    int x = 10, y = 10;
    int xmax = m_arrangeRoot->boundingRect().width()-60;
    int ymax = m_arrangeRoot->boundingRect().height()-60;
    while (true)
    {
        bool placeIt = true;
        plugin->setSizeAndPosition(QPointF(x, y), size);
        foreach (ArrangeItem* i, m_arrangeList)
        {
            if (item->collidesWithItem(i))
            {
                placeIt = false;
                break;
            }
            placeIt = true;
        }
        
        if (placeIt)
        {
            QRectF br = item->sceneBoundingRect();
            ArrangeItem * i = new ArrangeItem(item, plugin, plugin->instanceInfo(), br, true, m_arrangeRoot);
            m_arrangeList.append(i);            
            break;
        }

        x += 20;
        if (x >= xmax)
        {
            x = 0;
            y += 20;
        }
        if (y >= ymax)
        {
            QMessageBox::information(this, tr("New Desktop Widget"),
                                        tr("There is no free space to add new desktop widget"));
            if (item)
            {
                m_scene->removeItem(item);
                //delete item;
            }
            // TODO/FIXME: check for mem leaks one day...
            //if (plugin) delete plugin;
            //if (lib) delete lib;
            break;
        }
    } // while
}

void RazorWorkSpace::removePlugin()
{
    ArrangeItem * item = dynamic_cast<ArrangeItem*>(m_scene->itemAt(m_actRemovePlugin->data().toPointF()));
    if (!item)
    {
        qDebug() << "Mismatch in m_actRemovePlugin data(). Expected ArrangeItem got" << m_actRemovePlugin->data();
        return;
    }
    if (QMessageBox::question(this,
                              tr("Remove Desktop Widget?"),
                              tr("Really remove this widget '%1'?").arg(item->plugin()->instanceInfo()),
                            QMessageBox::Yes, QMessageBox::No)
        == QMessageBox::No)
    {
        return;
    }

    QGraphicsItem * related = item->related();
    m_scene->removeItem(related);
    
    item->plugin()->removeConfig();
    m_scene->removeItem(item);
    m_arrangeList.removeAll(item);
    delete related;
    delete item;
}

void RazorWorkSpace::configurePlugin()
{
    ArrangeItem * item = dynamic_cast<ArrangeItem*>(m_scene->itemAt(m_actRemovePlugin->data().toPointF()));
    qDebug() << "RazorWorkSpace::configurePlugin" << item;
    if (!item)
    {
        qDebug() << "Mismatch in m_actConfigurePlugin data(). Expected ArrangeItem got" << m_actRemovePlugin->data();
        return;
    }
    item->plugin()->configure();
}

void RazorWorkSpace::setDesktopBackground()
{
    DesktopBackgroundDialog dia(m_config, m_screen, size(), backgroundBrush(), this);
    if (dia.exec())
    {
        setBackgroundBrush(dia.background());
    }
}

void RazorWorkSpace::about()
{
    RazorAboutDLG dia;
}

DesktopWidgetPlugin * RazorWorkSpace::getPluginFromItem(QGraphicsItem * item)
{
    DesktopWidgetPlugin * plug = dynamic_cast<DesktopWidgetPlugin*>(item);
    if (!plug)
    {
        QGraphicsProxyWidget * w = dynamic_cast<QGraphicsProxyWidget*>(item);
        if (w)
            plug = dynamic_cast<DesktopWidgetPlugin*>(w->widget());
    }
    return plug;
}

RazorWorkSpace::~RazorWorkSpace()
{
}
