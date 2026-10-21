// =============================================================================
//  SCAPMS - Smart Clinic Appointment & Patient Management System
//  File   : src/gui/PatientPanel.h
//  Layer  : GUI
//  Purpose: Patient Records tab - table of patients plus a registration/editing
//           form whose fields change dynamically with the selected patient
//           type (General / Emergency / Follow-Up).
// =============================================================================
#ifndef SCAPMS_GUI_PATIENTPANEL_H
#define SCAPMS_GUI_PATIENTPANEL_H

#include <QWidget>

#include <memory>

#include "../model/PatientRecord.h"

class QCheckBox;
class QComboBox;
class QDateEdit;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QStackedWidget;
class QTableWidget;

namespace scapms::controller {
class ClinicManager;
}

namespace scapms::gui {

class PatientPanel : public QWidget {
    Q_OBJECT

public:
    explicit PatientPanel(controller::ClinicManager* manager, QWidget* parent = nullptr);

    void refresh();
    void focusSearch();
    void prepareNewPatient();

signals:
    void dataChanged();
    void statusMessage(const QString& message);

private slots:
    void onAddPatient();
    void onUpdatePatient();
    void onDeletePatient();
    void onClearForm();
    void onTypeChanged(int index);
    void onFilterChanged(const QString& text);
    void onSelectionChanged();
    void onRowActivated(int row, int column);

private:
    QWidget* createForm();
    QWidget* createActionBar();
    QWidget* createGeneralPage();
    QWidget* createEmergencyPage();
    QWidget* createFollowUpPage();
    void populateTable();
    void loadPatientIntoForm(const QString& patientId);
    void updatePreview();
    void updateIdFieldStyle();
    [[nodiscard]] std::unique_ptr<model::PatientRecord> buildPatientFromForm(
        const QString& patientId) const;
    [[nodiscard]] QString selectedPatientId() const;

    controller::ClinicManager* m_manager = nullptr;

    QLineEdit* m_filterEdit = nullptr;
    QLabel* m_tableInfoLabel = nullptr;
    QTableWidget* m_table = nullptr;

    QLineEdit* m_idEdit = nullptr;
    QLineEdit* m_nameEdit = nullptr;
    QSpinBox* m_ageSpin = nullptr;
    QComboBox* m_genderCombo = nullptr;
    QLineEdit* m_contactEdit = nullptr;
    QComboBox* m_typeCombo = nullptr;
    QDateEdit* m_registeredEdit = nullptr;
    QStackedWidget* m_extraStack = nullptr;

    QComboBox* m_insuranceCombo = nullptr;
    QLineEdit* m_chronicEdit = nullptr;

    QSpinBox* m_triageSpin = nullptr;
    QComboBox* m_arrivalCombo = nullptr;
    QCheckBox* m_admittedCheck = nullptr;

    QDateEdit* m_originalVisitEdit = nullptr;
    QSpinBox* m_intervalSpin = nullptr;
    QLineEdit* m_doctorEdit = nullptr;
    QLabel* m_dueLabel = nullptr;

    QLabel* m_previewLabel = nullptr;
    QPushButton* m_updateButton = nullptr;
    QPushButton* m_deleteButton = nullptr;

    QString m_editingPatientId;
};

} // namespace scapms::gui

#endif // SCAPMS_GUI_PATIENTPANEL_H
