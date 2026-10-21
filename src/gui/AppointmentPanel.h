// =============================================================================
//  SCAPMS - Smart Clinic Appointment & Patient Management System
//  File   : src/gui/AppointmentPanel.h
//  Layer  : GUI
//  Purpose: Appointments tab - schedule, complete, cancel and remove
//           appointments, with live eligibility feedback per patient type.
// =============================================================================
#ifndef SCAPMS_GUI_APPOINTMENTPANEL_H
#define SCAPMS_GUI_APPOINTMENTPANEL_H

#include <QWidget>

#include <vector>

#include "../model/Appointment.h"

class QComboBox;
class QDateEdit;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;
class QTimeEdit;

namespace scapms::controller {
class ClinicManager;
}

namespace scapms::gui {

class AppointmentPanel : public QWidget {
    Q_OBJECT

public:
    explicit AppointmentPanel(controller::ClinicManager* manager,
                              QWidget* parent = nullptr);

    void refresh();
    void focusScheduling();

signals:
    void dataChanged();
    void statusMessage(const QString& message);

private slots:
    void onSchedule();
    void onComplete();
    void onCancel();
    void onRemove();
    void onPatientChanged(int index);
    void onServiceChanged(int index);
    void onStatusFilterChanged(int index);

private:
    QWidget* createSchedulingForm();
    void populatePatients();
    void populateServices();
    void populateTable();
    void updateEligibility();
    [[nodiscard]] QString selectedAppointmentId() const;

    controller::ClinicManager* m_manager = nullptr;

    QComboBox* m_patientCombo = nullptr;
    QDateEdit* m_dateEdit = nullptr;
    QTimeEdit* m_timeEdit = nullptr;
    QComboBox* m_serviceCombo = nullptr;
    QLineEdit* m_clinicianEdit = nullptr;
    QLineEdit* m_notesEdit = nullptr;
    QLabel* m_eligibilityLabel = nullptr;
    QLabel* m_feePreviewLabel = nullptr;

    QComboBox* m_statusFilter = nullptr;
    QTableWidget* m_table = nullptr;
    QLabel* m_infoLabel = nullptr;
    QPushButton* m_completeButton = nullptr;
    QPushButton* m_cancelButton = nullptr;
    QPushButton* m_removeButton = nullptr;

    std::vector<model::Appointment> m_display;
};

} // namespace scapms::gui

#endif // SCAPMS_GUI_APPOINTMENTPANEL_H
