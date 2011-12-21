#ifndef QWIDGET_GIX_H
#define QWIDGET_GIX_H

#include <gi/gi.h>
#include <gi/property.h>
#include "qevent.h"

#include "qwidget.h"



QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QWidgetPrivate;

typedef time_t bigtime_t;


class QDrag;
struct QXdndDropTransaction
{
    bigtime_t timestamp;
    gi_window_id_t target;
    gi_window_id_t proxy_target;
    QWidget *targetWidget;
    QWidget *embedding_widget;
    QDrag *object;
};

struct QpfData;
extern Q_GUI_EXPORT QpfData *qt_pfData;

#define GIX_QT_ENABLE_DND 1

struct QpfData
{
    gi_window_id_t findClientWindow(gi_window_id_t, gi_atom_id_t, bool);
#ifdef GIX_QT_ENABLE_DND
    // from qclipboard_x11.cpp
    bool clipboardWaitForEvent(gi_window_id_t win, int type, gi_msg_t *event, int timeout);
    bool clipboardReadProperty(gi_window_id_t win, gi_atom_id_t property, bool deleteProperty,
                            QByteArray *buffer, int *size, gi_atom_id_t *type, int *format);
    QByteArray clipboardReadIncrementalProperty(gi_window_id_t win, gi_atom_id_t property, int nbytes, bool nullterm);

    // from qdnd_x11.cpp
    bool dndEnable(QWidget* w, bool on);
    static void xdndSetup();
    void xdndHandleEnter(QWidget *, const gi_msg_t *, bool);
    void xdndHandlePosition(QWidget *, const gi_msg_t *, bool);
    void xdndHandleStatus(QWidget *, const gi_msg_t *, bool);
    void xdndHandleLeave(QWidget *, const gi_msg_t *, bool);
    void xdndHandleDrop(QWidget *, const gi_msg_t *, bool);
    void xdndHandleFinished(QWidget *, const gi_msg_t *, bool);
    void xdndHandleSelectionRequest(const gi_msg_t *);
    static bool xdndHandleBadwindow();
    QByteArray xdndAtomToString(gi_atom_id_t a);
    gi_atom_id_t xdndStringToAtom(const char *);

    QString xdndMimeAtomToString(gi_atom_id_t a);
    gi_atom_id_t xdndMimeStringToAtom(const QString &mimeType);
    QStringList xdndMimeFormatsForAtom(gi_atom_id_t a);
    bool xdndMimeDataForAtom(gi_atom_id_t a, QMimeData *mimeData, QByteArray *data, gi_atom_id_t *atomFormat, int *dataFormat);
    QList<gi_atom_id_t> xdndMimeAtomsForFormat(const QString &format);
    QVariant xdndMimeConvertToFormat(gi_atom_id_t a, const QByteArray &data, const QString &format, QVariant::Type requestedType, const QByteArray &encoding);
    gi_atom_id_t xdndMimeAtomForFormat(const QString &format, QVariant::Type requestedType, const QList<gi_atom_id_t> &atoms, QByteArray *requestedEncoding);

    QList<QXdndDropTransaction> dndDropTransactions;

    // from qmotifdnd_x11.cpp
    void motifdndHandle(QWidget *, const gi_msg_t *, bool);
    void motifdndEnable(QWidget *, bool);
    QVariant motifdndObtainData(const char *format);
    QByteArray motifdndFormat(int n);
    bool motifdnd_active;
#endif

    // current focus model
    enum {
        FM_Unknown = -1,
        FM_Other = 0,
        FM_PointerRoot = 1
    };
    int focus_model;
        

    //QList<QWidget *> deferred_map;
    struct ScrollInProgress {
        long id;
        QWidget* scrolled_widget;
        int dx, dy;
    };
    long sip_serial;
    //QList<ScrollInProgress> sip_list;

    // window managers list of supported "stuff"
    gi_atom_id_t *net_supported_list;
    // list of virtual root windows
    //gi_window_id_t *net_virtual_root_list;
    // client leader window
    //gi_window_id_t wm_client_leader;

    time_t time;
    time_t userTime;

    // starts to ignore bad window errors from X
    static inline void ignoreBadwindow() {
        qt_pfData->ignore_badwindow = true;
        qt_pfData->seen_badwindow = false;
    }

    // ends ignoring bad window errors and returns whether an error had happened.
    static inline bool badwindow() {
        qt_pfData->ignore_badwindow = false;
        return qt_pfData->seen_badwindow;
    }

    bool ignore_badwindow;
    bool seen_badwindow;

    // options
    int color_count;
    bool custom_cmap;
   
    bool has_fontconfig;
    qreal fc_scale;
    bool fc_antialias;
    int fc_hint_style;

   
    /* Warning: if you modify this list, modify the names of atoms in qapplication_x11.cpp as well! */
    enum PFAtom {
        // window-manager <-> client protocols
        WM_PROTOCOLS,
        WM_DELETE_WINDOW,
        WM_TAKE_FOCUS,
        _NET_WM_PING,
        _NET_WM_CONTEXT_HELP,
        _NET_WM_SYNC_REQUEST,
        _NET_WM_SYNC_REQUEST_COUNTER,

        // ICCCM window state
        WM_STATE,
        WM_CHANGE_STATE,

        // Session management
        WM_CLIENT_LEADER,
        WM_WINDOW_ROLE,
        SM_CLIENT_ID,

        // Clipboard
        CLIPBOARD,
        INCR,
        TARGETS,
        MULTIPLE,
        TIMESTAMP,
        SAVE_TARGETS,
        CLIP_TEMPORARY,
        _QT_SELECTION,
        _QT_CLIPBOARD_SENTINEL,
        _QT_SELECTION_SENTINEL,
        CLIPBOARD_MANAGER,

        RESOURCE_MANAGER,

        _XSETROOT_ID,

        _QT_SCROLL_DONE,
        _QT_INPUT_ENCODING,

        _MOTIF_WM_HINTS,

        DTWM_IS_RUNNING,
        ENLIGHTENMENT_DESKTOP,
        _DT_SAVE_MODE,
        _SGI_DESKS_MANAGER,

        // EWMH (aka NETWM)
        _NET_SUPPORTED,
        _NET_VIRTUAL_ROOTS,
        _NET_WORKAREA,

        _NET_MOVERESIZE_WINDOW,
        _NET_WM_MOVERESIZE,

        _NET_WM_NAME,
        _NET_WM_ICON_NAME,
        _NET_WM_ICON,

        _NET_WM_PID,

        _NET_WM_WINDOW_OPACITY,

        _NET_WM_STATE,
        _NET_WM_STATE_ABOVE,
        _NET_WM_STATE_BELOW,
        _NET_WM_STATE_FULLSCREEN,
        _NET_WM_STATE_MAXIMIZED_HORZ,
        _NET_WM_STATE_MAXIMIZED_VERT,
        _NET_WM_STATE_MODAL,
        _NET_WM_STATE_STAYS_ON_TOP,
        _NET_WM_STATE_DEMANDS_ATTENTION,

        _NET_WM_USER_TIME,
        _NET_WM_USER_TIME_WINDOW,
        _NET_WM_FULL_PLACEMENT,

        _NET_WM_WINDOW_TYPE,
        _NET_WM_WINDOW_TYPE_DESKTOP,
        _NET_WM_WINDOW_TYPE_DOCK,
        _NET_WM_WINDOW_TYPE_TOOLBAR,
        _NET_WM_WINDOW_TYPE_MENU,
        _NET_WM_WINDOW_TYPE_UTILITY,
        _NET_WM_WINDOW_TYPE_SPLASH,
        _NET_WM_WINDOW_TYPE_DIALOG,
        _NET_WM_WINDOW_TYPE_DROPDOWN_MENU,
        _NET_WM_WINDOW_TYPE_POPUP_MENU,
        _NET_WM_WINDOW_TYPE_TOOLTIP,
        _NET_WM_WINDOW_TYPE_NOTIFICATION,
        _NET_WM_WINDOW_TYPE_COMBO,
        _NET_WM_WINDOW_TYPE_DND,
        _NET_WM_WINDOW_TYPE_NORMAL,
        _KDE_NET_WM_WINDOW_TYPE_OVERRIDE,

        _KDE_NET_WM_FRAME_STRUT,

        _NET_STARTUP_INFO,
        _NET_STARTUP_INFO_BEGIN,

        _NET_SUPPORTING_WM_CHECK,

        _NET_WM_CM_S0,

        _NET_SYSTEM_TRAY_VISUAL,

        _NET_ACTIVE_WINDOW,

        // Property formats
        COMPOUND_TEXT,
        TEXT,
        UTF8_STRING,

        // Xdnd
        XdndEnter,
        XdndPosition,
        XdndStatus,
        XdndLeave,
        XdndDrop,
        XdndFinished,
        XdndTypelist,
        XdndActionList,

        XdndSelection,

        XdndAware,
        XdndProxy,

        XdndActionCopy,
        XdndActionLink,
        XdndActionMove,
        XdndActionPrivate,

        // Motif DND
        _MOTIF_DRAG_AND_DROP_MESSAGE,
        _MOTIF_DRAG_INITIATOR_INFO,
        _MOTIF_DRAG_RECEIVER_INFO,
        _MOTIF_DRAG_WINDOW,
        _MOTIF_DRAG_TARGETS,

        XmTRANSFER_SUCCESS,
        XmTRANSFER_FAILURE,

        // Xkb
        _XKB_RULES_NAMES,

        // XEMBED
        _XEMBED,
        _XEMBED_INFO,

        XWacomStylus,
        XWacomCursor,
        XWacomEraser,

        XTabletStylus,
        XTabletEraser,

        NPredefinedAtoms,

        _QT_SETTINGS_TIMESTAMP = NPredefinedAtoms,
        NAtoms
    };
    gi_atom_id_t atoms[NAtoms];

};

extern QpfData *qt_pfData;
#define ATOM(x) qt_pfData->atoms[QpfData::x]
//#define X11 qt_pfData

extern "C" void get_click_speed(bigtime_t	*interval);

QT_END_NAMESPACE

QT_END_HEADER

#endif
