// =============================================================================
//  SCAPMS - Smart Clinic Appointment & Patient Management System
//  File   : src/gui/MainWindow.h
//  Layer  : GUI
//  Purpose: Application shell - menus, toolbar, status bar, timers and the tab
//           widget that hosts the seven panels. It owns the ClinicManager and
//           is the only class that knows about every panel.
// =============================================================================
#ifndef SCAPMS_GUI_MAINWINDOW_H
#define SCAPMS_GUI_MAINWINDOW_H

#include <QMainWindow>

#include <string>

#include "../controller/ClinicManager.h"

class QAction;
class QLabel;
class QTabWidget;
class QTimer;

namespace scapms::gui {

class AdminPanel;
class AppointmentPanel;
class DashboardPanel;
class PatientPanel;
class QueuePanel;
class ReportPanel;
class SearchPanel;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(std::string dataDirectory, QWidget* parent = nullptr);

    [[nodiscard]] controller::ClinicManager& manager() noexcept { return m_manager; }

    /// Loads the data folder, seeding the demonstration clinic on first run.
    void loadInitialData();

    /// Renders every tab to PNG files (used for the submission screenshots).
    void captureScreenshots(const QString& directory);

public slots:
    void selectTab(int index);
    void refreshAll();

private slots:
    void updateClock();
    void onTimerTick();
    void onNewSession();
    void onSave();
    void onLoad();
    void onUndo();
    void onFindPatient();
    void onAbout();
    void onStatusMessage(const QString& message);

private:
    void applyStyle();
    void createActions();
    void createMenus();
    void createToolBar();
    void createStatusBar();
    void createPanels();
    void connectPanels();
    void updateCounters();
    void stageScreenshotSession();
    void stageUndoHistory();

    controller::ClinicManager m_manager;

    QTabWidget* m_tabs = nullptr;
    DashboardPanel* m_dashboard = nullptr;
    PatientPanel* m_patientPanel = nullptr;
    AppointmentPanel* m_appointmentPanel = nullptr;
    QueuePanel* m_queuePanel = nullptr;
    SearchPanel* m_searchPanel = nullptr;
    ReportPanel* m_reportPanel = nullptr;
    AdminPanel* m_adminPanel = nullptr;

    QAction* m_saveAction = nullptr;
    QAction* m_loadAction = nullptr;
    QAction* m_newAction = nullptr;
    QAction* m_exitAction = nullptr;
    QAction* m_undoAction = nullptr;
    QAction* m_refreshAction = nullptr;
    QAction* m_findAction = nullptr;

    QLabel* m_patientCountLabel = nullptr;
    QLabel* m_queueCountLabel = nullptr;
    QLabel* m_clockLabel = nullptr;
    QLabel* m_reminderLabel = nullptr;

    QTimer* m_clockTimer = nullptr;
    QTimer* m_refreshTimer = nullptr;
};

} // namespace scapms::gui

#endif // SCAPMS_GUI_MAINWINDOW_H
