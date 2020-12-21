/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
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

#ifndef FINDUNQUALIFIED_H
#define FINDUNQUALIFIED_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include "qcoloroutput.h"
#include "checkidentifiers.h"

#include <QtQmlCompiler/private/qqmljstypedescriptionreader_p.h>
#include <QtQmlCompiler/private/qqmljsscope_p.h>
#include <QtQmlCompiler/private/qqmljsimporter_p.h>
#include <QtQmlCompiler/private/qqmljsimportvisitor_p.h>

#include <QtQml/private/qqmldirparser_p.h>
#include <QtQml/private/qqmljsastvisitor_p.h>
#include <QtQml/private/qqmljsast_p.h>

#include <QtCore/qscopedpointer.h>

class FindWarningVisitor : public QQmlJSImportVisitor
{
    Q_DISABLE_COPY_MOVE(FindWarningVisitor)
public:
    explicit FindWarningVisitor(
            QQmlJSImporter *importer, QStringList qmltypesFiles, QString code, QString fileName,
            bool silent, bool warnUnqualified, bool warnWithStatement, bool warnInheritanceCycle);
    ~FindWarningVisitor() override = default;
    bool check();

private:
    QHash<QQmlJS::SourceLocation, SignalHandler> m_signalHandlers;
    QQmlJS::SourceLocation m_pendingSingalHandler;

    MemberAccessChains m_memberAccessChains;

    QQmlJS::AST::ExpressionNode *m_fieldMemberBase = nullptr;
    QString m_code;
    QString m_rootId;
    QString m_filePath;
    QSet<QString> m_unknownImports;
    ColorOutput m_colorOut;
    bool m_visitFailed = false;

    bool m_warnUnqualified;
    bool m_warnWithStatement;
    bool m_warnInheritanceCycle;

    struct OutstandingConnection
    {
        QString targetName;
        QQmlJSScope::Ptr scope;
        QQmlJS::AST::UiObjectDefinition *uiod;
    };

    QVarLengthArray<OutstandingConnection, 3> m_outstandingConnections; // Connections whose target we have not encountered

    void checkInheritanceCycle(QQmlJSScope::ConstPtr scope);
    void checkGroupedScopes(QQmlJSScope::ConstPtr scope);
    void flushPendingSignalParameters();

    void throwRecursionDepthError() override;

    // work around compiler error in clang11
    using QQmlJSImportVisitor::visit;
    using QQmlJSImportVisitor::endVisit;

    // start block/scope handling
    bool visit(QQmlJS::AST::ExpressionStatement *ast) override;
    void endVisit(QQmlJS::AST::ExpressionStatement *ast) override;
    bool visit(QQmlJS::AST::Block *ast) override;
    bool visit(QQmlJS::AST::WithStatement *withStatement) override;

    /* --- end block handling --- */

    bool visit(QQmlJS::AST::UiObjectBinding *uiob) override;
    void endVisit(QQmlJS::AST::UiObjectBinding *uiob) override;
    bool visit(QQmlJS::AST::UiObjectDefinition *uiod) override;
    void endVisit(QQmlJS::AST::UiObjectDefinition *) override;
    bool visit(QQmlJS::AST::UiScriptBinding *uisb) override;

    // expression handling
    bool visit(QQmlJS::AST::IdentifierExpression *idexp) override;

    bool visit(QQmlJS::AST::PatternElement *) override;
    bool visit(QQmlJS::AST::FieldMemberExpression *idprop) override;
    void endVisit(QQmlJS::AST::FieldMemberExpression *) override;

    bool visit(QQmlJS::AST::BinaryExpression *) override;
    void endVisit(QQmlJS::AST::BinaryExpression *) override;
};

#endif // FINDUNQUALIFIED_H
