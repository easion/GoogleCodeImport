#include "qapplication_p.h"
#include "qsessionmanager.h"
#include "qapplication.h"
#include "qevent.h"
#include "qeventdispatcher_gix_p.h"
#include "qwidget.h"
#include "qwidget_p.h"
#include "private/qsystemtrayicon_p.h"
#include <QtGui>
#include <gi/gi.h>
#include "qwidget_p.h"
#include "qcursor_p.h"
#include <private/qkeymapper_p.h>
#include <private/qbackingstore_p.h>

#include <stdio.h>

extern "C" gi_bool_t gi_xdnd_selection_handler(gi_msg_t *msg);
extern "C" gi_bool_t gi_xdnd_client_handler(gi_msg_t *msg);


#ifndef QT_GUI_DOUBLE_CLICK_RADIUS
#define QT_GUI_DOUBLE_CLICK_RADIUS 5
#endif

/*****************************************************************************
  Internal variables and functions
 *****************************************************************************/
static WId curWin;

// ignore the next release event if return from a modal widget
Q_GUI_EXPORT bool qt_win_ignoreNextMouseReleaseEvent = false;

#if defined(QT_DEBUG)
static bool        appNoGrab        = false;        // mouse/keyboard grabbing
#endif

static bool        app_do_modal           = false;        // modal mode
extern QWidgetList *qt_modal_stack;

// flags for extensions for special Languages, currently only for RTL languages
bool         qt_use_rtl_extensions = false;

static gi_window_id_t        mouseActWindow             = 0;        // window where mouse is
static Qt::MouseButton  mouseButtonPressed   = Qt::NoButton; // last mouse button pressed
static Qt::MouseButtons mouseButtonState     = Qt::NoButton; // mouse button state
static time_t        mouseButtonPressTime = 0;        // when was a button pressed
static short        mouseXPos, mouseYPos;                // mouse pres position in act window
static short        mouseGlobalXPos, mouseGlobalYPos; // global mouse press position


// window where mouse buttons have been pressed
static gi_window_id_t pressed_window = 0;

// popup control
static bool replayPopupMouseEvent = false;
static bool popupGrabOk;


QWidget *qt_button_down = 0; // last widget to be pressed with the mouse
QPointer<QWidget> qt_last_mouse_receiver = 0;
static QWidget *qt_popup_down = 0;  // popup that contains the pressed widget

extern bool qt_xdnd_dragging;
QTextCodec * qt_input_mapper = 0;

/* Warning: if you modify this string, modify the list of atoms in qt_x11_p.h as well! */
static const char * pf_atomnames = {
    // window-manager <-> client protocols
    "WM_PROTOCOLS\0"
    "WM_DELETE_WINDOW\0"
    "WM_TAKE_FOCUS\0"
    "_NET_WM_PING\0"
    "_NET_WM_CONTEXT_HELP\0"
    "_NET_WM_SYNC_REQUEST\0"
    "_NET_WM_SYNC_REQUEST_COUNTER\0"

    // ICCCM window state
    "WM_STATE\0"
    "WM_CHANGE_STATE\0"

    // Session management
    "WM_CLIENT_LEADER\0"
    "WM_WINDOW_ROLE\0"
    "SM_CLIENT_ID\0"

    // Clipboard
    "CLIPBOARD\0"
    "INCR\0"
    "TARGETS\0"
    "MULTIPLE\0"
    "TIMESTAMP\0"
    "SAVE_TARGETS\0"
    "CLIP_TEMPORARY\0"
    "_QT_SELECTION\0"
    "_QT_CLIPBOARD_SENTINEL\0"
    "_QT_SELECTION_SENTINEL\0"
    "CLIPBOARD_MANAGER\0"

    "RESOURCE_MANAGER\0"

    "_XSETROOT_ID\0"

    "_QT_SCROLL_DONE\0"
    "_QT_INPUT_ENCODING\0"

    "_MOTIF_WM_HINTS\0"

    "DTWM_IS_RUNNING\0"
    "ENLIGHTENMENT_DESKTOP\0"
    "_DT_SAVE_MODE\0"
    "_SGI_DESKS_MANAGER\0"

    // EWMH (aka NETWM)
    "_NET_SUPPORTED\0"
    "_NET_VIRTUAL_ROOTS\0"
    "_NET_WORKAREA\0"

    "_NET_MOVERESIZE_WINDOW\0"
    "_NET_WM_MOVERESIZE\0"

    "_NET_WM_NAME\0"
    "_NET_WM_ICON_NAME\0"
    "_NET_WM_ICON\0"

    "_NET_WM_PID\0"

    "_NET_WM_WINDOW_OPACITY\0"

    "_NET_WM_STATE\0"
    "_NET_WM_STATE_ABOVE\0"
    "_NET_WM_STATE_BELOW\0"
    "_NET_WM_STATE_FULLSCREEN\0"
    "_NET_WM_STATE_MAXIMIZED_HORZ\0"
    "_NET_WM_STATE_MAXIMIZED_VERT\0"
    "_NET_WM_STATE_MODAL\0"
    "_NET_WM_STATE_STAYS_ON_TOP\0"
    "_NET_WM_STATE_DEMANDS_ATTENTION\0"

    "_NET_WM_USER_TIME\0"
    "_NET_WM_USER_TIME_WINDOW\0"
    "_NET_WM_FULL_PLACEMENT\0"

    "_NET_WM_WINDOW_TYPE\0"
    "_NET_WM_WINDOW_TYPE_DESKTOP\0"
    "_NET_WM_WINDOW_TYPE_DOCK\0"
    "_NET_WM_WINDOW_TYPE_TOOLBAR\0"
    "_NET_WM_WINDOW_TYPE_MENU\0"
    "_NET_WM_WINDOW_TYPE_UTILITY\0"
    "_NET_WM_WINDOW_TYPE_SPLASH\0"
    "_NET_WM_WINDOW_TYPE_DIALOG\0"
    "_NET_WM_WINDOW_TYPE_DROPDOWN_MENU\0"
    "_NET_WM_WINDOW_TYPE_POPUP_MENU\0"
    "_NET_WM_WINDOW_TYPE_TOOLTIP\0"
    "_NET_WM_WINDOW_TYPE_NOTIFICATION\0"
    "_NET_WM_WINDOW_TYPE_COMBO\0"
    "_NET_WM_WINDOW_TYPE_DND\0"
    "_NET_WM_WINDOW_TYPE_NORMAL\0"
    "_KDE_NET_WM_WINDOW_TYPE_OVERRIDE\0"

    "_KDE_NET_WM_FRAME_STRUT\0"

    "_NET_STARTUP_INFO\0"
    "_NET_STARTUP_INFO_BEGIN\0"

    "_NET_SUPPORTING_WM_CHECK\0"

    "_NET_WM_CM_S0\0"

    "_NET_SYSTEM_TRAY_VISUAL\0"

    "_NET_ACTIVE_WINDOW\0"

    // Property formats
    "COMPOUND_TEXT\0"
    "TEXT\0"
    "UTF8_STRING\0"

    // xdnd
    "XdndEnter\0"
    "XdndPosition\0"
    "XdndStatus\0"
    "XdndLeave\0"
    "XdndDrop\0"
    "XdndFinished\0"
    "XdndTypeList\0"
    "XdndActionList\0"

    "XdndSelection\0"

    "XdndAware\0"
    "XdndProxy\0"

    "XdndActionCopy\0"
    "XdndActionLink\0"
    "XdndActionMove\0"
    "XdndActionPrivate\0"

    // Motif DND
    "_MOTIF_DRAG_AND_DROP_MESSAGE\0"
    "_MOTIF_DRAG_INITIATOR_INFO\0"
    "_MOTIF_DRAG_RECEIVER_INFO\0"
    "_MOTIF_DRAG_WINDOW\0"
    "_MOTIF_DRAG_TARGETS\0"

    "XmTRANSFER_SUCCESS\0"
    "XmTRANSFER_FAILURE\0"

    // Xkb
    "_XKB_RULES_NAMES\0"

    // XEMBED
    "_XEMBED\0"
    "_XEMBED_INFO\0"

    // Wacom old. (before version 0.10)
    "Wacom Stylus\0"
    "Wacom Cursor\0"
    "Wacom Eraser\0"

    // Tablet
    "STYLUS\0"
    "ERASER\0"
};
Q_GUI_EXPORT QpfData *qt_pfData = 0;


typedef bool(*QpfFilterFunction)(gi_msg_t *event);

Q_GLOBAL_STATIC(QList<QpfFilterFunction>, pfFilters)

Q_GUI_EXPORT void qt_installpfEventFilter(QpfFilterFunction func)
{
    Q_ASSERT(func);

    if (QList<QpfFilterFunction> *list = pfFilters())
        list->append(func);
}

Q_GUI_EXPORT void qt_removepfEventFilter(QpfFilterFunction func)
{
    Q_ASSERT(func);

    if (QList<QpfFilterFunction> *list = pfFilters())
        list->removeOne(func);
}



bool QApplication::pfEventFilter(gi_msg_t *)
{
    return false;
}


static bool qt_pfEventFilter(gi_msg_t* ev)
{
    long unused;
    if (qApp->filterEvent(ev, &unused))
        return true;
    if (const QList<QpfFilterFunction> *list = pfFilters()) {
        for (QList<QpfFilterFunction>::const_iterator it = list->constBegin(); it != list->constEnd(); ++it) {
            if ((*it)(ev))
                return true;
        }
    }

    return qApp->pfEventFilter(ev);
}

bool qt_nograb()                                // application no-grab option
{
#if defined(QT_DEBUG)
    return appNoGrab;
#else
    return false;
#endif
}


class QETWidget : public QWidget                // event translator widget
{
public:
    QWidgetPrivate* d_func() { return QWidget::d_func(); }
    bool        pfWinEvent(gi_msg_t *m, long *r)        { return QWidget::pfWinEvent(m, r); }
    bool translateMouseEvent(const gi_msg_t *);
    void translatePaintEvent(const gi_msg_t *);
    bool translateConfigEvent(const gi_msg_t *);
    bool translateCloseEvent(const gi_msg_t *);
    bool translateWheelEvent(int global_x, int global_y, int delta, Qt::MouseButtons buttons,
                             Qt::KeyboardModifiers modifiers, Qt::Orientation orient);
#if !defined (QT_NO_TABLET)
    bool translateXinputEvent(const gi_msg_t*, QTabletDeviceData *tablet);
#endif
    bool translatePropertyEvent(const gi_msg_t *);
};


static void qt_gix_create_intern_atoms()
{
    const char *names[QpfData::NAtoms];
    const char *ptr = pf_atomnames;

    int i = 0;
    while (*ptr) {
        names[i++] = ptr;
        while (*ptr)
            ++ptr;
        ++ptr;
    }

    Q_ASSERT(i == QpfData::NPredefinedAtoms);

    QByteArray settings_atom_name("_QT_SETTINGS_TIMESTAMP_");
    settings_atom_name += "Gix GUI"; //XDisplayName(X11->displayName);
    names[i++] = settings_atom_name;

    Q_ASSERT(i == QpfData::NAtoms);

    for (i = 0; i < QpfData::NAtoms; ++i)
        qt_pfData->atoms[i] = gi_intern_atom( (char *)names[i], FALSE);
}

void qt_init(QApplicationPrivate *priv, int)
{
	int i;
	int fd;
	//qDebug("qt_init()");

    QColormap::initialize();
    QFont::initialize();
#ifndef QT_NO_CURSOR
    QCursorData::initialize();
#endif
    		
	QApplicationPrivate::gix_apply_settings();		
	fd = gi_init();
	if (fd <= 0)
	{
		fprintf(stderr, "No gi driver found\n");
		//QApplicationPrivate::reset_instance_pointer();
        exit(1);
	}

	qt_pfData = new QpfData;
	// Finally create all atoms
    qt_gix_create_intern_atoms();

	// Support protocols
    //X11->xdndSetup();
		
	if(priv->argc==1) {
		QString appDir = QCoreApplication::applicationDirPath();
		chdir(appDir.toUtf8());		
	}

	char *dfont = getenv("QT_GIX_FONT");
	if (dfont)
	{
	 //QFont f = QFont(QLatin1String("SimSun"), 12);
	 QFont f = QFont(QLatin1String(dfont), 11);

	QApplicationPrivate::setSystemFont(f);
	}

}

void qt_cleanup()
{
    QPixmapCache::clear();

#ifndef QT_NO_CURSOR
    QCursorData::cleanup();
#endif
    QFont::cleanup();
    QColormap::cleanup();

	gi_exit();
	delete qt_pfData;
    qt_pfData = 0;
	qDebug("qt_cleanup()");
}

void QApplicationPrivate::_q_alertTimeOut()
{
	qDebug("Unimplemented: void  QApplicationPrivate::_q_alertTimeOut()\n");
}

QString QApplicationPrivate::appName() const
{
	return QCoreApplicationPrivate::appName();
}

void QApplicationPrivate::createEventDispatcher()
{
	Q_Q(QApplication);

	if (q->type() != QApplication::Tty)
		eventDispatcher = new QEventDispatcherGix(q);
	else
		eventDispatcher = new QEventDispatcherUNIX(q);
}

/*****************************************************************************
  Popup widget mechanism

  openPopup()
        Adds a widget to the list of popup widgets
        Arguments:
            QWidget *widget        The popup widget to be added

  closePopup()
        Removes a widget from the list of popup widgets
        Arguments:
            QWidget *widget        The popup widget to be removed
 *****************************************************************************/

static int openPopupCount = 0;

void QApplicationPrivate::openPopup(QWidget *popup)
{
    Q_Q(QApplication);
    openPopupCount++;
    if (!QApplicationPrivate::popupWidgets) {                        // create list
        QApplicationPrivate::popupWidgets = new QWidgetList;
    }
    QApplicationPrivate::popupWidgets->append(popup);                // add to end of list

    if (QApplicationPrivate::popupWidgets->count() == 1 && !qt_nograb()){ // grab mouse/keyboard
        Q_ASSERT(popup->testAttribute(Qt::WA_WState_Created));
        int r = gi_grab_keyboard( popup->effectiveWinId(), false,
                              0, 0);
        if ((popupGrabOk = (r == 0))) {
            r = gi_grab_pointer( popup->effectiveWinId(), true,false,
                             (GI_MASK_BUTTON_DOWN | GI_MASK_BUTTON_UP | GI_MASK_MOUSE_MOVE
                              | GI_MASK_MOUSE_ENTER | GI_MASK_MOUSE_EXIT ),
                             0, GI_BUTTON_L|GI_BUTTON_R|GI_BUTTON_M);
            if (!(popupGrabOk = (r == 0))) {
                // transfer grab back to the keyboard grabber if any
                if (QWidgetPrivate::keyboardGrabber != 0)
                    QWidgetPrivate::keyboardGrabber->grabKeyboard();
                else
                    gi_ungrab_keyboard();
            }
        }
    }

    // popups are not focus-handled by the window system (the first
    // popup grabbed the keyboard), so we have to do that manually: A
    // new popup gets the focus
    if (popup->focusWidget()) {
        popup->focusWidget()->setFocus(Qt::PopupFocusReason);
    } else if (QApplicationPrivate::popupWidgets->count() == 1) { // this was the first popup
        if (QWidget *fw = QApplication::focusWidget()) {
            QFocusEvent e(QEvent::FocusOut, Qt::PopupFocusReason);
            q->sendEvent(fw, &e);
        }
    }
}

void QApplicationPrivate::closePopup(QWidget *popup)
{
    Q_Q(QApplication);
    if (!QApplicationPrivate::popupWidgets)
        return;
    QApplicationPrivate::popupWidgets->removeAll(popup);
    if (popup == qt_popup_down) {
        qt_button_down = 0;
        qt_popup_down = 0;
    }
    if (QApplicationPrivate::popupWidgets->count() == 0) {                // this was the last popup
        delete QApplicationPrivate::popupWidgets;
        QApplicationPrivate::popupWidgets = 0;
        if (!qt_nograb() && popupGrabOk) {        // grabbing not disabled

            if (popup->geometry().contains(QPoint(mouseGlobalXPos, mouseGlobalYPos))
                || popup->testAttribute(Qt::WA_NoMouseReplay)) {
                // mouse release event or inside
                replayPopupMouseEvent = false;
            } else {                                // mouse press event
                mouseButtonPressTime -= 10000;        // avoid double click
                replayPopupMouseEvent = true;
            }
            // transfer grab back to mouse grabber if any, otherwise release the grab
            if (QWidgetPrivate::mouseGrabber != 0)
                QWidgetPrivate::mouseGrabber->grabMouse();
            else
                gi_ungrab_pointer();

            // transfer grab back to keyboard grabber if any, otherwise release the grab
            if (QWidgetPrivate::keyboardGrabber != 0)
                QWidgetPrivate::keyboardGrabber->grabKeyboard();
            else
                gi_ungrab_keyboard();

            //XFlush(dpy);
        }
        if (QApplicationPrivate::active_window) {
            if (QWidget *fw = QApplicationPrivate::active_window->focusWidget()) {
                if (fw != QApplication::focusWidget()) {
                    fw->setFocus(Qt::PopupFocusReason);
                } else {
                    QFocusEvent e(QEvent::FocusIn, Qt::PopupFocusReason);
                    q->sendEvent(fw, &e);
                }
            }
        }
    } else {
        // popups are not focus-handled by the window system (the
        // first popup grabbed the keyboard), so we have to do that
        // manually: A popup was closed, so the previous popup gets
        // the focus.
        QWidget* aw = QApplicationPrivate::popupWidgets->last();
        if (QWidget *fw = aw->focusWidget())
            fw->setFocus(Qt::PopupFocusReason);

        // regrab the keyboard and mouse in case 'popup' lost the grab
        if (QApplicationPrivate::popupWidgets->count() == 1 && !qt_nograb()){ // grab mouse/keyboard

            Q_ASSERT(aw->testAttribute(Qt::WA_WState_Created));
            int r = gi_grab_keyboard( aw->effectiveWinId(), false,
                                  0, 0);
            if ((popupGrabOk = (r == 0))) {
                r = gi_grab_pointer( aw->effectiveWinId(), true,false,
                             (GI_MASK_BUTTON_DOWN | GI_MASK_BUTTON_UP | GI_MASK_MOUSE_MOVE
                              | GI_MASK_MOUSE_ENTER | GI_MASK_MOUSE_EXIT ),
                             0, GI_BUTTON_L|GI_BUTTON_R|GI_BUTTON_M );
                if (!(popupGrabOk = (r == 0))) {
                    // transfer grab back to keyboard grabber
                    if (QWidgetPrivate::keyboardGrabber != 0)
                        QWidgetPrivate::keyboardGrabber->grabKeyboard();
                    else
                        gi_ungrab_keyboard();
                }
            }
        }
    }
}

void QApplicationPrivate::initializeWidgetPaletteHash()
{
	qDebug("Unimplemented:  QApplicationPrivate::initializeWidgetPaletteHash\n");
}

/*! \internal
    apply the settings to the application
*/
bool QApplicationPrivate::gix_apply_settings()
{
    QSettings settings(QSettings::UserScope, QLatin1String("Trolltech"));

    settings.beginGroup(QLatin1String("Qt"));  

    QStringList strlist;
    int i;
    QPalette pal(Qt::black);
    int groupCount = 0;
    strlist = settings.value(QLatin1String("Palette/active")).toStringList();
    if (!strlist.isEmpty()) {
        ++groupCount;
        for (i = 0; i < qMin(strlist.count(), int(QPalette::NColorRoles)); i++)
            pal.setColor(QPalette::Active, (QPalette::ColorRole) i,
                         QColor(strlist[i]));
    }
    strlist = settings.value(QLatin1String("Palette/inactive")).toStringList();
    if (!strlist.isEmpty()) {
        ++groupCount;
        for (i = 0; i < qMin(strlist.count(), int(QPalette::NColorRoles)); i++)
            pal.setColor(QPalette::Inactive, (QPalette::ColorRole) i,
                         QColor(strlist[i]));
    }
    strlist = settings.value(QLatin1String("Palette/disabled")).toStringList();
    if (!strlist.isEmpty()) {
        ++groupCount;
        for (i = 0; i < qMin(strlist.count(), int(QPalette::NColorRoles)); i++)
            pal.setColor(QPalette::Disabled, (QPalette::ColorRole) i,
                         QColor(strlist[i]));
    }
 
/*    if (!appFont) { //gix2dev-yw
        QFont font(QApplication::font());
        QString fontDescription;
        if (fontDescription.isEmpty())
            fontDescription = settings.value(QLatin1String("font")).toString();
        if (!fontDescription .isEmpty()) {
            font.fromString(fontDescription );
            QApplicationPrivate::setSystemFont(font);
        }
    }*/

    // read new QStyle
    QString stylename = settings.value(QLatin1String("style")).toString();
    if (stylename.isEmpty() && QApplicationPrivate::styleOverride.isNull()) {
        QStringList availableStyles = QStyleFactory::keys();
    }

    static QString currentStyleName = stylename;
    if (QCoreApplication::startingUp()) {
        if (!stylename.isEmpty() && QApplicationPrivate::styleOverride.isNull())
            QApplicationPrivate::styleOverride = stylename;
    } else {
        if (currentStyleName != stylename) {
            currentStyleName = stylename;
            QApplication::setStyle(stylename);
        }
    }

    int num =
        settings.value(QLatin1String("doubleClickInterval"),
                       QApplication::doubleClickInterval()).toInt();
    QApplication::setDoubleClickInterval(num);

    num =
        settings.value(QLatin1String("cursorFlashTime"),
                       QApplication::cursorFlashTime()).toInt();
    QApplication::setCursorFlashTime(num);

    num =
        settings.value(QLatin1String("wheelScrollLines"),
                       QApplication::wheelScrollLines()).toInt();
    QApplication::setWheelScrollLines(num);

    QString colorspec = settings.value(QLatin1String("colorSpec"),
                                       QVariant(QLatin1String("default"))).toString();
    if (colorspec == QLatin1String("normal"))
        QApplication::setColorSpec(QApplication::NormalColor);
    else if (colorspec == QLatin1String("custom"))
        QApplication::setColorSpec(QApplication::CustomColor);
    else if (colorspec == QLatin1String("many"))
        QApplication::setColorSpec(QApplication::ManyColor);
    else if (colorspec != QLatin1String("default"))
        colorspec = QLatin1String("default");

/*    QString defaultcodec = settings.value(QLatin1String("defaultCodec"),
                                          QVariant(QLatin1String("none"))).toString();
    if (defaultcodec != QLatin1String("none")) {
        QTextCodec *codec = QTextCodec::codecForName(defaultcodec.toLatin1());
        if (codec)
            QTextCodec::setCodecForTr(codec);
    }*/
    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    if (codec) {
     	QTextCodec::setCodecForLocale(codec);
    }           	
    qt_input_mapper = QTextCodec::codecForName("UTF-8");
    

    int w = settings.value(QLatin1String("globalStrut/width")).toInt();
    int h = settings.value(QLatin1String("globalStrut/height")).toInt();
    QSize strut(w, h);
    if (strut.isValid())
        QApplication::setGlobalStrut(strut);

    QStringList effects = settings.value(QLatin1String("GUIEffects")).toStringList();
    QApplication::setEffectEnabled(Qt::UI_General,
                                   effects.contains(QLatin1String("general")));
    QApplication::setEffectEnabled(Qt::UI_AnimateMenu,
                                   effects.contains(QLatin1String("animatemenu")));
    QApplication::setEffectEnabled(Qt::UI_FadeMenu,
                                   effects.contains(QLatin1String("fademenu")));
    QApplication::setEffectEnabled(Qt::UI_AnimateCombo,
                                   effects.contains(QLatin1String("animatecombo")));
    QApplication::setEffectEnabled(Qt::UI_AnimateTooltip,
                                   effects.contains(QLatin1String("animatetooltip")));
    QApplication::setEffectEnabled(Qt::UI_FadeTooltip,
                                   effects.contains(QLatin1String("fadetooltip")));
    QApplication::setEffectEnabled(Qt::UI_AnimateToolBox,
                                   effects.contains(QLatin1String("animatetoolbox")));

  	settings.beginGroup(QLatin1String("Font Substitutions"));
    QStringList fontsubs = settings.childKeys();
    if (!fontsubs.isEmpty()) {
         QStringList::Iterator it = fontsubs.begin();
         for (; it != fontsubs.end(); ++it) {
             QString fam = *it;
             QStringList subs = settings.value(fam).toStringList();
             QFont::insertSubstitutions(fam, subs);
         }
    }
    settings.endGroup();    
    settings.endGroup(); // Qt
    return true;
}


void QApplicationPrivate::gix_initialize_style()
{
    if (QApplicationPrivate::app_style)
        return;

    //QApplicationPrivate::app_style = QStyleFactory::create(QLatin1String("gix"));        
    QApplicationPrivate::app_style = QStyleFactory::create(QLatin1String("gtk"));        
}

/*****************************************************************************
  Modal widgets; We have implemented our own modal widget mechanism
  to get total control.
  A modal widget without a parent becomes application-modal.
  A modal widget with a parent becomes modal to its parent and grandparents..

  QApplicationPrivate::enterModal()
        Enters modal state
        Arguments:
            QWidget *widget        A modal widget

  QApplicationPrivate::leaveModal()
        Leaves modal state for a widget
        Arguments:
            QWidget *widget        A modal widget
 *****************************************************************************/

bool QApplicationPrivate::modalState()
{
	//qDebug()<<"Unimplemented: QApplicationPrivate::modalState():"<<app_do_modal;
	return app_do_modal;
}

void QApplicationPrivate::enterModal_sys(QWidget *widget)
{
//	qDebug()<<"Unimplemented: QApplicationPrivate::enterModal_sys(). Widget:"<<widget;

    if (!qt_modal_stack)
        qt_modal_stack = new QWidgetList;

    QWidget *leave = qt_last_mouse_receiver;
    if (!leave)
        leave = QWidget::find((WId)curWin);
    QApplicationPrivate::dispatchEnterLeave(0, leave);
    qt_modal_stack->insert(0, widget);
    app_do_modal = true;
    curWin = 0;
    qt_last_mouse_receiver = 0;
}


bool qt_try_modal(QWidget *widget, gi_msg_t *event)
{
    if (QApplicationPrivate::tryModalHelper(widget))
        return true;

    // allow mouse release events to be sent to widgets that have been pressed
    if (event->type == GI_MSG_BUTTON_UP) {
        QWidget *alienWidget = widget->childAt(
			widget->mapFromGlobal(QPoint(event->params[0], 
						event->params[1])));
        if (widget == qt_button_down || 
			(alienWidget && alienWidget == qt_button_down))
            return true;
    }



    // disallow mouse/key events
    switch (event->type) {
    case GI_MSG_BUTTON_DOWN:
    case GI_MSG_BUTTON_UP:
    case GI_MSG_MOUSE_MOVE:
    case GI_MSG_KEY_DOWN:
    case GI_MSG_KEY_UP:
    case GI_MSG_MOUSE_ENTER:
    case GI_MSG_MOUSE_EXIT:
    case GI_MSG_CLIENT_MSG:
        return false;
    default:
        break;
    }

    return true;
}



void QApplicationPrivate::leaveModal_sys(QWidget *widget)
{
	//qDebug()<<"Unimplemented: QApplicationPrivate::leaveModal_sys(). Widget:"<<widget;

    if (qt_modal_stack && qt_modal_stack->removeAll(widget)) {
        if (qt_modal_stack->isEmpty()) {
            delete qt_modal_stack;
            qt_modal_stack = 0;
            QPoint p(QCursor::pos());
            QWidget* w = QApplication::widgetAt(p.x(), p.y());
            QWidget *leave = qt_last_mouse_receiver;
            if (!leave)
                leave = QWidget::find((WId)curWin);
            if (QWidget *grabber = QWidget::mouseGrabber()) {
                w = grabber;
                if (leave == w)
                    leave = 0;
            }
            QApplicationPrivate::dispatchEnterLeave(w, leave); // send synthetic enter event
            curWin = w ? w->effectiveWinId() : 0;
            qt_last_mouse_receiver = w;
        }
    }
    app_do_modal = qt_modal_stack != 0;
}


/*****************************************************************************
  Platform specific QApplication members
 *****************************************************************************/

#ifdef QT3_SUPPORT
void QApplication::setMainWidget(QWidget *mainWidget)
{
#ifndef QT_NO_DEBUG
    if (mainWidget && mainWidget->parentWidget() && mainWidget->isWindow())
        qWarning("QApplication::setMainWidget: New main widget (%s/%s) "
                  "has a parent",
                  mainWidget->metaObject()->className(), mainWidget->objectName().toLocal8Bit().constData());
#endif
    if (mainWidget)
        mainWidget->d_func()->createWinId();
    QApplicationPrivate::main_widget = mainWidget;
}
#endif

void QApplication::setDoubleClickInterval(int ms)
{
   //WinSetSysValue(HWND_DESKTOP, SV_DBLCLKTIME, ms);
	//set_click_speed(ms);
    QApplicationPrivate::mouse_double_click_time = ms;
}
#include "qwidget_gix.h"

extern "C" void get_click_speed(bigtime_t	*interval)
 {
	// *interval = 100;
	 *interval = 0;
 }

int QApplication::doubleClickInterval()
{
	bigtime_t	interval;
	get_click_speed(&interval);

	if (interval)
	{
	QApplicationPrivate::mouse_double_click_time = (int)(interval/1000);
	}		
	
	return QApplicationPrivate::mouse_double_click_time;
}

void QApplication::setKeyboardInputInterval(int ms)
{
    QApplicationPrivate::keyboard_input_time = ms;
}

int QApplication::keyboardInputInterval()
{
    // FIXME: get from the system
    return QApplicationPrivate::keyboard_input_time;
}

void QApplication::setWheelScrollLines(int n)
{
    QApplicationPrivate::wheel_scroll_lines = n;
}

int QApplication::wheelScrollLines()
{
    return QApplicationPrivate::wheel_scroll_lines;
}

void QApplication::setOverrideCursor(const QCursor &cursor)
{
	Q_UNUSED(cursor);
	qDebug("Unimplemented: QApplication::setOverrideCursor\n");
}

void QApplication::restoreOverrideCursor()
{
	qDebug("Unimplemented: QApplication::restoreOverrideCursor\n");
}

void QApplication::setEffectEnabled(Qt::UIEffect effect, bool enable)
{
	Q_UNUSED(effect);
	Q_UNUSED(enable);
//	qDebug("Unimplemented: QApplication::setEffectEnabled\n");
}

bool QApplication::isEffectEnabled(Qt::UIEffect effect)
{
	Q_UNUSED(effect);
//	qDebug("Unimplemented: QApplication::isEffectEnabled\n");
	return false;
}

void  QApplication::setCursorFlashTime(int msecs)
{
	QApplicationPrivate::cursor_flash_time = msecs;
}

int QApplication::cursorFlashTime()
{
	return QApplicationPrivate::cursor_flash_time;
}

QWidget *QApplication::topLevelAt(const QPoint &point)
{
#ifdef QT_NO_CURSOR
    Q_UNUSED(p);
    return 0;
#else
    //int screen = QCursor::pfScreen();
    int unused;
	int rv;

    int x = point.x();
    int y = point.y();
    gi_window_id_t target;
    if (!gi_translate_coordinates(
                               GI_DESKTOP_WINDOW_ID,
                               GI_DESKTOP_WINDOW_ID,
                               x, y, &unused, &unused, &target)) {
        return 0;
    }
    if (!target || target == GI_DESKTOP_WINDOW_ID)
        return 0;
    QWidget *w;
	gi_window_info_t info;
    w = QWidget::find((WId)target);

    while (!w && target && target != GI_DESKTOP_WINDOW_ID) {
		rv = gi_get_window_info(target, &info);
		if (rv < 0)
			break;
        target = info.parent;
        w = QWidget::find(target);
    }

    return w ? w->window() : 0;
#endif
}

void QApplication::beep()
{
}

void QApplication::alert(QWidget *widget, int duration)
{
}

void QApplicationPrivate::initializeMultitouch_sys()
{
}

void QApplicationPrivate::cleanupMultitouch_sys()
{
}


/*****************************************************************************
  Event translation; translates X11 events to Qt events
 *****************************************************************************/

//
// Mouse event translation
//
// Xlib doesn't give mouse double click events, so we generate them by
// comparing window, time and position between two mouse press events.
//

static Qt::MouseButtons translateMouseButtons(int s)
{
    Qt::MouseButtons ret = 0;
    if (s & GI_BUTTON_L)
        ret |= Qt::LeftButton;
    if (s & GI_BUTTON_M)
        ret |= Qt::MidButton;
    if (s & GI_BUTTON_R)
        ret |= Qt::RightButton;
    return ret;
}

Qt::KeyboardModifiers translateModifiers(int s)
{
    Qt::KeyboardModifiers ret = 0;
    if (s & G_MODIFIERS_SHIFT)
        ret |= Qt::ShiftModifier;
    if (s & G_MODIFIERS_CTRL)
        ret |= Qt::ControlModifier;
    if (s & G_MODIFIERS_ALT)
        ret |= Qt::AltModifier;
    if (s & G_MODIFIERS_META)
        ret |= Qt::MetaModifier;
    //if (s & qt_mode_switch_mask)
    //    ret |= Qt::GroupSwitchModifier;
    return ret;
}

bool QETWidget::translateMouseEvent(const gi_msg_t *event)
{
    if (!isWindow() && testAttribute(Qt::WA_NativeWindow))
        Q_ASSERT(internalWinId());

    Q_D(QWidget);
    QEvent::Type type;                                // event parameters
    QPoint pos;
    QPoint globalPos;
    Qt::MouseButton button = Qt::NoButton;
    Qt::MouseButtons buttons = 0;
    Qt::KeyboardModifiers modifiers;
    gi_msg_t nextEvent;
  
    if (event->type == GI_MSG_MOUSE_MOVE) { // mouse move
        
		gi_msg_t lastMotion = *event;
       
        type = QEvent::MouseMove;
        pos.rx() = lastMotion.body.rect.x;
        pos.ry() = lastMotion.body.rect.y;
        pos = d->mapFromWS(pos);
        globalPos.rx() = lastMotion.params[0];
        globalPos.ry() = lastMotion.params[1];
        buttons = translateMouseButtons(lastMotion.params[2]);
        modifiers = translateModifiers(lastMotion.params[3]);
        if (qt_button_down && !buttons)
            qt_button_down = 0;
    } else if (event->type == GI_MSG_MOUSE_ENTER 
		|| event->type == GI_MSG_MOUSE_EXIT) {
        gi_msg_t *xevent = (gi_msg_t *)event;
        //unsigned int xstate = event->xcrossing.state;
        type = QEvent::MouseMove;
        pos.rx() = xevent->body.rect.x;
        pos.ry() = xevent->body.rect.y;
        pos = d->mapFromWS(pos);
        globalPos.rx() = xevent->params[0];
        globalPos.ry() = xevent->params[1];
		#if 1
        buttons = translateMouseButtons(xevent->params[2]);
		#endif
        //modifiers = translateModifiers(xevent->xcrossing.state);
        if (qt_button_down && !buttons)
            qt_button_down = 0;
        if (qt_button_down)
            return true;
    } else {                                        // button press or release
        pos.rx() = event->body.rect.x;
        pos.ry() = event->body.rect.y;
        pos = d->mapFromWS(pos);
        globalPos.rx() = event->params[0];
        globalPos.ry() = event->params[1];
		int xbutton = event->type == 
			GI_MSG_BUTTON_UP? event->params[3]:event->params[2];

        buttons = translateMouseButtons(xbutton);
        modifiers = translateModifiers(event->params[3]);		

        switch (xbutton) {
        case GI_BUTTON_L: button = Qt::LeftButton; break;
        case GI_BUTTON_M: button = Qt::MidButton; break;
        case GI_BUTTON_R: button = Qt::RightButton; break;
        case GI_BUTTON_WHEEL_UP:
        case GI_BUTTON_WHEEL_DOWN:
        case GI_BUTTON_WHEEL_LEFT:
        case GI_BUTTON_WHEEL_RIGHT:
            // the fancy mouse wheel.

            // We are only interested in GI_MSG_BUTTON_DOWN.
            if (event->type == GI_MSG_BUTTON_DOWN){                
                int delta = 1;                

                // the delta is defined as multiples of
                // WHEEL_DELTA, which is set to 120. Future wheels
                // may offer a finer-resolution. A positive delta
                // indicates forward rotation, a negative one
                // backward rotation respectively.
                int btn = event->params[2];
                delta *= 120 * ((btn == GI_BUTTON_WHEEL_UP || btn == 6) ? 1 : -1);
                bool hor = (((btn == GI_BUTTON_WHEEL_UP || btn == GI_BUTTON_WHEEL_DOWN) && (modifiers & Qt::AltModifier)) ||
                            (btn == 6 || btn == 7));
                translateWheelEvent(globalPos.x(), globalPos.y(), delta, buttons,
                                    modifiers, (hor) ? Qt::Horizontal: Qt::Vertical);
            }
            return true;
        //case 8: button = Qt::XButton1; break;
        //case 9: button = Qt::XButton2; break;
        }
        if (event->type == GI_MSG_BUTTON_DOWN) {        // mouse button pressed
            buttons |= button;

            if (!qt_button_down) {
                qt_button_down = childAt(pos);        //magic for masked widgets
                if (!qt_button_down)
                    qt_button_down = this;
            }
            if (mouseActWindow == event->ev_window &&
                mouseButtonPressed == button &&
                (long)event->time -(long)mouseButtonPressTime
                < QApplication::doubleClickInterval() &&
                qAbs(event->body.rect.x - mouseXPos) < QT_GUI_DOUBLE_CLICK_RADIUS &&
                qAbs(event->body.rect.y - mouseYPos) < QT_GUI_DOUBLE_CLICK_RADIUS) {
                type = QEvent::MouseButtonDblClick;
                mouseButtonPressTime -= 2000;        // no double-click next time
            } else {
                type = QEvent::MouseButtonPress;
                mouseButtonPressTime = event->time;
            }
            mouseButtonPressed = button;        // save event params for
            mouseXPos = event->body.rect.x;                // future double click tests
            mouseYPos = event->body.rect.y;
            mouseGlobalXPos = globalPos.x();
            mouseGlobalYPos = globalPos.y();
        } else {                                // mouse button released
            buttons &= ~button;

            type = QEvent::MouseButtonRelease;
        }
    }
    mouseActWindow = effectiveWinId();                        // save some event params
    mouseButtonState = buttons;
    if (type == 0)                                // don't send event
        return false;

    if (qApp->d_func()->inPopupMode()) {                        // in popup mode
        QWidget *activePopupWidget = qApp->activePopupWidget();
        QWidget *popup = qApp->activePopupWidget();
        if (popup != this) {
            if (event->type == GI_MSG_MOUSE_EXIT)
                return false;
            if ((windowType() == Qt::Popup) && rect().contains(pos) && 0)
                popup = this;
            else                                // send to last popup
                pos = popup->mapFromGlobal(globalPos);
        }
        bool releaseAfter = false;
        QWidget *popupChild  = popup->childAt(pos);

        if (popup != qt_popup_down){
            qt_button_down = 0;
            qt_popup_down = 0;
        }

        switch (type) {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonDblClick:
            qt_button_down = popupChild;
            qt_popup_down = popup;
            break;
        case QEvent::MouseButtonRelease:
            releaseAfter = true;
            break;
        default:
            break;                                // nothing for mouse move
        }

        int oldOpenPopupCount = openPopupCount;

        if (popup->isEnabled()) {
            // deliver event
            replayPopupMouseEvent = false;
            QWidget *receiver = popup;
            QPoint widgetPos = pos;
            if (qt_button_down)
                receiver = qt_button_down;
            else if (popupChild)
                receiver = popupChild;
            if (receiver != popup)
                widgetPos = receiver->mapFromGlobal(globalPos);
            QWidget *alien = childAt(mapFromGlobal(globalPos));
            QMouseEvent e(type, widgetPos, globalPos, button, buttons, modifiers);
            QApplicationPrivate::sendMouseEvent(receiver, &e, alien, this, &qt_button_down, qt_last_mouse_receiver);
        } else {
            // close disabled popups when a mouse button is pressed or released
            switch (type) {
            case QEvent::MouseButtonPress:
            case QEvent::MouseButtonDblClick:
            case QEvent::MouseButtonRelease:
                popup->close();
                break;
            default:
                break;
            }
        }

        if (qApp->activePopupWidget() != activePopupWidget
            && replayPopupMouseEvent) {
            // the active popup was closed, replay the mouse event
            if (!(windowType() == Qt::Popup)) {
                qt_button_down = 0;
            }
            replayPopupMouseEvent = false;
        } else if (type == QEvent::MouseButtonPress
                   && button == Qt::RightButton
                   && (openPopupCount == oldOpenPopupCount)) {
            QWidget *popupEvent = popup;
            if (qt_button_down)
                popupEvent = qt_button_down;
            else if(popupChild)
                popupEvent = popupChild;
            QContextMenuEvent e(QContextMenuEvent::Mouse, pos, globalPos, modifiers);
            QApplication::sendSpontaneousEvent(popupEvent, &e);
        }

        if (releaseAfter) {
            qt_button_down = 0;
            qt_popup_down = 0;
        }
    } else {
        QWidget *alienWidget = childAt(pos);
        QWidget *widget = QApplicationPrivate::pickMouseReceiver(this, globalPos, pos, type, buttons,
                                                                 qt_button_down, alienWidget);
        if (!widget) {
            if (type == QEvent::MouseButtonRelease)
                QApplicationPrivate::mouse_buttons &= ~button;
            return false; // don't send event
        }

        int oldOpenPopupCount = openPopupCount;
        QMouseEvent e(type, pos, globalPos, button, buttons, modifiers);
        QApplicationPrivate::sendMouseEvent(widget, &e, alienWidget, this, &qt_button_down,
                                            qt_last_mouse_receiver);
        if (type == QEvent::MouseButtonPress
            && button == Qt::RightButton
            && (openPopupCount == oldOpenPopupCount)) {
            QContextMenuEvent e(QContextMenuEvent::Mouse, pos, globalPos, modifiers);
            QApplication::sendSpontaneousEvent(widget, &e);
        }
    }
    return true;
}


//
// Wheel event translation
//
bool QETWidget::translateWheelEvent(int global_x, int global_y, int delta,
                                    Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers,
                                    Qt::Orientation orient)
{
    const QPoint globalPos = QPoint(global_x, global_y);
    QPoint pos = mapFromGlobal(globalPos);
    QWidget *widget = childAt(pos);
    if (!widget)
        widget = this;
    else if (!widget->internalWinId())
        pos = widget->mapFromGlobal(globalPos);

#ifdef ALIEN_DEBUG
        qDebug() << "QETWidget::translateWheelEvent: receiver:" << widget << "pos:" << pos;
#endif

    // send the event to the widget or its ancestors
    {
        QWidget* popup = qApp->activePopupWidget();
        if (popup && window() != popup)
            popup->close();
#ifndef QT_NO_WHEELEVENT
        QWheelEvent e(pos, globalPos, delta, buttons, modifiers, orient);
        if (QApplication::sendSpontaneousEvent(widget, &e))
#endif
            return true;
    }

    // send the event to the widget that has the focus or its ancestors, if different
    if (widget != qApp->focusWidget() && (widget = qApp->focusWidget())) {
        if (widget && !widget->internalWinId())
            pos = widget->mapFromGlobal(globalPos);
        QWidget* popup = qApp->activePopupWidget();
        if (popup && widget != popup)
            popup->hide();
#ifndef QT_NO_WHEELEVENT
        QWheelEvent e(pos, globalPos, delta, buttons, modifiers, orient);
        if (QApplication::sendSpontaneousEvent(widget, &e))
#endif
        return true;
    }
    return false;
}



bool QETWidget::translatePropertyEvent(const gi_msg_t *event)
{
    Q_D(QWidget);
    if (!isWindow()) return true;

    return true;
}



//
// GI_MSG_CONFIGURENOTIFY (window move and resize) event translation

bool QETWidget::translateConfigEvent(const gi_msg_t *event)
{
	if (event->params[0] > GI_STRUCT_CHANGE_RESIZE){		
		return true;
	}
	else{
		
	}

    if (!testAttribute(Qt::WA_WState_Created)){ // in QWidget::create()
        return true;
	}
    if (testAttribute(Qt::WA_WState_ConfigPending)){
        return true;
	}
    if (testAttribute(Qt::WA_DontShowOnScreen)){
        return true;
	}

    // @todo there are other isWindow() checks below (same in Windows code).
    // Either they or this return statement are leftovers. The assertion may
    // tell the truth.
    Q_ASSERT(isWindow());
    if (!isWindow()){
        return true;
	}
	if (isMinimized()){		
        return true; 
	}

    setAttribute(Qt::WA_WState_ConfigPending);                // set config flag

	if (event->params[0] == GI_STRUCT_CHANGE_MOVE || event->params[0] == GI_STRUCT_CHANGE_RESIZE) {        // move event
        QPoint oldPos = geometry().topLeft();

        int a = (int) event->body.rect.x;
        int b = (int) event->body.rect.y;
        QPoint newCPos(a, b);

		if (newCPos != oldPos) {
            data->crect.moveTopLeft(newCPos);
            if (isVisible()) {
                QMoveEvent e(newCPos, oldPos); // cpos (client position)
                QApplication::sendSpontaneousEvent(this, &e);
            } else {
                QMoveEvent *e = new QMoveEvent(newCPos, oldPos);
                QApplication::postEvent(this, e);
            }
        }        

    }
    
    if (event->params[0] == GI_STRUCT_CHANGE_RESIZE) {                // resize event
	 QSize oldSize = data->crect.size();
        QSize newSize(event->body.rect.w, event->body.rect.h);
		data->crect.setSize(newSize);
		
        if (isWindow()) {                        // update title/icon text
            d_func()->createTLExtra();
            /*QString title;
            //if ((fStyle & WS_MINIMIZED))
                title = windowIconText();
            if (title.isEmpty())
                title = windowTitle();
            if (!title.isEmpty())
                d_func()->setWindowTitle_helper(title);
				*/
        }
        if (oldSize != newSize) {
            // Spontaneous (external to Qt) WM_SIZE messages should occur only
            // on top-level widgets. If we get them for a non top-level widget,
            // the result will most likely be incorrect because widget masks will
            // not be properly processed (i.e. in the way it is done in
            // QWidget::setGeometry_sys() when the geometry is changed from
            // within Qt). So far, I see no need to support this (who will ever
            // need to move a non top-level window of a foreign process?).
            Q_ASSERT(isWindow());
			
            if (isVisible()) {
                QTLWExtra *tlwExtra = d_func()->maybeTopData();
                static bool slowResize = qgetenv("QT_SLOW_TOPLEVEL_RESIZE").toInt();
                const bool hasStaticContents = tlwExtra && tlwExtra->backingStore
                                               && tlwExtra->backingStore->hasStaticContents();
                // If we have a backing store with static contents, we have to disable the top-level
                // resize optimization in order to get invalidated regions for resized widgets.
                // The optimization discards all invalidateBuffer() calls since we're going to
                // repaint everything anyways, but that's not the case with static contents.
                if (!slowResize && tlwExtra && !hasStaticContents)
                    tlwExtra->inTopLevelResize = true;
                QResizeEvent e(newSize, oldSize);
                QApplication::sendSpontaneousEvent(this, &e);
                if (d_func()->paintOnScreen()) {
                    QRegion updateRegion(rect());
                    if (testAttribute(Qt::WA_StaticContents))
                        updateRegion -= QRect(0, 0, oldSize.width(), oldSize.height());
                    // syncBackingStore() should have already flushed the widget
                    // contents to the screen, so no need to redraw the exposed
                    // areas in WM_PAINT once more
                    d_func()->syncBackingStore(updateRegion);
					//gi_clear_area(internalWinId(),0, 0, newSize.width(), newSize.height(),TRUE);
                    //WinValidateRegion(internalWinId(),
                     //                 updateRegion.handle(newSize.height()), FALSE);
                } else {
                    d_func()->syncBackingStore();
					gi_clear_area(internalWinId(),0, 0, newSize.width(), newSize.height(),TRUE);
                    // see above
                    //RECTL rcl = { 0, 0, newSize.width(), newSize.height() };
                    //WinValidateRect(internalWinId(), &rcl, FALSE);
                }
                if (!slowResize && tlwExtra)
                    tlwExtra->inTopLevelResize = false;
            } else {
                QResizeEvent *e = new QResizeEvent(newSize, oldSize);
                QApplication::postEvent(this, e);
            }
        }

    }
	
    setAttribute(Qt::WA_WState_ConfigPending, false);                // clear config flag
    return true;
}

//
// Close window event translation.
//
bool QETWidget::translateCloseEvent(const gi_msg_t *)
{
    Q_D(QWidget);
    return d->close_helper(QWidgetPrivate::CloseWithSpontaneousEvent);
}


int QApplication::pfClientMessage(QWidget* w, gi_msg_t* event, bool passive_only)
{
    if (w && !w->internalWinId())
        return 0;

    QETWidget *widget = (QETWidget*)w;

	if (event->body.client.client_type == GA_WM_PROTOCOLS) {
		gi_atom_id_t a = event->params[0];
		if (a == GA_WM_DELETE_WINDOW ) {
			if (passive_only) return 0;
			widget->translateCloseEvent(event);
		}
            
	}
	//else if (gi_xdnd_client_handler(event)){
	//	return 1;
	//}
	else{
		return 0;
	}
    return 1;
}



void QApplication::winFocus(QWidget *widget, bool gotFocus)
{
    if (d_func()->inPopupMode()) // some delayed focus event to ignore
        return;
    if (gotFocus) {
        setActiveWindow(widget);
        if (QApplicationPrivate::active_window
            && (QApplicationPrivate::active_window->windowType() == Qt::Dialog)) {
            // raise the entire application, not just the dialog
            QWidget* mw = QApplicationPrivate::active_window;

            while(mw->parentWidget() && (mw->windowType() == Qt::Dialog))
                mw = mw->parentWidget()->window();
            if (mw->testAttribute(Qt::WA_WState_Created) 
				&& mw != QApplicationPrivate::active_window){
				gi_wm_activate_window( mw->effectiveWinId());
                //SetWindowPos(mw->internalWinId(), HWND_TOP, 
				//	0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
			}

        }
    } else {
        setActiveWindow(0);
    }
}



int QApplication::pfProcessEvent(gi_msg_t* event)
{
    Q_D(QApplication);
    QScopedLoopLevelCounter loopLevelCounter(d->threadData);

#ifdef ALIEN_DEBUG
    //qDebug() << "QApplication::pfProcessEvent:" << event->type;
#endif

    QETWidget *widget = (QETWidget*)QWidget::find((WId)event->ev_window);	

    QETWidget *keywidget=0;
    bool grabbed=false;
    if (event->type==GI_MSG_KEY_DOWN || event->type==GI_MSG_KEY_UP) {
        keywidget = (QETWidget*)QWidget::keyboardGrabber();
        if (keywidget) {
            grabbed = true;
        } else if (!keywidget) {
            if (d->inPopupMode()) // no focus widget, see if we have a popup
                keywidget = (QETWidget*) (activePopupWidget()->focusWidget() ? activePopupWidget()->focusWidget() : activePopupWidget());
            else if (QApplicationPrivate::focus_widget)
                keywidget = (QETWidget*)QApplicationPrivate::focus_widget;
            else if (widget)
                keywidget = (QETWidget*)widget->window();
        }
    }
    
    if (qt_pfEventFilter(event))                // send through app filter
        return 1;

	long res = 0;

	res = 0;
	if (!widget){
		//qDebug() << "QApplication::pfProcessEvent: window " << event->ev_window << " Not found";
	}
    else if(widget->pfWinEvent(event, &res))                // send through widget filter
        return (int)(res);
	else{
	}

    if (event->type == GI_MSG_KEYMAPNOTIFY) {
        // keyboard mapping changed
        //XRefreshKeyboardMapping(&event->xmapping);

        QKeyMapper::changeKeyboard();
        return 0;
    }


    if (!widget) {                                // don't know this windows
        QWidget* popup = QApplication::activePopupWidget();
        if (popup) {

            /*
              That is more than suboptimal. The real solution should
              do some keyevent and buttonevent translation, so that
              the popup still continues to work as the user expects.
              Unfortunately this translation is currently only
              possible with a known widget. I'll change that soon
              (Matthias).
            */

            // Danger - make sure we don't lock the server
            switch (event->type) {
            case GI_MSG_BUTTON_DOWN:
            case GI_MSG_BUTTON_UP:
            case GI_MSG_KEY_DOWN:
            case GI_MSG_KEY_UP:
                do {
                    popup->close();
                } while ((popup = qApp->activePopupWidget()));
                return 1;
            }
        }
        return -1;
    }

    if (event->type == GI_MSG_KEY_DOWN || event->type == GI_MSG_KEY_UP)
        widget = keywidget; // send XKeyEvents through keywidget->pfEvent()

    if (app_do_modal)                                // modal event handling
        if (!qt_try_modal(widget, event)) {
            if (event->type == GI_MSG_CLIENT_MSG )//&& !widget->pfEvent(event))
                pfClientMessage(widget, event, true);
            return 1;
        }

    switch (event->type) {

    case GI_MSG_BUTTON_UP:                        // mouse event
        if (!d->inPopupMode() && !QWidget::mouseGrabber() && pressed_window != widget->internalWinId()
            && (widget = (QETWidget*) QWidget::find((WId)pressed_window)) == 0)
            break;
        // fall through intended
    case GI_MSG_BUTTON_DOWN:
		pressed_window = event->ev_window;        
        //if (event->type == GI_MSG_BUTTON_DOWN)
            //qt_net_update_user_time(widget->window(), X11->userTime);
        // fall through intended
    case GI_MSG_MOUSE_MOVE:

            if (widget->testAttribute(Qt::WA_TransparentForMouseEvents)) {
                QPoint pos(event->body.rect.x, event->body.rect.y);
                pos = widget->d_func()->mapFromWS(pos);
                QWidget *window = widget->window();
                pos = widget->mapTo(window, pos);
                if (QWidget *child = window->childAt(pos)) {
                    widget = static_cast<QETWidget *>(child);
                    pos = child->mapFrom(window, pos);
                    event->body.rect.x = pos.x();
                    event->body.rect.y = pos.y();
                }
            }
            widget->translateMouseEvent(event);
        break;

    case GI_MSG_KEY_DOWN:                                // keyboard event
        //qt_net_update_user_time(widget->window(), X11->userTime);
        // fallthrough intended
    case GI_MSG_KEY_UP:
        {
            if (keywidget && keywidget->isEnabled()) { // should always exist
                // qDebug("sending key event");
                 bool result = qt_keymapper_private()->translateKeyEvent(keywidget, event, grabbed);
            }
            break;
        }

    case GI_MSG_GRAPHICSEXPOSURE:
    case GI_MSG_EXPOSURE:                                // paint event
        widget->translatePaintEvent(event);
        break;

	case GI_MSG_WINDOW_DESTROY: 
		//return 0;
		break;

    case GI_MSG_CONFIGURENOTIFY:                        // window move/resize event	
            widget->translateConfigEvent(event);
        break;

    case GI_MSG_FOCUS_IN: {		
		gi_composition_form_t attr;		

		attr.x = 0;//winfo.x  + widget->rect.x;
		attr.y = 0;//winfo.y + 50 + widget->rect.y;
		
		gi_ime_associate_window(event->ev_window, &attr);
		// got focus
        if ((widget->windowType() == Qt::Desktop))
            break;
        if (d->inPopupMode()) // some delayed focus event to ignore
            break;
        if (!widget->isWindow())
            break;
        
#if 1		
        setActiveWindow(widget);
#else
		qApp->winFocus(widget, true);
 #endif		
       
    }
        break;

    case GI_MSG_FOCUS_OUT: 
		
	//if (event->effect_window == event->ev_window)
	//	 return true;
		
	gi_ime_associate_window(event->ev_window, NULL);
	
#if 1		
	// lost focus
        if ((widget->windowType() == Qt::Desktop))
            break;
        if (!widget->isWindow())
            break;
        
        if (!d->inPopupMode() && widget == QApplicationPrivate::active_window) {
            bool focus_will_change = false;
			/*gi_msg_t msg;

			if (gi_check_typed_message(&msg,0,GI_MASK_FOCUS_OUT) <= 0)
			{
				focus_will_change = true;
			}
			*/

			focus_will_change = true;

            if (!focus_will_change)
                setActiveWindow(0);
        }
#else
		if (widget && !widget->isWindow())
           qApp->winFocus(widget, false);
#endif
        break;

#if 1 //FIXME DPP
    case GI_MSG_MOUSE_ENTER: {                        // enter window
        if (QWidget::mouseGrabber() && (!d->inPopupMode() || widget->window() != activePopupWidget()))
            break;      
		
        if (qt_button_down && !d->inPopupMode())
            break;

		QPoint gpos;
		gpos = QPoint(event->body.rect.x, event->body.rect.y);
		//gpos = QCursor::pos();

        QWidget *alien = widget->childAt(
			widget->d_func()->mapFromWS(gpos));
        QWidget *enter = alien ? alien : widget;
        QWidget *leave = 0;
        if (qt_last_mouse_receiver && !qt_last_mouse_receiver->internalWinId())
            leave = qt_last_mouse_receiver;
        else
            leave = QWidget::find(curWin);

        // ### Alien: enter/leave might be wrong here with overlapping siblings
        // if the enter widget is native and stacked under a non-native widget.
        QApplicationPrivate::dispatchEnterLeave(enter, leave);
        curWin = widget->internalWinId();
        qt_last_mouse_receiver = enter;
        if (!d->inPopupMode() || widget->window() == activePopupWidget())
            widget->translateMouseEvent(event); //we don't get GI_MSG_MOUSE_MOVE, emulate it
    }
        break;

    case GI_MSG_MOUSE_EXIT: {                        // leave window
        QWidget *mouseGrabber = QWidget::mouseGrabber();
        if (mouseGrabber && !d->inPopupMode())
            break;
        if (curWin && widget->internalWinId() != curWin)
            break;
        
        if (!(widget->windowType() == Qt::Desktop))
            widget->translateMouseEvent(event); //we don't get GI_MSG_MOUSE_MOVE, emulate it

        QWidget* enter = 0;
        QPoint enterPoint;

        
        if (qt_button_down && !d->inPopupMode())
            break;

        if (!curWin)
            QApplicationPrivate::dispatchEnterLeave(widget, 0);

        if (enter) {
            QWidget *alienEnter = enter->childAt(enterPoint);
            if (alienEnter)
                enter = alienEnter;
        }

        QWidget *leave = qt_last_mouse_receiver ? qt_last_mouse_receiver : widget;
        QWidget *activePopupWidget = qApp->activePopupWidget();

		QPoint gpos;
		gpos = QPoint(event->body.rect.x, event->body.rect.y);
		//gpos = QCursor::pos();

        if (mouseGrabber && activePopupWidget && leave == activePopupWidget)
            enter = mouseGrabber;
        else if (enter != widget && mouseGrabber) {
            if (!widget->rect().contains(widget->d_func()->mapFromWS(gpos)))
                break;
        }

        QApplicationPrivate::dispatchEnterLeave(enter, leave);
        qt_last_mouse_receiver = enter;
		gi_msg_t ev;

        if (enter && QApplicationPrivate::tryModalHelper(enter, 0)) {
            QWidget *nativeEnter = enter->internalWinId() ? enter : enter->nativeParentWidget();
            curWin = nativeEnter->internalWinId();
            static_cast<QETWidget *>(nativeEnter)->translateMouseEvent(&ev); //we don't get GI_MSG_MOUSE_MOVE, emulate it
        } else {
            curWin = 0;
            qt_last_mouse_receiver = 0;
        }
    }
        break;
#endif

    case GI_MSG_WINDOW_HIDE:                                // window hidden
        if (widget->isWindow()) {
            Q_ASSERT(widget->testAttribute(Qt::WA_WState_Created));

            if (widget->windowType() != Qt::Popup && !widget->testAttribute(Qt::WA_DontShowOnScreen)) {
                widget->setAttribute(Qt::WA_Mapped, false);
                if (widget->isVisible()) {
                    //widget->d_func()->topData()->spont_unmapped = 1;
                    QHideEvent e;
                    QApplication::sendSpontaneousEvent(widget, &e);
                    widget->d_func()->hideChildren(true);
                }
            }
        }
        break;

    case GI_MSG_WINDOW_SHOW:                                // window shown
        if (widget->isWindow()) {
            // if we got a MapNotify when we were not waiting for it, it most
            // likely means the user has already asked to hide the window before
            // it ever being shown, so we try to withdraw a window after sending
            // the QShowEvent.
            bool pendingHide = widget->testAttribute(Qt::WA_WState_ExplicitShowHide) && widget->testAttribute(Qt::WA_WState_Hidden);

            if (widget->windowType() != Qt::Popup) {
                widget->setAttribute(Qt::WA_Mapped);
                //if (widget->d_func()->topData()->spont_unmapped) {
                    //widget->d_func()->topData()->spont_unmapped = 0;
                    widget->d_func()->showChildren(true);
                    QShowEvent e;
                    QApplication::sendSpontaneousEvent(widget, &e);

                    // show() must have been called on this widget in
                    // order to reach this point, but we could have
                    // cleared these 2 attributes in case something
                    // previously forced us into WithdrawnState
                    // (e.g. kdocker)
                    widget->setAttribute(Qt::WA_WState_ExplicitShowHide, true);
                    widget->setAttribute(Qt::WA_WState_Visible, true);
                //}
            }
            if (pendingHide) // hide the window
                gi_hide_window( widget->internalWinId());
                //XWithdrawWindow( widget->internalWinId());
        }
        break;

    case GI_MSG_CLIENT_MSG:                        // client message
		{
			return pfClientMessage(widget,event,FALSE);
		}

    case GI_MSG_REPARENT: {                      // window manager reparents	
        break;
    }



	case GI_MSG_SELECTIONNOTIFY: {
		//gi_xdnd_selection_handler(event);
	  break;
	}

#if 0
    case GI_MSG_SELECTIONNOTIFY: {
        //XSelectionRequestEvent *req = &event->xselectionrequest;
        if (! req)
            break;

        if (ATOM(XdndSelection) && req->selection == ATOM(XdndSelection)) {
            X11->xdndHandleSelectionRequest(req);

        } else if (qt_clipboard) {
            QClipboardEvent e(reinterpret_cast<QEventPrivate*>(event));
            QApplication::sendSpontaneousEvent(qt_clipboard, &e);
        }
        break;
    }
    case SelectionClear: {
        XSelectionClearEvent *req = &event->xselectionclear;
        // don't deliver dnd events to the clipboard, it gets confused
        if (! req || (ATOM(XdndSelection) && req->selection == ATOM(XdndSelection)))
            break;

        if (qt_clipboard && !qt_pfData->use_xfixes) {
            QClipboardEvent e(reinterpret_cast<QEventPrivate*>(event));
            QApplication::sendSpontaneousEvent(qt_clipboard, &e);
        }
        break;
    }

    case SelectionNotify: {
        XSelectionEvent *req = &event->xselection;
        // don't deliver dnd events to the clipboard, it gets confused
        if (! req || (ATOM(XdndSelection) && req->selection == ATOM(XdndSelection)))
            break;

        if (qt_clipboard) {
            QClipboardEvent e(reinterpret_cast<QEventPrivate*>(event));
            QApplication::sendSpontaneousEvent(qt_clipboard, &e);
        }
        break;
    }
#endif

	case GI_MSG_CREATENOTIFY:
		break;

    case GI_MSG_PROPERTYNOTIFY:
        // some properties changed
        if (event->ev_window == GI_DESKTOP_WINDOW_ID) {
            // root properties for the first screen
            /*if (!qt_pfData->use_xfixes && event->xproperty.atom == ATOM(_QT_CLIPBOARD_SENTINEL)) {
                if (qt_check_clipboard_sentinel()) {
                    emit clipboard()->changed(QClipboard::Clipboard);
                    emit clipboard()->dataChanged();
                }
            } else if (!qt_pfData->use_xfixes && event->xproperty.atom == ATOM(_QT_SELECTION_SENTINEL)) {
                if (qt_check_selection_sentinel()) {
                    emit clipboard()->changed(QClipboard::Selection);
                    emit clipboard()->selectionChanged();
                }
            } else if (QApplicationPrivate::obey_desktop_settings) {
                if (event->xproperty.atom == ATOM(RESOURCE_MANAGER))
                    qt_set_gix_resources();
                else if (event->xproperty.atom == ATOM(_QT_SETTINGS_TIMESTAMP))
                    qt_set_gix_resources();
            }*/
        }
         else if (widget) {
            widget->translatePropertyEvent(event);
        }  else {
            return -1; // don't know this window
        }
        break;

    default:
		printf("%s: %s() got line %d unknow event %d\n",
		__FILE__,__FUNCTION__,__LINE__, event->type);
        break;
    }

    return 0;
}




void QETWidget::translatePaintEvent(const gi_msg_t *event)
{
    Q_D(QWidget);
    if (!isWindow() && testAttribute(Qt::WA_NativeWindow))
        Q_ASSERT(internalWinId());

    Q_ASSERT(testAttribute(Qt::WA_WState_Created));
    setAttribute(Qt::WA_PendingUpdate, false);

    QRect  paintRect(event->body.rect.x, event->body.rect.y,
                     event->body.rect.w, event->body.rect.h);
    

	paintRect.translate(data->wrect.topLeft());

    //if (!paintRect.isEmpty() && !testAttribute(Qt::WA_WState_ConfigPending))
        d_func()->syncBackingStore(paintRect);

	//d_func()->hd = 0;
}



