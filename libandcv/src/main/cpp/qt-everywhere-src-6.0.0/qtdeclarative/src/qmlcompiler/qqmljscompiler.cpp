/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#include "qqmljscompiler_p.h"

#include <private/qqmlirbuilder_p.h>
#include <private/qqmljslexer_p.h>
#include <private/qqmljsparser_p.h>
#include <private/qqmljsloadergenerator_p.h>

#include <QtCore/qfile.h>
#include <QtCore/qloggingcategory.h>

Q_LOGGING_CATEGORY(lcAotCompiler, "qml.compiler.aot", QtWarningMsg);

QT_BEGIN_NAMESPACE

static const int FileScopeCodeIndex = -1;

static QSet<QString> getIllegalNames()
{
    QSet<QString> illegalNames;
    for (const char **g = QV4::Compiler::Codegen::s_globalNames; *g != nullptr; ++g)
        illegalNames.insert(QString::fromLatin1(*g));
    return illegalNames;
}

Q_GLOBAL_STATIC_WITH_ARGS(QSet<QString>, illegalNames, (getIllegalNames()));


void QQmlJSCompileError::print()
{
    fprintf(stderr, "%s\n", qPrintable(message));
}

QQmlJSCompileError QQmlJSCompileError::augment(const QString &contextErrorMessage) const
{
    QQmlJSCompileError augmented;
    augmented.message = contextErrorMessage + message;
    return augmented;
}

static QString diagnosticErrorMessage(const QString &fileName, const QQmlJS::DiagnosticMessage &m)
{
    QString message;
    message = fileName + QLatin1Char(':') + QString::number(m.loc.startLine) + QLatin1Char(':');
    if (m.loc.startColumn > 0)
        message += QString::number(m.loc.startColumn) + QLatin1Char(':');

    if (m.isError())
        message += QLatin1String(" error: ");
    else
        message += QLatin1String(" warning: ");
    message += m.message;
    return message;
}

void QQmlJSCompileError::appendDiagnostic(const QString &inputFileName,
                                          const QQmlJS::DiagnosticMessage &diagnostic)
{
    if (!message.isEmpty())
        message += QLatin1Char('\n');
    message += diagnosticErrorMessage(inputFileName, diagnostic);
}

void QQmlJSCompileError::appendDiagnostics(const QString &inputFileName,
                                           const QList<QQmlJS::DiagnosticMessage> &diagnostics)
{
    for (const QQmlJS::DiagnosticMessage &diagnostic: diagnostics)
        appendDiagnostic(inputFileName, diagnostic);
}

// Ensure that ListElement objects keep all property assignments in their string form
static void annotateListElements(QmlIR::Document *document)
{
    QStringList listElementNames;

    for (const QV4::CompiledData::Import *import : qAsConst(document->imports)) {
        const QString uri = document->stringAt(import->uriIndex);
        if (uri != QStringLiteral("QtQml.Models") && uri != QStringLiteral("QtQuick"))
            continue;

         QString listElementName = QStringLiteral("ListElement");
         const QString qualifier = document->stringAt(import->qualifierIndex);
         if (!qualifier.isEmpty()) {
             listElementName.prepend(QLatin1Char('.'));
             listElementName.prepend(qualifier);
         }
         listElementNames.append(listElementName);
    }

    if (listElementNames.isEmpty())
        return;

    for (QmlIR::Object *object : qAsConst(document->objects)) {
        if (!listElementNames.contains(document->stringAt(object->inheritedTypeNameIndex)))
            continue;
        for (QmlIR::Binding *binding = object->firstBinding(); binding; binding = binding->next) {
            if (binding->type != QV4::CompiledData::Binding::Type_Script)
                continue;
            binding->stringIndex = document->registerString(object->bindingAsString(document, binding->value.compiledScriptIndex));
        }
    }
}

static bool checkArgumentsObjectUseInSignalHandlers(const QmlIR::Document &doc,
                                                    QQmlJSCompileError *error)
{
    for (QmlIR::Object *object: qAsConst(doc.objects)) {
        for (auto binding = object->bindingsBegin(); binding != object->bindingsEnd(); ++binding) {
            if (binding->type != QV4::CompiledData::Binding::Type_Script)
                continue;
            const QString propName =  doc.stringAt(binding->propertyNameIndex);
            if (!propName.startsWith(QLatin1String("on"))
                || propName.length() < 3
                || !propName.at(2).isUpper())
                continue;
            auto compiledFunction = doc.jsModule.functions.value(object->runtimeFunctionIndices.at(binding->value.compiledScriptIndex));
            if (!compiledFunction)
                continue;
            if (compiledFunction->usesArgumentsObject == QV4::Compiler::Context::ArgumentsObjectUsed) {
                error->message = QLatin1Char(':') + QString::number(compiledFunction->line) + QLatin1Char(':');
                if (compiledFunction->column > 0)
                    error->message += QString::number(compiledFunction->column) + QLatin1Char(':');

                error->message += QLatin1String(" error: The use of eval() or the use of the arguments object in signal handlers is\n"
                                                "not supported when compiling qml files ahead of time. That is because it's ambiguous if \n"
                                                "any signal parameter is called \"arguments\". Similarly the string passed to eval might use\n"
                                                "\"arguments\". Unfortunately we cannot distinguish between it being a parameter or the\n"
                                                "JavaScript arguments object at this point.\n"
                                                "Consider renaming the parameter of the signal if applicable or moving the code into a\n"
                                                "helper function.");
                return false;
            }
        }
    }
    return true;
}

bool qCompileQmlFile(const QString &inputFileName, QQmlJSSaveFunction saveFunction,
                     QQmlJSAotCompiler *aotCompiler, QQmlJSCompileError *error)
{
    QmlIR::Document irDocument(/*debugMode*/false);

    QString sourceCode;
    {
        QFile f(inputFileName);
        if (!f.open(QIODevice::ReadOnly)) {
            error->message = QLatin1String("Error opening ") + inputFileName + QLatin1Char(':') + f.errorString();
            return false;
        }
        sourceCode = QString::fromUtf8(f.readAll());
        if (f.error() != QFileDevice::NoError) {
            error->message = QLatin1String("Error reading from ") + inputFileName + QLatin1Char(':') + f.errorString();
            return false;
        }
    }

    {
        QmlIR::IRBuilder irBuilder(*illegalNames());
        if (!irBuilder.generateFromQml(sourceCode, inputFileName, &irDocument)) {
            error->appendDiagnostics(inputFileName, irBuilder.errors);
            return false;
        }
    }

    annotateListElements(&irDocument);
    QQmlJSAotFunctionMap aotFunctionsByIndex;

    {
        QmlIR::JSCodeGen v4CodeGen(&irDocument, *illegalNames());

        if (aotCompiler)
            aotCompiler->setDocument(&irDocument);

        for (QmlIR::Object *object: qAsConst(irDocument.objects)) {
            if (object->functionsAndExpressions->count == 0)
                continue;
            QList<QmlIR::CompiledFunctionOrExpression> functionsToCompile;
            for (QmlIR::CompiledFunctionOrExpression *foe = object->functionsAndExpressions->first; foe; foe = foe->next)
                functionsToCompile << *foe;
            const QVector<int> runtimeFunctionIndices = v4CodeGen.generateJSCodeForFunctionsAndBindings(functionsToCompile);
            if (v4CodeGen.hasError()) {
                error->appendDiagnostic(inputFileName, v4CodeGen.error());
                return false;
            }

            QQmlJS::MemoryPool *pool = irDocument.jsParserEngine.pool();
            object->runtimeFunctionIndices.allocate(pool, runtimeFunctionIndices);

            if (!aotCompiler)
                continue;

            aotCompiler->setScopeObject(object);

            aotFunctionsByIndex[FileScopeCodeIndex] = aotCompiler->globalCode();

            std::for_each(object->bindingsBegin(), object->bindingsEnd(), [&](const QmlIR::Binding &binding) {

                switch (binding.type) {
                case QmlIR::Binding::Type_Boolean:
                case QmlIR::Binding::Type_Number:
                case QmlIR::Binding::Type_String:
                case QmlIR::Binding::Type_Null:
                    return;
                default:
                    break;
                }

                qCDebug(lcAotCompiler) << "Compiling binding for property"
                                       << irDocument.stringAt(binding.propertyNameIndex);
                auto result = aotCompiler->compileBinding(binding);
                if (auto *error = std::get_if<QQmlJS::DiagnosticMessage>(&result)) {
                    qCDebug(lcAotCompiler) << "Could not compile binding:"
                                             << diagnosticErrorMessage(inputFileName, *error);
                } else if (auto *func = std::get_if<QQmlJSAotFunction>(&result)) {
                    qCInfo(lcAotCompiler) << "Generated code:" << func->code;
                    aotFunctionsByIndex[runtimeFunctionIndices[binding.value.compiledScriptIndex]] = *func;
                }
            });

            std::for_each(object->functionsBegin(), object->functionsEnd(),
                          [&](const QmlIR::Function &function) {

                qCDebug(lcAotCompiler) << "Compiling function"
                                       << irDocument.stringAt(function.nameIndex);
                auto result = aotCompiler->compileFunction(function);
                if (auto *error = std::get_if<QQmlJS::DiagnosticMessage>(&result)) {
                    qCDebug(lcAotCompiler) << "Could not compile function:"
                                           << diagnosticErrorMessage(inputFileName, *error);
                } else if (auto *func = std::get_if<QQmlJSAotFunction>(&result)) {
                    qCInfo(lcAotCompiler) << "Generated code:" << func->code;
                    aotFunctionsByIndex[runtimeFunctionIndices[function.index]] = *func;
                }
            });
        }

        if (!checkArgumentsObjectUseInSignalHandlers(irDocument, error)) {
            *error = error->augment(inputFileName);
            return false;
        }

        QmlIR::QmlUnitGenerator generator;
        irDocument.javaScriptCompilationUnit = v4CodeGen.generateCompilationUnit(/*generate unit*/false);
        generator.generate(irDocument);

        const quint32 saveFlags
                = QV4::CompiledData::Unit::StaticData
                | QV4::CompiledData::Unit::PendingTypeCompilation;
        QV4::CompiledData::SaveableUnitPointer saveable(irDocument.javaScriptCompilationUnit.data,
                                                        saveFlags);
        if (!saveFunction(saveable, aotFunctionsByIndex, &error->message))
            return false;
    }
    return true;
}

bool qCompileJSFile(const QString &inputFileName, const QString &inputFileUrl, QQmlJSSaveFunction saveFunction, QQmlJSCompileError *error)
{
    QV4::CompiledData::CompilationUnit unit;

    QString sourceCode;
    {
        QFile f(inputFileName);
        if (!f.open(QIODevice::ReadOnly)) {
            error->message = QLatin1String("Error opening ") + inputFileName + QLatin1Char(':') + f.errorString();
            return false;
        }
        sourceCode = QString::fromUtf8(f.readAll());
        if (f.error() != QFileDevice::NoError) {
            error->message = QLatin1String("Error reading from ") + inputFileName + QLatin1Char(':') + f.errorString();
            return false;
        }
    }

    const bool isModule = inputFileName.endsWith(QLatin1String(".mjs"));
    if (isModule) {
        QList<QQmlJS::DiagnosticMessage> diagnostics;
        // Precompiled files are relocatable and the final location will be set when loading.
        QString url;
        unit = QV4::Compiler::Codegen::compileModule(/*debugMode*/false, url, sourceCode,
                                                     QDateTime(), &diagnostics);
        error->appendDiagnostics(inputFileName, diagnostics);
        if (!unit.unitData())
            return false;
    } else {
        QmlIR::Document irDocument(/*debugMode*/false);

        QQmlJS::Engine *engine = &irDocument.jsParserEngine;
        QmlIR::ScriptDirectivesCollector directivesCollector(&irDocument);
        QQmlJS::Directives *oldDirs = engine->directives();
        engine->setDirectives(&directivesCollector);
        auto directivesGuard = qScopeGuard([engine, oldDirs]{
            engine->setDirectives(oldDirs);
        });

        QQmlJS::AST::Program *program = nullptr;

        {
            QQmlJS::Lexer lexer(engine);
            lexer.setCode(sourceCode, /*line*/1, /*parseAsBinding*/false);
            QQmlJS::Parser parser(engine);

            bool parsed = parser.parseProgram();

            error->appendDiagnostics(inputFileName, parser.diagnosticMessages());

            if (!parsed)
                return false;

            program = QQmlJS::AST::cast<QQmlJS::AST::Program*>(parser.rootNode());
            if (!program) {
                lexer.setCode(QStringLiteral("undefined;"), 1, false);
                parsed = parser.parseProgram();
                Q_ASSERT(parsed);
                program = QQmlJS::AST::cast<QQmlJS::AST::Program*>(parser.rootNode());
                Q_ASSERT(program);
            }
        }

        {
            QmlIR::JSCodeGen v4CodeGen(&irDocument, *illegalNames());
            v4CodeGen.generateFromProgram(inputFileName, inputFileUrl, sourceCode, program,
                                          &irDocument.jsModule, QV4::Compiler::ContextType::ScriptImportedByQML);
            if (v4CodeGen.hasError()) {
                error->appendDiagnostic(inputFileName, v4CodeGen.error());
                return false;
            }

            // Precompiled files are relocatable and the final location will be set when loading.
            irDocument.jsModule.fileName.clear();
            irDocument.jsModule.finalUrl.clear();

            irDocument.javaScriptCompilationUnit = v4CodeGen.generateCompilationUnit(/*generate unit*/false);
            QmlIR::QmlUnitGenerator generator;
            generator.generate(irDocument);
            unit = std::move(irDocument.javaScriptCompilationUnit);
        }
    }

    QQmlJSAotFunctionMap empty;
    return saveFunction(QV4::CompiledData::SaveableUnitPointer(unit.data), empty, &error->message);
}

bool qSaveQmlJSUnitAsCpp(const QString &inputFileName, const QString &outputFileName, const QV4::CompiledData::SaveableUnitPointer &unit, const QQmlJSAotFunctionMap &aotFunctions, QString *errorString)
{
#if QT_CONFIG(temporaryfile)
    QSaveFile f(outputFileName);
#else
    QFile f(outputFileName);
#endif
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        *errorString = f.errorString();
        return false;
    }

    auto writeStr = [&f, errorString](const QByteArray &data) {
        if (f.write(data) != data.size()) {
            *errorString = f.errorString();
            return false;
        }
        return true;
    };

    if (!writeStr("// "))
        return false;

    if (!writeStr(inputFileName.toUtf8()))
        return false;

    if (!writeStr("\n"))
        return false;

    if (!writeStr("#include <QtQml/qqmlprivate.h>\n"))
        return false;

    if (!aotFunctions.isEmpty()) {
        QStringList includes;

        for (const auto &function : aotFunctions)
            includes.append(function.includes);

        std::sort(includes.begin(), includes.end());
        const auto end = std::unique(includes.begin(), includes.end());
        for (auto it = includes.begin(); it != end; ++it) {
            if (!writeStr(QStringLiteral("#include <%1>\n").arg(*it).toUtf8()))
                return false;
        }
    }

    if (!writeStr(QByteArrayLiteral("namespace QmlCacheGeneratedCode {\nnamespace ")))
        return false;

    if (!writeStr(qQmlJSSymbolNamespaceForPath(inputFileName).toUtf8()))
        return false;

    if (!writeStr(QByteArrayLiteral(" {\nextern const unsigned char qmlData alignas(16) [] = {\n")))
        return false;

    unit.saveToDisk<uchar>([&writeStr](const uchar *begin, quint32 size) {
        QByteArray hexifiedData;
        {
            QTextStream stream(&hexifiedData);
            const uchar *end = begin + size;
            stream << Qt::hex;
            int col = 0;
            for (const uchar *data = begin; data < end; ++data, ++col) {
                if (data > begin)
                    stream << ',';
                if (col % 8 == 0) {
                    stream << '\n';
                    col = 0;
                }
                stream << "0x" << *data;
            }
            stream << '\n';
        }
        return writeStr(hexifiedData);
    });



    if (!writeStr("};\n"))
        return false;

    writeStr(aotFunctions[FileScopeCodeIndex].code.toUtf8().constData());
    if (aotFunctions.size() <= 1) {
        // FileScopeCodeIndex is always there, but it may be the only one.
        writeStr("extern const QQmlPrivate::AOTCompiledFunction aotBuiltFunctions[] = { { 0, QMetaType::fromType<void>(), nullptr } };");
    } else {
        writeStr(R"(template <typename Binding>
                 void wrapCall(QQmlContext *context, QObject *scopeObject, void *dataPtr, Binding &&binding) {
                 using return_type = std::invoke_result_t<Binding, QQmlContext*, QObject*>;
                 if constexpr (std::is_same_v<return_type, void>) {
                 Q_UNUSED(dataPtr);
                 binding(context, scopeObject);
                 } else {
                 auto result = binding(context, scopeObject);
                 *reinterpret_cast<return_type *>(dataPtr) = std::move(result);
                 }
                 }        )");

        writeStr("extern const QQmlPrivate::AOTCompiledFunction aotBuiltFunctions[] = {");

        QString header = QStringLiteral("[](QQmlContext *context, QObject *scopeObject, void *dataPtr) {\n");
        header += QStringLiteral("wrapCall(context, scopeObject, dataPtr, [](QQmlContext *context, QObject *scopeObject) {");
        header += QStringLiteral("Q_UNUSED(context); Q_UNUSED(scopeObject);\n");

        QString footer = QStringLiteral("});}\n");

        for (QQmlJSAotFunctionMap::ConstIterator func = aotFunctions.constBegin(),
             end = aotFunctions.constEnd();
             func != end; ++func) {

            if (func.key() == FileScopeCodeIndex)
                continue;

            QString function = header + func.value().code + footer;

            writeStr(QStringLiteral("{ %1, QMetaType::fromType<%2>(), %3 },")
                     .arg(func.key()).arg(func.value().returnType).arg(function)
                     .toUtf8().constData());
        }

        // Conclude the list with a nullptr
        writeStr("{ 0, QMetaType::fromType<void>(), nullptr }");
        writeStr("};\n");
    }

    if (!writeStr("}\n}\n"))
        return false;

#if QT_CONFIG(temporaryfile)
    if (!f.commit()) {
        *errorString = f.errorString();
        return false;
    }
#endif

    return true;
}

QT_END_NAMESPACE
