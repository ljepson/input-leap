/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2014-2016 Symless Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <QtCore/QObject>
#include <QtCore/QTimer>

#include "ZeroconfRecord.h"

class QSocketNotifier;

#include <avahi-client/client.h>
#include <avahi-client/publish.h>
#include <avahi-common/alternative.h>
#include <avahi-common/error.h>
#include <avahi-common/malloc.h>
#include <avahi-common/thread-watch.h>
#include <memory>

class ZeroconfRegisterAvahi : public QObject
{
    Q_OBJECT

public:
    ZeroconfRegisterAvahi(QObject* parent = nullptr);
    ~ZeroconfRegisterAvahi();

    void registerService(const ZeroconfRecord& record, quint16 servicePort);
    inline ZeroconfRecord registeredRecord() const { return finalRecord; }

Q_SIGNALS:
    void error(int error);
    void serviceRegistered(const ZeroconfRecord& record);

private slots:
    void reconnectToAvahi();

private:
    static void client_callback(AvahiClient *c, AvahiClientState state, void* userdata);
    static void entry_group_callback(AvahiEntryGroup *g, AvahiEntryGroupState state, void* userdata);

    void create_services();
    void reset();

private:
    AvahiClient *client_;
    AvahiEntryGroup *group_;
    AvahiThreadedPoll *threaded_poll_;

    ZeroconfRecord pendingRecord;
    ZeroconfRecord finalRecord;
    quint16 servicePort_;

    QTimer* reconnectTimer_;
    bool needsReconnect_;
};