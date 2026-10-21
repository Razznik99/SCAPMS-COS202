// =============================================================================
//  SCAPMS - Smart Clinic Appointment & Patient Management System
//  File   : src/main.cpp
//  Purpose: Application entry point. Parses the command line, applies the
//           theme, then starts the GUI, the automated test suite or the
//           screenshot generator.
//
//  Usage:
//     SCAPMS                                   start the GUI
//     SCAPMS --data <folder>                   use a specific data folder
//     SCAPMS --selftest                        run the automated tests
//     SCAPMS --capture <folder>                render each tab to PNG
// =============================================================================
#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QStyleFactory>
#include <QStringList>
#include <QTextStream>

#include "gui/MainWindow.h"
#include "../tests/SelfTest.h"

namespace {

/// Resolves the folder that holds patients.csv / appointments.csv.
/// Priority: --data, $SCAPMS_DATA_DIR, <exe>/data, <exe>/../data, ./data.
QString resolveDataDirectory(const QString& explicitDirectory) {
    if (!explicitDirectory.trimmed().isEmpty()) {
        return QDir(explicitDirectory).absolutePath();
    }

    const QByteArray fromEnvironment = qgetenv("SCAPMS_DATA_DIR");
    if (!fromEnvironment.isEmpty()) {
        return QDir(QString::fromLocal8Bit(fromEnvironment)).absolutePath();
    }

    const QString applicationDirectory = QCoreApplication::applicationDirPath();
    const QStringList candidates{applicationDirectory + "/data",
                                 applicationDirectory + "/../data",
                                 QDir::currentPath() + "/data"};
    for (const auto& candidate : candidates) {
        if (QFileInfo::exists(candidate)) {
            return QDir(candidate).absolutePath();
        }
    }
    return QDir(applicationDirectory + "/data").absolutePath();
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication application(argc, argv);
    QApplication::setOrganizationName("COS202 Programming II");
    QApplication::setApplicationName("SCAPMS");
    QApplication::setApplicationVersion("1.0.0");

    if (auto* fusion = QStyleFactory::create("Fusion")) {
        application.setStyle(fusion);
    }
    QFont font("Segoe UI", 9);
    if (font.pointSize() < 9) {
        font.setPointSize(9);
    }
    application.setFont(font);

    QCommandLineParser parser;
    parser.setApplicationDescription(
        "SCAPMS - Smart Clinic Appointment & Patient Management System "
        "(COS202 Programming II lab assessment).");
    parser.addHelpOption();
    parser.addVersionOption();

    const QCommandLineOption selfTestOption(
        "selftest", "Run the automated test suite and exit.");
    const QCommandLineOption captureOption(
        "capture", "Render every tab to PNG files in <directory> and exit.",
        "directory");
    const QCommandLineOption dataOption(
        "data", "Data folder containing patients.csv and appointments.csv.",
        "directory");
    parser.addOption(selfTestOption);
    parser.addOption(captureOption);
    parser.addOption(dataOption);
    parser.process(application);

    if (parser.isSet(selfTestOption)) {
        QTextStream(stdout) << "SCAPMS self-test mode\n";
        const int failures = scapms::tests::runAllTests();
        return failures == 0 ? 0 : 1;
    }

    const QString dataDirectory = resolveDataDirectory(parser.value(dataOption));

    scapms::gui::MainWindow window(dataDirectory.toStdString());
    window.loadInitialData();
    window.show();

    if (parser.isSet(captureOption)) {
        QApplication::processEvents();
        window.captureScreenshots(parser.value(captureOption));
        return 0;
    }

    return application.exec();
}
