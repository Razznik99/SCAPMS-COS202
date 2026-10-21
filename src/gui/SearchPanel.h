// =============================================================================
//  SCAPMS - Smart Clinic Appointment & Patient Management System
//  File   : src/gui/SearchPanel.h
//  Layer  : GUI
//  Purpose: Search & Sort tab. The user picks the search technique and the sort
//           algorithm/field from combo boxes; the panel reports comparisons and
//           elapsed time so the cost of each algorithm is visible.
// =============================================================================
#ifndef SCAPMS_GUI_SEARCHPANEL_H
#define SCAPMS_GUI_SEARCHPANEL_H

#include <QWidget>

#include <vector>

#include "../controller/SearchEngine.h"
#include "../controller/SortEngine.h"

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QStackedWidget;
class QTableWidget;

namespace scapms::controller {
class ClinicManager;
}

namespace scapms::gui {

class SearchPanel : public QWidget {
    Q_OBJECT

public:
    explicit SearchPanel(controller::ClinicManager* manager, QWidget* parent = nullptr);

    void refresh();
    void focusSearch();

    /// Applies a sort exactly as the SORT button would (also used to stage the
    /// documentation screenshots).
    void applySort(controller::SortField field, controller::SortAlgorithm algorithm);

    /// Runs a search exactly as the Search button would.
    void applySearch(controller::SearchMode mode, const QString& query);

signals:
    void statusMessage(const QString& message);

private slots:
    void onSearch();
    void onSort();
    void onResetView();
    void onModeChanged(int index);
    void onSortFieldChanged(int index);
    void onSortAlgorithmChanged(int index);

private:
    QWidget* createSearchBox();
    QWidget* createSortBox();
    void populateTable(const std::vector<model::PatientRecord*>& records);
    void updateStrategyText();
    void updateSortRationale();
    [[nodiscard]] controller::SearchMode currentMode() const;
    [[nodiscard]] controller::SortField currentField() const;
    [[nodiscard]] controller::SortAlgorithm currentAlgorithm() const;

    controller::ClinicManager* m_manager = nullptr;
    std::vector<model::PatientRecord*> m_records;

    QComboBox* m_modeCombo = nullptr;
    QStackedWidget* m_queryStack = nullptr;
    QLineEdit* m_queryEdit = nullptr;
    QComboBox* m_categoryCombo = nullptr;

    QComboBox* m_fieldCombo = nullptr;
    QComboBox* m_algorithmCombo = nullptr;

    QTableWidget* m_table = nullptr;
    QLabel* m_strategyLabel = nullptr;
    QLabel* m_rationaleLabel = nullptr;
    QLabel* m_resultLabel = nullptr;
    QLabel* m_metricsLabel = nullptr;

    bool m_sorted = false;
    controller::SortField m_appliedField = controller::SortField::PatientName;
    controller::SortAlgorithm m_appliedAlgorithm = controller::SortAlgorithm::Selection;
};

} // namespace scapms::gui

#endif // SCAPMS_GUI_SEARCHPANEL_H
