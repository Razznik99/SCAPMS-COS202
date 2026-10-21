// =============================================================================
//  SCAPMS - Smart Clinic Appointment & Patient Management System
//  File   : src/gui/QueuePanel.h
//  Layer  : GUI
//  Purpose: Waiting Queue tab - shows the std::queue of waiting patients, lets
//           staff add patients, call the next patient (emergency first, then
//           FIFO) and remove entries. A QTimer refreshes the waiting times and
//           raises a long-wait alert.
// =============================================================================
#ifndef SCAPMS_GUI_QUEUEPANEL_H
#define SCAPMS_GUI_QUEUEPANEL_H

#include <QWidget>

class QComboBox;
class QLabel;
class QPushButton;
class QTableWidget;
class QTimer;

namespace scapms::controller {
class ClinicManager;
}

namespace scapms::gui {

class QueuePanel : public QWidget {
    Q_OBJECT

public:
    explicit QueuePanel(controller::ClinicManager* manager, QWidget* parent = nullptr);

    void refresh();

signals:
    void dataChanged();
    void statusMessage(const QString& message);

private slots:
    void onAddToQueue();
    void onCallNext();
    void onRemoveSelected();
    void onTick();

private:
    QWidget* createQueueBox();
    QWidget* createAddBox();
    void populateCandidates();
    void populateQueue();
    [[nodiscard]] int waitingMinutes(const QString& joinedAt) const;
    [[nodiscard]] QString selectedPatientId() const;

    controller::ClinicManager* m_manager = nullptr;
    QTableWidget* m_queueTable = nullptr;
    QComboBox* m_patientCombo = nullptr;
    QLabel* m_statsLabel = nullptr;
    QLabel* m_alertLabel = nullptr;
    QPushButton* m_callNextButton = nullptr;
    QTimer* m_timer = nullptr;
};

} // namespace scapms::gui

#endif // SCAPMS_GUI_QUEUEPANEL_H
