// =============================================================================
//  SCAPMS - Smart Clinic Appointment & Patient Management System
//  File   : src/gui/DashboardPanel.h
//  Layer  : GUI
//  Purpose: Landing dashboard - live statistic cards, the next appointments, a
//           category breakdown, the recently accessed patients and the
//           appointment / queue reminder banner.
// =============================================================================
#ifndef SCAPMS_GUI_DASHBOARDPANEL_H
#define SCAPMS_GUI_DASHBOARDPANEL_H

#include <QWidget>

class QLabel;
class QListWidget;
class QTableWidget;

namespace scapms::controller {
class ClinicManager;
}

namespace scapms::gui {

class DashboardPanel : public QWidget {
    Q_OBJECT

public:
    explicit DashboardPanel(controller::ClinicManager* manager, QWidget* parent = nullptr);

    void refresh();

signals:
    void statusMessage(const QString& message);

private:
    void updateCards();
    void updateUpcoming();
    void updateBreakdown();
    void updateRecent();
    void updateBanner();

    controller::ClinicManager* m_manager = nullptr;

    QLabel* m_dateLabel = nullptr;
    QLabel* m_bannerLabel = nullptr;

    QLabel* m_totalPatientsValue = nullptr;
    QLabel* m_todayValue = nullptr;
    QLabel* m_pendingValue = nullptr;
    QLabel* m_completedValue = nullptr;
    QLabel* m_queueValue = nullptr;
    QLabel* m_emergencyValue = nullptr;
    QLabel* m_revenueValue = nullptr;
    QLabel* m_undoValue = nullptr;

    QTableWidget* m_upcomingTable = nullptr;
    QLabel* m_breakdownLabel = nullptr;
    QListWidget* m_recentList = nullptr;
    QLabel* m_recentHint = nullptr;
};

} // namespace scapms::gui

#endif // SCAPMS_GUI_DASHBOARDPANEL_H
