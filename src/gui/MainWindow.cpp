// =============================================================================
//  File   : src/gui/MainWindow.cpp
// =============================================================================
#include "MainWindow.h"

#include <QAction>
#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeySequence>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPixmap>
#include <QSize>
#include <QStatusBar>
#include <QStyle>
#include <QTabWidget>
#include <QThread>
#include <QTime>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

#include <vector>
#include <memory>

#include "../controller/DemoData.h"
#include "../model/GeneralPatient.h"
#include "../utils/DateUtils.h"
#include "../utils/Exceptions.h"
#include "../utils/FileHandler.h"
#include "../utils/StringUtils.h"
#include "AdminPanel.h"
#include "AppointmentPanel.h"
#include "DashboardPanel.h"
#include "PatientPanel.h"
#include "QueuePanel.h"
#include "ReportPanel.h"
#include "SearchPanel.h"
#include "UiHelpers.h"

namespace scapms::gui {

using scapms::controller::ClinicManager;

MainWindow::MainWindow(std::string dataDirectory, QWidget* parent)
    : QMainWindow(parent), m_manager(std::move(dataDirectory)) {
    setWindowTitle("SCAPMS - Smart Clinic Appointment & Patient Management System");
    setWindowIcon(style()->standardIcon(QStyle::SP_ComputerIcon));
    setMinimumSize(1180, 760);
    resize(1500, 950);

    auto* central = new QWidget(this);
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ---- branded header ----------------------------------------------------
    auto* header = new QFrame(central);
    header->setObjectName("headerBar");
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(18, 10, 18, 10);
    headerLayout->setSpacing(12);

    auto* logo = new QLabel(header);
    logo->setVisible(false);

    auto* brandBlock = new QVBoxLayout();
    brandBlock->setSpacing(0);
    auto* brandTitle = new QLabel("SCAPMS", header);
    brandTitle->setObjectName("brandTitle");
    auto* brandSubtitle = new QLabel(
        "Smart Clinic Appointment & Patient Management System", header);
    brandSubtitle->setObjectName("brandSubtitle");
    brandBlock->addWidget(brandTitle);
    brandBlock->addWidget(brandSubtitle);
    headerLayout->addLayout(brandBlock);
    headerLayout->addStretch(1);

    m_reminderLabel = new QLabel(header);
    m_reminderLabel->setObjectName("brandSubtitle");
    headerLayout->addWidget(m_reminderLabel);
    root->addWidget(header);

    // ---- tabs --------------------------------------------------------------
    m_tabs = new QTabWidget(central);
    m_tabs->setDocumentMode(false);
    m_tabs->setMovable(false);
    root->addWidget(m_tabs, 1);
    setCentralWidget(central);

    applyStyle();
    createActions();
    createMenus();
    createToolBar();
    createStatusBar();
    createPanels();
    connectPanels();

    // ---- timers (timer events) --------------------------------------------
    m_clockTimer = new QTimer(this);
    m_clockTimer->setInterval(1000);
    connect(m_clockTimer, &QTimer::timeout, this, &MainWindow::updateClock);
    m_clockTimer->start();

    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(30000);
    connect(m_refreshTimer, &QTimer::timeout, this, &MainWindow::onTimerTick);
    m_refreshTimer->start();

    updateClock();
}

void MainWindow::applyStyle() {
    const QString qss = appStyleSheet();
    if (!qss.isEmpty()) {
        setStyleSheet(qss);
    }
}

void MainWindow::createActions() {
    m_newAction = new QAction(style()->standardIcon(QStyle::SP_FileIcon),
                              "&New Session", this);
    m_newAction->setShortcut(QKeySequence::New);
    m_newAction->setStatusTip("Clear the current clinic session and start empty.");
    connect(m_newAction, &QAction::triggered, this, &MainWindow::onNewSession);

    m_saveAction = new QAction(style()->standardIcon(QStyle::SP_DialogSaveButton),
                               "&Save Data", this);
    m_saveAction->setShortcut(QKeySequence::Save);
    m_saveAction->setStatusTip("Write patients, appointments and services to CSV files.");
    connect(m_saveAction, &QAction::triggered, this, &MainWindow::onSave);

    m_loadAction = new QAction(style()->standardIcon(QStyle::SP_DialogOpenButton),
                               "&Load Data", this);
    m_loadAction->setShortcut(QKeySequence::Open);
    m_loadAction->setStatusTip("Load a clinic data folder from disk.");
    connect(m_loadAction, &QAction::triggered, this, &MainWindow::onLoad);

    m_exitAction = new QAction("E&xit", this);
    m_exitAction->setShortcut(QKeySequence::Quit);
    m_exitAction->setStatusTip("Close the application.");
    connect(m_exitAction, &QAction::triggered, this, &QWidget::close);

    m_undoAction = new QAction(style()->standardIcon(QStyle::SP_ArrowBack),
                               "&Undo Last Operation", this);
    m_undoAction->setShortcut(QKeySequence::Undo);
    m_undoAction->setStatusTip("Reverse the most recent administrative operation.");
    connect(m_undoAction, &QAction::triggered, this, &MainWindow::onUndo);

    m_refreshAction = new QAction(style()->standardIcon(QStyle::SP_BrowserReload),
                                  "&Refresh", this);
    m_refreshAction->setShortcut(QKeySequence::Refresh);
    m_refreshAction->setStatusTip("Recompute every panel from the database.");
    connect(m_refreshAction, &QAction::triggered, this, &MainWindow::refreshAll);

    m_findAction = new QAction("&Find Patient", this);
    m_findAction->setShortcut(QKeySequence::Find);
    m_findAction->setStatusTip("Jump to the patient filter.");
    connect(m_findAction, &QAction::triggered, this, &MainWindow::onFindPatient);
}

void MainWindow::createMenus() {
    auto* fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction(m_newAction);
    fileMenu->addAction(m_saveAction);
    fileMenu->addAction(m_loadAction);
    fileMenu->addSeparator();
    fileMenu->addAction(m_exitAction);

    auto* editMenu = menuBar()->addMenu("&Edit");
    editMenu->addAction(m_undoAction);
    editMenu->addAction(m_findAction);
    editMenu->addAction(m_refreshAction);

    auto* viewMenu = menuBar()->addMenu("&View");
    for (int index = 0; index < 7; ++index) {
        const QStringList names{"Dashboard", "Patient Records", "Appointments",
                                "Waiting Queue", "Search & Sort", "Reports", "Admin"};
        auto* action = new QAction(names.at(index), this);
        action->setShortcut(QKeySequence(QString("Ctrl+%1").arg(index + 1)));
        connect(action, &QAction::triggered, this, [this, index]() { selectTab(index); });
        viewMenu->addAction(action);
    }

    auto* helpMenu = menuBar()->addMenu("&Help");
    auto* aboutAction = new QAction("&About SCAPMS", this);
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAbout);
    helpMenu->addAction(aboutAction);

    auto* aboutQtAction = new QAction("About &Qt", this);
    connect(aboutQtAction, &QAction::triggered, qApp, &QApplication::aboutQt);
    helpMenu->addAction(aboutQtAction);
}

void MainWindow::createToolBar() {
    auto* toolbar = addToolBar("Main");
    toolbar->setMovable(false);
    toolbar->setIconSize(QSize(18, 18));
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    auto* patientsAction = new QAction(style()->standardIcon(QStyle::SP_FileDialogDetailedView),
                                       "Patients", this);
    patientsAction->setToolTip("Open the patient register.");
    connect(patientsAction, &QAction::triggered, this, [this]() {
        selectTab(1);
        m_patientPanel->prepareNewPatient();
    });

    auto* scheduleAction = new QAction(style()->standardIcon(QStyle::SP_FileDialogListView),
                                       "Schedule", this);
    scheduleAction->setToolTip("Schedule a new appointment.");
    connect(scheduleAction, &QAction::triggered, this, [this]() {
        selectTab(2);
        m_appointmentPanel->focusScheduling();
    });

    auto* queueAction = new QAction(style()->standardIcon(QStyle::SP_ArrowForward),
                                    "Queue", this);
    queueAction->setToolTip("Open the waiting queue.");
    connect(queueAction, &QAction::triggered, this, [this]() { selectTab(3); });

    auto* searchAction = new QAction(style()->standardIcon(QStyle::SP_FileDialogContentsView),
                                     "Search", this);
    searchAction->setToolTip("Search and sort patient records.");
    connect(searchAction, &QAction::triggered, this, [this]() {
        selectTab(4);
        m_searchPanel->focusSearch();
    });

    toolbar->addAction(patientsAction);
    toolbar->addAction(scheduleAction);
    toolbar->addAction(queueAction);
    toolbar->addAction(searchAction);
    toolbar->addSeparator();
    toolbar->addAction(m_saveAction);
    toolbar->addAction(m_undoAction);
}

void MainWindow::createStatusBar() {
    statusBar()->showMessage("Ready.");

    m_patientCountLabel = new QLabel("Patients: 0", this);
    m_queueCountLabel = new QLabel("Queue: 0", this);
    m_clockLabel = new QLabel("--:--:--", this);

    statusBar()->addPermanentWidget(m_patientCountLabel);
    statusBar()->addPermanentWidget(m_queueCountLabel);
    statusBar()->addPermanentWidget(m_clockLabel);
}

void MainWindow::createPanels() {
    m_dashboard = new DashboardPanel(&m_manager, this);
    m_patientPanel = new PatientPanel(&m_manager, this);
    m_appointmentPanel = new AppointmentPanel(&m_manager, this);
    m_queuePanel = new QueuePanel(&m_manager, this);
    m_searchPanel = new SearchPanel(&m_manager, this);
    m_reportPanel = new ReportPanel(&m_manager, this);
    m_adminPanel = new AdminPanel(&m_manager, this);

    m_tabs->addTab(m_dashboard, "Dashboard");
    m_tabs->addTab(m_patientPanel, "Patient Records");
    m_tabs->addTab(m_appointmentPanel, "Appointments");
    m_tabs->addTab(m_queuePanel, "Waiting Queue");
    m_tabs->addTab(m_searchPanel, "Search && Sort");
    m_tabs->addTab(m_reportPanel, "Reports");
    m_tabs->addTab(m_adminPanel, "Admin");
}

void MainWindow::connectPanels() {
    connect(m_patientPanel, &PatientPanel::dataChanged, this, &MainWindow::refreshAll);
    connect(m_appointmentPanel, &AppointmentPanel::dataChanged, this,
            &MainWindow::refreshAll);
    connect(m_queuePanel, &QueuePanel::dataChanged, this, &MainWindow::refreshAll);
    connect(m_adminPanel, &AdminPanel::dataChanged, this, &MainWindow::refreshAll);

    connect(m_patientPanel, &PatientPanel::statusMessage, this,
            &MainWindow::onStatusMessage);
    connect(m_appointmentPanel, &AppointmentPanel::statusMessage, this,
            &MainWindow::onStatusMessage);
    connect(m_queuePanel, &QueuePanel::statusMessage, this,
            &MainWindow::onStatusMessage);
    connect(m_reportPanel, &ReportPanel::statusMessage, this,
            &MainWindow::onStatusMessage);
    connect(m_searchPanel, &SearchPanel::statusMessage, this,
            &MainWindow::onStatusMessage);
    connect(m_dashboard, &DashboardPanel::statusMessage, this,
            &MainWindow::onStatusMessage);
}

void MainWindow::loadInitialData() {
    const std::string directory = m_manager.dataDirectory();
    const std::string patientFile =
        scapms::model::ClinicDatabase::patientsPath(directory);

    try {
        if (scapms::utils::FileHandler::exists(patientFile)) {
            m_manager.load(directory);
            statusBar()->showMessage(
                QString("Loaded %1 patients and %2 appointments from %3.")
                    .arg(m_manager.database().patientCount())
                    .arg(m_manager.database().appointmentCount())
                    .arg(toQt(directory)),
                8000);
        } else {
            controller::DemoData::seed(m_manager);
            m_manager.save();
            statusBar()->showMessage(
                QString("First run: demonstration clinic created in %1.")
                    .arg(toQt(directory)),
                8000);
        }
    } catch (const scapms::utils::ClinicException& error) {
        QMessageBox::warning(
            this, "Could not load clinic data",
            QString("%1\n\nA fresh demonstration clinic is shown instead. Use "
                    "File > Load Data to open another folder.")
                .arg(error.what()));
        controller::DemoData::seed(m_manager);
    } catch (const std::exception& error) {
        QMessageBox::critical(this, "Unexpected data error", error.what());
    }

    refreshAll();
}

void MainWindow::refreshAll() {
    if (m_dashboard == nullptr) {
        return;
    }
    m_dashboard->refresh();
    m_patientPanel->refresh();
    m_appointmentPanel->refresh();
    m_queuePanel->refresh();
    m_searchPanel->refresh();
    m_reportPanel->refresh();
    m_adminPanel->refresh();
    updateCounters();
}

void MainWindow::updateCounters() {
    const auto stats = m_manager.stats();
    if (m_patientCountLabel != nullptr) {
        m_patientCountLabel->setText(
            QString("Patients: %1  |  Appointments: %2")
                .arg(stats.totalPatients)
                .arg(stats.totalAppointments));
        m_queueCountLabel->setText(
            QString("Queue: %1 (%2 priority)").arg(stats.queueLength).arg(stats.priorityQueueLength));
    }
    if (m_undoAction != nullptr) {
        m_undoAction->setEnabled(m_manager.canUndo());
        m_undoAction->setText(m_manager.canUndo()
                                  ? QString("&Undo Last Operation (%1)")
                                        .arg(m_manager.undoDepth())
                                  : QString("&Undo Last Operation"));
    }
}

void MainWindow::updateClock() {
    if (m_clockLabel != nullptr) {
        m_clockLabel->setText(QDateTime::currentDateTime().toString("ddd dd MMM  HH:mm:ss"));
    }
}

void MainWindow::onTimerTick() {
    // Timer event: periodic dashboard refresh + reminder for imminent appointments.
    refreshAll();

    const std::string today = scapms::utils::todayIso();
    const QTime now = QTime::currentTime();
    for (const auto& appointment : m_manager.database().appointments()) {
        if (!appointment.isPending() || appointment.date() != today) {
            continue;
        }
        const QTime slot = QTime::fromString(toQt(appointment.time()), "HH:mm");
        if (!slot.isValid()) {
            continue;
        }
        const int minutes = static_cast<int>(now.secsTo(slot)) / 60;
        if (minutes >= 0 && minutes <= 30) {
            const auto* patient =
                m_manager.database().findPatientById(appointment.patientId());
            const QString reminder =
                QString("Reminder: %1 has an appointment at %2 (in %3 minutes).")
                    .arg(patient ? toQt(patient->name()) : QString("A patient"),
                         toQt(appointment.time()))
                    .arg(minutes);
            m_reminderLabel->setText(reminder);
            statusBar()->showMessage(reminder, 10000);
            return;
        }
    }
    m_reminderLabel->clear();

    if (m_manager.queueLength() > 0) {
        statusBar()->showMessage(
            QString("%1 patient(s) waiting for consultation.")
                .arg(m_manager.queueLength()),
            8000);
    }
}

void MainWindow::onNewSession() {
    const auto answer = QMessageBox::question(
        this, "New session",
        "Clear all patients, appointments, queues and undo history?\n\n"
        "Unsaved changes will be lost.",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }
    m_manager.newSession();
    refreshAll();
    statusBar()->showMessage("New empty session started.", 6000);
}

void MainWindow::onSave() {
    try {
        m_manager.save();
        refreshAll();
        statusBar()->showMessage(
            "Saved to " + toQt(m_manager.dataDirectory()) + ".", 6000);
    } catch (const scapms::utils::ClinicException& error) {
        QMessageBox::warning(this, "Save failed", error.what());
    } catch (const std::exception& error) {
        QMessageBox::critical(this, "Save failed", error.what());
    }
}

void MainWindow::onLoad() {
    const QString directory = QFileDialog::getExistingDirectory(
        this, "Choose the data folder (contains patients.csv)",
        toQt(m_manager.dataDirectory()));
    if (directory.isEmpty()) {
        return;
    }
    try {
        m_manager.load(directory.toStdString());
        refreshAll();
        statusBar()->showMessage("Loaded data from " + directory + ".", 6000);
    } catch (const scapms::utils::ClinicException& error) {
        QMessageBox::warning(this, "Load failed",
                             QString("%1\n\nThe current session is unchanged.")
                                 .arg(error.what()));
    } catch (const std::exception& error) {
        QMessageBox::critical(this, "Load failed", error.what());
    }
}

void MainWindow::onUndo() {
    try {
        const std::string description = m_manager.undoLastOperation();
        refreshAll();
        statusBar()->showMessage("Undone: " + toQt(description), 7000);
    } catch (const scapms::utils::ClinicException& error) {
        QMessageBox::information(this, "Nothing to undo", error.what());
    } catch (const std::exception& error) {
        QMessageBox::critical(this, "Undo failed", error.what());
    }
}

void MainWindow::onFindPatient() {
    selectTab(1);
    m_patientPanel->focusSearch();
}

void MainWindow::onAbout() {
    QMessageBox::about(
        this, "About SCAPMS",
        "<h2>SCAPMS</h2>"
        "<p><b>Smart Clinic Appointment &amp; Patient Management System</b></p>"
        "<p>COS202 Programming II - Lab Assessment Project</p>"
        "<p>Built with C++17 and the Qt 6 Widgets framework.</p>"
        "<ul>"
        "<li>Abstract <code>PatientRecord</code> base class with three concrete "
        "patient types and the <code>Appointable</code> interface.</li>"
        "<li>Hand-written searching (linear, binary, recursive) and sorting "
        "(selection, insertion, merge, quick).</li>"
        "<li><code>std::vector</code>, <code>std::queue</code>, "
        "<code>std::stack</code> and a fixed-size array cache.</li>"
        "<li>CSV persistence, undo stack, five reports, timers and validation.</li>"
        "</ul>");
}

void MainWindow::onStatusMessage(const QString& message) {
    statusBar()->showMessage(message, 7000);
}

void MainWindow::selectTab(int index) {
    if (m_tabs != nullptr && index >= 0 && index < m_tabs->count()) {
        m_tabs->setCurrentIndex(index);
    }
}

void MainWindow::stageScreenshotSession() {
    // The waiting queue is deliberately session-only data (it is not part of the
    // persisted clinic files), so a reload starts with an empty queue. For the
    // documentation screenshots a realistic queue is staged here.
    if (m_manager.queueLength() == 0) {
        for (const char* patientId : {"P-0002", "P-0005", "P-0006", "P-0008", "P-0014"}) {
            try {
                m_manager.joinQueue(patientId);
            } catch (const std::exception&) {
                // Staging must never abort the capture run.
            }
        }
    }
    for (const char* patientId : {"P-0001", "P-0003", "P-0002", "P-0005"}) {
        (void)m_manager.patient(patientId);
    }
    refreshAll();
}

void MainWindow::stageUndoHistory() {
    // Two recorded administrative operations so that the undo stack, the
    // history list and the Ctrl+Z action are visible in the screenshot.
    try {
        m_manager.updatePatient(
            "P-0012", std::make_unique<model::GeneralPatient>(
                          "P-0012", "Yusuf Lawal", 27, "+234 809 555 0112", "Male",
                          scapms::utils::todayIso(), "Reliance HMO",
                          "Peptic ulcer - under review"));
    } catch (const std::exception&) {
    }
    try {
        for (const auto& appointment : m_manager.database().appointments()) {
            if (appointment.id() == "APT-0011" && appointment.isPending()) {
                m_manager.cancelAppointment("APT-0011");
                break;
            }
        }
    } catch (const std::exception&) {
    }
    try {
        m_manager.deletePatient("P-0010");
    } catch (const std::exception&) {
    }
    refreshAll();
}

void MainWindow::captureScreenshots(const QString& directory) {
    QDir().mkpath(directory);
    resize(1500, 950);

    stageScreenshotSession();
    // A short, realistic administration history so the undo stack is populated
    // consistently across every screenshot (14 -> 13 patients after the delete).
    stageUndoHistory();
    statusBar()->showMessage("Ready.");

    const QStringList names{"01-dashboard",       "02-patient-records",
                            "03-appointments",    "04-waiting-queue",
                            "05-search-and-sort", "06-reports",
                            "07-admin-undo"};
    for (int index = 0; index < m_tabs->count(); ++index) {
        if (index == 4) {
            // Show a real sort result (with comparisons/moves/time) rather than
            // the untouched register order.
            m_searchPanel->applySort(controller::SortField::PatientName,
                                     controller::SortAlgorithm::Merge);
        }
        selectTab(index);
        QApplication::processEvents();
        QThread::msleep(150);
        QApplication::processEvents();
        const QPixmap shot = grab();
        const QString file = directory + "/" + names.value(index, QString("tab")) + ".png";
        shot.save(file, "PNG");
    }
}

} // namespace scapms::gui
