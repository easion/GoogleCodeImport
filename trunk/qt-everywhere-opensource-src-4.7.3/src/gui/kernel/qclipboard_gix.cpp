
 //#define QCLIPBOARD_DEBUG
 //#define QCLIPBOARD_DEBUG_VERBOSE

#ifdef QCLIPBOARD_DEBUG
#  define DEBUG qDebug
#else
#  define DEBUG if (false) qDebug
#endif

#ifdef QCLIPBOARD_DEBUG_VERBOSE
#  define VDEBUG qDebug
#else
#  define VDEBUG if (false) qDebug
#endif

#include "qplatformdefs.h"

#include "qclipboard.h"
#include "qclipboard_p.h"

#ifndef QT_NO_CLIPBOARD

#include "qabstracteventdispatcher.h"
#include "qapplication.h"
#include "qdesktopwidget.h"
#include "qbitmap.h"
#include "qiodevice.h"
#include "qbuffer.h"
#include "qtextcodec.h"
#include "qlist.h"
#include "qmap.h"
#include "qapplication_p.h"
#include "qevent.h"
//#include "qt_x11_p.h"
//#include "qx11info_x11.h"
#include "qimagewriter.h"
#include "qelapsedtimer.h"
#include "qvariant.h"
#include "qdnd_p.h"
#include <private/qwidget_p.h>
#include "qwidget_p.h"


QT_BEGIN_NAMESPACE

const int xdnd_version = 5;
bool markWidgetFlags(QWidget* w, long flags, bool set);
#define Q_WIDGET_WITH_DND_FLAGS 0x10

QByteArray xdndAtomToString(gi_atom_id_t a)
{
    if (!a) return 0;

    if (a == GA_STRING || a == ATOM(UTF8_STRING)) {
        return "text/plain"; // some Xdnd clients are dumb
    }
    char *atom = gi_get_atom_name( a);
    QByteArray result = atom;
    free(atom);
    return result;
}

gi_atom_id_t xdndMimeStringToAtom(const QString &mimeType)
{
    if (mimeType.isEmpty())
        return 0;
    return gi_intern_atom( mimeType.toLatin1().constData(), FALSE);
}



static gi_bool_t
checkForClipboardEvents(gi_window_id_t wid, gi_event_mask_t mask,
	 gi_msg_t *e, void *arg)
{
  if (e->type != GI_MSG_SELECTIONNOTIFY)
    return FALSE;

  if(gi_message_selection_subtype(e) == G_SEL_REQUEST){
	  if (gi_message_request_selection_selection(e) == GA_PRIMARY 
		  || gi_message_request_selection_selection(e) == ATOM(CLIPBOARD) ){
		  return TRUE;
	  }
  }
  if (gi_message_selection_subtype(e) == G_SEL_CLEAR){
	  if (gi_message_clear_selection_property(e) == GA_PRIMARY 
		  || gi_message_clear_selection_property(e) == ATOM(CLIPBOARD) ){
		  return TRUE;
	  }
  }
  return FALSE;

  /*return ((e->type == SelectionRequest && (e->xselectionrequest.selection == GA_PRIMARY
                                             || e->xselectionrequest.selection == ATOM(CLIPBOARD)))
            || (e->type == SelectionClear && (e->xselectionclear.selection == GA_PRIMARY
                                             || e->xselectionclear.selection == ATOM(CLIPBOARD))));
 */
}


gi_bool_t
XCheckIfEvent( gi_msg_t * ev, g_if_event_callback predicate, void* arg)
{
	//return _XIfEvent( ev, predicate, arg, 0);
	return gi_get_typed_event_pred(0, 0,  ev, 0, predicate, NULL);
}

//fixme dpp
bool clipboardWaitForEvent(gi_window_id_t win, unsigned type, gi_msg_t *event, int timeout)
{
    QElapsedTimer started;
    started.start();
    QElapsedTimer now = started;

	VDEBUG("clipboardWaitForEvent: type = %d, timeout = %d time = %p...",type,timeout,time(NULL));
	do {
		if (gi_check_typed_message(event,win,type))
			return true;

		// process other clipboard events, since someone is probably requesting data from us
		gi_msg_t e;
		if (XCheckIfEvent( &e, checkForClipboardEvents, 0))
			qApp->pfProcessEvent(&e);

		now.start();

		// sleep 50 ms, so we don't use up CPU cycles all the time.
		struct timeval usleep_tv;
		usleep_tv.tv_sec = 0;
		usleep_tv.tv_usec = 50000;
		select(0, 0, 0, 0, &usleep_tv);
	} while (started.msecsTo(now) < timeout);
	VDEBUG("clipboardWaitForEvent: type = %d, timeout = %d time = %p leave",type,timeout,time(NULL));

	return false;
}

//$$$ middle of xdndObtainData
gi_atom_id_t xdndMimeAtomForFormat(const QString &format, QVariant::Type requestedType, const QList<gi_atom_id_t> &atoms, QByteArray *encoding)
{
    encoding->clear();

    // find matches for string types
    if (format == QLatin1String("text/plain")) {
        if (atoms.contains(ATOM(UTF8_STRING)))
            return ATOM(UTF8_STRING);
        if (atoms.contains(ATOM(COMPOUND_TEXT)))
            return ATOM(COMPOUND_TEXT);
        if (atoms.contains(ATOM(TEXT)))
            return ATOM(TEXT);
        if (atoms.contains(GA_STRING))
            return GA_STRING;
    }

    // find matches for uri types
    if (format == QLatin1String("text/uri-list")) {
        gi_atom_id_t a = xdndMimeStringToAtom(format);
        if (a && atoms.contains(a))
            return a;
        a = xdndMimeStringToAtom(QLatin1String("text/x-moz-url"));
        if (a && atoms.contains(a))
            return a;
    }

    // find match for image
    if (format == QLatin1String("image/ppm")) {
        if (atoms.contains(GA_PIXMAP))
            return GA_PIXMAP;
    }

    // for string/text requests try to use a format with a well-defined charset
    // first to avoid encoding problems
    if (requestedType == QVariant::String
        && format.startsWith(QLatin1String("text/"))
        && !format.contains(QLatin1String("charset="))) {

        QString formatWithCharset = format;
        formatWithCharset.append(QLatin1String(";charset=utf-8"));

        gi_atom_id_t a = xdndMimeStringToAtom(formatWithCharset);
        if (a && atoms.contains(a)) {
            *encoding = "utf-8";
            return a;
        }
    }

    gi_atom_id_t a = xdndMimeStringToAtom(format);
    if (a && atoms.contains(a))
        return a;

    return 0;
}

QString xdndMimeAtomToString(gi_atom_id_t a)
{
    QString atomName;
    if (a) {
        char *atom = gi_get_atom_name( a);
        atomName = QString::fromLatin1(atom);
        free(atom);
    }
    return atomName;
}

//$$$
QVariant xdndMimeConvertToFormat(gi_atom_id_t a, const QByteArray &data, const QString &format, QVariant::Type requestedType, const QByteArray &encoding)
{
    QString atomName = xdndMimeAtomToString(a);
    if (atomName == format)
        return data;

    if (!encoding.isEmpty()
        && atomName == format + QLatin1String(";charset=") + QString::fromLatin1(encoding)) {

        if (requestedType == QVariant::String) {
            QTextCodec *codec = QTextCodec::codecForName(encoding);
            if (codec)
                return codec->toUnicode(data);
        }

        return data;
    }

    // special cases for string types
    if (format == QLatin1String("text/plain")) {
        if (a == ATOM(UTF8_STRING))
            return QString::fromUtf8(data);
        if (a == GA_STRING)
            return QString::fromLatin1(data);
        if (a == ATOM(TEXT) || a == ATOM(COMPOUND_TEXT))
            // #### might be wrong for COMPUND_TEXT
            return QString::fromLocal8Bit(data, data.size());
    }

    // special case for uri types
    if (format == QLatin1String("text/uri-list")) {
        if (atomName == QLatin1String("text/x-moz-url")) {
            // we expect this as utf16 <url><space><title>
            // the first part is a url that should only contain ascci char
            // so it should be safe to check that the second char is 0
            // to verify that it is utf16
            if (data.size() > 1 && data.at(1) == 0)
                return QString::fromRawData((const QChar *)data.constData(),
                                data.size() / 2).split(QLatin1Char('\n')).first().toLatin1();
        }
    }

    // special cas for images
    if (format == QLatin1String("image/ppm")) {
        if (a == GA_PIXMAP && data.size() == sizeof(gi_window_id_t)) {
	  printf("#### %s() got line %d Successed\n",__FUNCTION__,__LINE__);
#if 0
            gi_window_id_t xpm = *((gi_window_id_t*)data.data());
            if (!xpm)
                return QByteArray();
            QPixmap qpm = QPixmap::fromGixPixmap(xpm);
            QImageWriter imageWriter;
            imageWriter.setFormat("PPMRAW");
            QImage imageToWrite = qpm.toImage();
            QBuffer buf;
            buf.open(QIODevice::WriteOnly);
            imageWriter.setDevice(&buf);
            imageWriter.write(imageToWrite);
            return buf.buffer();
#endif
        }
    }
    return QVariant();
}

QStringList xdndMimeFormatsForAtom(gi_atom_id_t a)
{
    QStringList formats;
    if (a) {
        QString atomName = xdndMimeAtomToString(a);
        formats.append(atomName);

        // special cases for string type
        if (a == ATOM(UTF8_STRING) || a == GA_STRING
            || a == ATOM(TEXT) || a == ATOM(COMPOUND_TEXT))
            formats.append(QLatin1String("text/plain"));

        // special cases for uris
        if (atomName == QLatin1String("text/x-moz-url"))
            formats.append(QLatin1String("text/uri-list"));

        // special case for images
        if (a == GA_PIXMAP)
            formats.append(QLatin1String("image/ppm"));
    }
    return formats;
}


class QExtraWidget : public QWidget
{
    Q_DECLARE_PRIVATE(QWidget)
public:
    inline QWExtra* extraData();
    inline QTLWExtra* topData();
};

inline QWExtra* QExtraWidget::extraData() { return d_func()->extraData(); }
inline QTLWExtra* QExtraWidget::topData() { return d_func()->topData(); }


static bool clipboardEnable(QWidget* w, bool on)
{
    VDEBUG ( "xdndEnable for %p, on=%d\n" , w , on);
    if (on) {
        QWidget * xdnd_widget = 0;

		if ((w->windowType() == Qt::Desktop)) 
			return false;
        
        xdnd_widget = w->window();
        
        if (xdnd_widget) {
            VDEBUG ( "setting XdndAware for %p window %d" , xdnd_widget , xdnd_widget->effectiveWinId());
            gi_atom_id_t atm = (gi_atom_id_t)xdnd_version;
            Q_ASSERT(xdnd_widget->testAttribute(Qt::WA_WState_Created));
            gi_change_property( xdnd_widget->effectiveWinId(), ATOM(XdndAware),
                             GA_ATOM, 32, G_PROP_MODE_Replace, (unsigned char *)&atm, 1);
            return true;
        } else {
            return false;
        }
    } else {
        if ((w->windowType() == Qt::Desktop)) {
            gi_delete_property( w->internalWinId(), ATOM(XdndProxy));
            //delete xdnd_data.desktop_proxy;
            //xdnd_data.desktop_proxy = 0;
        } else {
            VDEBUG ( "not deleting XDndAware");
        }
        return true;
    }
}

bool markWidgetFlags(QWidget* w, long flags, bool set)
{
	if (!((QExtraWidget*)w)->topData())
	{
		VDEBUG ( "markWidgetFlags failed");
		return false;
	}
    if (((QExtraWidget*)w)->topData()->winFlags & 0xffffffff == flags)
        return true; // been there, done that
	if(set)
      ((QExtraWidget*)w)->topData()->winFlags = flags;
	return false;
}

bool dndEnable(QWidget* w, bool on)
{
    w = w->window();

    if (bool(((QExtraWidget*)w)->topData()->dnd) == on)
        return true; // been there, done that
    ((QExtraWidget*)w)->topData()->dnd = on ? 1 : 0;

    //motifdndEnable(w, on);
    return clipboardEnable(w, on);
}

static int clipboard_timeout = 5000; // 5s timeout on clipboard operations

static QWidget * owner = 0;
static QWidget *requestor = 0;
static bool timer_event_clear = false;
static int timer_id = 0;

static int pending_timer_id = 0;
static bool pending_clipboard_changed = false;
static bool pending_selection_changed = false;
#define GI_X_NONE 0
#define CurrentTime          0L	/* special Time */

// event capture mechanism for qt_xclb_wait_for_event
static bool waiting_for_data = false;
static bool has_captured_event = false;
static gi_window_id_t capture_event_win = GI_X_NONE;
static int capture_event_type = -1;
static gi_msg_t captured_event;

class QClipboardWatcher; // forward decl
static QClipboardWatcher *selection_watcher = 0;
static QClipboardWatcher *clipboard_watcher = 0;

static void cleanup()
{
    delete owner;
    delete requestor;
    owner = 0;
    requestor = 0;
}
static
void setupOwner()
{
    if (owner)
        return;
    owner = new QWidget(0);	
    owner->setObjectName(QLatin1String("internal clipboard owner"));
	markWidgetFlags(owner,Q_WIDGET_WITH_DND_FLAGS,true);
    owner->createWinId();
    requestor = new QWidget(0);
    requestor->setObjectName(QLatin1String("internal clipboard requestor"));
	markWidgetFlags(requestor,Q_WIDGET_WITH_DND_FLAGS,true);
    requestor->createWinId();
    // We don't need this internal widgets to appear in QApplication::topLevelWidgets()
    if (QWidgetPrivate::allWidgets) {
        QWidgetPrivate::allWidgets->remove(owner);
        QWidgetPrivate::allWidgets->remove(requestor);
    }

    qAddPostRoutine(cleanup);
}


class QClipboardWatcher : public QInternalMimeData {
public:
    QClipboardWatcher(QClipboard::Mode mode);
    ~QClipboardWatcher();
    bool empty() const;
    virtual bool hasFormat_sys(const QString &mimetype) const;
    virtual QStringList formats_sys() const;

    QVariant retrieveData_sys(const QString &mimetype, QVariant::Type type) const;
    QByteArray getDataInFormat(gi_atom_id_t fmtatom) const;

    gi_atom_id_t atom;
    mutable QStringList formatList;
    mutable QByteArray format_atoms;
};

class QClipboardData
{
private:
    QMimeData *&mimeDataRef() const
    {
        if(mode == QClipboard::Selection)
            return selectionData;
        return clipboardData;
    }

public:
    QClipboardData(QClipboard::Mode mode);
    ~QClipboardData();

    void setSource(QMimeData* s)
    {
        if ((mode == QClipboard::Selection && selectionData == s)
            || clipboardData == s) {
            return;
        }

        if (selectionData != clipboardData) {
            delete mimeDataRef();
        }

        mimeDataRef() = s;
    }

    QMimeData *source() const
    {
        return mimeDataRef();
    }

    void clear()
    {
        timestamp = CurrentTime;
        if (selectionData == clipboardData) {
            mimeDataRef() = 0;
        } else {
            QMimeData *&src = mimeDataRef();
            delete src;
            src = 0;
        }
    }

    static QMimeData *selectionData;
    static QMimeData *clipboardData;
    bigtime_t timestamp;
    QClipboard::Mode mode;
};

QMimeData *QClipboardData::selectionData = 0;
QMimeData *QClipboardData::clipboardData = 0;

QClipboardData::QClipboardData(QClipboard::Mode clipboardMode)
{
    timestamp = CurrentTime;
    mode = clipboardMode;
}

QClipboardData::~QClipboardData()
{ clear(); }


static QClipboardData *internalCbData = 0;
static QClipboardData *internalSelData = 0;

static void cleanupClipboardData()
{
    delete internalCbData;
    internalCbData = 0;
}

static QClipboardData *clipboardData()
{
    if (internalCbData == 0) {
        internalCbData = new QClipboardData(QClipboard::Clipboard);
        qAddPostRoutine(cleanupClipboardData);
    }
    return internalCbData;
}

static void cleanupSelectionData()
{
    delete internalSelData;
    internalSelData = 0;
}

static QClipboardData *selectionData()
{
    if (internalSelData == 0) {
        internalSelData = new QClipboardData(QClipboard::Selection);
        qAddPostRoutine(cleanupSelectionData);
    }
    return internalSelData;
}

class QClipboardINCRTransaction
{
public:
    QClipboardINCRTransaction(gi_window_id_t w, gi_atom_id_t p, gi_atom_id_t t, int f, QByteArray d, unsigned int i);
    ~QClipboardINCRTransaction(void);

	bool pfWinEvent(gi_msg_t *message, long *result);

    gi_window_id_t window;
    gi_atom_id_t property, target;
    int format;
    QByteArray data;
    unsigned int increment;
    unsigned int offset;
};

typedef QMap<gi_window_id_t,QClipboardINCRTransaction*> TransactionMap;
static TransactionMap *transactions = 0;
static QApplication::EventFilter prev_event_filter = 0;
static int incr_timer_id = 0;

static bool qt_x11_incr_event_filter(void *message, long *result)
{
    gi_msg_t *event = reinterpret_cast<gi_msg_t *>(message);
    TransactionMap::Iterator it = transactions->find(event->ev_window);
    if (it != transactions->end()) {
        if ((*it)->pfWinEvent(event,result) != 0)
            return true;
    }
    if (prev_event_filter)
        return prev_event_filter(event, result);
    return false;
}

/*
  called when no INCR activity has happened for 'clipboard_timeout'
  milliseconds... we assume that all unfinished transactions have
  timed out and remove everything from the transaction map
*/
static void qt_xclb_incr_timeout(void)
{
    qWarning("QClipboard: Timed out while sending data");

    while (transactions)
        delete *transactions->begin();
}

QClipboardINCRTransaction::QClipboardINCRTransaction(gi_window_id_t w, gi_atom_id_t p, gi_atom_id_t t, int f,
                                                     QByteArray d, unsigned int i)
    : window(w), property(p), target(t), format(f), data(d), increment(i), offset(0u)
{
    DEBUG("QClipboard: sending %d bytes (INCR transaction %p)", d.size(), this);

    gi_set_events_mask( window, GI_MASK_PROPERTYNOTIFY);

    if (! transactions) {
        VDEBUG("QClipboard: created INCR transaction map");
        transactions = new TransactionMap;
        prev_event_filter = qApp->setEventFilter(qt_x11_incr_event_filter);
        incr_timer_id = QApplication::clipboard()->startTimer(clipboard_timeout);
    }
    transactions->insert(window, this);
}

QClipboardINCRTransaction::~QClipboardINCRTransaction(void)
{
    VDEBUG("QClipboard: destroyed INCR transacton %p", this);

    gi_set_events_mask( window, GI_MASK_NONE);

    transactions->remove(window);
    if (transactions->isEmpty()) {
        VDEBUG("QClipboard: no more INCR transactions");
        delete transactions;
        transactions = 0;

        (void)qApp->setEventFilter(prev_event_filter);

        if (incr_timer_id != 0) {
            QApplication::clipboard()->killTimer(incr_timer_id);
            incr_timer_id = 0;
        }
    }
}

bool QClipboardINCRTransaction::pfWinEvent(gi_msg_t *message, long *result)
{
	gi_msg_t *event = reinterpret_cast<gi_msg_t *>(message);

    if (event->type != GI_MSG_PROPERTYNOTIFY
        || (gi_message_property_notify_state(event) != G_PROPERTY_DELETE
            || gi_message_property_notify_atom(event) != property))
        return 0;

    // restart the INCR timer
    if (incr_timer_id) QApplication::clipboard()->killTimer(incr_timer_id);
    incr_timer_id = QApplication::clipboard()->startTimer(clipboard_timeout);

    unsigned int bytes_left = data.size() - offset;
    if (bytes_left > 0) {
        unsigned int xfer = qMin(increment, bytes_left);
        VDEBUG("QClipboard: sending %d bytes, %d remaining (INCR transaction %p)",
               xfer, bytes_left - xfer, this);

        gi_change_property( window, property, target, format,
                        G_PROP_MODE_Replace, (uchar *) data.data() + offset, xfer);
        offset += xfer;
    } else {
        // INCR transaction finished...
        gi_change_property( window, property, target, format,
                        G_PROP_MODE_Replace, (uchar *) data.data(), 0);
        delete this;
    }

    return 1;
}


QClipboardWatcher::QClipboardWatcher(QClipboard::Mode mode)
    : QInternalMimeData()
{
    switch (mode) {
    case QClipboard::Selection:
        atom = GA_PRIMARY;
        break;

    case QClipboard::Clipboard:
        atom = ATOM(CLIPBOARD);
        break;

    default:
        qWarning("QClipboardWatcher: Internal error: Unsupported clipboard mode");
        break;
    }

    setupOwner();
}

QClipboardWatcher::~QClipboardWatcher()
{
    if(selection_watcher == this)
        selection_watcher = 0;
    if(clipboard_watcher == this)
        clipboard_watcher = 0;
}


bool QClipboardWatcher::empty() const
{
    //Display *dpy = qt_pfData->display;
    gi_window_id_t win = gi_get_selection_owner( atom);

    if(win == requestor->internalWinId()) {
        qWarning("QClipboardWatcher::empty: Internal error: Application owns the selection");
        return true;
    }

    return win == GI_X_NONE;
}
bool QClipboardWatcher::hasFormat_sys(const QString &format) const
{
    QStringList list = formats();
    return list.contains(format);
}

typedef struct {
    unsigned char *value;		/* same as Property routines */
    gi_atom_id_t encoding;			/* prop type */
    int format;				/* prop data format: 8, 16, or 32 */
    unsigned long nitems;		/* number of data items in value */
} XTextProperty;


void XFreeStringList (char **list)
{
    if (list) {
	if (list[0]) free (list[0]);
	free ((char *) list);
	list = NULL;
    }
}

int
XmbTextPropertyToTextList(
    const XTextProperty *text_prop,
    char ***list_ret,
    int *count_ret)
{
	printf("#### %s() got line %d Successed\n",__FUNCTION__,__LINE__);
	return -1;
}


static inline int maxSelectionIncr()
{ return  65536*4; }

bool clipboardReadProperty(gi_window_id_t win, gi_atom_id_t property, bool deleteProperty,
                                     QByteArray *buffer, int *size, gi_atom_id_t *type, int *format)
{
    int           maxsize = maxSelectionIncr();
    ulong  bytes_left; // bytes_after
    ulong  length;     // nitems
    uchar *data;
    gi_atom_id_t   dummy_type;
    int    dummy_format;
    int    r;

    if (!type)                                // allow null args
        type = &dummy_type;
    if (!format)
        format = &dummy_format;

    // Don't read anything, just get the size of the property data
    r = gi_get_window_property( win, property, 0, 0, FALSE,
                            G_ANY_PROP_TYPE, type, format,
                            &length, &bytes_left, &data);
    if (r != 0 || (type && *type == GI_X_NONE)) {
        buffer->resize(0);
        return false;
    }
    free((char*)data);

    int  offset = 0, buffer_offset = 0, format_inc = 1, proplen = bytes_left;

    VDEBUG("QClipboard: read_property(): initial property length: %d", proplen);

    switch (*format) {
    case 8:
    default:
        format_inc = sizeof(char) / 1;
        break;

    case 16:
        format_inc = sizeof(short) / 2;
        proplen *= sizeof(short) / 2;
        break;

    case 32:
        format_inc = sizeof(long) / 4;
        proplen *= sizeof(long) / 4;
        break;
    }

    int newSize = proplen;
    buffer->resize(newSize);

    bool ok = (buffer->size() == newSize);
    VDEBUG("QClipboard: read_property(): buffer resized to %d", buffer->size());

    if (ok && newSize) {
        // could allocate buffer

        while (bytes_left) {
            // more to read...

            r = gi_get_window_property( win, property, offset, maxsize/4,
                                   FALSE, G_ANY_PROP_TYPE, type, format,
                                   &length, &bytes_left, &data);
            if (r != 0 || (type && *type == GI_X_NONE))
                break;

            offset += length / (32 / *format);
            length *= format_inc * (*format) / 8;

            // Here we check if we get a buffer overflow and tries to
            // recover -- this shouldn't normally happen, but it doesn't
            // hurt to be defensive
            if ((int)(buffer_offset + length) > buffer->size()) {
                length = buffer->size() - buffer_offset;

                // escape loop
                bytes_left = 0;
            }

            memcpy(buffer->data() + buffer_offset, data, length);
            buffer_offset += length;

            free((char*)data);
        }

        if (*format == 8 && *type == ATOM(COMPOUND_TEXT)) {
            // convert COMPOUND_TEXT to a multibyte string
            XTextProperty textprop;
            textprop.encoding = *type;
            textprop.format = *format;
            textprop.nitems = buffer_offset;
            textprop.value = (unsigned char *) buffer->data();

            char **list_ret = 0;
            int count;
            if (XmbTextPropertyToTextList( &textprop, &list_ret,
                         &count) == 0 && count && list_ret) {
                offset = buffer_offset = strlen(list_ret[0]);
                buffer->resize(offset);
                memcpy(buffer->data(), list_ret[0], offset);
            }
            if (list_ret) XFreeStringList(list_ret);
        }
    }

    // correct size, not 0-term.
    if (size)
        *size = buffer_offset;

    VDEBUG("QClipboard: read_property(): buffer size %d, buffer offset %d, offset %d",
           buffer->size(), buffer_offset, offset);

    if (deleteProperty)
        gi_delete_property( win, property);

    //XFlush(display);

    return ok;
}

QByteArray clipboardReadIncrementalProperty(gi_window_id_t win, gi_atom_id_t property, int nbytes, bool nullterm)
{
    gi_msg_t event;

    QByteArray buf;
    QByteArray tmp_buf;
    bool alloc_error = false;
    int  length;
    int  offset = 0;

    if (nbytes > 0) {
        // Reserve buffer + zero-terminator (for text data)
        // We want to complete the INCR transfer even if we cannot
        // allocate more memory
        buf.resize(nbytes+1);
        alloc_error = buf.size() != nbytes+1;
    }

    for (;;) {
        //XFlush(display);
        if (!clipboardWaitForEvent(win,GI_MASK_PROPERTYNOTIFY,&event,clipboard_timeout))
            break;
        if (gi_message_property_notify_atom(&event) != property ||
             gi_message_property_notify_state(&event) != G_PROPERTY_NEW)
            continue;
        if (clipboardReadProperty(win, property, true, &tmp_buf, &length, 0, 0)) {
            if (length == 0) {                // no more data, we're done
                if (nullterm) {
                    buf.resize(offset+1);
                    buf[offset] = '\0';
                } else {
                    buf.resize(offset);
                }
                return buf;
            } else if (!alloc_error) {
                if (offset+length > (int)buf.size()) {
                    buf.resize(offset+length+65535);
                    if (buf.size() != offset+length+65535) {
                        alloc_error = true;
                        length = buf.size() - offset;
                    }
                }
                memcpy(buf.data()+offset, tmp_buf.constData(), length);
                tmp_buf.resize(0);
                offset += length;
            }
        } else {
            break;
        }
    }

    // timed out ... create a new requestor window, otherwise the requestor
    // could consider next request to be still part of this timed out request
    delete requestor;
    requestor = new QWidget(0);
    requestor->setObjectName(QLatin1String("internal clipboard requestor"));
	markWidgetFlags(requestor,Q_WIDGET_WITH_DND_FLAGS,true);
    // We don't need this internal widget to appear in QApplication::topLevelWidgets()
    if (QWidgetPrivate::allWidgets)
        QWidgetPrivate::allWidgets->remove(requestor);

    return QByteArray();
}


//$$$
QList<gi_atom_id_t> xdndMimeAtomsForFormat(const QString &format)
{
    QList<gi_atom_id_t> atoms;
    atoms.append(xdndMimeStringToAtom(format));

    // special cases for strings
    if (format == QLatin1String("text/plain")) {
        atoms.append(ATOM(UTF8_STRING));
        atoms.append(GA_STRING);
        atoms.append(ATOM(TEXT));
        atoms.append(ATOM(COMPOUND_TEXT));
    }

    // special cases for uris
    if (format == QLatin1String("text/uri-list")) {
        atoms.append(xdndMimeStringToAtom(QLatin1String("text/x-moz-url")));
    }

    //special cases for images
    if (format == QLatin1String("image/ppm"))
        atoms.append(GA_PIXMAP);
    if (format == QLatin1String("image/pbm"))
        atoms.append(GA_BITMAP);

    return atoms;
}


//$$$
bool xdndMimeDataForAtom(gi_atom_id_t a, QMimeData *mimeData, QByteArray *data, gi_atom_id_t *atomFormat, int *dataFormat)
{
    bool ret = false;
    *atomFormat = a;
    *dataFormat = 8;
    QString atomName = xdndMimeAtomToString(a);
    if (QInternalMimeData::hasFormatHelper(atomName, mimeData)) {
        *data = QInternalMimeData::renderDataHelper(atomName, mimeData);
        if (atomName == QLatin1String("application/x-color"))
            *dataFormat = 16;
        ret = true;
    } else {
        if ((a == ATOM(UTF8_STRING) || a == GA_STRING
             || a == ATOM(TEXT) || a == ATOM(COMPOUND_TEXT))
            && QInternalMimeData::hasFormatHelper(QLatin1String("text/plain"), mimeData)) {
            if (a == ATOM(UTF8_STRING)){
                *data = QInternalMimeData::renderDataHelper(QLatin1String("text/plain"), mimeData);
                ret = true;
            } else if (a == GA_STRING) {
                *data = QString::fromUtf8(QInternalMimeData::renderDataHelper(
                        QLatin1String("text/plain"), mimeData)).toLocal8Bit();
                ret = true;
            } else if (a == ATOM(TEXT) || a == ATOM(COMPOUND_TEXT)) {
                // the ICCCM states that TEXT and COMPOUND_TEXT are in the
                // encoding of choice, so we choose the encoding of the locale
                QByteArray strData = QString::fromUtf8(QInternalMimeData::renderDataHelper(
                                     QLatin1String("text/plain"), mimeData)).toLocal8Bit();
                char *list[] = { strData.data(), NULL };
				printf("#### %s() got line %d Successed\n",__FUNCTION__,__LINE__);

				#if 0

                XICCEncodingStyle style = (a == ATOM(COMPOUND_TEXT))
                                        ? XCompoundTextStyle : XStdICCTextStyle;
                XTextProperty textprop;
                if (list[0] != NULL
                    && XmbTextListToTextProperty( list, 1, style,
                                                 &textprop) == 0) {
                    *atomFormat = textprop.encoding;
                    *dataFormat = textprop.format;
                    *data = QByteArray((const char *) textprop.value, textprop.nitems * textprop.format / 8);
                    ret = true;

                    DEBUG("    textprop type %lx\n"
                    "    textprop name '%s'\n"
                    "    format %d\n"
                    "    %ld items\n"
                    "    %d bytes\n",
                    textprop.encoding,
                    qt_pfData->xdndMimeAtomToString(textprop.encoding).toLatin1().data(),
                    textprop.format, textprop.nitems, data->size());

                    free(textprop.value);
                }
				#endif
            }
        } else if (atomName == QLatin1String("text/x-moz-url") &&
                   QInternalMimeData::hasFormatHelper(QLatin1String("text/uri-list"), mimeData)) {
            QByteArray uri = QInternalMimeData::renderDataHelper(
                             QLatin1String("text/uri-list"), mimeData).split('\n').first();
            QString mozUri = QString::fromLatin1(uri, uri.size());
            mozUri += QLatin1Char('\n');
            *data = QByteArray(reinterpret_cast<const char *>(mozUri.utf16()), mozUri.length() * 2);
            ret = true;
        } else if ((a == GA_PIXMAP || a == GA_BITMAP) && mimeData->hasImage()) {
            QPixmap pm = qvariant_cast<QPixmap>(mimeData->imageData());
            if (a == GA_BITMAP && pm.depth() != 1) {
                QImage img = pm.toImage();
                img = img.convertToFormat(QImage::Format_MonoLSB);
                pm = QPixmap::fromImage(img);
            }
            QDragManager *dm = QDragManager::self();
            if (dm) {
				printf("#### %s() got line %d Successed\n",__FUNCTION__,__LINE__);

				#if 0 //fixme dpp
                Pixmap handle = pm.handle();
                *data = QByteArray((const char *) &handle, sizeof(Pixmap));
                dm->xdndMimeTransferedPixmap[dm->xdndMimeTransferedPixmapIndex] = pm;
                dm->xdndMimeTransferedPixmapIndex =
                            (dm->xdndMimeTransferedPixmapIndex + 1) % 2;
                ret = true;
				#endif
            }
        } else {
            DEBUG("QClipboard: xdndMimeDataForAtom(): converting to type '%s' is not supported", qPrintable(atomName));
        }
    }
    return ret && data != 0;
}

static gi_atom_id_t send_targets_selection(QClipboardData *d, gi_window_id_t window, gi_atom_id_t property)
{
    QVector<gi_atom_id_t> types;
    QStringList formats = QInternalMimeData::formatsHelper(d->source());
    for (int i = 0; i < formats.size(); ++i) {
        QList<gi_atom_id_t> atoms = xdndMimeAtomsForFormat(formats.at(i));
        for (int j = 0; j < atoms.size(); ++j) {
            if (!types.contains(atoms.at(j)))
                types.append(atoms.at(j));
        }
    }
    types.append(ATOM(TARGETS));
    types.append(ATOM(MULTIPLE));
    types.append(ATOM(TIMESTAMP));
    types.append(ATOM(SAVE_TARGETS));

    gi_change_property( window, property, GA_ATOM, 32,
                    G_PROP_MODE_Replace, (uchar *) types.data(), types.size());
    return property;
}

static gi_atom_id_t send_selection(QClipboardData *d, gi_atom_id_t target, gi_window_id_t window, gi_atom_id_t property)
{
    gi_atom_id_t atomFormat = target;
    int dataFormat = 0;
    QByteArray data;

    QByteArray fmt = xdndAtomToString(target);
    if (fmt.isEmpty()) { // Not a MIME type we have
        DEBUG("QClipboard: send_selection(): converting to type '%s' is not supported", fmt.data());
        return GI_X_NONE;
    }
    DEBUG("QClipboard: send_selection(): converting to type '%s'", fmt.data());

    if (xdndMimeDataForAtom(target, d->source(), &data, &atomFormat, &dataFormat)) {

        VDEBUG("QClipboard: send_selection():\n"
          "    property type %lx\n"
          "    property name '%s'\n"
          "    format %d\n"
          "    %d bytes\n",
          target, xdndMimeAtomToString(atomFormat).toLatin1().data(), dataFormat, data.size());

         // don't allow INCR transfers when using MULTIPLE or to
        // Motif clients (since Motif doesn't support INCR)
        static gi_atom_id_t motif_clip_temporary = ATOM(CLIP_TEMPORARY);
        bool allow_incr = property != motif_clip_temporary;

        // X_ChangeProperty protocol request is 24 bytes
        const int increment = (65536 * 4) - 24;
        if (data.size() > increment && allow_incr) {
            long bytes = data.size();
            gi_change_property( window, property,
                            ATOM(INCR), 32, G_PROP_MODE_Replace, (uchar *) &bytes, 1);

            (void)new QClipboardINCRTransaction(window, property, atomFormat, dataFormat, data, increment);
            return property;
        }

        // make sure we can perform the gi_change_property in a single request
        if (data.size() > increment)
            return GI_X_NONE; // ### perhaps use several gi_change_property calls w/ PropModeAppend?
        int dataSize = data.size() / (dataFormat / 8);
        // use a single request to transfer data
        gi_change_property( window, property, atomFormat,
                        dataFormat, G_PROP_MODE_Replace, (uchar *) data.data(),
                        dataSize);
    }
    return property;
}

QByteArray QClipboardWatcher::getDataInFormat(gi_atom_id_t fmtatom) const
{
    QByteArray buf;

    //Display *dpy = qt_pfData->display;
    requestor->createWinId();
    gi_window_id_t   win = requestor->internalWinId();
    Q_ASSERT(requestor->testAttribute(Qt::WA_WState_Created));

    DEBUG("QClipboardWatcher::getDataInFormat: selection '%s' format '%s'",
          xdndAtomToString(atom).data(), xdndAtomToString(fmtatom).data());

    gi_set_events_mask( win, GI_MASK_NONE); // don't listen for any events

    gi_delete_property( win, ATOM(_QT_SELECTION));
    gi_convert_selection( atom, fmtatom, ATOM(_QT_SELECTION), win, qt_pfData->time);
    //XSync(dpy, false);

    VDEBUG("QClipboardWatcher::getDataInFormat: waiting for GI_MSG_SELECTIONNOTIFY event");

    gi_msg_t xevent;
    if (!clipboardWaitForEvent(win,GI_MASK_SELECTIONNOTIFY,&xevent,clipboard_timeout) ||
         gi_message_notify_selection_property(&xevent) == GI_X_NONE) {
        DEBUG("QClipboardWatcher::getDataInFormat: format not available");
        return buf;
    }

    VDEBUG("QClipboardWatcher::getDataInFormat: fetching data...");

    gi_atom_id_t   type;
    gi_set_events_mask( win, GI_MASK_PROPERTYNOTIFY);

    if (clipboardReadProperty(win, ATOM(_QT_SELECTION), true, &buf, 0, &type, 0)) {
        if (type == ATOM(INCR)) {
            int nbytes = buf.size() >= 4 ? *((int*)buf.data()) : 0;
            buf = clipboardReadIncrementalProperty(win, ATOM(_QT_SELECTION), nbytes, false);
        }
    }

    gi_set_events_mask( win, GI_MASK_NONE);

    DEBUG("QClipboardWatcher::getDataInFormat: %d bytes received", buf.size());

    return buf;
}

QStringList QClipboardWatcher::formats_sys() const
{
    if (empty())
        return QStringList();

    if (!formatList.count()) {
        // get the list of targets from the current clipboard owner - we do this
        // once so that multiple calls to this function don't require multiple
        // server round trips...

        format_atoms = getDataInFormat(ATOM(TARGETS));

        if (format_atoms.size() > 0) {
            gi_atom_id_t *targets = (gi_atom_id_t *) format_atoms.data();
            int size = format_atoms.size() / sizeof(gi_atom_id_t);

            for (int i = 0; i < size; ++i) {
                if (targets[i] == 0)
                    continue;

                QStringList formatsForAtom = xdndMimeFormatsForAtom(targets[i]);
                for (int j = 0; j < formatsForAtom.size(); ++j) {
                    if (!formatList.contains(formatsForAtom.at(j)))
                        formatList.append(formatsForAtom.at(j));
                }
                VDEBUG("    format: %s", xdndAtomToString(targets[i]).data());
                VDEBUG("    data:\n%s\n", getDataInFormat(targets[i]).data());
            }
            DEBUG("QClipboardWatcher::format: %d formats available", formatList.count());
        }
    }

    return formatList;
}

QVariant QClipboardWatcher::retrieveData_sys(const QString &fmt, QVariant::Type requestedType) const
{
    if (fmt.isEmpty() || empty())
        return QByteArray();

    (void)formats(); // trigger update of format list
    DEBUG("QClipboardWatcher::data: fetching format '%s'", fmt.toLatin1().data());

    QList<gi_atom_id_t> atoms;
    gi_atom_id_t *targets = (gi_atom_id_t *) format_atoms.data();
    int size = format_atoms.size() / sizeof(gi_atom_id_t);
    for (int i = 0; i < size; ++i)
        atoms.append(targets[i]);

    QByteArray encoding;
    gi_atom_id_t fmtatom = xdndMimeAtomForFormat(fmt, requestedType, atoms, &encoding);

    if (fmtatom == 0)
        return QVariant();

    return xdndMimeConvertToFormat(fmtatom, getDataInFormat(fmtatom), fmt, requestedType, encoding);
}

////////////////////////////////////////////////////////////////////////////////


//MRESULT QClipboardData::message(unsigned long msg, MPARAM mp1, MPARAM mp2)
bool pfWinEvent(gi_msg_t *message, long *result)
{
    DEBUG(("QClipboardData::message: END"));
    return FALSE;
}

////////////////////////////////////////////////////////////////////////////////


void QClipboard::setMimeData(QMimeData *src, Mode mode)
{
    gi_atom_id_t atom, sentinel_atom;
    QClipboardData *d;
    switch (mode) {
    case Selection:
        atom = GA_PRIMARY;
        sentinel_atom = ATOM(_QT_SELECTION_SENTINEL);
        d = selectionData();
        break;

    case Clipboard:
        atom = ATOM(CLIPBOARD);
        sentinel_atom = ATOM(_QT_CLIPBOARD_SENTINEL);
        d = clipboardData();
        break;

    default:
        qWarning("QClipboard::setMimeData: unsupported mode '%d'", mode);
        return;
    }

    //Display *dpy = qt_pfData->display;
    gi_window_id_t newOwner;

    if (! src) { // no data, clear clipboard contents
        newOwner = GI_X_NONE;
        d->clear();
    } else {
        setupOwner();

        newOwner = owner->internalWinId();

        d->setSource(src);
        d->timestamp = qt_pfData->time;
    }

    gi_window_id_t prevOwner = gi_get_selection_owner( atom);
    // use qt_pfData->time, since d->timestamp == CurrentTime when clearing
    gi_set_selection_owner( atom, newOwner, qt_pfData->time);

    if (mode == Selection)
        emitChanged(QClipboard::Selection);
    else
        emitChanged(QClipboard::Clipboard);

    if (gi_get_selection_owner( atom) != newOwner) {
        qWarning("QClipboard::setData: Cannot set X11 selection owner for %s",
                 xdndAtomToString(atom).data());
        d->clear();
        return;
    }

    // Signal to other Qt processes that the selection has changed
    gi_window_id_t owners[2];
    owners[0] = newOwner;
    owners[1] = prevOwner;
    gi_change_property( QApplication::desktop()->screen(0)->internalWinId(),
                     sentinel_atom, GA_WINDOW, 32, G_PROP_MODE_Replace,
                     (unsigned char*)&owners, 2);
}

void QClipboard::clear(Mode mode)
{
    setMimeData(0, Clipboard);
}


typedef struct {
	//int type;
	//unsigned long serial;	/* # of last request processed by server */
	//Bool send_event;	/* true if this came from a SendEvent request */
	//Display *display;	/* Display the event was read from */
	gi_window_id_t owner;
	gi_window_id_t requestor;
	gi_atom_id_t selection;
	gi_atom_id_t target;
	gi_atom_id_t property;
	long time;
} XSelectionRequestEvent;



bool QClipboard::event(QEvent *e)
{
    if (e->type() == QEvent::Timer) {
        QTimerEvent *te = (QTimerEvent *) e;

        if (waiting_for_data) // should never happen
            return false;

        if (te->timerId() == timer_id) {
            killTimer(timer_id);
            timer_id = 0;

            timer_event_clear = true;
            if (selection_watcher) // clear selection
                selectionData()->clear();
            if (clipboard_watcher) // clear clipboard
                clipboardData()->clear();
            timer_event_clear = false;

            return true;
        } else if (te->timerId() == pending_timer_id) {
            // I hate klipper
            killTimer(pending_timer_id);
            pending_timer_id = 0;

            if (pending_clipboard_changed) {
                pending_clipboard_changed = false;
                clipboardData()->clear();
                emitChanged(QClipboard::Clipboard);
            }
            if (pending_selection_changed) {
                pending_selection_changed = false;
                selectionData()->clear();
                emitChanged(QClipboard::Selection);
            }

            return true;
        } else if (te->timerId() == incr_timer_id) {
            killTimer(incr_timer_id);
            incr_timer_id = 0;

            qt_xclb_incr_timeout();

            return true;
        } else {
            return QObject::event(e);
        }
    } else if (e->type() != QEvent::Clipboard) {
        return QObject::event(e);
    }

    gi_msg_t *xevent = (gi_msg_t *)(((QClipboardEvent *)e)->data());
    //Display *dpy = qt_pfData->display;

    if (!xevent) {
        // That means application exits and we need to give clipboard
        // content to the clipboard manager.
        // First we check if there is a clipboard manager.
        if (gi_get_selection_owner( ATOM(CLIPBOARD_MANAGER)) == GI_X_NONE
            || !owner)
            return true;

        gi_window_id_t ownerId = owner->internalWinId();
        Q_ASSERT(ownerId);
        // we delete the property so the manager saves all TARGETS.
        gi_delete_property( ownerId, ATOM(_QT_SELECTION));
        gi_convert_selection( ATOM(CLIPBOARD_MANAGER), ATOM(SAVE_TARGETS),
                          ATOM(_QT_SELECTION), ownerId, qt_pfData->time);

        gi_msg_t event;
        // waiting until the clipboard manager fetches the content.
        if (!clipboardWaitForEvent(ownerId, GI_MASK_SELECTIONNOTIFY, &event, 10000)) {
            qWarning("QClipboard: Unable to receive an event from the "
                     "clipboard manager in a reasonable time");
        }

        return true;
    }
//fixme

#if 1
	if (xevent->type != GI_MSG_SELECTIONNOTIFY)
    return true;

    switch (gi_message_selection_subtype(xevent)) {

    case G_SEL_CLEAR:
        // new selection owner
        if (gi_message_clear_selection_property(xevent) == GA_PRIMARY) {
            QClipboardData *d = selectionData();

            // ignore the event if it was generated before we gained selection ownership
            if (d->timestamp != CurrentTime && gi_message_clear_selection_time(xevent) <= d->timestamp)
                break;

            DEBUG("QClipboard: new selection owner 0x%lx at time %lx (ours %lx)",
                  gi_get_selection_owner( GA_PRIMARY),
                  gi_message_clear_selection_time(xevent), d->timestamp);

            if (! waiting_for_data) {
                d->clear();
                emitChanged(QClipboard::Selection);
            } else {
                pending_selection_changed = true;
                if (! pending_timer_id)
                    pending_timer_id = QApplication::clipboard()->startTimer(0);
            }
        } else if (gi_message_clear_selection_property(xevent) == ATOM(CLIPBOARD)) {
            QClipboardData *d = clipboardData();

            // ignore the event if it was generated before we gained selection ownership
            if (d->timestamp != CurrentTime && gi_message_clear_selection_time(xevent) <= d->timestamp)
                break;

            DEBUG("QClipboard: new clipboard owner 0x%lx at time %lx (%lx)",
                  gi_get_selection_owner( ATOM(CLIPBOARD)),
                  gi_message_clear_selection_time(xevent), d->timestamp);

            if (! waiting_for_data) {
                d->clear();
                emitChanged(QClipboard::Clipboard);
            } else {
                pending_clipboard_changed = true;
                if (! pending_timer_id)
                    pending_timer_id = QApplication::clipboard()->startTimer(0);
            }
        } else {
            qWarning("QClipboard: Unknown SelectionClear event received");
            return false;
        }
        break;

    case G_SEL_NOTIFY:
        /*
          Something has delivered data to us, but this was not caught
          by QClipboardWatcher::getDataInFormat()

          Just skip the event to prevent Bad Things (tm) from
          happening later on...
        */
        break;

    case G_SEL_REQUEST:
        {
            // someone wants our data
			XSelectionRequestEvent xselectionrequest;
            XSelectionRequestEvent *req = &xselectionrequest;

			req->owner = gi_message_request_selection_window(xevent);
			req->requestor = gi_message_request_selection_requestor(xevent);
			req->selection = gi_message_request_selection_selection(xevent);
			req->target = gi_message_request_selection_target(xevent);
			req->property = gi_message_request_selection_property(xevent);
			req->time = gi_message_request_selection_time(xevent);

            if (requestor && req->requestor == requestor->internalWinId())
                break;

            gi_msg_t event;
            event.type      = GI_MSG_SELECTIONNOTIFY;
			gi_message_selection_subtype(&event) = G_SEL_NOTIFY;
            gi_message_notify_selection_requestor(&event) = req->requestor;
            gi_message_notify_selection_selection(&event) = req->selection;
            gi_message_notify_selection_target(&event)    = req->target;
            gi_message_notify_selection_property(&event)  = GI_X_NONE;
            gi_message_notify_selection_time(&event)      = req->time;

            DEBUG("QClipboard: SelectionRequest from %lx\n"
                  "    selection 0x%lx (%s) target 0x%lx (%s)",
                  req->requestor,
                  req->selection,
                  xdndAtomToString(req->selection).data(),
                  req->target,
                  xdndAtomToString(req->target).data());

            QClipboardData *d;
            if (req->selection == GA_PRIMARY) {
                d = selectionData();
            } else if (req->selection == ATOM(CLIPBOARD)) {
                d = clipboardData();
            } else {
                qWarning("QClipboard: Unknown selection '%lx'", req->selection);
                gi_send_event( req->requestor, FALSE, GI_MASK_NONE, &event);
                break;
            }

            if (! d->source()) {
                qWarning("QClipboard: Cannot transfer data, no data available");
                gi_send_event( req->requestor, FALSE, GI_MASK_NONE, &event);
                break;
            }

            DEBUG("QClipboard: SelectionRequest at time %lx (ours %lx)",
                  req->time, d->timestamp);

            if (d->timestamp == CurrentTime // we don't own the selection anymore
                || (req->time != CurrentTime && req->time < d->timestamp)) {
                DEBUG("QClipboard: SelectionRequest too old");
                gi_send_event( req->requestor, FALSE, GI_MASK_NONE, &event);
                break;
            }

            gi_atom_id_t xa_targets = ATOM(TARGETS);
            gi_atom_id_t xa_multiple = ATOM(MULTIPLE);
            gi_atom_id_t xa_timestamp = ATOM(TIMESTAMP);

            struct AtomPair { gi_atom_id_t target; gi_atom_id_t property; } *multi = 0;
            gi_atom_id_t multi_type = GI_X_NONE;
            int multi_format = 0;
            int nmulti = 0;
            int imulti = -1;
            bool multi_writeback = false;

            if (req->target == xa_multiple) {
                QByteArray multi_data;
                if (req->property == GI_X_NONE
                    || !clipboardReadProperty(req->requestor, req->property, false, &multi_data,
                                                   0, &multi_type, &multi_format)
                    || multi_format != 32) {
                    // MULTIPLE property not formatted correctly
                    gi_send_event( req->requestor, FALSE, GI_MASK_NONE, &event);
                    break;
                }
                nmulti = multi_data.size()/sizeof(*multi);
                multi = new AtomPair[nmulti];
                memcpy(multi,multi_data.data(),multi_data.size());
                imulti = 0;
            }

            for (; imulti < nmulti; ++imulti) {
                gi_atom_id_t target;
                gi_atom_id_t property;

                if (multi) {
                    target = multi[imulti].target;
                    property = multi[imulti].property;
                } else {
                    target = req->target;
                    property = req->property;
                    if (property == GI_X_NONE) // obsolete client
                        property = target;
                }

                gi_atom_id_t ret = GI_X_NONE;
                if (target == GI_X_NONE || property == GI_X_NONE) {
                    ;
                } else if (target == xa_timestamp) {
                    if (d->timestamp != CurrentTime) {
                        gi_change_property( req->requestor, property, GA_INTEGER, 32,
                                        G_PROP_MODE_Replace, (uchar *) &d->timestamp, 1);
                        ret = property;
                    } else {
                        qWarning("QClipboard: Invalid data timestamp");
                    }
                } else if (target == xa_targets) {
                    ret = send_targets_selection(d, req->requestor, property);
                } else {
                    ret = send_selection(d, target, req->requestor, property);
                }

                if (nmulti > 0) {
                    if (ret == GI_X_NONE) {
                        multi[imulti].property = GI_X_NONE;
                        multi_writeback = true;
                    }
                } else {
                    gi_message_notify_selection_property(&event) = ret;
                    break;
                }
            }

            if (nmulti > 0) {
                if (multi_writeback) {
                    // according to ICCCM 2.6.2 says to put None back
                    // into the original property on the requestor window
                    gi_change_property( req->requestor, req->property, multi_type, 32,
                                    G_PROP_MODE_Replace, (uchar *) multi, nmulti * 2);
                }

                delete [] multi;
                gi_message_notify_selection_property(&event) = req->property;
            }

            // send selection notify to requestor
            gi_send_event( req->requestor, FALSE, GI_MASK_NONE, &event);

            DEBUG("QClipboard: GI_MSG_SELECTIONNOTIFY to 0x%lx\n"
                  "    property 0x%lx (%s)",
                  req->requestor, gi_message_notify_selection_property(&event),
                  xdndAtomToString(gi_message_notify_selection_property(&event)).data());
        }
        break;
    }
#endif

    return true;
}

void QClipboard::connectNotify(const char *signal)
{
}

const QMimeData *QClipboard::mimeData(Mode mode) const
{
    QClipboardData *d = 0;
    switch (mode) {
    case Selection:
        d = selectionData();
        break;
    case Clipboard:
        d = clipboardData();
        break;
    default:
        qWarning("QClipboard::mimeData: unsupported mode '%d'", mode);
        return 0;
    }

    if (! d->source() && ! timer_event_clear) {
        if (mode == Selection) {
            if (! selection_watcher)
                selection_watcher = new QClipboardWatcher(mode);
            d->setSource(selection_watcher);
        } else {
            if (! clipboard_watcher)
                clipboard_watcher = new QClipboardWatcher(mode);
            d->setSource(clipboard_watcher);
        }

        if (! timer_id) {
            // start a zero timer - we will clear cached data when the timer
            // times out, which will be the next time we hit the event loop...
            // that way, the data is cached long enough for calls within a single
            // loop/function, but the data doesn't linger around in case the selection
            // changes
            QClipboard *that = ((QClipboard *) this);
            timer_id = that->startTimer(0);
        }
    }

    return d->source();
}

bool QClipboard::supportsMode(Mode mode) const
{
    return (mode == Clipboard || mode == Selection);
}

bool QClipboard::ownsMode(Mode mode) const
{
    if (mode == Clipboard)
        return clipboardData()->timestamp != CurrentTime;
    else if(mode == Selection)
        return selectionData()->timestamp != CurrentTime;
    else
        return false;
}

void QClipboard::ownerDestroyed()
{
    // not used
}

QT_END_NAMESPACE

#endif // QT_NO_CLIPBOARD
