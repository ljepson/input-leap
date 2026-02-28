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

#include "ZeroconfRegisterAvahi.h"

#include <QtCore/QDebug>

ZeroconfRegisterAvahi::ZeroconfRegisterAvahi(QObject* parent) :
    QObject(parent),
    client_(nullptr),
    group_(nullptr),
    threaded_poll_(nullptr),
    servicePort_(0),
    reconnectTimer_(new QTimer(this)),
    needsReconnect_(false)
{
    connect(reconnectTimer_, &QTimer::timeout, this, &ZeroconfRegisterAvahi::reconnectToAvahi);
    reconnectTimer_->setSingleShot(true);
}

ZeroconfRegisterAvahi::~ZeroconfRegisterAvahi()
{
    reset();
}

void ZeroconfRegisterAvahi::reset()
{
    if (group_) {
        avahi_entry_group_free(group_);
        group_ = nullptr;
    }

    if (client_) {
        avahi_client_free(client_);
        client_ = nullptr;
    }

    if (threaded_poll_) {
        avahi_threaded_poll_stop(threaded_poll_);
        avahi_threaded_poll_free(threaded_poll_);
        threaded_poll_ = nullptr;
    }
}

void ZeroconfRegisterAvahi::registerService(const ZeroconfRecord& record, quint16 servicePort)
{
    pendingRecord = record;
    servicePort_ = servicePort;

    if (client_) {
        qWarning("Warning: Already registered a service for this object");
        return;
    }

    // Create the threaded poll object
    threaded_poll_ = avahi_threaded_poll_new();
    if (!threaded_poll_) {
        qCritical("Failed to create Avahi threaded poll object");
        Q_EMIT this->error(AVAHI_ERR_NO_MEMORY);
        return;
    }

    // Create a new client
    int error;
    client_ = avahi_client_new(avahi_threaded_poll_get(threaded_poll_),
                               AVAHI_CLIENT_NO_FAIL,
                               client_callback,
                               this,
                               &error);

    if (!client_) {
        qCritical("Failed to create Avahi client: %s", avahi_strerror(error));
        Q_EMIT this->error(error);
        reset();
        return;
    }

    // Start the threaded poll loop
    if (avahi_threaded_poll_start(threaded_poll_) < 0) {
        qCritical("Failed to start Avahi threaded poll");
        Q_EMIT this->error(AVAHI_ERR_FAILURE);
        reset();
        return;
    }
}

void ZeroconfRegisterAvahi::reconnectToAvahi()
{
    if (needsReconnect_) {
        needsReconnect_ = false;

        if (client_) {
            avahi_client_free(client_);
            client_ = nullptr;
        }

        if (group_) {
            avahi_entry_group_free(group_);
            group_ = nullptr;
        }

        // Recreate client
        int error;
        client_ = avahi_client_new(avahi_threaded_poll_get(threaded_poll_),
                                   AVAHI_CLIENT_NO_FAIL,
                                   client_callback,
                                   this,
                                   &error);

        if (!client_) {
            qCritical("Failed to reconnect to Avahi client: %s", avahi_strerror(error));
            Q_EMIT this->error(error);
        }
    }
}

void ZeroconfRegisterAvahi::client_callback(AvahiClient *c, AvahiClientState state, void* userdata)
{
    ZeroconfRegisterAvahi* self = static_cast<ZeroconfRegisterAvahi*>(userdata);

    switch (state) {
        case AVAHI_CLIENT_S_RUNNING:
            // The server has startup successfully and registered its host
            // name on the network, so it's time to create our services
            self->create_services();
            break;

        case AVAHI_CLIENT_FAILURE:
            qCritical("Avahi client failure: %s", avahi_strerror(avahi_client_errno(c)));

            if (avahi_client_errno(c) == AVAHI_ERR_DISCONNECTED) {
                // Try to reconnect after a short delay
                self->needsReconnect_ = true;
                self->reconnectTimer_->start(1000); // 1 second delay
            } else {
                Q_EMIT self->error(avahi_client_errno(c));
            }
            break;

        case AVAHI_CLIENT_S_COLLISION:
        case AVAHI_CLIENT_S_REGISTERING:
            // The server records are now being established. This
            // might be caused by a host name change. We need to wait
            // for our own records to register until the host name is
            // properly established.
            if (self->group_) {
                avahi_entry_group_reset(self->group_);
            }
            break;

        case AVAHI_CLIENT_CONNECTING:
            // Client connecting, wait for it to complete
            break;
    }
}

void ZeroconfRegisterAvahi::entry_group_callback(AvahiEntryGroup *g, AvahiEntryGroupState state, void* userdata)
{
    ZeroconfRegisterAvahi* self = static_cast<ZeroconfRegisterAvahi*>(userdata);

    switch (state) {
        case AVAHI_ENTRY_GROUP_ESTABLISHED:
            // The entry group has been established successfully
            self->finalRecord = self->pendingRecord;
            Q_EMIT self->serviceRegistered(self->finalRecord);
            break;

        case AVAHI_ENTRY_GROUP_COLLISION: {
            // A service name collision with a remote service happened
            char *n = avahi_alternative_service_name(self->pendingRecord.serviceName.toUtf8().data());
            self->pendingRecord.serviceName = QString::fromUtf8(n);
            avahi_free(n);

            qWarning("Service name collision, renaming service to '%s'",
                     self->pendingRecord.serviceName.toUtf8().constData());

            // And recreate the services
            self->create_services();
            break;
        }

        case AVAHI_ENTRY_GROUP_FAILURE:
            qCritical("Entry group failure: %s", avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(g))));
            Q_EMIT self->error(avahi_client_errno(avahi_entry_group_get_client(g)));
            break;

        case AVAHI_ENTRY_GROUP_UNCOMMITED:
        case AVAHI_ENTRY_GROUP_REGISTERING:
            // Normal states during registration
            break;
    }
}

void ZeroconfRegisterAvahi::create_services()
{
    if (!group_) {
        group_ = avahi_entry_group_new(client_, entry_group_callback, this);
        if (!group_) {
            qCritical("avahi_entry_group_new() failed: %s", avahi_strerror(avahi_client_errno(client_)));
            Q_EMIT this->error(avahi_client_errno(client_));
            return;
        }
    }

    // If the group is empty (either because it was just created, or
    // because it was reset previously), add our entries.
    if (avahi_entry_group_is_empty(group_)) {
        int ret = avahi_entry_group_add_service(
            group_,
            AVAHI_IF_UNSPEC,
            AVAHI_PROTO_UNSPEC,
            static_cast<AvahiPublishFlags>(0),
            pendingRecord.serviceName.toUtf8().constData(),
            pendingRecord.registeredType.toUtf8().constData(),
            pendingRecord.replyDomain.isEmpty() ? nullptr : pendingRecord.replyDomain.toUtf8().constData(),
            nullptr,
            servicePort_,
            nullptr);

        if (ret < 0) {
            if (ret == AVAHI_ERR_COLLISION) {
                // A service name collision with a local service happened
                char *n = avahi_alternative_service_name(pendingRecord.serviceName.toUtf8().data());
                pendingRecord.serviceName = QString::fromUtf8(n);
                avahi_free(n);

                qWarning("Service name collision, renaming service to '%s'",
                         pendingRecord.serviceName.toUtf8().constData());

                avahi_entry_group_reset(group_);
                create_services();
                return;
            }

            qCritical("Failed to add service: %s", avahi_strerror(ret));
            Q_EMIT this->error(ret);
            return;
        }

        // Tell the server to register the service
        if ((ret = avahi_entry_group_commit(group_)) < 0) {
            qCritical("Failed to commit entry group: %s", avahi_strerror(ret));
            Q_EMIT this->error(ret);
            return;
        }
    }
}