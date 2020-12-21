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

#include "indextrianglestopology.h"

#include <QVector3D>

IndexTrianglesTopology::IndexTrianglesTopology()
{
    updateData();
}

void IndexTrianglesTopology::updateData()
{
    QByteArray vertexData;
    QByteArray indexData;
    setPrimitiveType(QQuick3DGeometry::PrimitiveType::Triangles);
    addAttribute(QQuick3DGeometry::Attribute::PositionSemantic, 0,
                 QQuick3DGeometry::Attribute::ComponentType::F32Type);
    addAttribute(QQuick3DGeometry::Attribute::NormalSemantic, 16,
                 QQuick3DGeometry::Attribute::ComponentType::F32Type);
    addAttribute(QQuick3DGeometry::Attribute::IndexSemantic, 0,
                 QQuick3DGeometry::Attribute::ComponentType::U16Type);
    setStride(32);

    // Two Triangles with Vertex + Normals
    const int vertexBufferSize = sizeof(float) * 4 * 2 * 4; // 2 x vec4 x 4
    vertexData.resize(vertexBufferSize);

    float *data = reinterpret_cast<float *>(vertexData.data());
    // Vertex 0
    data[0] = 0.0f;
    data[1] = 1.0f;
    data[2] = 0.0f;
    data[3] = 1.0f;
    // Normal 0
    data[4] = 0.0f;
    data[5] = 0.0f;
    data[6] = 1.0f;
    data[7] = 0.0f;

    // Vertex 1
    data[8] = -1.0f;
    data[9] = 0.0f;
    data[10] = 0.0f;
    data[11] = 1.0f;
    // Normal 1
    data[12] = 0.0f;
    data[13] = 0.0f;
    data[14] = 1.0f;
    data[15] = 0.0f;

    // Vertex 2
    data[16] = 1.0f;
    data[17] = 0.0f;
    data[18] = 0.0f;
    data[19] = 1.0f;
    // Normal 2
    data[20] = 0.0f;
    data[21] = 0.0f;
    data[22] = 1.0f;
    data[23] = 0.0f;

    // Vertex 3
    data[24] = 0.0f;
    data[25] = -1.0f;
    data[26] = 0.0f;
    data[27] = 1.0f;
    // Normal 3
    data[28] = 0.0f;
    data[29] = 0.0f;
    data[30] = 1.0f;
    data[31] = 0.0f;

    setVertexData(vertexData);
    indexData.resize(6 * sizeof(quint16));

    quint16 *iData = reinterpret_cast<quint16 *>(indexData.data());
    iData[0] = quint16(0);
    iData[1] = quint16(1);
    iData[2] = quint16(2);
    iData[3] = quint16(2);
    iData[4] = quint16(1);
    iData[5] = quint16(3);
    setIndexData(indexData);
    setBounds(QVector3D(-1.0f, -1.0f, 0.0f), QVector3D(1.0f, 1.0f, 0.0f));
}
