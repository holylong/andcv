/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qnetworkaccessbackend_p.h"
#include "qnetworkreplyimpl_p.h"
#include "qnetworkaccessmanager_p.h"
#include "qnetworkrequest.h"
#include "qnetworkreply.h"
#include "qnetworkreply_p.h"
#include "QtCore/qmutex.h"
#include "QtCore/qstringlist.h"

#include "qnetworkaccesscachebackend_p.h"
#include "qabstractnetworkcache.h"
#include "qhostinfo.h"

#include "private/qnoncontiguousbytedevice_p.h"

QT_BEGIN_NAMESPACE

class QNetworkAccessBackendFactoryData: public QList<QNetworkAccessBackendFactory *>
{
public:
    QNetworkAccessBackendFactoryData()
    {
        valid.ref();
    }
    ~QNetworkAccessBackendFactoryData()
    {
        QMutexLocker locker(&mutex); // why do we need to lock?
        valid.deref();
    }

    QRecursiveMutex mutex;
    //this is used to avoid (re)constructing factory data from destructors of other global classes
    static QBasicAtomicInt valid;
};
Q_GLOBAL_STATIC(QNetworkAccessBackendFactoryData, factoryData)
QBasicAtomicInt QNetworkAccessBackendFactoryData::valid = Q_BASIC_ATOMIC_INITIALIZER(0);

class QNetworkAccessBackendPrivate : public QObjectPrivate
{
public:
    QNetworkAccessBackend::TargetTypes m_targetTypes;
    QNetworkAccessBackend::SecurityFeatures m_securityFeatures;
    QNetworkAccessBackend::IOFeatures m_ioFeatures;
    QSharedPointer<QNonContiguousByteDevice> uploadByteDevice;
    QIODevice *wrappedUploadByteDevice;
    QNetworkReplyImplPrivate *m_reply = nullptr;
    QNetworkAccessManagerPrivate *m_manager = nullptr;

    bool m_canCache = false;
    bool m_isSynchronous = false;
};

QNetworkAccessBackend *
QNetworkAccessManagerPrivate::findBackend(QNetworkAccessManager::Operation op,
                                          const QNetworkRequest &request)
{
    if (QNetworkAccessBackendFactoryData::valid.loadRelaxed()) {
        QMutexLocker locker(&factoryData()->mutex);
        QNetworkAccessBackendFactoryData::ConstIterator it = factoryData()->constBegin(),
                                                           end = factoryData()->constEnd();
        while (it != end) {
            QNetworkAccessBackend *backend = (*it)->create(op, request);
            if (backend) {
                backend->setManagerPrivate(this);
                return backend; // found a factory that handled our request
            }
            ++it;
        }
    }
    return nullptr;
}

QStringList QNetworkAccessManagerPrivate::backendSupportedSchemes() const
{
    if (QNetworkAccessBackendFactoryData::valid.loadRelaxed()) {
        QMutexLocker locker(&factoryData()->mutex);
        QNetworkAccessBackendFactoryData::ConstIterator it = factoryData()->constBegin();
        QNetworkAccessBackendFactoryData::ConstIterator end = factoryData()->constEnd();
        QStringList schemes;
        while (it != end) {
            schemes += (*it)->supportedSchemes();
            ++it;
        }
        return schemes;
    }
    return QStringList();
}

/*!
    \enum QNetworkAccessBackend::TargetType

    Use the values in this enum to specify what type of target
    the plugin supports. Setting the right type can be important,
    for example: proxyList() is only available for a Networked
    plugin.

    \value Networked
        The plugin supports and expect to connect to networked
        resources. E.g. over TCP, UDP or similar.
    \value Local
        The plugin supports and expects to access local files,
        generate data and/or locally connected devices.
*/

/*!
    \enum QNetworkAccessBackend::SecurityFeature

    Use the values in this enum to specify what type of security
    features the plugin may utilize. Setting the right type(s)
    can be important, for example: setSslConfiguration() may not
    be called for any plugin that do not claim to support TLS.

    \value None
        No specific features are claimed to be supported.
    \value TLS
        The plugin supports and expects to use TLS.
*/

/*!
    \enum QNetworkAccessBackend::IOFeature

    Use the values in this enum to specify what type of IO
    features the plugin may utilize.

    \value None
        No specific features are claimed to be supported.
    \value ZeroCopy
        The plugin will have raw data available in contiguous
        segments and can return a pointer to the data at request.
        Claiming to support this requires implementing readPointer()
        and advanceReadPointer().
    \value NeedResetableUpload
        The plugin may encounter scenarios where data to upload that
        has already been consumed needs to be restored and re-sent.
        E.g. some data was consumed and sent before a redirect
        response was received, and after the redirect the
        previously-consumed data needs to be re-sent.
    \omitvalue SupportsSynchronousMode
*/

/*!
    Constructs the QNetworkAccessBackend.
    You can opt in to specific backend behaviors with \a targetTypes,
    \a securityFeatures and \a ioFeatures.
    See their respective enums and values for more information.

    \sa TargetType, SecurityFeature, IOFeature
*/
QNetworkAccessBackend::QNetworkAccessBackend(TargetTypes targetTypes,
                                             SecurityFeatures securityFeatures,
                                             IOFeatures ioFeatures)
    : QObject(*(new QNetworkAccessBackendPrivate), nullptr)
{
    Q_D(QNetworkAccessBackend);
    d->m_targetTypes = targetTypes;
    d->m_securityFeatures = securityFeatures;
    d->m_ioFeatures = ioFeatures;
}

/*!
    \overload
*/
QNetworkAccessBackend::QNetworkAccessBackend(TargetTypes targetTypes)
    : QNetworkAccessBackend(targetTypes, SecurityFeature::None, IOFeature::None)
{
}

/*!
    \overload
*/
QNetworkAccessBackend::QNetworkAccessBackend(TargetTypes targetTypes,
                                             SecurityFeatures securityFeatures)
    : QNetworkAccessBackend(targetTypes, securityFeatures, IOFeature::None)
{
}

/*!
    \overload
*/
QNetworkAccessBackend::QNetworkAccessBackend(TargetTypes targetTypes, IOFeatures ioFeatures)
    : QNetworkAccessBackend(targetTypes, SecurityFeature::None, ioFeatures)
{
}

/*!
    Destructs the QNetworkAccessBackend base class.
*/
QNetworkAccessBackend::~QNetworkAccessBackend() { }

/*!
    Returns the security related features that the backend claims to
    support.

    \sa SecurityFeature
*/
QNetworkAccessBackend::SecurityFeatures QNetworkAccessBackend::securityFeatures() const noexcept
{
    return d_func()->m_securityFeatures;
}

/*!
    Returns the TargetTypes that the backend claims to target.

    \sa TargetType
*/
QNetworkAccessBackend::TargetTypes QNetworkAccessBackend::targetTypes() const noexcept
{
    return d_func()->m_targetTypes;
}

/*!
    Returns the I/O features that the backend claims to support.

    \sa IOFeature
*/
QNetworkAccessBackend::IOFeatures QNetworkAccessBackend::ioFeatures() const noexcept
{
    return d_func()->m_ioFeatures;
}

/*!
    Prepares the backend and calls open().
    E.g. for TargetType::Networked it will prepare proxyList().

    \sa TargetType, targetTypes
*/
bool QNetworkAccessBackend::start()
{
    Q_D(QNetworkAccessBackend);
#ifndef QT_NO_NETWORKPROXY
    if (targetTypes() & QNetworkAccessBackend::TargetType::Networked)
        d->m_reply->proxyList = d->m_manager->queryProxy(QNetworkProxyQuery(url()));
#endif

    // now start the request
    open();
    return true;
}

#if QT_CONFIG(ssl)
/*!
    Passes a \a configuration with the user's desired TLS
    configuration. If you don't have the TLS security feature this
    may not be called.

    \sa SecurityFeature, securityFeatures
*/
void QNetworkAccessBackend::setSslConfiguration(const QSslConfiguration &configuration)
{
    Q_UNUSED(configuration);
    if (securityFeatures() & SecurityFeature::TLS) {
        qWarning("Backend (%s) claiming to use TLS hasn't overridden setSslConfiguration.",
                 metaObject()->className());
    }
}

/*!
    Override this and return the QSslConfiguration used if you
    have the TLS security feature

    \sa SecurityFeature, securityFeatures
*/
QSslConfiguration QNetworkAccessBackend::sslConfiguration() const
{
    if (securityFeatures() & SecurityFeature::TLS) {
        qWarning("Backend (%s) claiming to use TLS hasn't overridden sslConfiguration.",
                 metaObject()->className());
    }
    return {};
}
#endif

/*!
    This function will be called when the user wants to ignore
    all TLS handshake errors. Derive this function if TLS is
    supported.

    \sa SecurityFeature, securityFeatures
*/
void QNetworkAccessBackend::ignoreSslErrors()
{
    if (securityFeatures() & SecurityFeature::TLS) {
        qWarning("Backend (%s) claiming to use TLS hasn't overridden ignoreSslErrors.",
                 metaObject()->className());
    }
}

/*!
    This function will be called when the user wants to ignore
    specific \a errors. Derive this function if TLS is supported.

    \sa SecurityFeature, securityFeatures
*/
void QNetworkAccessBackend::ignoreSslErrors(const QList<QSslError> &errors)
{
    Q_UNUSED(errors);
    if (securityFeatures() & SecurityFeature::TLS) {
        qWarning("Backend (%s) claiming to use TLS hasn't overridden ignoreSslErrors.",
                 metaObject()->className());
    }
}

/*!
    The data which the returned value views must stay valid until
    at least the next call to a non-const function. advanceReadPointer
    will be called if any of the data was used.

    Note: This will only be called if IOFeature::ZeroCopy was
    specified in the call to the constructor.

    \sa advanceReadPointer, read
*/
QByteArrayView QNetworkAccessBackend::readPointer()
{
    if (ioFeatures() & IOFeature::ZeroCopy) {
        qWarning("Backend (%s) claiming to support ZeroCopy hasn't overridden readPointer.",
                 metaObject()->className());
    }
    return {};
}

/*!
    This function is to notify your class that \a distance
    bytes have been read using readPointer and next time
    readPointer() is called those bytes should not be included.

    Note: This will only be called if IOFeature::ZeroCopy was
    specified in the call to the constructor.

    \sa readPointer
*/
void QNetworkAccessBackend::advanceReadPointer(qint64 distance)
{
    Q_UNUSED(distance);
    if (ioFeatures() & IOFeature::ZeroCopy) {
        qWarning("Backend (%s) claiming to support ZeroCopy hasn't overridden advanceReadPointer.",
                 metaObject()->className());
    }
}

/*!
    Implement this function to support reading from the resource
    made available by your plugin.
    Store data in \a data, up to a maximum of \a maxlen bytes.
    Then return the total amount of bytes that was copied.

    \sa readPointer, wantToRead
*/
qint64 QNetworkAccessBackend::read(char *data, qint64 maxlen)
{
    Q_UNUSED(data);
    Q_UNUSED(maxlen);
    if ((ioFeatures() & IOFeature::ZeroCopy) == 0) {
        qWarning("Backend (%s) is not ZeroCopy and has not implemented read(...)!",
                 metaObject()->className());
    }
    return 0;
}

/*!
    This is called before we read if there are no bytes available
    and we are ready to read more. Return \c true if new data was
    made available.

    \sa read, readPointer
*/
bool QNetworkAccessBackend::wantToRead()
{
    // Base implementation does nothing
    return false;
}

#if QT_CONFIG(networkproxy)
/*!
    Returns a list of proxies configured for the URL returned by
    url().

    It is only valid to call this function if TargetType::Networked
    was specified in the call to the constructor.
*/
QList<QNetworkProxy> QNetworkAccessBackend::proxyList() const
{
    Q_ASSERT(targetTypes() & TargetType::Networked);
    return d_func()->m_reply->proxyList;
}
#endif

/*!
    Returns the current URL of the reply
*/
QUrl QNetworkAccessBackend::url() const
{
    return d_func()->m_reply->url;
}

/*!
    Sets the URL of the reply. This could e.g. be needed if a
    redirect or similar was performed.
*/
void QNetworkAccessBackend::setUrl(const QUrl &url)
{
    d_func()->m_reply->url = url;
}

/*!
    Returns the value of the \a header.
    If no such header was known it returns a default-constructed
    QVariant.

    \sa setHeader, rawHeader, setRawHeader
*/
QVariant QNetworkAccessBackend::header(QNetworkRequest::KnownHeaders header) const
{
    return d_func()->m_reply->cookedHeaders.value(header);
}

/*!
    Sets the value of the \a header to \a value.
    This can be queried on the QNetworkReply instance which was
    returned when calling one of the appropriate functions on
    QNetworkAccessManager.

    \sa header, rawHeader, setRawHeader
*/
void QNetworkAccessBackend::setHeader(QNetworkRequest::KnownHeaders header, const QVariant &value)
{
    d_func()->m_reply->setCookedHeader(header, value);
}

/*!
    Returns the value of the \a header.
    If no such header was known it returns a default-constructed
    QVariant.

    \sa setHeader, rawHeader, setRawHeader
*/
QByteArray QNetworkAccessBackend::rawHeader(const QByteArray &header) const
{
    return d_func()->m_reply->q_func()->rawHeader(header);
}

/*!
    Sets the value of the \a header to \a value.

    This value is accessible on the QNetworkReply instance which was
    returned when calling one of the appropriate functions on
    QNetworkAccessManager.

    \sa header, rawHeader, setRawHeader
*/
void QNetworkAccessBackend::setRawHeader(const QByteArray &header, const QByteArray &value)
{
    d_func()->m_reply->setRawHeader(header, value);
}

/*!
    Returns the operation which was requested when calling
    QNetworkAccessManager.
*/
QNetworkAccessManager::Operation QNetworkAccessBackend::operation() const
{
    return d_func()->m_reply->operation;
}

/*!
    Returns \c true if setCachingEnabled was previously called with \c true.
    Returns \c false otherwise, which is the default value.

    \sa setCachingEnabled
*/
bool QNetworkAccessBackend::isCachingEnabled() const
{
    return d_func()->m_canCache;
}

/*!
    If \a canCache is \c true then this hints to us that we can cache
    the reply that is created.

    \sa isCachingEnabled
*/
void QNetworkAccessBackend::setCachingEnabled(bool canCache)
{
    d_func()->m_canCache = canCache;
}

/*!
    Set \a attribute to \a value. If \c{value.isValid()} returns
    \c false then the attribute is unset.

    This value is accessible on the QNetworkReply instance which was
    returned when calling one of the appropriate functions on
    QNetworkAccessManager.
*/
void QNetworkAccessBackend::setAttribute(QNetworkRequest::Attribute attribute,
                                         const QVariant &value)
{
    Q_D(QNetworkAccessBackend);
    if (value.isValid())
        d->m_reply->attributes.insert(attribute, value);
    else
        d->m_reply->attributes.remove(attribute);
}

/*!
    Creates a QIODevice for the data provided to upload, if any.

    Emission of upload progress is handled internally as the device
    gets read from.

    Returns a pointer to a device with data or nullptr if there was
    no data to upload.
*/
QIODevice *QNetworkAccessBackend::createUploadByteDevice()
{
    Q_D(QNetworkAccessBackend);

    if (d->m_reply->outgoingDataBuffer)
        d->uploadByteDevice =
                QNonContiguousByteDeviceFactory::createShared(d->m_reply->outgoingDataBuffer);
    else if (d->m_reply->outgoingData) {
        d->uploadByteDevice =
                QNonContiguousByteDeviceFactory::createShared(d->m_reply->outgoingData);
    } else {
        return nullptr;
    }

    // We want signal emissions only for normal asynchronous uploads
    if (!isSynchronous()) {
        connect(d->uploadByteDevice.data(), &QNonContiguousByteDevice::readProgress, this,
                [this](qint64 a, qint64 b) {
                    Q_D(QNetworkAccessBackend);
                    if (!d->m_reply->isFinished)
                        d->m_reply->emitUploadProgress(a, b);
                });
    }

    d->wrappedUploadByteDevice = QNonContiguousByteDeviceFactory::wrap(d->uploadByteDevice.data());
    return d->wrappedUploadByteDevice;
}

/*!
    Returns the upload byte device associated with the current
    request. This does not create the request but simply returns
    the pointer stored in this base class so it doesn't need to be
    stored in the subclass too.
*/
QIODevice *QNetworkAccessBackend::uploadByteDevice()
{
    return d_func()->wrappedUploadByteDevice;
}

/*!
    \internal
    Returns \c true if synchronous mode is enabled.
    If it is disabled or not supported it will return \c {false}.
*/
bool QNetworkAccessBackend::isSynchronous() const
{
    return d_func()->m_isSynchronous;
}

/*!
    \internal
    Enables or disables synchronous mode depending on \a synchronous
    if the backend supports it. Otherwise it will always be disabled.
*/
void QNetworkAccessBackend::setSynchronous(bool synchronous)
{
    if ((ioFeatures() & IOFeature::SupportsSynchronousMode) == 0)
        return;
    d_func()->m_isSynchronous = synchronous;
}

/*!
    Call this slot when you have more data available to notify
    the backend that we can attempt to read again.
*/
void QNetworkAccessBackend::readyRead()
{
    d_func()->m_reply->backendNotify(QNetworkReplyImplPrivate::NotifyDownstreamReadyWrite);
}

/*!
    Call this slot when there will be no more data available,
    regardless of whether the transfer was successful or unsuccessful.
    For unsuccessful transfers make sure to call error() first!
*/
void QNetworkAccessBackend::finished()
{
    d_func()->m_reply->finished();
}

/*!
    Call this slot if an error occurs. An error would be something
    you cannot recover from (e.g. the file requested is missing).
    The \a code and \a errorString is transferred to and stored in
    the QNetworkReply and the \a code is emitted through the
    QNetworkReply::errorOccurred() signal.
*/
void QNetworkAccessBackend::error(QNetworkReply::NetworkError code, const QString &errorString)
{
    Q_ASSERT(!d_func()->m_reply->isFinished);
    d_func()->m_reply->error(code, errorString);
}

#ifndef QT_NO_NETWORKPROXY
/*!
    Call this slot if, when connecting through a proxy, it requests
    authentication. This may cause the
    QNetworkAccessManager::proxyAuthenticationRequired() signal to be
    emitted if the credentials are not already stored in an internal
    cache.
    To be able to make the lookup in the cache and potentially the
    subsequent request the \a proxy needs to be known. The credentials
    will be stored in \a authenticator. While \a authenticator is a
    pointer, passing \c nullptr is invalid.
*/
void QNetworkAccessBackend::proxyAuthenticationRequired(const QNetworkProxy &proxy,
                                                        QAuthenticator *authenticator)
{
    Q_D(QNetworkAccessBackend);
    Q_ASSERT(authenticator);
    d->m_manager->proxyAuthenticationRequired(QUrl(), proxy, isSynchronous(), authenticator,
                                              &d->m_reply->lastProxyAuthentication);
}
#endif

/*!
    Call this slot if the remote resource requests authentication.
    This may cause the
    QNetworkAccessManager::authenticationRequired() signal to be
    emitted if the credentials are not already stored in an internal
    cache.
    The credentials will be stored in \a authenticator. While
    \a authenticator is a pointer, passing \c nullptr is invalid.
*/
void QNetworkAccessBackend::authenticationRequired(QAuthenticator *authenticator)
{
    Q_D(QNetworkAccessBackend);
    Q_ASSERT(authenticator);
    d->m_manager->authenticationRequired(authenticator, d->m_reply->q_func(), isSynchronous(),
                                         d->m_reply->url, &d->m_reply->urlForLastAuthentication);
}

/*!
    Call this slot, if appropriate, after having processed and
    updated metadata (e.g. headers).
*/
void QNetworkAccessBackend::metaDataChanged()
{
    d_func()->m_reply->metaDataChanged();
}

/*!
    Call this slot if, when connecting to the resource, a redirect
    to \a destination was requested.
*/
void QNetworkAccessBackend::redirectionRequested(const QUrl &destination)
{
    d_func()->m_reply->redirectionRequested(destination);
}

/*!
    \internal
*/
void QNetworkAccessBackend::setReplyPrivate(QNetworkReplyImplPrivate *reply)
{
    d_func()->m_reply = reply;
}

/*!
    \internal
*/
void QNetworkAccessBackend::setManagerPrivate(QNetworkAccessManagerPrivate *manager)
{
    d_func()->m_manager = manager;
}

/*!
    Returns the network cache object that was available when the
    request was started. Returns \c nullptr if none was available.
*/
QAbstractNetworkCache *QNetworkAccessBackend::networkCache() const
{
    return d_func()->m_manager->networkCache;
}

// -- QNetworkAccessBackendFactory
/*!
    Constructs QNetworkAccessBackendFactory
*/
QNetworkAccessBackendFactory::QNetworkAccessBackendFactory()
{
    if (factoryData())
        factoryData->append(this);
}

/*!
    Destructs QNetworkAccessBackendFactory
*/
QNetworkAccessBackendFactory::~QNetworkAccessBackendFactory() = default;

QT_END_NAMESPACE
