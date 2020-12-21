/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef TST_QMLTYPEREGISTRAR_H
#define TST_QMLTYPEREGISTRAR_H

#include "foreign.h"

#include <QtQml/qqml.h>
#include <QtCore/qproperty.h>

class ExcessiveVersion : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int palette READ palette WRITE setPalette NOTIFY paletteChanged REVISION(6, 0))

public:
    int palette() const { return m_palette; }


    void setPalette(int palette)
    {
        if (m_palette == palette)
        return;

        m_palette = palette;
        emit paletteChanged();
    }

signals:
    Q_REVISION(6, 0) void paletteChanged();

private:
    int m_palette = 0;
};

class SizeEnums
{
    Q_GADGET
    QML_NAMED_ELEMENT(SizeEnums)
    QML_UNCREATABLE("Element is not creatable.")

public:
    enum Unit { Pixel, Centimeter, Inch, Point };
    Q_ENUM(Unit)
};

class SizeValueType : public SizeEnums
{
    QSize v;
    Q_GADGET
    Q_PROPERTY(int width READ width WRITE setWidth FINAL)
    QML_NAMED_ELEMENT(MySize)
    QML_FOREIGN(SizeGadget)

public:
    Q_INVOKABLE QString sizeToString() const
    {
        return QString::fromLatin1("%1x%2").arg(v.width()).arg(v.height());
    }

    int width() const { return v.width(); }
    void setWidth(int width) { v.setWidth(width); }
};

class Local : public Foreign
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int someProperty MEMBER someProperty BINDABLE bindableSomeProperty)
public:
    enum Flag {
        Flag1 = 0x1,
        Flag2 = 0x2,
        Flag3 = 0x4,
        Flag4 = 0x8
    };
    Q_DECLARE_FLAGS(Flags, Flag)
    Q_FLAG(Flags)

    QBindable<int> bindableSomeProperty() {return QBindable<int>(&someProperty);}

    QProperty<int> someProperty;
};

class tst_qmltyperegistrar : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void qmltypesHasForeign();
    void qmltypesHasHppClassAndNoext();
    void qmltypesHasFileNames();
    void qmltypesHasFlags();
    void superAndForeignTypes();
    void accessSemantics();
    void isBindable();
    void restrictToImportVersion();
    void pastMajorVersions();

private:
    QByteArray qmltypesData;
};

#endif // TST_QMLTYPEREGISTRAR_H
