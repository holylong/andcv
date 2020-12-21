/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QARRAYDATAPOINTER_H
#define QARRAYDATAPOINTER_H

#include <QtCore/qarraydataops.h>
#include <QtCore/qcontainertools_impl.h>

QT_BEGIN_NAMESPACE

template <class T>
struct QArrayDataPointer
{
private:
    typedef QTypedArrayData<T> Data;
    typedef QArrayDataOps<T> DataOps;

public:
    enum {
        pass_parameter_by_value =
                std::is_arithmetic<T>::value || std::is_pointer<T>::value || std::is_enum<T>::value
    };

    typedef typename std::conditional<pass_parameter_by_value, T, const T &>::type parameter_type;

    constexpr QArrayDataPointer() noexcept
        : d(nullptr), ptr(nullptr), size(0)
    {
    }

    QArrayDataPointer(const QArrayDataPointer &other) noexcept
        : d(other.d), ptr(other.ptr), size(other.size)
    {
        ref();
    }

    constexpr QArrayDataPointer(Data *header, T *adata, qsizetype n = 0) noexcept
        : d(header), ptr(adata), size(n)
    {
    }

    explicit QArrayDataPointer(QPair<QTypedArrayData<T> *, T *> adata, qsizetype n = 0) noexcept
        : d(adata.first), ptr(adata.second), size(n)
    {
    }

    static QArrayDataPointer fromRawData(const T *rawData, qsizetype length) noexcept
    {
        Q_ASSERT(rawData || !length);
        return { nullptr, const_cast<T *>(rawData), length };
    }

    QArrayDataPointer &operator=(const QArrayDataPointer &other) noexcept
    {
        QArrayDataPointer tmp(other);
        this->swap(tmp);
        return *this;
    }

    QArrayDataPointer(QArrayDataPointer &&other) noexcept
        : d(other.d), ptr(other.ptr), size(other.size)
    {
        other.d = nullptr;
        other.ptr = nullptr;
        other.size = 0;
    }

    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(QArrayDataPointer)

    DataOps &operator*() noexcept
    {
        return *static_cast<DataOps *>(this);
    }

    DataOps *operator->() noexcept
    {
        return static_cast<DataOps *>(this);
    }

    const DataOps &operator*() const noexcept
    {
        return *static_cast<const DataOps *>(this);
    }

    const DataOps *operator->() const noexcept
    {
        return static_cast<const DataOps *>(this);
    }

    ~QArrayDataPointer()
    {
        if (!deref()) {
            (*this)->destroyAll();
            Data::deallocate(d);
        }
    }

    bool isNull() const noexcept
    {
        return !ptr;
    }

    T *data() noexcept { return ptr; }
    const T *data() const noexcept { return ptr; }

    T *begin() noexcept { return data(); }
    T *end() noexcept { return data() + size; }
    const T *begin() const noexcept { return data(); }
    const T *end() const noexcept { return data() + size; }
    const T *constBegin() const noexcept { return data(); }
    const T *constEnd() const noexcept { return data() + size; }

    void swap(QArrayDataPointer &other) noexcept
    {
        qSwap(d, other.d);
        qSwap(ptr, other.ptr);
        qSwap(size, other.size);
    }

    void clear() noexcept(std::is_nothrow_destructible<T>::value)
    {
        QArrayDataPointer tmp;
        swap(tmp);
    }

    void detach(QArrayDataPointer *old = nullptr)
    {
        if (needsDetach())
            reallocateAndGrow(QArrayData::GrowsAtEnd, 0, old);
    }

    // pass in a pointer to a default constructed QADP, to keep it alive beyond the detach() call
    void detachAndGrow(QArrayData::GrowthPosition where, qsizetype n, QArrayDataPointer *old = nullptr)
    {
        if (!needsDetach()) {
            if (!n ||
                (where == QArrayData::GrowsAtBeginning && freeSpaceAtBegin() >= n) ||
                (where == QArrayData::GrowsAtEnd && freeSpaceAtEnd() >= n))
                return;
        }
        reallocateAndGrow(where, n, old);
    }

    Q_NEVER_INLINE void reallocateAndGrow(QArrayData::GrowthPosition where, qsizetype n, QArrayDataPointer *old = nullptr)
    {
        if constexpr (QTypeInfo<T>::isRelocatable && alignof(T) <= alignof(std::max_align_t)) {
            if (where == QArrayData::GrowsAtEnd && !old && !needsDetach() && n > 0) {
                (*this)->reallocate(constAllocatedCapacity() - freeSpaceAtEnd() + n, QArrayData::Grow); // fast path
                return;
            }
        }

        QArrayDataPointer dp(allocateGrow(*this, n, where));
        if (where == QArrayData::GrowsAtBeginning) {
            dp.ptr += n;
            Q_ASSERT(dp.freeSpaceAtBegin() >= n);
        } else {
            Q_ASSERT(dp.freeSpaceAtEnd() >= n);
        }
        if (size) {
            qsizetype toCopy = size;
            if (n < 0)
                toCopy += n;
            if (needsDetach() || old)
                dp->copyAppend(begin(), begin() + toCopy);
            else
                dp->moveAppend(begin(), begin() + toCopy);
            dp.d->flags = flags();
            Q_ASSERT(dp.size == toCopy);
        }

        swap(dp);
        if (old)
            old->swap(dp);
    }

    // forwards from QArrayData
    qsizetype allocatedCapacity() noexcept { return d ? d->allocatedCapacity() : 0; }
    qsizetype constAllocatedCapacity() const noexcept { return d ? d->constAllocatedCapacity() : 0; }
    void ref() noexcept { if (d) d->ref(); }
    bool deref() noexcept { return !d || d->deref(); }
    bool isMutable() const noexcept { return d; }
    bool isShared() const noexcept { return !d || d->isShared(); }
    bool isSharedWith(const QArrayDataPointer &other) const noexcept { return d && d == other.d; }
    bool needsDetach() const noexcept { return !d || d->needsDetach(); }
    qsizetype detachCapacity(qsizetype newSize) const noexcept { return d ? d->detachCapacity(newSize) : newSize; }
    const typename Data::ArrayOptions flags() const noexcept { return d ? typename Data::ArrayOption(d->flags) : Data::ArrayOptionDefault; }
    void setFlag(typename Data::ArrayOptions f) noexcept { Q_ASSERT(d); d->flags |= f; }
    void clearFlag(typename Data::ArrayOptions f) noexcept { if (d) d->flags &= ~f; }

    Data *d_ptr() noexcept { return d; }
    void setBegin(T *begin) noexcept { ptr = begin; }

    qsizetype freeSpaceAtBegin() const noexcept
    {
        if (d == nullptr)
            return 0;
        return this->ptr - Data::dataStart(d, alignof(typename Data::AlignmentDummy));
    }

    qsizetype freeSpaceAtEnd() const noexcept
    {
        if (d == nullptr)
            return 0;
        return d->constAllocatedCapacity() - freeSpaceAtBegin() - this->size;
    }

    // allocate and grow. Ensure that at the minimum requiredSpace is available at the requested end
    static QArrayDataPointer allocateGrow(const QArrayDataPointer &from, qsizetype n, QArrayData::GrowthPosition position)
    {
        // calculate new capacity. We keep the free capacity at the side that does not have to grow
        // to avoid quadratic behavior with mixed append/prepend cases

        // use qMax below, because constAllocatedCapacity() can be 0 when using fromRawData()
        qsizetype minimalCapacity = qMax(from.size, from.constAllocatedCapacity()) + n;
        // subtract the free space at the side we want to allocate. This ensures that the total size requested is
        // the existing allocation at the other side + size + n.
        minimalCapacity -= (position == QArrayData::GrowsAtEnd) ? from.freeSpaceAtEnd() : from.freeSpaceAtBegin();
        qsizetype capacity = from.detachCapacity(minimalCapacity);
        const bool grows = capacity > from.constAllocatedCapacity();
        auto [header, dataPtr] = Data::allocate(capacity, grows ? QArrayData::Grow : QArrayData::KeepSize);
        const bool valid = header != nullptr && dataPtr != nullptr;
        if (!valid)
            return QArrayDataPointer(header, dataPtr);

        // Idea: * when growing backwards, adjust pointer to prepare free space at the beginning
        //       * when growing forward, adjust by the previous data pointer offset

        // TODO: what's with CapacityReserved?
        dataPtr += (position == QArrayData::GrowsAtBeginning) ? qMax(0, (header->alloc - from.size - n) / 2)
                                                    : from.freeSpaceAtBegin();
        header->flags = from.flags();
        return QArrayDataPointer(header, dataPtr);
    }

    friend bool operator==(const QArrayDataPointer &lhs, const QArrayDataPointer &rhs) noexcept
    {
        return lhs.data() == rhs.data() && lhs.size == rhs.size;
    }

    friend bool operator!=(const QArrayDataPointer &lhs, const QArrayDataPointer &rhs) noexcept
    {
        return lhs.data() != rhs.data() || lhs.size != rhs.size;
    }

    Data *d;
    T *ptr;
    qsizetype size;
};

template <class T>
inline void qSwap(QArrayDataPointer<T> &p1, QArrayDataPointer<T> &p2) noexcept
{
    p1.swap(p2);
}

////////////////////////////////////////////////////////////////////////////////
//  Q_ARRAY_LITERAL

// The idea here is to place a (read-only) copy of header and array data in an
// mmappable portion of the executable (typically, .rodata section).

// Hide array inside a lambda
#define Q_ARRAY_LITERAL(Type, ...) \
    ([]() -> QArrayDataPointer<Type> { \
        static Type const data[] = { __VA_ARGS__ }; \
        return QArrayDataPointer<Type>::fromRawData(const_cast<Type *>(data), std::size(data)); \
    }())
/**/

QT_END_NAMESPACE

#endif // include guard