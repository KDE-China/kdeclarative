/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2014  David Edmundson <davidedmundson@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef KHNSHELPER_H
#define KHNSHELPER_H

#include <QObject>
#include <QQuickItem>
#include <QPointer>

namespace KNS3 {
    class DownloadDialog;
}



/**
 * This class wraps GHNS Dialogs
 * It is a private class to be used by GHNSButton
 *
 */
class GHNSHelper : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString ksnrc MEMBER m_ksnrc NOTIFY ksnrcChanged)
    Q_PROPERTY(QString ksnrc MEMBER m_title NOTIFY titleChanged)

public:
    explicit GHNSHelper(QObject* parent = nullptr);

    virtual ~GHNSHelper();

public Q_SLOTS:
    void getNewStuff(QQuickItem *context);

Q_SIGNALS:
    void entriesChanged();
    void ksnrcChanged();
    void titleChanged();

private:
    QString m_ksnrc;
    QString m_title;
    QPointer<KNS3::DownloadDialog> m_newStuffDialog;
    Q_DISABLE_COPY(GHNSHelper)
};



#endif
