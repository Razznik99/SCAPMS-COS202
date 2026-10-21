// =============================================================================
//  File   : src/gui/SearchPanel.cpp
// =============================================================================
#include "SearchPanel.h"

#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QStringList>
#include <QTableWidget>
#include <QVBoxLayout>

#include "../controller/ClinicManager.h"
#include "../utils/Exceptions.h"
#include "../utils/StringUtils.h"
#include "PillDelegate.h"
#include "UiHelpers.h"

namespace scapms::gui {

using scapms::controller::ClinicManager;
using scapms::controller::SearchMode;
using scapms::controller::SortAlgorithm;
using scapms::controller::SortEngine;
using scapms::controller::SortField;

SearchPanel::SearchPanel(ClinicManager* manager, QWidget* parent)
    : QWidget(parent), m_manager(manager) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(10);

    auto* header = new QHBoxLayout();
    header->addWidget(createSectionTitle("Search & Sort", this));
    header->addStretch(1);
    auto* resetViewButton = new QPushButton("Reset to register order", this);
    resetViewButton->setObjectName("secondaryButton");
    resetViewButton->setToolTip("Restore the original registration order.");
    header->addWidget(resetViewButton);
    root->addLayout(header);

    auto* controls = new QHBoxLayout();
    controls->setSpacing(10);
    controls->addWidget(createSearchBox(), 1);
    controls->addWidget(createSortBox(), 1);
    root->addLayout(controls);

    m_strategyLabel = createHintLabel("", this);
    root->addWidget(m_strategyLabel);

    m_table = new QTableWidget(0, 7, this);
    m_table->setHorizontalHeaderLabels({"Patient ID", "Name", "Age", "Category", "Priority",
                                        "Contact", "Next Appointment"});
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_table->setItemDelegateForColumn(3, new PillDelegate(PillDelegate::Kind::Category, this));
    root->addWidget(m_table, 1);

    m_metricsLabel = new QLabel(this);
    m_metricsLabel->setObjectName("bannerLabel");
    m_metricsLabel->setWordWrap(true);
    m_metricsLabel->setText(
        "Run a search or apply a sort to see the algorithm statistics here.");
    root->addWidget(m_metricsLabel);

    m_rationaleLabel = new QLabel(this);
    m_rationaleLabel->setObjectName("hintLabel");
    m_rationaleLabel->setWordWrap(true);
    root->addWidget(m_rationaleLabel);

    m_resultLabel = createHintLabel("", this);
    root->addWidget(m_resultLabel);

    connect(resetViewButton, &QPushButton::clicked, this, &SearchPanel::onResetView);
    connect(m_modeCombo, &QComboBox::currentIndexChanged, this,
            &SearchPanel::onModeChanged);
    connect(m_fieldCombo, &QComboBox::currentIndexChanged, this,
            &SearchPanel::onSortFieldChanged);
    connect(m_algorithmCombo, &QComboBox::currentIndexChanged, this,
            &SearchPanel::onSortAlgorithmChanged);

    m_sorted = false;
    refresh();
    updateStrategyText();
    updateSortRationale();
}

QWidget* SearchPanel::createSearchBox() {
    auto* box = new QGroupBox("Search patient records", this);
    auto* layout = new QVBoxLayout(box);

    auto* form = new QFormLayout();
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    m_modeCombo = new QComboBox(box);
    m_modeCombo->addItem("Linear search - Patient ID",
                         static_cast<int>(SearchMode::LinearById));
    m_modeCombo->addItem("Binary search - Patient Name (sorted)",
                         static_cast<int>(SearchMode::BinaryByName));
    m_modeCombo->addItem("Recursive search - Patient Name",
                         static_cast<int>(SearchMode::RecursiveByName));
    m_modeCombo->addItem("Category filter", static_cast<int>(SearchMode::CategoryFilter));
    m_modeCombo->addItem("Automatic (data-aware)", static_cast<int>(SearchMode::Auto));
    m_modeCombo->setToolTip("Choose the searching algorithm to apply.");
    form->addRow("Search algorithm", m_modeCombo);

    m_queryStack = new QStackedWidget(box);

    auto* queryPage = new QWidget(m_queryStack);
    auto* queryLayout = new QVBoxLayout(queryPage);
    queryLayout->setContentsMargins(0, 0, 0, 0);
    m_queryEdit = new QLineEdit(queryPage);
    m_queryEdit->setPlaceholderText("e.g. P-0004 or a name fragment");
    m_queryEdit->setClearButtonEnabled(true);
    m_queryEdit->setToolTip("Press Enter or click Search.");
    queryLayout->addWidget(m_queryEdit);
    m_queryStack->addWidget(queryPage);

    auto* categoryPage = new QWidget(m_queryStack);
    auto* categoryLayout = new QVBoxLayout(categoryPage);
    categoryLayout->setContentsMargins(0, 0, 0, 0);
    m_categoryCombo = new QComboBox(categoryPage);
    m_categoryCombo->addItems({"General", "Emergency", "Follow-Up"});
    categoryLayout->addWidget(m_categoryCombo);
    m_queryStack->addWidget(categoryPage);

    form->addRow("Search for", m_queryStack);
    layout->addLayout(form);

    auto* buttons = new QHBoxLayout();
    auto* searchButton = new QPushButton("Search", box);
    searchButton->setToolTip("Run the selected search algorithm.");
    auto* clearButton = new QPushButton("Clear results", box);
    clearButton->setObjectName("secondaryButton");
    buttons->addWidget(searchButton);
    buttons->addWidget(clearButton);
    buttons->addStretch(1);
    layout->addLayout(buttons);

    connect(searchButton, &QPushButton::clicked, this, &SearchPanel::onSearch);
    connect(clearButton, &QPushButton::clicked, this, &SearchPanel::onResetView);
    connect(m_queryEdit, &QLineEdit::returnPressed, this, &SearchPanel::onSearch);
    return box;
}

QWidget* SearchPanel::createSortBox() {
    auto* box = new QGroupBox("Sort patient records", this);
    auto* layout = new QVBoxLayout(box);

    auto* form = new QFormLayout();
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    m_fieldCombo = new QComboBox(box);
    m_fieldCombo->addItem("Patient Name", static_cast<int>(SortField::PatientName));
    m_fieldCombo->addItem("Patient ID", static_cast<int>(SortField::PatientId));
    m_fieldCombo->addItem("Age", static_cast<int>(SortField::Age));
    m_fieldCombo->addItem("Appointment Date",
                          static_cast<int>(SortField::AppointmentDate));
    m_fieldCombo->setToolTip("The field the records are ordered by.");
    form->addRow("Sort by", m_fieldCombo);

    m_algorithmCombo = new QComboBox(box);
    m_algorithmCombo->addItem("Selection Sort",
                              static_cast<int>(SortAlgorithm::Selection));
    m_algorithmCombo->addItem("Insertion Sort",
                              static_cast<int>(SortAlgorithm::Insertion));
    m_algorithmCombo->addItem("Merge Sort", static_cast<int>(SortAlgorithm::Merge));
    m_algorithmCombo->addItem("Quick Sort", static_cast<int>(SortAlgorithm::Quick));
    m_algorithmCombo->setToolTip("All four algorithms are implemented by hand in "
                                 "SortEngine.cpp.");
    form->addRow("Sort algorithm", m_algorithmCombo);
    layout->addLayout(form);

    auto* sortButton = new QPushButton("SORT", box);
    sortButton->setToolTip("Apply the selected sort to the working set below.");
    layout->addWidget(sortButton);
    layout->addStretch(1);

    connect(sortButton, &QPushButton::clicked, this, &SearchPanel::onSort);
    return box;
}

SearchMode SearchPanel::currentMode() const {
    return static_cast<SearchMode>(m_modeCombo->currentData().toInt());
}

SortField SearchPanel::currentField() const {
    return static_cast<SortField>(m_fieldCombo->currentData().toInt());
}

SortAlgorithm SearchPanel::currentAlgorithm() const {
    return static_cast<SortAlgorithm>(m_algorithmCombo->currentData().toInt());
}

void SearchPanel::onModeChanged(int /*index*/) {
    const bool categoryMode = currentMode() == SearchMode::CategoryFilter;
    m_queryStack->setCurrentIndex(categoryMode ? 1 : 0);
    updateStrategyText();
}

void SearchPanel::updateStrategyText() {
    m_strategyLabel->setText(
        toQt(scapms::controller::SearchEngine::strategyExplanation(
            currentMode(), m_records.size())));
}

void SearchPanel::updateSortRationale() {
    m_rationaleLabel->setText(
        toQt(SortEngine::rationale(currentField(), currentAlgorithm(), m_records.size())));
}

void SearchPanel::onSortFieldChanged(int /*index*/) {
    updateSortRationale();
}

void SearchPanel::onSortAlgorithmChanged(int /*index*/) {
    updateSortRationale();
}

void SearchPanel::onSearch() {
    QString query;
    if (currentMode() == SearchMode::CategoryFilter) {
        query = m_categoryCombo->currentText();
    } else {
        query = m_queryEdit->text().trimmed();
        if (query.isEmpty()) {
            QMessageBox::information(this, "Search value required",
                                     "Enter a patient ID or name (or a name fragment).");
            m_queryEdit->setFocus();
            return;
        }
    }

    try {
        const auto result = m_manager->searchIn(m_records, currentMode(),
                                                query.toStdString());
        populateTable(result.matches);
        m_metricsLabel->setText(
            QString("<b>%1</b> &middot; %2 comparison(s) &middot; %3 &micro;s &middot; "
                    "%4 match(es) out of %5 records")
                .arg(toQt(result.algorithm))
                .arg(result.stats.comparisons)
                .arg(result.stats.micros)
                .arg(result.matches.size())
                .arg(m_records.size()));
        m_resultLabel->setText(toQt(result.note));
        emit statusMessage(toQt(result.algorithm) + ": " +
                           QString::number(result.matches.size()) + " match(es).");
    } catch (const scapms::utils::ValidationException& error) {
        QMessageBox::warning(this, "Invalid search", error.what());
        m_resultLabel->setText(toQt(error.what()));
    } catch (const scapms::utils::ClinicException& error) {
        QMessageBox::warning(this, "Search failed", error.what());
    } catch (const std::exception& error) {
        QMessageBox::critical(this, "Unexpected error", error.what());
    }
}

void SearchPanel::onSort() {
    try {
        const auto outcome = m_manager->sortPatients(m_records, currentField(),
                                                      currentAlgorithm());
        m_sorted = true;
        m_appliedField = outcome.field;
        m_appliedAlgorithm = outcome.algorithm;
        populateTable(m_records);
        m_metricsLabel->setText(
            QString("<b>%1</b> by <b>%2</b> &middot; %3 record(s) &middot; %4 "
                    "comparison(s) &middot; %5 move(s) &middot; %6 &micro;s")
                .arg(toQt(outcome.algorithmName), toQt(outcome.fieldName))
                .arg(outcome.itemCount)
                .arg(outcome.stats.comparisons)
                .arg(outcome.stats.moves)
                .arg(outcome.stats.micros));
        m_resultLabel->setText(
            QString("Sorted %1 records by %2 using %3.")
                .arg(outcome.itemCount)
                .arg(toQt(outcome.fieldName), toQt(outcome.algorithmName)));
        emit statusMessage("Sorted by " + toQt(outcome.fieldName) + " using " +
                           toQt(outcome.algorithmName) + ".");
    } catch (const std::exception& error) {
        QMessageBox::critical(this, "Sort failed", error.what());
    }
}

void SearchPanel::onResetView() {
    m_sorted = false;
    refresh();
    m_metricsLabel->setText("Working set restored to registration order.");
    m_resultLabel->clear();
}

void SearchPanel::populateTable(const std::vector<model::PatientRecord*>& records) {
    m_table->setUpdatesEnabled(false);
    m_table->setRowCount(0);
    for (const auto* record : records) {
        if (record == nullptr) {
            continue;
        }
        const int row = m_table->rowCount();
        m_table->insertRow(row);
        const QString next = toQt(m_manager->earliestPendingAppointmentDate(record->id()));
        const QStringList values{
            toQt(record->id()),
            toQt(record->name()),
            QString::number(record->age()),
            toQt(record->category()),
            toQt(record->priorityLabel()),
            toQt(record->contact()),
            next.isEmpty() ? QString("-") : next};

        for (int column = 0; column < values.size(); ++column) {
            auto* item = new QTableWidgetItem(values.at(column));
            if (column == 2) {
                item->setTextAlignment((Qt::AlignRight | Qt::AlignVCenter));
            }
            m_table->setItem(row, column, item);
        }
        m_table->setRowHeight(row, 32);
    }
    m_table->setUpdatesEnabled(true);
}

void SearchPanel::refresh() {
    m_records = m_manager->patients();
    if (m_sorted) {
        (void)m_manager->sortPatients(m_records, m_appliedField, m_appliedAlgorithm);
    }
    populateTable(m_records);
    updateStrategyText();
    updateSortRationale();
}

void SearchPanel::focusSearch() {
    if (currentMode() == SearchMode::CategoryFilter) {
        m_categoryCombo->setFocus();
    } else {
        m_queryEdit->setFocus();
        m_queryEdit->selectAll();
    }
}

void SearchPanel::applySort(SortField field, SortAlgorithm algorithm) {
    const int fieldIndex = m_fieldCombo->findData(static_cast<int>(field));
    if (fieldIndex >= 0) {
        m_fieldCombo->setCurrentIndex(fieldIndex);
    }
    const int algorithmIndex = m_algorithmCombo->findData(static_cast<int>(algorithm));
    if (algorithmIndex >= 0) {
        m_algorithmCombo->setCurrentIndex(algorithmIndex);
    }
    onSort();
}

void SearchPanel::applySearch(SearchMode mode, const QString& query) {
    const int modeIndex = m_modeCombo->findData(static_cast<int>(mode));
    if (modeIndex >= 0) {
        m_modeCombo->setCurrentIndex(modeIndex);
    }
    if (mode == SearchMode::CategoryFilter) {
        const int categoryIndex = m_categoryCombo->findText(query);
        if (categoryIndex >= 0) {
            m_categoryCombo->setCurrentIndex(categoryIndex);
        }
    } else {
        m_queryEdit->setText(query);
    }
    onSearch();
}

} // namespace scapms::gui
