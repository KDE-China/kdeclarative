/*
    Copyright 2011 Marco Martin <notmart@gmail.com>
    Copyright 2013 Sebastian Kügler <sebas@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "mouseeventlistener.h"

#include <QGuiApplication>
#include <QStyleHints>
#include <QEvent>
#include <QMouseEvent>
#include <QTimer>
#include <QQuickWindow>
#include <QScreen>
#include <QDebug>

MouseEventListener::MouseEventListener(QQuickItem *parent)
    : QQuickItem(parent),
    m_pressed(false),
    m_pressAndHoldEvent(nullptr),
    m_lastEvent(nullptr),
    m_containsMouse(false),
    m_acceptedButtons(Qt::LeftButton)
{
    m_pressAndHoldTimer = new QTimer(this);
    m_pressAndHoldTimer->setSingleShot(true);
    connect(m_pressAndHoldTimer, SIGNAL(timeout()),
            this, SLOT(handlePressAndHold()));

    qmlRegisterType<KDeclarativeMouseEvent>();
    qmlRegisterType<KDeclarativeWheelEvent>();

    setFiltersChildMouseEvents(true);
    setAcceptedMouseButtons(Qt::LeftButton|Qt::RightButton|Qt::MidButton|Qt::XButton1|Qt::XButton2);
}

MouseEventListener::~MouseEventListener()
{
}

Qt::MouseButtons MouseEventListener::acceptedButtons() const
{
    return m_acceptedButtons;
}

Qt::CursorShape MouseEventListener::cursorShape() const
{
    return cursor().shape();
}

void MouseEventListener::setCursorShape(Qt::CursorShape shape)
{
    if (cursor().shape() == shape) {
        return;
    }

    setCursor(shape);

    emit cursorShapeChanged();
}

void MouseEventListener::setAcceptedButtons(Qt::MouseButtons buttons)
{
    if (buttons == m_acceptedButtons) {
        return;
    }

    m_acceptedButtons = buttons;
    emit acceptedButtonsChanged();
}

void MouseEventListener::setHoverEnabled(bool enable)
{
    if (enable == acceptHoverEvents()) {
        return;
    }

    setAcceptHoverEvents(enable);
    emit hoverEnabledChanged(enable);
}

bool MouseEventListener::hoverEnabled() const
{
    return acceptHoverEvents();
}

bool MouseEventListener::isPressed() const
{
    return m_pressed;
}

void MouseEventListener::hoverEnterEvent(QHoverEvent *event)
{
    Q_UNUSED(event);

    m_containsMouse = true;
    emit containsMouseChanged(true);
}

void MouseEventListener::hoverLeaveEvent(QHoverEvent *event)
{
    Q_UNUSED(event);

    m_containsMouse = false;
    emit containsMouseChanged(false);
}

void MouseEventListener::hoverMoveEvent(QHoverEvent * event)
{
    if (m_lastEvent == event) {
        return;
    }

    QQuickWindow *w = window();
    QPoint screenPos;
    if (w) {
        screenPos = w->mapToGlobal(event->pos());
    }

    KDeclarativeMouseEvent dme(event->pos().x(), event->pos().y(), screenPos.x(), screenPos.y(), Qt::NoButton, Qt::NoButton, event->modifiers(), nullptr);
    emit positionChanged(&dme);
}

bool MouseEventListener::containsMouse() const
{
    return m_containsMouse;
}

void MouseEventListener::mousePressEvent(QMouseEvent *me)
{
    if (m_lastEvent == me || !(me->buttons() & m_acceptedButtons)) {
        me->setAccepted(false);
        return;
    }

    //FIXME: when a popup window is visible: a click anywhere hides it: but the old qquickitem will continue to think it's under the mouse
    //doesn't seem to be any good way to properly reset this.
    //this msolution will still caused a missed click after the popup is gone, but gets the situation unblocked.
    QPoint viewPosition;
    if (window()) {
        viewPosition = window()->position();
    }

    if (!QRectF(mapToScene(QPoint(0, 0)) + viewPosition, QSizeF(width(), height())).contains(me->screenPos())) {
        me->ignore();
        return;
    }
    m_buttonDownPos = me->screenPos();

    KDeclarativeMouseEvent dme(me->pos().x(), me->pos().y(), me->screenPos().x(), me->screenPos().y(), me->button(), me->buttons(), me->modifiers(), screenForGlobalPos(me->globalPos()));
    if (!m_pressAndHoldEvent) {
        m_pressAndHoldEvent = new KDeclarativeMouseEvent(me->pos().x(), me->pos().y(), me->screenPos().x(), me->screenPos().y(), me->button(), me->buttons(), me->modifiers(), screenForGlobalPos(me->globalPos()));
    }

    m_pressed = true;
    emit pressed(&dme);
    emit pressedChanged();

    if (dme.isAccepted()) {
        me->setAccepted(true);
        return;
    }

    m_pressAndHoldTimer->start(QGuiApplication::styleHints()->mousePressAndHoldInterval());
}

void MouseEventListener::mouseMoveEvent(QMouseEvent *me)
{
    if (m_lastEvent == me || !(me->buttons() & m_acceptedButtons)) {
        me->setAccepted(false);
        return;
    }

    if (QPointF(me->screenPos() - m_buttonDownPos).manhattanLength() > QGuiApplication::styleHints()->startDragDistance() && m_pressAndHoldTimer->isActive()) {
        m_pressAndHoldTimer->stop();
    }

    KDeclarativeMouseEvent dme(me->pos().x(), me->pos().y(), me->screenPos().x(), me->screenPos().y(), me->button(), me->buttons(), me->modifiers(), screenForGlobalPos(me->globalPos()));
    emit positionChanged(&dme);

    if (dme.isAccepted()) {
        me->setAccepted(true);
    }
}

void MouseEventListener::mouseReleaseEvent(QMouseEvent *me)
{
    if (m_lastEvent == me) {
        me->setAccepted(false);
        return;
    }

    KDeclarativeMouseEvent dme(me->pos().x(), me->pos().y(), me->screenPos().x(), me->screenPos().y(), me->button(), me->buttons(), me->modifiers(), screenForGlobalPos(me->globalPos()));
    m_pressed = false;
    emit released(&dme);
    emit pressedChanged();

    if (boundingRect().contains(me->pos()) && m_pressAndHoldTimer->isActive()) {
        emit clicked(&dme);
        m_pressAndHoldTimer->stop();
    }

    if (dme.isAccepted()) {
        me->setAccepted(true);
    }
}

void MouseEventListener::wheelEvent(QWheelEvent *we)
{
    if (m_lastEvent == we) {
        return;
    }

    KDeclarativeWheelEvent dwe(we->pos(), we->globalPos(), we->delta(), we->buttons(), we->modifiers(), we->orientation());
    emit wheelMoved(&dwe);
}

void MouseEventListener::handlePressAndHold()
{
    if (m_pressed) {
        emit pressAndHold(m_pressAndHoldEvent);

        delete m_pressAndHoldEvent;
        m_pressAndHoldEvent = nullptr;
    }
}

bool MouseEventListener::childMouseEventFilter(QQuickItem *item, QEvent *event)
{
    if (!isEnabled()) {
        return false;
    }

    //don't filter other mouseeventlisteners
    if (qobject_cast<MouseEventListener *>(item)) {
        return false;
    }

    switch (event->type()) {
    case QEvent::MouseButtonPress: {
        m_lastEvent = event;
        QMouseEvent *me = static_cast<QMouseEvent *>(event);

        if (!(me->buttons() & m_acceptedButtons)) {
            break;
        }

        //the parent will receive events in its own coordinates
        const QPointF myPos = mapFromScene(me->windowPos());

        KDeclarativeMouseEvent dme(myPos.x(), myPos.y(), me->screenPos().x(), me->screenPos().y(), me->button(), me->buttons(), me->modifiers(), screenForGlobalPos(me->globalPos()));
        delete m_pressAndHoldEvent;
        m_pressAndHoldEvent = new KDeclarativeMouseEvent(myPos.x(), myPos.y(), me->screenPos().x(), me->screenPos().y(), me->button(), me->buttons(), me->modifiers(), screenForGlobalPos(me->globalPos()));

        //qDebug() << "pressed in sceneEventFilter";
        m_buttonDownPos = me->screenPos();
        m_pressed = true;
        emit pressed(&dme);
        emit pressedChanged();

        if (dme.isAccepted()) {
            return true;
        }

        m_pressAndHoldTimer->start(QGuiApplication::styleHints()->mousePressAndHoldInterval());

        break;
    }
    case QEvent::HoverMove: {
        if (!acceptHoverEvents()) {
            break;
        }
        m_lastEvent = event;
        QHoverEvent *he = static_cast<QHoverEvent *>(event);
        const QPointF myPos = item->mapToItem(this, he->pos());

        QQuickWindow *w = window();
        QPoint screenPos;
        if (w) {
            screenPos = w->mapToGlobal(myPos.toPoint());
        }

        KDeclarativeMouseEvent dme(myPos.x(), myPos.y(), screenPos.x(), screenPos.y(), Qt::NoButton, Qt::NoButton, he->modifiers(), nullptr);
        //qDebug() << "positionChanged..." << dme.x() << dme.y();
        emit positionChanged(&dme);

        if (dme.isAccepted()) {
            return true;
        }
        break;
    }
    case QEvent::MouseMove: {
        m_lastEvent = event;
        QMouseEvent *me = static_cast<QMouseEvent *>(event);
        if (!(me->buttons() & m_acceptedButtons)) {
            break;
        }

        const QPointF myPos = mapFromScene(me->windowPos());
        KDeclarativeMouseEvent dme(myPos.x(), myPos.y(), me->screenPos().x(), me->screenPos().y(), me->button(), me->buttons(), me->modifiers(), screenForGlobalPos(me->globalPos()));
        //qDebug() << "positionChanged..." << dme.x() << dme.y();

        //stop the pressandhold if mouse moved enough
        if (QPointF(me->screenPos() - m_buttonDownPos).manhattanLength() > QGuiApplication::styleHints()->startDragDistance() && m_pressAndHoldTimer->isActive()) {
            m_pressAndHoldTimer->stop();

        //if the mouse moves and we are waiting to emit a press and hold event, update the co-ordinates
        //as there is no update function, delete the old event and create a new one
        } else if (m_pressAndHoldEvent) {
            delete m_pressAndHoldEvent;
            m_pressAndHoldEvent = new KDeclarativeMouseEvent(myPos.x(), myPos.y(), me->screenPos().x(), me->screenPos().y(), me->button(), me->buttons(), me->modifiers(), screenForGlobalPos(me->globalPos()));
        }
        emit positionChanged(&dme);

        if (dme.isAccepted()) {
            return true;
        }
        break;
    }
    case QEvent::MouseButtonRelease: {
        m_lastEvent = event;
        QMouseEvent *me = static_cast<QMouseEvent *>(event);

        const QPointF myPos = mapFromScene(me->windowPos());
        KDeclarativeMouseEvent dme(myPos.x(), myPos.y(), me->screenPos().x(), me->screenPos().y(), me->button(), me->buttons(), me->modifiers(), screenForGlobalPos(me->globalPos()));
        m_pressed = false;

        emit released(&dme);
        emit pressedChanged();

        if (QPointF(me->screenPos() - m_buttonDownPos).manhattanLength() <= QGuiApplication::styleHints()->startDragDistance() && m_pressAndHoldTimer->isActive()) {
            emit clicked(&dme);
            m_pressAndHoldTimer->stop();
        }

        if (dme.isAccepted()) {
            return true;
        }
        break;
    }
    case QEvent::UngrabMouse: {
        m_lastEvent = event;
        handleUngrab();
        break;
    }
    case QEvent::Wheel: {
        m_lastEvent = event;
        QWheelEvent *we = static_cast<QWheelEvent *>(event);
        KDeclarativeWheelEvent dwe(we->pos(), we->globalPos(), we->delta(), we->buttons(), we->modifiers(), we->orientation());
        emit wheelMoved(&dwe);
        break;
    }
    default:
        break;
    }

    return QQuickItem::childMouseEventFilter(item, event);
//    return false;
}

QScreen* MouseEventListener::screenForGlobalPos(const QPoint& globalPos)
{
    foreach(QScreen *screen, QGuiApplication::screens()) {
        if (screen->geometry().contains(globalPos)) {
            return screen;
        }
    }
    return nullptr;
}

void MouseEventListener::mouseUngrabEvent()
{
    handleUngrab();

    QQuickItem::mouseUngrabEvent();
}

void MouseEventListener::touchUngrabEvent()
{
    handleUngrab();

    QQuickItem::touchUngrabEvent();
}

void MouseEventListener::handleUngrab()
{
    if (m_pressed) {
        m_pressAndHoldTimer->stop();

        m_pressed = false;
        emit pressedChanged();

        emit canceled();
    }
}
