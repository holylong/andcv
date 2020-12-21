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

#include <QtCore/qcoreapplication.h>
#include <QtCore/qcommandlineparser.h>
#include <QtCore/qtextstream.h>
#include <QtCore/qfile.h>
#include <QtCore/qdir.h>
#include <QtCore/qtemporarydir.h>
#include <QtCore/qprocess.h>
#include <QtCore/qdebug.h>
#include <QtShaderTools/private/qshaderbaker_p.h>
#include <QtGui/private/qshader_p_p.h>

enum class FileType
{
    Binary,
    Text
};

static bool writeToFile(const QByteArray &buf, const QString &filename, FileType fileType)
{
    QFile f(filename);
    QIODevice::OpenMode flags = QIODevice::WriteOnly;
    if (fileType == FileType::Text)
        flags |= QIODevice::Text;
    if (!f.open(flags)) {
        qWarning("Failed to open %s for writing", qPrintable(filename));
        return false;
    }
    f.write(buf);
    return true;
}

static QByteArray readFile(const QString &filename, FileType fileType)
{
    QFile f(filename);
    QIODevice::OpenMode flags = QIODevice::ReadOnly;
    if (fileType == FileType::Text)
        flags |= QIODevice::Text;
    if (!f.open(flags)) {
        qWarning("Failed to open %s", qPrintable(filename));
        return QByteArray();
    }
    return f.readAll();
}

static QString writeTemp(const QTemporaryDir &tempDir, const QString &filename, const QShaderCode &s, FileType fileType)
{
    const QString fullPath = tempDir.path() + QLatin1String("/") + filename;
    if (writeToFile(s.shader(), fullPath, fileType))
        return fullPath;
    else
        return QString();
}

static bool runProcess(const QString &binary, const QStringList &arguments,
                       QByteArray *output, QByteArray *errorOutput)
{
    QProcess p;
    p.start(binary, arguments);
    const QString cmd = binary + QLatin1Char(' ') + arguments.join(QLatin1Char(' '));
    qDebug("%s", qPrintable(cmd));
    if (!p.waitForStarted()) {
        qWarning("Failed to run %s: %s", qPrintable(cmd), qPrintable(p.errorString()));
        return false;
    }
    if (!p.waitForFinished()) {
        qWarning("%s timed out", qPrintable(cmd));
        return false;
    }

    if (p.exitStatus() == QProcess::CrashExit) {
        qWarning("%s crashed", qPrintable(cmd));
        return false;
    }

    *output = p.readAllStandardOutput();
    *errorOutput = p.readAllStandardError();

    if (p.exitCode() != 0) {
        qWarning("%s returned non-zero error code %d", qPrintable(cmd), p.exitCode());
        return false;
    }

    return true;
}

static QString stageStr(QShader::Stage stage)
{
    switch (stage) {
    case QShader::VertexStage:
        return QStringLiteral("Vertex");
    case QShader::TessellationControlStage:
        return QStringLiteral("TessellationControl");
    case QShader::TessellationEvaluationStage:
        return QStringLiteral("TessellationEvaluation");
    case QShader::GeometryStage:
        return QStringLiteral("Geometry");
    case QShader::FragmentStage:
        return QStringLiteral("Fragment");
    case QShader::ComputeStage:
        return QStringLiteral("Compute");
    default:
        Q_UNREACHABLE();
    }
}

static QString sourceStr(QShader::Source source)
{
    switch (source) {
    case QShader::SpirvShader:
        return QStringLiteral("SPIR-V");
    case QShader::GlslShader:
        return QStringLiteral("GLSL");
    case QShader::HlslShader:
        return QStringLiteral("HLSL");
    case QShader::DxbcShader:
        return QStringLiteral("DXBC");
    case QShader::MslShader:
        return QStringLiteral("MSL");
    case QShader::DxilShader:
        return QStringLiteral("DXIL");
    case QShader::MetalLibShader:
        return QStringLiteral("metallib");
    default:
        Q_UNREACHABLE();
    }
}

static QString sourceVersionStr(const QShaderVersion &v)
{
    QString s = v.version() ? QString::number(v.version()) : QString();
    if (v.flags().testFlag(QShaderVersion::GlslEs))
        s += QLatin1String(" es");

    return s;
}

static QString sourceVariantStr(const QShader::Variant &v)
{
    switch (v) {
    case QShader::StandardShader:
        return QLatin1String("Standard");
    case QShader::BatchableVertexShader:
        return QLatin1String("Batchable");
    default:
        Q_UNREACHABLE();
    }
}

static void dump(const QShader &bs)
{
    QTextStream ts(stdout);
    ts << "Stage: " << stageStr(bs.stage()) << "\n";
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
    ts << "QSB_VERSION: " << QShaderPrivate::get(&bs)->qsbVersion << "\n";
#endif
    const QList<QShaderKey> keys = bs.availableShaders();
    ts << "Has " << keys.count() << " shaders: (unordered list)\n";
    for (int i = 0; i < keys.count(); ++i) {
        ts << "  Shader " << i << ": " << sourceStr(keys[i].source())
            << " " << sourceVersionStr(keys[i].sourceVersion())
            << " [" << sourceVariantStr(keys[i].sourceVariant()) << "]\n";
    }
    ts << "\n";
    ts << "Reflection info: " << bs.description().toJson() << "\n\n";
    for (int i = 0; i < keys.count(); ++i) {
        ts << "Shader " << i << ": " << sourceStr(keys[i].source())
            << " " << sourceVersionStr(keys[i].sourceVersion())
            << " [" << sourceVariantStr(keys[i].sourceVariant()) << "]\n";
        QShaderCode shader = bs.shader(keys[i]);
        if (!shader.entryPoint().isEmpty())
            ts << "Entry point: " << shader.entryPoint() << "\n";
        if (const QShader::NativeResourceBindingMap *map = bs.nativeResourceBindingMap(keys[i])) {
            ts << "Native resource binding map:\n";
            for (auto mapIt = map->cbegin(), mapItEnd = map->cend(); mapIt != mapItEnd; ++mapIt)
                ts << mapIt.key() << " -> [" << mapIt.value().first << ", " << mapIt.value().second << "]\n";
        }
        ts << "Contents:\n";
        switch (keys[i].source()) {
        case QShader::SpirvShader:
            Q_FALLTHROUGH();
        case QShader::DxbcShader:
            Q_FALLTHROUGH();
        case QShader::DxilShader:
            Q_FALLTHROUGH();
        case QShader::MetalLibShader:
            ts << "Binary of " << shader.shader().size() << " bytes\n\n";
            break;
        default:
            ts << shader.shader() << "\n";
            break;
        }
        ts << "\n************************************\n\n";
    }
}

static void extract(const QShader &bs, const QString &what, bool batchable, const QString &outfn)
{
    if (what == QLatin1String("reflect")) {
        const QByteArray reflect = bs.description().toJson();
        if (writeToFile(reflect, outfn, FileType::Text))
            qDebug("Reflection data written to %s", qPrintable(outfn));
        return;
    }

    if (what.contains(QLatin1Char('.'))) {
        const QStringList typeAndVersion = what.split(QLatin1Char('.'), Qt::SkipEmptyParts);
        QShader::Source src;
        if (typeAndVersion[0] == QLatin1String("spirv"))
            src = QShader::SpirvShader;
        else if (typeAndVersion[0] == QLatin1String("glsl"))
            src = QShader::GlslShader;
        else if (typeAndVersion[0] == QLatin1String("hlsl"))
            src = QShader::HlslShader;
        else if (typeAndVersion[0] == QLatin1String("msl"))
            src = QShader::MslShader;
        else if (typeAndVersion[0] == QLatin1String("dxbc"))
            src = QShader::DxbcShader;
        else if (typeAndVersion[0] == QLatin1String("dxil"))
            src = QShader::DxilShader;
        else if (typeAndVersion[0] == QLatin1String("metallib"))
            src = QShader::MetalLibShader;
        else
            return;

        QShaderVersion::Flags flags;
        QString version = typeAndVersion[1];
        if (version.endsWith(QLatin1String(" es"))) {
            version = version.left(version.count() - 3);
            flags |= QShaderVersion::GlslEs;
        } else if (version.endsWith(QLatin1String("es"))) {
            version = version.left(version.count() - 2);
            flags |= QShaderVersion::GlslEs;
        }
        const int ver = version.toInt();

        const QShader::Variant variant = batchable ? QShader::BatchableVertexShader : QShader::StandardShader;
        const QString variantStr = sourceVariantStr(variant);

        const QShaderCode code = bs.shader({ src, { ver, flags }, variant });
        if (!code.shader().isEmpty()) {
            if (writeToFile(code.shader(), outfn, FileType::Binary)) {
                const QString shaderTypeString = sourceStr(src);
                qDebug("%s %d%s code (variant %s) written to %s. Entry point is '%s'.",
                       qPrintable(shaderTypeString), ver, flags.testFlag(QShaderVersion::GlslEs) ? " es" : "",
                       qPrintable(variantStr),
                       qPrintable(outfn), code.entryPoint().constData());
            }
        }
    }
}

static QByteArray fxcProfile(const QShader &bs, const QShaderKey &k)
{
    QByteArray t;

    switch (bs.stage()) {
    case QShader::VertexStage:
        t += QByteArrayLiteral("vs_");
        break;
    case QShader::TessellationControlStage:
        t += QByteArrayLiteral("hs_");
        break;
    case QShader::TessellationEvaluationStage:
        t += QByteArrayLiteral("ds_");
        break;
    case QShader::GeometryStage:
        t += QByteArrayLiteral("gs_");
        break;
    case QShader::FragmentStage:
        t += QByteArrayLiteral("ps_");
        break;
    case QShader::ComputeStage:
        t += QByteArrayLiteral("cs_");
        break;
    default:
        break;
    }

    const int major = k.sourceVersion().version() / 10;
    const int minor = k.sourceVersion().version() % 10;
    t += QByteArray::number(major);
    t += '_';
    t += QByteArray::number(minor);

    return t;
}

static void replaceShaderContents(QShader *shaderPack,
                                  const QShaderKey &originalKey,
                                  QShader::Source newType,
                                  const QByteArray &contents,
                                  const QByteArray &entryPoint)
{
    QShaderKey newKey = originalKey;
    newKey.setSource(newType);
    QShaderCode shader(contents, entryPoint);
    shaderPack->setShader(newKey, shader);
    if (newKey != originalKey) {
        if (const QShader::NativeResourceBindingMap *map = shaderPack->nativeResourceBindingMap(originalKey)) {
            // Cannot just throw *map in setResourceBinding() because that will insert into
            // the table map is referencing into... Make a temporary to be safe.
            auto mapDeref = *map;
            shaderPack->setResourceBindingMap(newKey, mapDeref);
            shaderPack->removeResourceBindingMap(originalKey);
        }
        shaderPack->removeShader(originalKey);
    }
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QCommandLineParser cmdLineParser;
    const QString appDesc = QString::asprintf("Qt Shader Baker (using QShader from Qt %s)", qVersion());
    cmdLineParser.setApplicationDescription(appDesc);
    app.setApplicationVersion(QLatin1String(QT_VERSION_STR));
    cmdLineParser.addHelpOption();
    cmdLineParser.addVersionOption();
    cmdLineParser.addPositionalArgument(QLatin1String("file"), QObject::tr("Vulkan GLSL source file to compile"), QObject::tr("file"));
    QCommandLineOption batchableOption({ "b", "batchable" }, QObject::tr("Also generates rewritten vertex shader for Qt Quick scene graph batching."));
    cmdLineParser.addOption(batchableOption);
    QCommandLineOption batchLocOption("zorder-loc",
                                      QObject::tr("The extra vertex input location when rewriting for batching. Defaults to 7."),
                                      QObject::tr("location"));
    cmdLineParser.addOption(batchLocOption);
    QCommandLineOption glslOption("glsl",
                                  QObject::tr("Comma separated list of GLSL versions to generate. (for example, \"100 es,120,330\")"),
                                  QObject::tr("versions"));
    cmdLineParser.addOption(glslOption);
    QCommandLineOption hlslOption("hlsl",
                                  QObject::tr("Comma separated list of HLSL (Shader Model) versions to generate. F.ex. 50 is 5.0, 51 is 5.1."),
                                  QObject::tr("versions"));
    cmdLineParser.addOption(hlslOption);
    QCommandLineOption mslOption("msl",
                                 QObject::tr("Comma separated list of Metal Shading Language versions to generate. F.ex. 12 is 1.2, 20 is 2.0."),
                                 QObject::tr("versions"));
    cmdLineParser.addOption(mslOption);
    QCommandLineOption debugInfoOption("g", QObject::tr("Generate full debug info for SPIR-V and DXBC"));
    cmdLineParser.addOption(debugInfoOption);
    QCommandLineOption spirvOptOption("O", QObject::tr("Invoke spirv-opt to optimize SPIR-V for performance"));
    cmdLineParser.addOption(spirvOptOption);
    QCommandLineOption outputOption({ "o", "output" },
                                     QObject::tr("Output file for the shader pack."),
                                     QObject::tr("filename"));
    cmdLineParser.addOption(outputOption);
    QCommandLineOption fxcOption({ "c", "fxc" }, QObject::tr("In combination with --hlsl invokes fxc to store DXBC instead of HLSL."));
    cmdLineParser.addOption(fxcOption);
    QCommandLineOption mtllibOption({ "t", "metallib" },
                                    QObject::tr("In combination with --msl builds a Metal library with xcrun metal(lib) and stores that instead of the source."));
    cmdLineParser.addOption(mtllibOption);
    QCommandLineOption defineOption({ "D", "define" }, QObject::tr("Define macro"), QObject::tr("name[=value]"));
    cmdLineParser.addOption(defineOption);
    QCommandLineOption perTargetCompileOption({ "p", "per-target" }, QObject::tr("Enable per-target compilation. (instead of source->SPIRV->targets, do "
                                                                                 "source->SPIRV->target separately for each target)"));
    cmdLineParser.addOption(perTargetCompileOption);
    QCommandLineOption dumpOption({ "d", "dump" }, QObject::tr("Switches to dump mode. Input file is expected to be a shader pack."));
    cmdLineParser.addOption(dumpOption);
    QCommandLineOption extractOption({ "x", "extract" }, QObject::tr("Switches to extract mode. Input file is expected to be a shader pack. "
                                                                     "Result is written to the output specified by -o. Pass -b to choose the batchable variant. "
                                                                     "<what>=reflect|spirv.<version>|glsl.<version>|..."),
                                     QObject::tr("what"));
    cmdLineParser.addOption(extractOption);

    cmdLineParser.process(app);

    if (cmdLineParser.positionalArguments().isEmpty()) {
        cmdLineParser.showHelp();
        return 0;
    }

    QShaderBaker baker;
    for (const QString &fn : cmdLineParser.positionalArguments()) {
        if (cmdLineParser.isSet(dumpOption) || cmdLineParser.isSet(extractOption)) {
            QByteArray buf = readFile(fn, FileType::Binary);
            if (!buf.isEmpty()) {
                QShader bs = QShader::fromSerialized(buf);
                if (bs.isValid()) {
                    if (cmdLineParser.isSet(dumpOption)) {
                        dump(bs);
                    } else {
                        if (cmdLineParser.isSet(outputOption)) {
                            extract(bs, cmdLineParser.value(extractOption), cmdLineParser.isSet(batchableOption),
                                    cmdLineParser.value(outputOption));
                        } else {
                            qWarning("No output file specified");
                        }
                    }
                } else {
                    qWarning("Failed to deserialize %s", qPrintable(fn));
                }
            }
            continue;
        }

        baker.setSourceFileName(fn);

        baker.setPerTargetCompilation(cmdLineParser.isSet(perTargetCompileOption));

        QShaderBaker::SpirvOptions spirvOptions;
        // We either want full debug info, or none at all (so no variable names
        // either - that too can be stripped after the SPIRV-Cross stage).
        if (cmdLineParser.isSet(debugInfoOption))
            spirvOptions |= QShaderBaker::SpirvOption::GenerateFullDebugInfo;
        else
            spirvOptions |= QShaderBaker::SpirvOption::StripDebugAndVarInfo;

        baker.setSpirvOptions(spirvOptions);

        QList<QShader::Variant> variants;
        variants << QShader::StandardShader;
        if (cmdLineParser.isSet(batchableOption)) {
            variants << QShader::BatchableVertexShader;
            if (cmdLineParser.isSet(batchLocOption))
                baker.setBatchableVertexShaderExtraInputLocation(cmdLineParser.value(batchLocOption).toInt());
        }

        baker.setGeneratedShaderVariants(variants);

        QList<QShaderBaker::GeneratedShader> genShaders;

        genShaders << qMakePair(QShader::SpirvShader, QShaderVersion(100));

        if (cmdLineParser.isSet(glslOption)) {
            const QStringList versions = cmdLineParser.value(glslOption).trimmed().split(',');
            for (QString version : versions) {
                QShaderVersion::Flags flags;
                if (version.endsWith(QLatin1String(" es"))) {
                    version = version.left(version.count() - 3);
                    flags |= QShaderVersion::GlslEs;
                } else if (version.endsWith(QLatin1String("es"))) {
                    version = version.left(version.count() - 2);
                    flags |= QShaderVersion::GlslEs;
                }
                bool ok = false;
                int v = version.toInt(&ok);
                if (ok)
                    genShaders << qMakePair(QShader::GlslShader, QShaderVersion(v, flags));
                else
                    qWarning("Ignoring invalid GLSL version %s", qPrintable(version));
            }
        }

        if (cmdLineParser.isSet(hlslOption)) {
            const QStringList versions = cmdLineParser.value(hlslOption).trimmed().split(',');
            for (QString version : versions) {
                bool ok = false;
                int v = version.toInt(&ok);
                if (ok)
                    genShaders << qMakePair(QShader::HlslShader, QShaderVersion(v));
                else
                    qWarning("Ignoring invalid HLSL (Shader Model) version %s", qPrintable(version));
            }
        }

        if (cmdLineParser.isSet(mslOption)) {
            const QStringList versions = cmdLineParser.value(mslOption).trimmed().split(',');
            for (QString version : versions) {
                bool ok = false;
                int v = version.toInt(&ok);
                if (ok)
                    genShaders << qMakePair(QShader::MslShader, QShaderVersion(v));
                else
                    qWarning("Ignoring invalid MSL version %s", qPrintable(version));
            }
        }

        baker.setGeneratedShaders(genShaders);

        if (cmdLineParser.isSet(defineOption)) {
            QByteArray preamble;
            const QStringList defines = cmdLineParser.values(defineOption);
            for (const QString &def : defines) {
                const QStringList defs = def.split(QLatin1Char('='), Qt::SkipEmptyParts);
                if (!defs.isEmpty()) {
                    preamble.append("#define");
                    for (const QString &s : defs) {
                        preamble.append(' ');
                        preamble.append(s.toUtf8());
                    }
                    preamble.append('\n');
                }
            }
            baker.setPreamble(preamble);
        }

        QShader bs = baker.bake();
        if (!bs.isValid()) {
            qWarning("Shader baking failed: %s", qPrintable(baker.errorMessage()));
            return 1;
        }

        // post processing: run spirv-opt when requested for each entry with
        // type SpirvShader and replace the contents
        if (cmdLineParser.isSet(spirvOptOption)) {
            QTemporaryDir tempDir;
            if (!tempDir.isValid()) {
                qWarning("Failed to create temporary directory");
                return 1;
            }
            auto skeys = bs.availableShaders();
            for (QShaderKey &k : skeys) {
                if (k.source() == QShader::SpirvShader) {
                    QShaderCode s = bs.shader(k);

                    const QString tmpIn = writeTemp(tempDir, QLatin1String("qsb_spv_temp"), s, FileType::Binary);
                    const QString tmpOut = tempDir.path() + QLatin1String("/qsb_spv_temp_out");
                    if (tmpIn.isEmpty())
                        return 1;

                    const QStringList arguments({
                                                    QLatin1String("-O"),
                                                    QDir::toNativeSeparators(tmpIn),
                                                    QLatin1String("-o"), QDir::toNativeSeparators(tmpOut)
                                                });
                    QByteArray output;
                    QByteArray errorOutput;
                    bool success = runProcess(QLatin1String("spirv-opt"), arguments, &output, &errorOutput);
                    if (!success) {
                        if (!output.isEmpty() || !errorOutput.isEmpty()) {
                            qDebug("%s\n%s",
                                   qPrintable(output.constData()),
                                   qPrintable(errorOutput.constData()));
                        }
                        return 1;
                    }

                    const QByteArray bytecode = readFile(tmpOut, FileType::Binary);
                    if (bytecode.isEmpty())
                        return 1;

                    replaceShaderContents(&bs, k, QShader::SpirvShader, bytecode, s.entryPoint());
                }
            }
        }

        // post processing: run fxc when requested for each entry with type
        // HlslShader and add a new entry with type DxbcShader and remove the
        // original HlslShader entry
        if (cmdLineParser.isSet(fxcOption)) {
            QTemporaryDir tempDir;
            if (!tempDir.isValid()) {
                qWarning("Failed to create temporary directory");
                return 1;
            }
            auto skeys = bs.availableShaders();
            for (QShaderKey &k : skeys) {
                if (k.source() == QShader::HlslShader) {
                    QShaderCode s = bs.shader(k);

                    const QString tmpIn = writeTemp(tempDir, QLatin1String("qsb_hlsl_temp"), s, FileType::Text);
                    const QString tmpOut = tempDir.path() + QLatin1String("/qsb_hlsl_temp_out");
                    if (tmpIn.isEmpty())
                        return 1;

                    const QByteArray typeArg = fxcProfile(bs, k);
                    QStringList arguments({
                                              QLatin1String("/nologo"),
                                              QLatin1String("/E"), QString::fromLocal8Bit(s.entryPoint()),
                                              QLatin1String("/T"), QString::fromLocal8Bit(typeArg),
                                              QLatin1String("/Fo"), QDir::toNativeSeparators(tmpOut)
                                          });
                    if (cmdLineParser.isSet(debugInfoOption))
                        arguments << QLatin1String("/Od") << QLatin1String("/Zi");
                    arguments.append(QDir::toNativeSeparators(tmpIn));
                    QByteArray output;
                    QByteArray errorOutput;
                    bool success = runProcess(QLatin1String("fxc"), arguments, &output, &errorOutput);
                    if (!success) {
                        if (!output.isEmpty() || !errorOutput.isEmpty()) {
                            qDebug("%s\n%s",
                                   qPrintable(output.constData()),
                                   qPrintable(errorOutput.constData()));
                        }
                        return 1;
                    }

                    const QByteArray bytecode = readFile(tmpOut, FileType::Binary);
                    if (bytecode.isEmpty())
                        return 1;

                    replaceShaderContents(&bs, k, QShader::DxbcShader, bytecode, s.entryPoint());
                }
            }
        }

        // post processing: run xcrun metal and metallib when requested for
        // each entry with type MslShader and add a new entry with type
        // MetalLibShader and remove the original MslShader entry
        if (cmdLineParser.isSet(mtllibOption)) {
            QTemporaryDir tempDir;
            if (!tempDir.isValid()) {
                qWarning("Failed to create temporary directory");
                return 1;
            }
            auto skeys = bs.availableShaders();
            for (const QShaderKey &k : skeys) {
                if (k.source() == QShader::MslShader) {
                    QShaderCode s = bs.shader(k);

                    // having the .metal file extension may matter for the external tools here, so use that
                    const QString tmpIn = writeTemp(tempDir, QLatin1String("qsb_msl_temp.metal"), s, FileType::Text);
                    const QString tmpInterm = tempDir.path() + QLatin1String("/qsb_msl_temp_air");
                    const QString tmpOut = tempDir.path() + QLatin1String("/qsb_msl_temp_out");
                    if (tmpIn.isEmpty())
                        return 1;

                    qDebug("About to invoke xcrun with metal and metallib.\n"
                           "  qsb is set up for XCode 10. For earlier versions the -c argument may need to be removed.\n"
                           "  If getting unable to find utility \"metal\", do xcode-select --switch /Applications/Xcode.app/Contents/Developer");
                    const QString binary = QLatin1String("xcrun");
                    const QStringList baseArguments{QLatin1String("-sdk"), QLatin1String("macosx")};
                    QStringList arguments = baseArguments;
                    arguments.append({QLatin1String("metal"), QLatin1String("-c"), QDir::toNativeSeparators(tmpIn),
                                      QLatin1String("-o"), QDir::toNativeSeparators(tmpInterm)});
                    QByteArray output;
                    QByteArray errorOutput;
                    bool success = runProcess(binary, arguments, &output, &errorOutput);
                    if (!success) {
                        if (!output.isEmpty() || !errorOutput.isEmpty()) {
                            qDebug("%s\n%s",
                                   qPrintable(output.constData()),
                                   qPrintable(errorOutput.constData()));
                        }
                        return 1;
                    }

                    arguments = baseArguments;
                    arguments.append({QLatin1String("metallib"), QDir::toNativeSeparators(tmpInterm),
                                      QLatin1String("-o"), QDir::toNativeSeparators(tmpOut)});
                    output.clear();
                    errorOutput.clear();
                    success = runProcess(binary, arguments, &output, &errorOutput);
                    if (!success) {
                        if (!output.isEmpty() || !errorOutput.isEmpty()) {
                            qDebug("%s\n%s",
                                   qPrintable(output.constData()),
                                   qPrintable(errorOutput.constData()));
                        }
                        return 1;
                    }

                    const QByteArray bytecode = readFile(tmpOut, FileType::Binary);
                    if (bytecode.isEmpty())
                        return 1;

                    replaceShaderContents(&bs, k, QShader::MetalLibShader, bytecode, s.entryPoint());
                }
            }
        }

        if (cmdLineParser.isSet(outputOption))
            writeToFile(bs.serialized(), cmdLineParser.value(outputOption), FileType::Binary);
    }

    return 0;
}
