// =============================================================================
//  SCAPMS - Smart Clinic Appointment & Patient Management System
//  File   : src/gui/AdminPanel.h
//  Layer  : GUI
//  Purpose: Administration tab - the undo stack viewer (std::stack), the
//           persistence controls, the frequently-accessed-patient cache
//           (fixed-size array) and a runtime-polymorphism demonstration.
// =============================================================================
#ifndef SCAPMS_GUI_ADMINPANEL_H
#define SCAPMS_GUI_ADMINPANEL_H

#include <QWidget>

class QLabel;
class QListWidget;
class QPushButton;

namespace scapms::controller {
class ClinicManager;
}

namespace scapms::gui {

class AdminPanel : public QWidget {
    Q_OBJECT

public:
    explicit AdminPanel(controller::ClinicManager* manager, QWidget* parent = nullptr);

    void refresh();

signals:
    void dataChanged();
    void statusMessage(const QString& message);

private slots:
    void onUndo();
    void onSave();
    void onLoad();
    void onNewSession();
    void onImportPatients();
    void onExportPatients();
    void onShowPolymorphism();

private:
    QWidget* createUndoBox();
    QWidget* createDataBox();
    QWidget* createCacheBox();
    QWidget* createPolymorphismBox();
    void updateUndoList();
    void updateCacheList();

    controller::ClinicManager* m_manager = nullptr;

    QListWidget* m_undoList = nullptr;
    QLabel* m_undoHint = nullptr;
    QPushButton* m_undoButton = nullptr;

    QLabel* m_dataPathLabel = nullptr;
    QListWidget* m_cacheList = nullptr;
    QLabel* m_cacheHint = nullptr;
};

} // namespace scapms::gui

#endif // SCAPMS_GUI_ADMINPANEL_H
