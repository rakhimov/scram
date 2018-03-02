/*
 * Copyright (C) 2015-2018 Olzhas Rakhimov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/// @file
/// The main entrance to the SCRAM GUI.

#include <exception>
#include <iostream>
#include <string>
#include <vector>

#include <QApplication>
#include <QCoreApplication>
#include <QIcon>
#include <QLibraryInfo>
#include <QMessageBox>
#include <QSettings>
#include <QString>
#include <QTranslator>

#include <boost/exception/diagnostic_information.hpp>
#include <boost/program_options.hpp>

#include "language.h"
#include "mainwindow.h"

#include "src/error.h"
#include "src/version.h"

namespace po = boost::program_options;

namespace {

/// Parses the command-line arguments.
///
/// @param[in] argc  Count of arguments.
/// @param[in] argv  Values of arguments.
/// @param[out] vm  Variables map of program options.
///
/// @returns 0 for success.
/// @returns 1 for errored state.
/// @returns -1 for information only state like help and version.
int parseArguments(int argc, char *argv[], po::variables_map *vm) noexcept
{
    const char *usage = "Usage:    scram-gui [options] [input-files]...";
    po::options_description desc("Options");
    // clang-format off
    desc.add_options()
            ("help", "Display this help message")
            ("project", po::value<std::string>()->value_name("path"),
             "Project file with analysis configurations");
    // clang-format on
    try {
        po::store(po::parse_command_line(argc, argv, desc), *vm);
    } catch (const std::exception &err) {
        std::cerr << "Option error: " << err.what() << "\n\n"
                  << usage << "\n\n"
                  << desc << "\n";
        return 1;
    }

    po::notify(*vm);

    // Process command-line arguments.
    if (vm->count("help")) {
        std::cout << usage << "\n\n" << desc << "\n";
        return -1;
    }
    desc.add_options()("input-files", po::value<std::vector<std::string>>());
    po::positional_options_description p;
    p.add("input-files", -1);

    po::store(
        po::command_line_parser(argc, argv).options(desc).positional(p).run(),
        *vm);
    po::notify(*vm);
    return 0;
}

/// Produces critical notification dialog
/// for errors that are not expected to crash the application.
void notifyError(const QString &title, const QString &text,
                 const QString &detail = {}) noexcept
{
    QMessageBox message(QMessageBox::Critical, title, text, QMessageBox::Ok);
    message.setDetailedText(detail);
    message.exec();
}

/// Guards the application from crashes on escaped internal exceptions.
class GuardedApplication : public QApplication
{
public:
    using QApplication::QApplication;

    bool notify(QObject *receiver, QEvent *event) override
    {
        try {
            return QApplication::notify(receiver, event);
        } catch (const scram::Error &err) {
            std::string message = boost::diagnostic_information(err);
            qCritical("%s", message.c_str());
            notifyError(QStringLiteral("Internal SCRAM Error"),
                        QStringLiteral("Uncaught exception."),
                        QString::fromStdString(message));
        } catch (const std::exception &err) {
            qCritical("%s", err.what());
            notifyError(QStringLiteral("Internal Exception Error"),
                        QStringLiteral("Uncaught foreign exception."),
                        QString::fromUtf8(err.what()));
        } catch (...) {
            qCritical("Unknown exception type.");
            notifyError(QStringLiteral("Internal Exception Error"),
                        QStringLiteral("Unknown exception type."));
        }
        return false;
    }
};

/// Installs translators to the main application.
///
/// @param[in,out] app  The application to register translators.
///
/// @pre No application window has been created.
void installTranslators(GuardedApplication *app)
{
    QString language = QSettings().value(QStringLiteral("language")).toString();
    if (language.isEmpty())
        language = QLocale::system().name();

    if (language.startsWith(QStringLiteral("en")))
        return; // The default language.

    auto loadTs = [&app, &language](const char *domain, const QString &tsPath) {
        auto *translator = new QTranslator(app);
        if (translator->load(QStringLiteral("%1_%2").arg(
                                 QString::fromLatin1(domain), language),
                             tsPath)) {
            app->installTranslator(translator);
            return true;
        }
        delete translator;
        qCritical("Missing translations: %s_%s", domain,
                  language.toStdString().data());
        return false;
    };

    auto scramTsPath = QString::fromStdString(scram::gui::translationsPath());
    if (!loadTs("scramgui", scramTsPath))
        return; // The language is not available or installed.

    QString qtTsPath = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
    loadTs("qtbase", qtTsPath);
    loadTs("qt", qtTsPath);
}

} // namespace

/// Command-line SCRAM GUI entrance.
///
/// @param[in] argc  Argument count.
/// @param[in] argv  Argument vector (including Qt args).
///
/// @returns 0 for success.
///
/// @note There are implicit Qt-specific options
///       (e.g., -style, -stylesheet, -platform, -widgetcount, -reverse).
///       These options are not listed in the help/usage of SCRAM GUI,
///       but they can be invoked for debugging or advanced GUI customization.
int main(int argc, char *argv[])
{
    // Keep the following commented code!
    // In some static build configurations,
    // the resources may fail to load.
    // However, the most distributions are expected to be shared builds,
    // so the explicit load should not be used, but it is kept for debugging.
    /* Q_INIT_RESOURCE(res); */
    GuardedApplication app(argc, argv);

    QCoreApplication::setOrganizationName(QStringLiteral("scram"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("scram-pra.org"));
    QCoreApplication::setApplicationName(QStringLiteral("scram"));
    QCoreApplication::setApplicationVersion(QString::fromLatin1(SCRAM_VERSION));

    if (QIcon::themeName().isEmpty())
        QIcon::setThemeName(QStringLiteral("tango"));
    else
        QIcon::setThemeName(QStringLiteral("Humanity"));

    installTranslators(&app);

    scram::gui::MainWindow w;
    w.show();

    if (argc > 1) {
        po::variables_map vm;
        int ret = parseArguments(argc, argv, &vm);
        if (ret == 1)
            return 1;
        if (ret == -1)
            return 0;
        std::vector<std::string> inputFiles;
        try {
            if (vm.count("input-files"))
                inputFiles = vm["input-files"].as<std::vector<std::string>>();
            if (vm.count("project")) {
                w.setConfig(vm["project"].as<std::string>(), inputFiles);
            } else {
                w.addInputFiles(inputFiles);
            }
        } catch (const boost::exception &err) {
            std::string message = boost::diagnostic_information(err);
            qCritical("%s", message.c_str());
            return 1;
        }
    }
    return app.exec();
}
