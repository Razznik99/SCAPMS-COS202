// =============================================================================
//  SCAPMS - Smart Clinic Appointment & Patient Management System
//  File   : src/gui/ReportPanel.h
//  Layer  : GUI
//  Purpose: Reports tab - generates the clinic's five simple reports and
//           exports the current report to CSV through a file dialog.
// =============================================================================
#ifndef SCAPMS_GUI_REPORTPANEL_H
#define SCAPMS_GUI_REPORTPANEL_H

#include <QWidget>

#include "../controller/ReportGenerator.h"

class QComboBox;
class QDateEdit;
class QLabel;
class QPushButton;
class QTableWidget;

namespace scapms::controller {
class ClinicManager;
}

namespace scapms::gui {

class ReportPanel : public QWidget {
    Q_OBJECT

public:
    explicit ReportPanel(controller::ClinicManager* manager, QWidget* parent = nullptr);

    void refresh();

signals:
    void statusMessage(const QString& message);

private slots:
    void onGenerate();
    void onExportCsv();
    void onReportTypeChanged(int index);

private:
    [[nodiscard]] controller::ReportType currentType() const;
    void generate(bool notify);
    void render(const controller::ReportResult& report);

    controller::ClinicManager* m_manager = nullptr;
    QComboBox* m_typeCombo = nullptr;
    QDateEdit* m_dateEdit = nullptr;
    QLabel* m_titleLabel = nullptr;
    QLabel* m_descriptionLabel = nullptr;
    QLabel* m_summaryLabel = nullptr;
    QTableWidget* m_table = nullptr;
    QPushButton* m_exportButton = nullptr;

    controller::ReportResult m_currentReport;
    bool m_generated = false;
};

} // namespace scapms::gui

#endif // SCAPMS_GUI_REPORTPANEL_H
