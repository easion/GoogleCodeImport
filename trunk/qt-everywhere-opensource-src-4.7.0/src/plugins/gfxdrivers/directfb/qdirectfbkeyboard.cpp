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

#include "qdirectfbkeyboard.h"

#ifndef QT_NO_QWS_DIRECTFB

#include "qdirectfbscreen.h"
#include <qobject.h>
#include <qsocketnotifier.h>
#include <qhash.h>

#include <gi/gi.h>
#include <gi/property.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

QT_BEGIN_NAMESPACE

class KeyMap : public QHash<gi_keycode_t, Qt::Key>
{
public:
    KeyMap();
};

Q_GLOBAL_STATIC(KeyMap, keymap);

class QGixKeyboardHandlerPrivate : public QObject
{
    Q_OBJECT
public:
    QGixKeyboardHandlerPrivate(QGixKeyboardHandler *handler);
    ~QGixKeyboardHandlerPrivate();

    void suspend();
    void resume();

private:
    QGixKeyboardHandler *handler;
    //IGixEventBuffer *eventBuffer;
    QSocketNotifier *keyboardNotifier;
    gi_msg_t event;
    int bytesRead;
    int lastUnicode, lastKeycode;
    Qt::KeyboardModifiers lastModifiers;
private Q_SLOTS:
    void readKeyboardData();
};

QGixKeyboardHandlerPrivate::QGixKeyboardHandlerPrivate(QGixKeyboardHandler *h)
    : handler(h),  keyboardNotifier(0), bytesRead(0),
      lastUnicode(0), lastKeycode(0), lastModifiers(0)
{
    Q_ASSERT(qt_screen);

    /*IGix *fb = QGixScreen::instance()->dfb();
    if (!fb) {
        qCritical("QGixKeyboardHandler: Gix not initialized");
        return;
    }

    int result;
    result = fb->CreateInputEventBuffer(fb, DICAPS_KEYS, DFB_TRUE,
                                        &eventBuffer);
    if (result != DFB_OK) {
        PLATFORM_ERROR("QGixKeyboardHandler: "
                      "Unable to create input event buffer", result);
        return;
    }
	*/

    int fd;

	fd = gi_get_connection_fd();

    //result = eventBuffer->CreateFileDescriptor(eventBuffer, &fd);
    if (fd < 0) {
        PLATFORM_ERROR("QGixKeyboardHandler: "
                      "Unable to create file descriptor");
        return;
    }

    int flags = ::fcntl(fd, F_GETFL, 0);
    ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    memset(&event, 0, sizeof(event));

    keyboardNotifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
    connect(keyboardNotifier, SIGNAL(activated(int)),
            this, SLOT(readKeyboardData()));
    resume();
}

void QGixKeyboardHandlerPrivate::suspend()
{
    keyboardNotifier->setEnabled(false);
}

void QGixKeyboardHandlerPrivate::resume()
{
    //eventBuffer->Reset(eventBuffer);
    //keyboardNotifier->setEnabled(true);
}

QGixKeyboardHandlerPrivate::~QGixKeyboardHandlerPrivate()
{
    //if (eventBuffer)
        //eventBuffer->Release(eventBuffer);
}

void QGixKeyboardHandlerPrivate::readKeyboardData()
{
    if(!qt_screen)
        return;

    for (;;) {
        // GetEvent returns DFB_UNSUPPORTED after CreateFileDescriptor().
        // This seems stupid and I really hope it's a bug which will be fixed.

        // int ret = eventBuffer->GetEvent(eventBuffer, &event);

        char *buf = reinterpret_cast<char*>(&event);
        int ret = ::read(keyboardNotifier->socket(),
                         buf + bytesRead, sizeof(gi_msg_t) - bytesRead);
        if (ret == -1) {
            if (errno != EAGAIN)
                qWarning("QGixKeyboardHandlerPrivate::readKeyboardData(): %s",
                         strerror(errno));
            return;
        }

        Q_ASSERT(ret >= 0);
        bytesRead += ret;
        if (bytesRead < int(sizeof(gi_msg_t)))
            break;
        bytesRead = 0;

#if 0 //FIXME DPP
        Q_ASSERT(event.clazz == DFEC_INPUT);

        const DFBInputEvent input = event.input;

        Qt::KeyboardModifiers modifiers = Qt::NoModifier;

        // Not implemented:
        // if (input.modifiers & DIMM_SUPER)
        // if (input.modifiers & DIMM_HYPER)

        if (!(input.flags & DIEF_KEYSYMBOL) ||
            !(input.flags & DIEF_KEYID) ||
            !(input.type & (DIET_KEYPRESS|DIET_KEYRELEASE)))
        {
            static bool first = true;
            if (first) {
                qWarning("QGixKeyboardHandler - Getting unexpected non-keyboard related events");
                first = false;
            }
            break;
        }

        if (input.flags & DIEF_MODIFIERS) {
            if (input.modifiers & DIMM_SHIFT)
                modifiers |= Qt::ShiftModifier;
            if (input.modifiers & DIMM_CONTROL)
                modifiers |= Qt::ControlModifier;
            if (input.modifiers & DIMM_ALT)
                modifiers |= Qt::AltModifier;
            if (input.modifiers & DIMM_ALTGR)
                modifiers |= Qt::AltModifier;
            if (input.modifiers & DIMM_META)
                modifiers |= Qt::MetaModifier;
        }


        const bool press = input.type & DIET_KEYPRESS;
        gi_keycode_t symbol = input.key_symbol;
        int unicode = -1;
        int keycode = 0;

        keycode = keymap()->value(symbol);
        if (DFB_KEY_TYPE(symbol) == DIKT_UNICODE)
            unicode = symbol;

        if (unicode != -1 || keycode != 0) {
            bool autoRepeat = false;
            if (press) {
                if (unicode == lastUnicode && keycode == lastKeycode && modifiers == lastModifiers) {
                    autoRepeat = true;
                } else {
                    lastUnicode = unicode;
                    lastKeycode = keycode;
                    lastModifiers = modifiers;
                }
            } else {
                lastUnicode = lastKeycode = -1;
                lastModifiers = 0;
            }
            if (autoRepeat) {
                handler->processKeyEvent(unicode, keycode,
                                         modifiers, false, autoRepeat);

            }

            handler->processKeyEvent(unicode, keycode,
                                     modifiers, press, autoRepeat);
        }
#endif
    }
}

QGixKeyboardHandler::QGixKeyboardHandler(const QString &device)
    : QWSKeyboardHandler()
{
    Q_UNUSED(device);
    d = new QGixKeyboardHandlerPrivate(this);
}

QGixKeyboardHandler::~QGixKeyboardHandler()
{
    delete d;
}

KeyMap::KeyMap()
{
    insert(G_KEY_BACKSPACE             , Qt::Key_Backspace);
    insert(G_KEY_TAB                   , Qt::Key_Tab);
    insert(G_KEY_RETURN                , Qt::Key_Return);
    insert(G_KEY_ESCAPE                , Qt::Key_Escape);
    insert(G_KEY_DELETE                , Qt::Key_Delete);

    insert(G_KEY_LEFT           , Qt::Key_Left);
    insert(G_KEY_RIGHT          , Qt::Key_Right);
    insert(G_KEY_UP             , Qt::Key_Up);
    insert(G_KEY_DOWN           , Qt::Key_Down);
    insert(G_KEY_INSERT                , Qt::Key_Insert);
    insert(G_KEY_HOME                  , Qt::Key_Home);
    insert(G_KEY_END                   , Qt::Key_End);
    insert(G_KEY_PAGEUP               , Qt::Key_PageUp);
    insert(G_KEY_PAGEDOWN             , Qt::Key_PageDown);
    insert(G_KEY_PRINT                 , Qt::Key_Print);
    insert(G_KEY_PAUSE                 , Qt::Key_Pause);
    //insert(G_KEY_GOTO                  , Qt::Key_OpenUrl);
    insert(G_KEY_CLEAR                 , Qt::Key_Clear);
    insert(G_KEY_MENU                  , Qt::Key_Menu);
    insert(G_KEY_HELP                  , Qt::Key_Help);

    //insert(G_KEY_INTERNET              , Qt::Key_HomePage);
    //insert(G_KEY_MAIL                  , Qt::Key_LaunchMail);
    //insert(G_KEY_FAVORITES             , Qt::Key_Favorites);

#if 0
    insert(G_KEY_SELECT                , Qt::Key_Select);
    insert(G_KEY_BACK                  , Qt::Key_Back);
    insert(G_KEY_FORWARD               , Qt::Key_Forward);
    insert(G_KEY_VOLUME_UP             , Qt::Key_VolumeUp);
    insert(G_KEY_VOLUME_DOWN           , Qt::Key_VolumeDown);
    insert(G_KEY_MUTE                  , Qt::Key_VolumeMute);
    insert(G_KEY_PLAYPAUSE             , Qt::Key_Pause);
    insert(G_KEY_PLAY                  , Qt::Key_MediaPlay);
    insert(G_KEY_STOP                  , Qt::Key_MediaStop);
    insert(G_KEY_RECORD                , Qt::Key_MediaRecord);
    insert(G_KEY_PREVIOUS              , Qt::Key_MediaPrevious);
    insert(G_KEY_NEXT                  , Qt::Key_MediaNext);

    insert(G_KEY_DEAD_ABOVEDOT         , Qt::Key_Dead_Abovedot);
    insert(G_KEY_DEAD_ABOVERING        , Qt::Key_Dead_Abovering);
    insert(G_KEY_DEAD_ACUTE            , Qt::Key_Dead_Acute);
    insert(G_KEY_DEAD_BREVE            , Qt::Key_Dead_Breve);
    insert(G_KEY_DEAD_CARON            , Qt::Key_Dead_Caron);
    insert(G_KEY_DEAD_CEDILLA          , Qt::Key_Dead_Cedilla);
    insert(G_KEY_DEAD_CIRCUMFLEX       , Qt::Key_Dead_Circumflex);
    insert(G_KEY_DEAD_DIAERESIS        , Qt::Key_Dead_Diaeresis);
    insert(G_KEY_DEAD_DOUBLEACUTE      , Qt::Key_Dead_Doubleacute);
    insert(G_KEY_DEAD_GRAVE            , Qt::Key_Dead_Grave);
    insert(G_KEY_DEAD_IOTA             , Qt::Key_Dead_Iota);
    insert(G_KEY_DEAD_MACRON           , Qt::Key_Dead_Macron);
    insert(G_KEY_DEAD_OGONEK           , Qt::Key_Dead_Ogonek);
    insert(G_KEY_DEAD_SEMIVOICED_SOUND , Qt::Key_Dead_Semivoiced_Sound);
    insert(G_KEY_DEAD_TILDE            , Qt::Key_Dead_Tilde);
    insert(G_KEY_DEAD_VOICED_SOUND     , Qt::Key_Dead_Voiced_Sound);
    insert(G_KEY_SPACE                 , Qt::Key_Space);
    insert(G_KEY_EXCLAMATION_MARK      , Qt::Key_Exclam);
    insert(G_KEY_QUOTATION             , Qt::Key_QuoteDbl);
    insert(G_KEY_NUMBER_SIGN           , Qt::Key_NumberSign);
    insert(G_KEY_DOLLAR_SIGN           , Qt::Key_Dollar);
    insert(G_KEY_PERCENT_SIGN          , Qt::Key_Percent);
    insert(G_KEY_AMPERSAND             , Qt::Key_Ampersand);
    insert(G_KEY_APOSTROPHE            , Qt::Key_Apostrophe);
    insert(G_KEY_PARENTHESIS_LEFT      , Qt::Key_ParenLeft);
    insert(G_KEY_PARENTHESIS_RIGHT     , Qt::Key_ParenRight);
    insert(G_KEY_ASTERISK              , Qt::Key_Asterisk);
    insert(G_KEY_PLUS_SIGN             , Qt::Key_Plus);
    insert(G_KEY_COMMA                 , Qt::Key_Comma);
    insert(G_KEY_MINUS_SIGN            , Qt::Key_Minus);
    insert(G_KEY_PERIOD                , Qt::Key_Period);
    insert(G_KEY_SLASH                 , Qt::Key_Slash);

    insert(G_KEY_COLON                 , Qt::Key_Colon);
    insert(G_KEY_SEMICOLON             , Qt::Key_Semicolon);
    insert(G_KEY_LESS_THAN_SIGN        , Qt::Key_Less);
    insert(G_KEY_EQUALS_SIGN           , Qt::Key_Equal);
    insert(G_KEY_GREATER_THAN_SIGN     , Qt::Key_Greater);
    insert(G_KEY_QUESTION_MARK         , Qt::Key_Question);
    insert(G_KEY_AT                    , Qt::Key_At);

    insert(G_KEY_SQUARE_BRACKET_LEFT   , Qt::Key_BracketLeft);
    insert(G_KEY_BACKSLASH             , Qt::Key_Backslash);
    insert(G_KEY_SQUARE_BRACKET_RIGHT  , Qt::Key_BracketRight);
    insert(G_KEY_CIRCUMFLEX_ACCENT     , Qt::Key_AsciiCircum);
    insert(G_KEY_UNDERSCORE            , Qt::Key_Underscore);

    insert(G_KEY_CURLY_BRACKET_LEFT    , Qt::Key_BraceLeft);
    insert(G_KEY_VERTICAL_BAR          , Qt::Key_Bar);
    insert(G_KEY_CURLY_BRACKET_RIGHT   , Qt::Key_BraceRight);
    insert(G_KEY_TILDE                 , Qt::Key_AsciiTilde);

#endif

    insert(G_KEY_F1                    , Qt::Key_F1);
    insert(G_KEY_F2                    , Qt::Key_F2);
    insert(G_KEY_F3                    , Qt::Key_F3);
    insert(G_KEY_F4                    , Qt::Key_F4);
    insert(G_KEY_F5                    , Qt::Key_F5);
    insert(G_KEY_F6                    , Qt::Key_F6);
    insert(G_KEY_F7                    , Qt::Key_F7);
    insert(G_KEY_F8                    , Qt::Key_F8);
    insert(G_KEY_F9                    , Qt::Key_F9);
    insert(G_KEY_F10                   , Qt::Key_F10);
    insert(G_KEY_F11                   , Qt::Key_F11);
    insert(G_KEY_F12                   , Qt::Key_F12);

    insert(G_KEY_LSHIFT                 , Qt::Key_Shift);
    insert(G_KEY_LCTRL               , Qt::Key_Control);
    insert(G_KEY_LALT                   , Qt::Key_Alt);
    //insert(G_KEY_LHYPER                 , Qt::Key_Hyper_L); // ???
    //insert(G_KEY_lALTGR                 , Qt::Key_AltGr);

    insert(G_KEY_LMETA                  , Qt::Key_Meta);
    insert(G_KEY_LSUPER                 , Qt::Key_Super_L); // ???

    insert(G_KEY_CAPSLOCK             , Qt::Key_CapsLock);
    insert(G_KEY_NUMLOCK              , Qt::Key_NumLock);
    insert(G_KEY_SCROLLOCK           , Qt::Key_ScrollLock);


    insert(G_KEY_0                     , Qt::Key_0);
    insert(G_KEY_1                     , Qt::Key_1);
    insert(G_KEY_2                     , Qt::Key_2);
    insert(G_KEY_3                     , Qt::Key_3);
    insert(G_KEY_4                     , Qt::Key_4);
    insert(G_KEY_5                     , Qt::Key_5);
    insert(G_KEY_6                     , Qt::Key_6);
    insert(G_KEY_7                     , Qt::Key_7);
    insert(G_KEY_8                     , Qt::Key_8);
    insert(G_KEY_9                     , Qt::Key_9);

    /*insert(G_KEY_CAPITAL_A             , Qt::Key_A);
    insert(G_KEY_CAPITAL_B             , Qt::Key_B);
    insert(G_KEY_CAPITAL_C             , Qt::Key_C);
    insert(G_KEY_CAPITAL_D             , Qt::Key_D);
    insert(G_KEY_CAPITAL_E             , Qt::Key_E);
    insert(G_KEY_CAPITAL_F             , Qt::Key_F);
    insert(G_KEY_CAPITAL_G             , Qt::Key_G);
    insert(G_KEY_CAPITAL_H             , Qt::Key_H);
    insert(G_KEY_CAPITAL_I             , Qt::Key_I);
    insert(G_KEY_CAPITAL_J             , Qt::Key_J);
    insert(G_KEY_CAPITAL_K             , Qt::Key_K);
    insert(G_KEY_CAPITAL_L             , Qt::Key_L);
    insert(G_KEY_CAPITAL_M             , Qt::Key_M);
    insert(G_KEY_CAPITAL_N             , Qt::Key_N);
    insert(G_KEY_CAPITAL_O             , Qt::Key_O);
    insert(G_KEY_CAPITAL_P             , Qt::Key_P);
    insert(G_KEY_CAPITAL_Q             , Qt::Key_Q);
    insert(G_KEY_CAPITAL_R             , Qt::Key_R);
    insert(G_KEY_CAPITAL_S             , Qt::Key_S);
    insert(G_KEY_CAPITAL_T             , Qt::Key_T);
    insert(G_KEY_CAPITAL_U             , Qt::Key_U);
    insert(G_KEY_CAPITAL_V             , Qt::Key_V);
    insert(G_KEY_CAPITAL_W             , Qt::Key_W);
    insert(G_KEY_CAPITAL_X             , Qt::Key_X);
    insert(G_KEY_CAPITAL_Y             , Qt::Key_Y);
    insert(G_KEY_CAPITAL_Z             , Qt::Key_Z);
	*/

    insert(G_KEY_a               , Qt::Key_A);
    insert(G_KEY_b               , Qt::Key_B);
    insert(G_KEY_c               , Qt::Key_C);
    insert(G_KEY_d               , Qt::Key_D);
    insert(G_KEY_e               , Qt::Key_E);
    insert(G_KEY_f               , Qt::Key_F);
    insert(G_KEY_g               , Qt::Key_G);
    insert(G_KEY_h               , Qt::Key_H);
    insert(G_KEY_i               , Qt::Key_I);
    insert(G_KEY_j               , Qt::Key_J);
    insert(G_KEY_k               , Qt::Key_K);
    insert(G_KEY_l               , Qt::Key_L);
    insert(G_KEY_m               , Qt::Key_M);
    insert(G_KEY_n               , Qt::Key_N);
    insert(G_KEY_o               , Qt::Key_O);
    insert(G_KEY_p               , Qt::Key_P);
    insert(G_KEY_q               , Qt::Key_Q);
    insert(G_KEY_r               , Qt::Key_R);
    insert(G_KEY_s               , Qt::Key_S);
    insert(G_KEY_t               , Qt::Key_T);
    insert(G_KEY_u               , Qt::Key_U);
    insert(G_KEY_v               , Qt::Key_V);
    insert(G_KEY_w               , Qt::Key_W);
    insert(G_KEY_x               , Qt::Key_X);
    insert(G_KEY_y               , Qt::Key_Y);
    insert(G_KEY_z               , Qt::Key_Z);

}

QT_END_NAMESPACE
#include "qdirectfbkeyboard.moc"
#endif // QT_NO_QWS_DIRECTFB
