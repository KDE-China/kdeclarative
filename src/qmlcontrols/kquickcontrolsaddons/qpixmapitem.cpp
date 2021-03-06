/*
 *   Copyright 2011 Marco Martin <mart@kde.org>
 *   Copyright 2015 Luca Beltrame <lbeltrame@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "qpixmapitem.h"

#include <QPainter>


QPixmapItem::QPixmapItem(QQuickItem *parent)
    : QQuickPaintedItem(parent),
      m_smooth(false),
      m_fillMode(QPixmapItem::Stretch)
{
    setFlag(ItemHasContents, true);

}


QPixmapItem::~QPixmapItem()
{
}

void QPixmapItem::setPixmap(const QPixmap &pixmap)
{
    bool oldPixmapNull = m_pixmap.isNull();
    m_pixmap = pixmap;
    updatePaintedRect();
    update();
    emit nativeWidthChanged();
    emit nativeHeightChanged();
    emit pixmapChanged();
    if (oldPixmapNull != m_pixmap.isNull()) {
        emit nullChanged();
    }
}

QPixmap QPixmapItem::pixmap() const
{
    return m_pixmap;
}

void QPixmapItem::resetPixmap()
{
    setPixmap(QPixmap());
}

void QPixmapItem::setSmooth(const bool smooth)
{
    if (smooth == m_smooth) {
        return;
    }
    m_smooth = smooth;
    update();
}

bool QPixmapItem::smooth() const
{
    return m_smooth;
}

int QPixmapItem::nativeWidth() const
{
    return m_pixmap.size().width() / m_pixmap.devicePixelRatio();
}

int QPixmapItem::nativeHeight() const
{
    return m_pixmap.size().height() / m_pixmap.devicePixelRatio();
}

QPixmapItem::FillMode QPixmapItem::fillMode() const
{
    return m_fillMode;
}

void QPixmapItem::setFillMode(QPixmapItem::FillMode mode)
{
    if (mode == m_fillMode) {
        return;
    }

    m_fillMode = mode;
    updatePaintedRect();
    update();
    emit fillModeChanged();

}

void QPixmapItem::paint(QPainter *painter)
{
    if (m_pixmap.isNull()) {
        return;
    }
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, m_smooth);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, m_smooth);

    if (m_fillMode == TileVertically) {
        painter->scale(width()/(qreal)m_pixmap.width(), 1);
    }

    if (m_fillMode == TileHorizontally) {
        painter->scale(1, height()/(qreal)m_pixmap.height());
    }

    if (m_fillMode >= Tile) {
        painter->drawTiledPixmap(m_paintedRect, m_pixmap);
    } else {
        painter->drawPixmap(m_paintedRect, m_pixmap, m_pixmap.rect());
    }

    painter->restore();
}

bool QPixmapItem::isNull() const
{
    return m_pixmap.isNull();
}


int QPixmapItem::paintedWidth() const
{
    if (m_pixmap.isNull()) {
        return 0;
    }

    return m_paintedRect.width();
}

int QPixmapItem::paintedHeight() const
{
    if (m_pixmap.isNull()) {
        return 0;
    }

    return m_paintedRect.height();
}

void QPixmapItem::updatePaintedRect()
{

    if (m_pixmap.isNull()) {
        return;
    }

    QRect sourceRect = m_paintedRect;

    QRect destRect;

    switch (m_fillMode) {
    case PreserveAspectFit: {
        QSize scaled = m_pixmap.size();

        scaled.scale(boundingRect().size().toSize(), Qt::KeepAspectRatio);
        destRect = QRect(QPoint(0, 0), scaled);
        destRect.moveCenter(boundingRect().center().toPoint());
        break;
    }
    case PreserveAspectCrop: {
        QSize scaled = m_pixmap.size();

        scaled.scale(boundingRect().size().toSize(), Qt::KeepAspectRatioByExpanding);
        destRect = QRect(QPoint(0, 0), scaled);
        destRect.moveCenter(boundingRect().center().toPoint());
        break;
    }
    case TileVertically: {
        destRect = boundingRect().toRect();
        destRect.setWidth(destRect.width() / (width()/(qreal)m_pixmap.width()));
        break;
    }
    case TileHorizontally: {
        destRect = boundingRect().toRect();
        destRect.setHeight(destRect.height() / (height()/(qreal)m_pixmap.height()));
        break;
    }
    case Stretch:
    case Tile:
    default:
        destRect = boundingRect().toRect();
    }

    if (destRect != sourceRect) {
        m_paintedRect = destRect;
        emit paintedHeightChanged();
        emit paintedWidthChanged();
    }
}

void QPixmapItem::geometryChanged(const QRectF& newGeometry, const QRectF& oldGeometry)
{
    QQuickPaintedItem::geometryChanged(newGeometry, oldGeometry);
    updatePaintedRect();
}

