/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:GPL$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquick3dgeometry_p.h"
#include "qquick3dscenemanager_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrendergeometry_p.h>

/*!
    \qmltype Geometry
    \inherits Object3D
    \inqmlmodule QtQuick3D
    \instantiates QQuick3DGeometry
    \brief Base type for custom geometry.

    Custom geometry allows using application-generated vertex and index data,
    that can possibly change dynamically as well. To use custom geometry, do
    not assign a \c{.mesh} file as the \l{Model::source}{source} to a Model.
    Instead, set its \l{Model::geometry}{geometry} property to reference a
    Geometry object.

    A typical way of implementing custom geometry is by creating a
    \l QQuick3DGeometry subclass in C++ and registering the new type for use
    with QML.

    It is also possible to use the built-in custom geometry provider
    \l GridGeometry in the \c Helpers module. The following is an example of
    \l GridGeometry. Any application-provided Geometry subclass can be taken into
    use in the same manner.

    \code
    import QtQuick3D.Helpers

    Model {
        geometry: GridGeometry {
        }
        materials: [
            DefaultMaterial {
                emissiveColor: "white"
                lighting: DefaultMaterial.NoLighting
            }
        ]
    }
    \endcode

    \sa {Qt Quick 3D - Custom Geometry Example}, Model, QQuick3DGeometry
*/

/*!
    \class QQuick3DGeometry
    \inmodule QtQuick3D
    \inherits QQuick3DObject
    \since 5.15
    \brief Base class for defining custom geometry.

    The QQuick3DGeometry can be used to specify custom geometry for a Model in
    the Qt Quick 3D scene.

    While not strictly required, the typical usage is to inherit from this
    class. The subclass is then exposed to QML by registering it to the type
    system. The \l{Model::geometry}{geometry} property of a Model can then be
    set to reference an instance of the registered type.

    The high-level structure of such a class is typically similar to the following:

    \code
    class CustomGeometry : public QQuick3DGeometry
    {
    public:
        CustomGeometry() { rebuildGeometry(); }

        void setSomething() {
           // Change relevant internal data.
           // ...

           // Then rebuild the vertex and index data and pass it to QQuick3DGeometry.
           rebuildGeometry();

           // Finally, trigger an update. This is relevant in case nothing else
           // is changing in the scene; this way we make sure a new frame will
           // be rendered.
           update();
        }

    private:
        void rebuildGeometry()
        {
            QByteArray vertices;
            QByteArray indices;
            ...
            setPrimitiveType(Lines);
            setVertexBuffer(vertices);
            setIndexBuffer(indices);
            setStride(3 * sizeof(float)); // e.g. when having 3 components per vertex
            setBounds(...); // minimum and maximum extents, for picking
            addAttribute(PositionSemantic, 0, F32Type);
            ...
        }
    };
    \endcode

    This class can then be registered as a QML type and used with \l {QtQuick3D::Model}{Model}.

    In Qt 5 type registration happened with qmlRegisterType:
    \code
    qmlRegisterType<CustomGeometry>("Example", 1, 0, "CustomGeometry");
    \endcode

    In Qt 6 the default approach is to use automatic registration with the help
    of the build system. Instead of calling qmlRegisterType, the \c{.pro} file
    can now contain:

    \code
    CONFIG += qmltypes
    QML_IMPORT_NAME = Example
    QML_IMPORT_MAJOR_VERSION = 1
    \endcode

    Alternatively, with CMake the equivalent is:
    \code
    set_target_properties(application PROPERTIES
        QT_QML_MODULE_VERSION 1.0
        QT_QML_MODULE_URI Example
    )
    qt6_qml_type_registration(application)
    \endcode

    The class implementation should add QML_NAMED_ELEMENT:

    \code
    class CustomGeometry : public QQuick3DGeometry
    {
        Q_OBJECT
        QML_NAMED_ELEMENT(CustomGeometry)
        ...
    };
    \endcode

    The QML code can then use the custom type:

    \code
    import Example 1.0

    Model {
        id: customModel
        geometry: CustomGeometry {
        }
    }
    \endcode

    At minimum, a custom geometry should have the following specified:

    \list
    \li vertex data,
    \li vertex stride,
    \li primitive type,
    \li an attribute with PositionSemantic.
    \endlist

    These are sufficient to render the mesh. For indexed drawing, the index
    buffer data and an attribute with IndexSemantic needs to be specified as
    well. In order to support picking (input), the class must specify the bounding volume using setBounds().
    For proper lighting, an attribute with NormalSemantic is needed. When the
    material uses texturing, at least one set of UV coordinates must be
    provided and described in an TexCoordSemantic attribute. Some materials may
    require tangents and binormals as well.

    As a concrete, minimal example, the following class would provide geometry
    for a single triangle:

    \code
        class ExampleGeometry : public QQuick3DGeometry
        {
            Q_OBJECT
            QML_NAMED_ELEMENT(ExampleGeometry)

        public:
            ExampleGeometry();

        private:
            void updateData();
        };

        ExampleGeometry::ExampleGeometry()
        {
            updateData();
        }

        void ExampleGeometry::updateData()
        {
            QByteArray v;
            v.resize(3 * 3 * sizeof(float));
            float *p = reinterpret_cast<float *>(v.data());

            // a triangle, front face = counter-clockwise
            *p++ = -1.0f; *p++ = -1.0f; *p++ = 0.0f;
            *p++ = 1.0f; *p++ = -1.0f; *p++ = 0.0f;
            *p++ = 0.0f; *p++ = 1.0f; *p++ = 0.0f;

            setVertexData(v);
            setStride(3 * sizeof(float));

            setPrimitiveType(QQuick3DGeometry::PrimitiveType::Triangles);

            addAttribute(QQuick3DGeometry::Attribute::PositionSemantic,
                         0,
                         QQuick3DGeometry::Attribute::F32Type);
        }
    \endcode

    Depending on the lighting in the scene, the result of referencing this
    geometry from a Model:

    \image customgeometry.jpg

    \note Vertex data is expected to follow OpenGL conventions. This means the
    data must be provided with the assumption that the Y axis is pointing up in
    the normalized device coordinate system, and that front faces have a
    counter clockwise winding.

    \sa Model, Geometry
*/

QT_BEGIN_NAMESPACE

QQuick3DGeometryPrivate::QQuick3DGeometryPrivate()
    : QQuick3DObjectPrivate(QQuick3DObjectPrivate::Type::Geometry)
{

}

QQuick3DGeometry::QQuick3DGeometry(QQuick3DObject *parent)
    : QQuick3DObject(*new QQuick3DGeometryPrivate, parent)
{

}

QQuick3DGeometry::~QQuick3DGeometry()
{

}

/*!
    Returns the vertex buffer data set by setVertexData.
*/
QByteArray QQuick3DGeometry::vertexData() const
{
    const Q_D(QQuick3DGeometry);
    return d->m_vertexBuffer;
}

/*!
    Returns the index buffer data.
*/
QByteArray QQuick3DGeometry::indexData() const
{
    const Q_D(QQuick3DGeometry);
    return d->m_indexBuffer;
}

/*!
    Returns the number of attributes defined for this geometry.

    \sa attribute
*/
int QQuick3DGeometry::attributeCount() const
{
    const Q_D(QQuick3DGeometry);
    return d->m_attributeCount;
}

/*!
    Returns attribute definition number \a index

    The attribute definitions are numbered from 0 to \c {attributeCount() - 1}
*/
QQuick3DGeometry::Attribute QQuick3DGeometry::attribute(int index) const
{
    const Q_D(QQuick3DGeometry);
    return d->m_attributes[index];
}

/*!
    Returns the primitive type used when rendering. The default is \c Triangles.

    \sa setPrimitiveType
*/
QQuick3DGeometry::PrimitiveType QQuick3DGeometry::primitiveType() const
{
    const Q_D(QQuick3DGeometry);
    return d->m_primitiveType;
}

/*!
    Returns the minimum coordinate of the bounding volume.

    \sa setBounds
*/
QVector3D QQuick3DGeometry::boundsMin() const
{
    const Q_D(QQuick3DGeometry);
    return d->m_min;
}

/*!
    Returns the maximum coordinate of the bounding volume.

    \sa setBounds
*/
QVector3D QQuick3DGeometry::boundsMax() const
{
    const Q_D(QQuick3DGeometry);
    return d->m_max;
}

/*!
    Returns the byte stride of the vertex buffer.

    \sa setStride
*/
int QQuick3DGeometry::stride() const
{
    const Q_D(QQuick3DGeometry);
    return d->m_stride;
}

void QQuick3DGeometry::markAllDirty()
{
    QQuick3DObject::markAllDirty();
}

/*!
    Sets the vertex buffer \a data. The buffer should hold all the vertex data
    packed in the array, as described by the attribute definitions. Note that
    this does not include attributes with \c IndexSemantic, which belong in the
    index buffer.

    \sa addAttribute, setStride, setIndexData
*/
void QQuick3DGeometry::setVertexData(const QByteArray &data)
{
    Q_D(QQuick3DGeometry);
    d->m_vertexBuffer = data;
    d->m_geometryChanged = true;
}

/*!
    \overload
    Updates a subset of the vertex buffer. \a offset specifies the offset in
    bytes, \a data specifies the size and the data.

    This function will not resize the buffer. If \c {offset + data.size()} is
    greater than the current size of the buffer, the overshooting data will
    be ignored.

    \note The partial update functions for vertex and index data do not offer
    any guarantee on how such changes are implemented internally. Depending on
    the underlying implementation, even partial changes may lead to updating
    the entire graphics resource.
*/
void QQuick3DGeometry::setVertexData(int offset, const QByteArray &data)
{
    Q_D(QQuick3DGeometry);
    if (offset >= d->m_vertexBuffer.size())
        return;

    const size_t len = qMin(d->m_vertexBuffer.size() - offset, data.size());
    memcpy(d->m_vertexBuffer.data() + offset, data.data(), len);

    d->m_geometryChanged = true;
}

/*!
    Sets the index buffer to \a data. To use indexed drawing, add an attribute with \c IndexSemantic

    \sa addAttribute
*/
void QQuick3DGeometry::setIndexData(const QByteArray &data)
{
    Q_D(QQuick3DGeometry);
    d->m_indexBuffer = data;
    d->m_geometryChanged = true;
}

/*!
    \overload
    Updates a subset of the index buffer. \a offset specifies the offset in
    bytes, \a data specifies the size and the data.

    This function will not resize the buffer. If \c {offset + data.size()} is
    greater than the current size of the buffer, the overshooting data will
    be ignored.

    \note The partial update functions for vertex and index data do not offer
    any guarantee on how such changes are implemented internally. Depending on
    the underlying implementation, even partial changes may lead to updating
    the entire graphics resource.
*/
void QQuick3DGeometry::setIndexData(int offset, const QByteArray &data)
{
    Q_D(QQuick3DGeometry);
    if (offset >= d->m_indexBuffer.size())
        return;

    const size_t len = qMin(d->m_indexBuffer.size() - offset, data.size());
    memcpy(d->m_indexBuffer.data() + offset, data.data(), len);

    d->m_geometryChanged = true;
}

/*!
    Sets the stride of the vertex buffer to \a stride, measured in bytes.
    This is the distance between two consecutive vertices in the buffer.

    For example, a tightly packed vertex buffer for a geometry using
    \c PositionSemantic, \c IndexSemantic, and \c ColorSemantic will have stride
    \c 28 (Seven floats in total: Three for position, four for color, and none for indexes,
    which do not go in the vertex buffer.)

    \sa addAttribute
*/
void QQuick3DGeometry::setStride(int stride)
{
    Q_D(QQuick3DGeometry);
    if (stride != d->m_stride) {
        d->m_stride = stride;
        d->m_geometryChanged = true;
    }
}

/*!
    Sets the bounding volume of the geometry to the cube defined by the points \a min and \a max.
    This is used for \l {View3D::pick}{picking}.
*/
void QQuick3DGeometry::setBounds(const QVector3D &min, const QVector3D &max)
{
    Q_D(QQuick3DGeometry);
    d->m_max = max;
    d->m_min = min;
    d->m_geometryBoundsChanged = true;
}

/*!
    Sets the primitive type used for rendering to \a type.

    \value Points The primitives are points.
    \value LineStrip The primitives are lines in a strip.
    \value Lines The primitives are lines in a list.
    \value TriangleStrip The primitives are triangles in a strip.
    \value TriangleFan The primitives are triangles in a fan. Be aware that
    triangle fans may not be supported at run time, depending on the underlying
    graphics API.
    \value Triangles The primitives are triangles in a list.

    The initial value is \c Triangles.

    \note Be aware that triangle fans (TriangleFan) may not be supported at run
    time, depending on the underlying graphics API. For example, with Direct 3D
    this topology will not be functional at all.

    \note The point size for Points and the line width for Lines and LineStrip
    are controlled by the \l{PrincipledMaterial::pointSize}{material}. Be aware
    however that sizes other than 1 may not be supported at run time, depending
    on the underlying graphics API.

*/
void QQuick3DGeometry::setPrimitiveType(PrimitiveType type)
{
    Q_D(QQuick3DGeometry);
    if (d->m_primitiveType != type) {
        d->m_primitiveType = type;
        d->m_geometryChanged = true;
    }
}

/*!
    Adds vertex attribute description. Each attribute has a \a semantic, which specifies
    the usage of the attribute and the number of components it has, an \a offset from the
    beginning to the vertex to the attribute location inside a vertex and a \a componentType
    specifying the datatype and size of the attribute.

    The semantic can be one of the following:

    \value PositionSemantic The attribute is a position. 3 components: \e x, \e y, and \e z
    \value NormalSemantic The attribute is a normal vector. 3 components: \e x, \e y, and \e z
    \value TexCoordSemantic The attribute is a texture coordinate. 2 components: \e u and \e v
    \value TangentSemantic The attribute is a tangent vector. 3 components: \e x, \e y, and \e z
    \value BinormalSemantic The attribute is a binormal vector. 3 components: \e x, \e y, and \e z
    \value JointSemantic The attribute is a joint index vector for  \l {Vertex Skinning}{skinning}. 4 components: joint index 1-4
    \value WeightSemantic The attribute is a weight vector for  \l {Vertex Skinning}{skinning}. 4 components: joint weight 1-4
    \value ColorSemantic The attribute is a vertex color vector. 4 components: \e r, \e g, \e b, and \e a

    In addition, \a semantic can be \c IndexSemantic. In this case the attribute does not represent an entry in the vertex
    buffer, but rather describes the index data in the index buffer. Since there is always just one index per vertex, \a offset
    makes no sense for the index buffer, and should be left at zero.

    The component type can be one of the following:

    \value U16Type The attribute is an unsigned 16-bit integer.
    \value U32Type The attribute is an unsigned 32-bit integer.
    \value I32Type The attribute is a signed 32-bit integer.
    \value F32Type The attribute is a single-precision float.

    \note The joint index data is typically \c I32Type. \c F32Type is also supported
    in order to enable functioning with APIs, such as OpenGL ES 2.0, that do not
    support integer vertex input attributes.

*/
void QQuick3DGeometry::addAttribute(Attribute::Semantic semantic, int offset,
                  Attribute::ComponentType componentType)
{
    Q_D(QQuick3DGeometry);
    if (d->m_attributeCount >= QQuick3DGeometryPrivate::MAX_ATTRIBUTE_COUNT)
        return;
    d->m_attributes[d->m_attributeCount].semantic = semantic;
    d->m_attributes[d->m_attributeCount].offset = offset;
    d->m_attributes[d->m_attributeCount].componentType = componentType;
    d->m_attributeCount++;
    d->m_geometryChanged = true;
}

/*!
    \overload

    Adds vertex attribute description. Each attribute has a semantic, which specifies
    the usage of the attribute and the number of components it has, an offset from the
    beginning to the vertex to the attribute location inside a vertex and a componentType
    specifying the datatype and size of the attribute.
*/
void QQuick3DGeometry::addAttribute(const Attribute &attribute)
{
    Q_D(QQuick3DGeometry);
    if (d->m_attributeCount >= QQuick3DGeometryPrivate::MAX_ATTRIBUTE_COUNT)
        return;
    d->m_attributes[d->m_attributeCount++] = attribute;
    d->m_geometryChanged = true;
}

/*!
    Resets the geometry to its initial state, clearing previously set vertex and index data as well as attributes.
*/
void QQuick3DGeometry::clear()
{
    Q_D(QQuick3DGeometry);
    d->m_vertexBuffer.clear();
    d->m_indexBuffer.clear();
    d->m_attributeCount = 0;
    d->m_primitiveType = PrimitiveType::Triangles;
    d->m_geometryChanged = true;
    d->m_min = {};
    d->m_max = {};
}

static QSSGRenderGeometry::PrimitiveType mapPrimitiveType(QQuick3DGeometry::PrimitiveType t)
{
    switch (t) {
    case QQuick3DGeometry::PrimitiveType::Points:
        return QSSGRenderGeometry::Points;
    case QQuick3DGeometry::PrimitiveType::LineStrip:
        return QSSGRenderGeometry::LineStrip;
    case QQuick3DGeometry::PrimitiveType::Lines:
        return QSSGRenderGeometry::Lines;
    case QQuick3DGeometry::PrimitiveType::TriangleStrip:
        return QSSGRenderGeometry::TriangleStrip;
    case QQuick3DGeometry::PrimitiveType::TriangleFan:
        return QSSGRenderGeometry::TriangleFan;
    case QQuick3DGeometry::PrimitiveType::Triangles:
        return QSSGRenderGeometry::Triangles;
    default:
        Q_ASSERT(false);
        return QSSGRenderGeometry::Triangles;
    }
}

static QSSGRenderGeometry::Attribute::Semantic mapSemantic(QQuick3DGeometry::Attribute::Semantic s)
{
    switch (s) {
    case QQuick3DGeometry::Attribute::IndexSemantic:
        return QSSGRenderGeometry::Attribute::IndexSemantic;
    case QQuick3DGeometry::Attribute::PositionSemantic:
        return QSSGRenderGeometry::Attribute::PositionSemantic;
    case QQuick3DGeometry::Attribute::NormalSemantic:
        return QSSGRenderGeometry::Attribute::NormalSemantic;
    case QQuick3DGeometry::Attribute::TexCoordSemantic:
        return QSSGRenderGeometry::Attribute::TexCoordSemantic;
    case QQuick3DGeometry::Attribute::TangentSemantic:
        return QSSGRenderGeometry::Attribute::TangentSemantic;
    case QQuick3DGeometry::Attribute::BinormalSemantic:
        return QSSGRenderGeometry::Attribute::BinormalSemantic;
    case QQuick3DGeometry::Attribute::JointSemantic:
        return QSSGRenderGeometry::Attribute::JointSemantic;
    case QQuick3DGeometry::Attribute::WeightSemantic:
        return QSSGRenderGeometry::Attribute::WeightSemantic;
    case QQuick3DGeometry::Attribute::ColorSemantic:
        return QSSGRenderGeometry::Attribute::ColorSemantic;
    default:
        Q_ASSERT(false);
        return QSSGRenderGeometry::Attribute::PositionSemantic;
    }
}

static QSSGRenderGeometry::Attribute::ComponentType mapComponentType(QQuick3DGeometry::Attribute::ComponentType t)
{
    switch (t) {
    case QQuick3DGeometry::Attribute::U16Type:
        return QSSGRenderGeometry::Attribute::U16Type;
    case QQuick3DGeometry::Attribute::U32Type:
        return QSSGRenderGeometry::Attribute::U32Type;
    case QQuick3DGeometry::Attribute::I32Type:
        return QSSGRenderGeometry::Attribute::I32Type;
    case QQuick3DGeometry::Attribute::F32Type:
        return QSSGRenderGeometry::Attribute::F32Type;
    default:
        Q_ASSERT(false);
        return QSSGRenderGeometry::Attribute::F32Type;
    }
}

/*!
    \internal
 */
QSSGRenderGraphObject *QQuick3DGeometry::updateSpatialNode(QSSGRenderGraphObject *node)
{
    Q_D(QQuick3DGeometry);
    if (!node) {
        markAllDirty();
        node = new QSSGRenderGeometry();
        emit geometryNodeDirty();
    }

    QSSGRenderGeometry *geometry = static_cast<QSSGRenderGeometry *>(node);
    if (d->m_geometryChanged) {
        geometry->setBounds(d->m_min, d->m_max);
        geometry->setStride(d->m_stride);
        geometry->setIndexData(d->m_indexBuffer);
        geometry->setVertexData(d->m_vertexBuffer);
        geometry->setPrimitiveType(mapPrimitiveType(d->m_primitiveType));
        geometry->clearAttributes();
        for (int i = 0; i < d->m_attributeCount; ++i) {
            geometry->addAttribute(mapSemantic(d->m_attributes[i].semantic),
                                   d->m_attributes[i].offset,
                                   mapComponentType(d->m_attributes[i].componentType));
        }
        d->m_geometryChanged = false;
    }
    if (d->m_geometryBoundsChanged) {
        geometry->setBounds(d->m_min, d->m_max);
        emit geometryNodeDirty();
        d->m_geometryBoundsChanged = false;
    }

    return node;
}

QT_END_NAMESPACE