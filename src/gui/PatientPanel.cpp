// =============================================================================
//  File   : src/gui/PatientPanel.cpp
// =============================================================================
#include "PatientPanel.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QStringList>
#include <QTableWidget>
#include <QVBoxLayout>

#include <algorithm>

#include "../controller/ClinicManager.h"
#include "../model/EmergencyPatient.h"
#include "../model/FollowUpPatient.h"
#include "../model/GeneralPatient.h"
#include "../utils/DateUtils.h"
#include "../utils/Exceptions.h"
#include "../utils/StringUtils.h"
#include "PillDelegate.h"
#include "UiHelpers.h"

namespace scapms::gui {

using scapms::controller::ClinicManager;

PatientPanel::PatientPanel(ClinicManager* manager, QWidget* parent)
    : QWidget(parent), m_manager(manager) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(10);

    auto* header = new QHBoxLayout();
    header->addWidget(createSectionTitle("Patient Records", this));
    header->addStretch(1);

    m_filterEdit = new QLineEdit(this);
    m_filterEdit->setPlaceholderText("Filter by ID, name, category or contact...");
    m_filterEdit->setClearButtonEnabled(true);
    m_filterEdit->setFixedWidth(320);
    m_filterEdit->setToolTip("Live filter over the patient register (text-field event).");
    header->addWidget(m_filterEdit);

    auto* newButton = new QPushButton("New Patient", this);
    newButton->setObjectName("secondaryButton");
    newButton->setToolTip("Clear the form and generate a fresh patient ID.");
    header->addWidget(newButton);

    root->addLayout(header);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setChildrenCollapsible(false);

    // ---- table -------------------------------------------------------------
    auto* tableBox = new QWidget(splitter);
    auto* tableLayout = new QVBoxLayout(tableBox);
    tableLayout->setContentsMargins(0, 0, 0, 0);
    tableLayout->setSpacing(6);

    m_table = new QTableWidget(0, 8, tableBox);
    m_table->setHorizontalHeaderLabels({"Patient ID", "Name", "Age", "Gender", "Contact",
                                        "Category", "Priority", "Fee"});
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(7, QHeaderView::ResizeToContents);
    m_table->setItemDelegateForColumn(5, new PillDelegate(PillDelegate::Kind::Category, this));
    m_table->setToolTip("Double-click a row to load it into the form for editing.");
    tableLayout->addWidget(m_table, 1);

    m_tableInfoLabel = createHintLabel("", tableBox);
    tableLayout->addWidget(m_tableInfoLabel);

    // ---- form + action bar -------------------------------------------------
    auto* rightPane = new QWidget(splitter);
    auto* rightLayout = new QVBoxLayout(rightPane);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(8);

    auto* formScroll = new QScrollArea(rightPane);
    formScroll->setWidgetResizable(true);
    formScroll->setFrameShape(QFrame::NoFrame);
    formScroll->setWidget(createForm());
    rightLayout->addWidget(formScroll, 1);
    rightLayout->addWidget(createActionBar());

    splitter->addWidget(tableBox);
    splitter->addWidget(rightPane);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    splitter->setSizes({760, 440});
    root->addWidget(splitter, 1);

    connect(newButton, &QPushButton::clicked, this, &PatientPanel::onClearForm);
    connect(m_filterEdit, &QLineEdit::textChanged, this, &PatientPanel::onFilterChanged);
    connect(m_table, &QTableWidget::itemSelectionChanged, this,
            &PatientPanel::onSelectionChanged);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &PatientPanel::onRowActivated);
    connect(m_typeCombo, &QComboBox::currentIndexChanged, this,
            &PatientPanel::onTypeChanged);

    refresh();
    onClearForm();
}

QWidget* PatientPanel::createForm() {
    auto* box = new QGroupBox("Patient details", this);
    auto* layout = new QVBoxLayout(box);
    layout->setSpacing(8);

    auto* form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignLeft);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->setSpacing(8);

    m_idEdit = new QLineEdit(box);
    m_idEdit->setPlaceholderText("auto-generated, e.g. P-0015");
    m_idEdit->setToolTip("Leave blank to let the system allocate the next patient ID.");
    form->addRow("Patient ID *", m_idEdit);

    m_nameEdit = new QLineEdit(box);
    m_nameEdit->setPlaceholderText("Surname First-name");
    form->addRow("Full name *", m_nameEdit);

    m_ageSpin = new QSpinBox(box);
    m_ageSpin->setRange(0, 120);
    m_ageSpin->setValue(30);
    m_ageSpin->setSuffix(" years");
    m_ageSpin->setToolTip("Age must be between 0 and 120 (validated again by the model).");
    form->addRow("Age *", m_ageSpin);

    m_genderCombo = new QComboBox(box);
    m_genderCombo->addItems({"Female", "Male", "Other"});
    form->addRow("Gender *", m_genderCombo);

    m_contactEdit = new QLineEdit(box);
    m_contactEdit->setPlaceholderText("+234 803 555 0100");
    m_contactEdit->setToolTip("7 to 15 digits; spaces, + - ( ) / are allowed.");
    form->addRow("Contact *", m_contactEdit);

    m_typeCombo = new QComboBox(box);
    m_typeCombo->addItems({"General Patient", "Emergency Patient", "Follow-Up Patient"});
    m_typeCombo->setToolTip("Changing the type swaps the extra fields below "
                            "(dynamic components).");
    form->addRow("Patient type *", m_typeCombo);

    m_registeredEdit = new QDateEdit(QDate::currentDate(), box);
    m_registeredEdit->setCalendarPopup(true);
    m_registeredEdit->setDisplayFormat("yyyy-MM-dd");
    m_registeredEdit->setMaximumDate(QDate::currentDate());
    form->addRow("Registered on", m_registeredEdit);

    layout->addLayout(form);

    m_extraStack = new QStackedWidget(box);
    m_extraStack->addWidget(createGeneralPage());
    m_extraStack->addWidget(createEmergencyPage());
    m_extraStack->addWidget(createFollowUpPage());
    layout->addWidget(m_extraStack);

    m_previewLabel = new QLabel(box);
    m_previewLabel->setObjectName("bannerLabel");
    m_previewLabel->setWordWrap(true);
    layout->addWidget(m_previewLabel);
    layout->addStretch(1);

    connect(m_idEdit, &QLineEdit::textChanged, this, &PatientPanel::updateIdFieldStyle);
    connect(m_nameEdit, &QLineEdit::textChanged, this, &PatientPanel::updatePreview);
    connect(m_contactEdit, &QLineEdit::textChanged, this, &PatientPanel::updatePreview);
    connect(m_ageSpin, &QSpinBox::valueChanged, this, &PatientPanel::updatePreview);
    connect(m_genderCombo, &QComboBox::currentIndexChanged, this,
            &PatientPanel::updatePreview);

    return box;
}

QWidget* PatientPanel::createActionBar() {
    auto* bar = new QWidget(this);
    auto* buttons = new QHBoxLayout(bar);
    buttons->setContentsMargins(0, 0, 0, 0);

    auto* addButton = new QPushButton("Add Patient", bar);
    addButton->setToolTip("Register the patient in the clinic database.");
    m_updateButton = new QPushButton("Update", bar);
    m_updateButton->setObjectName("secondaryButton");
    m_updateButton->setToolTip("Save changes to the patient loaded in the form.");
    m_deleteButton = new QPushButton("Delete", bar);
    m_deleteButton->setObjectName("dangerButton");
    m_deleteButton->setToolTip("Delete the selected patient (undoable with Ctrl+Z).");
    auto* clearButton = new QPushButton("Clear", bar);
    clearButton->setObjectName("secondaryButton");

    buttons->addWidget(addButton);
    buttons->addWidget(m_updateButton);
    buttons->addWidget(m_deleteButton);
    buttons->addStretch(1);
    buttons->addWidget(clearButton);

    connect(addButton, &QPushButton::clicked, this, &PatientPanel::onAddPatient);
    connect(m_updateButton, &QPushButton::clicked, this, &PatientPanel::onUpdatePatient);
    connect(m_deleteButton, &QPushButton::clicked, this, &PatientPanel::onDeletePatient);
    connect(clearButton, &QPushButton::clicked, this, &PatientPanel::onClearForm);

    m_updateButton->setEnabled(false);
    m_deleteButton->setEnabled(false);
    return bar;
}

QWidget* PatientPanel::createGeneralPage() {
    auto* page = new QWidget(this);
    auto* form = new QFormLayout(page);
    form->setContentsMargins(0, 4, 0, 0);

    m_insuranceCombo = new QComboBox(page);
    m_insuranceCombo->setEditable(true);
    m_insuranceCombo->addItems({"Self-Pay", "NHIS", "AXA Mansard", "Reliance HMO",
                                "Hygeia HMO"});
    m_insuranceCombo->setToolTip("Insurance provider or Self-Pay when uninsured.");
    form->addRow("Insurance", m_insuranceCombo);

    m_chronicEdit = new QLineEdit("None", page);
    m_chronicEdit->setToolTip("Known chronic condition, or None.");
    form->addRow("Chronic condition", m_chronicEdit);

    auto* hint = createHintLabel("Routine outpatient - 20 minute slot - standard "
                                 "consultation fee.",
                                 page);
    form->addRow("", hint);
    return page;
}

QWidget* PatientPanel::createEmergencyPage() {
    auto* page = new QWidget(this);
    auto* form = new QFormLayout(page);
    form->setContentsMargins(0, 4, 0, 0);

    m_triageSpin = new QSpinBox(page);
    m_triageSpin->setRange(1, 5);
    m_triageSpin->setValue(3);
    m_triageSpin->setPrefix("Level ");
    m_triageSpin->setToolTip("1 = immediate resuscitation, 5 = non-urgent.");
    form->addRow("Triage level", m_triageSpin);

    m_arrivalCombo = new QComboBox(page);
    m_arrivalCombo->addItems({"Ambulance", "Walk-in", "Referral"});
    form->addRow("Arrival mode", m_arrivalCombo);

    m_admittedCheck = new QCheckBox("Admitted to the clinic ward", page);
    form->addRow("", m_admittedCheck);

    auto* hint = createHintLabel("Emergency patients are seen immediately and are "
                                 "walk-in only, so they cannot be booked into the "
                                 "appointment calendar. They can join the priority "
                                 "queue.",
                                 page);
    hint->setObjectName("warningLabel");
    form->addRow("", hint);
    return page;
}

QWidget* PatientPanel::createFollowUpPage() {
    auto* page = new QWidget(this);
    auto* form = new QFormLayout(page);
    form->setContentsMargins(0, 4, 0, 0);

    m_originalVisitEdit = new QDateEdit(QDate::currentDate().addDays(-14), page);
    m_originalVisitEdit->setCalendarPopup(true);
    m_originalVisitEdit->setDisplayFormat("yyyy-MM-dd");
    m_originalVisitEdit->setMaximumDate(QDate::currentDate());
    form->addRow("Original visit", m_originalVisitEdit);

    m_intervalSpin = new QSpinBox(page);
    m_intervalSpin->setRange(1, 365);
    m_intervalSpin->setValue(14);
    m_intervalSpin->setSuffix(" days");
    form->addRow("Follow-up in", m_intervalSpin);

    m_doctorEdit = new QLineEdit("Dr. Adeyemi", page);
    form->addRow("Referring doctor", m_doctorEdit);

    m_dueLabel = createHintLabel("Review due: -", page);
    form->addRow("", m_dueLabel);

    connect(m_originalVisitEdit, &QDateEdit::dateChanged, this, &PatientPanel::updatePreview);
    connect(m_intervalSpin, &QSpinBox::valueChanged, this, &PatientPanel::updatePreview);
    connect(m_doctorEdit, &QLineEdit::textChanged, this, &PatientPanel::updatePreview);
    return page;
}

// =============================================================================
//  Table
// =============================================================================
void PatientPanel::populateTable() {
    const QString selectedId = selectedPatientId();
    const QString filter = m_filterEdit->text().trimmed();
    const auto patients = m_manager->patients();

    int shown = 0;
    m_table->setUpdatesEnabled(false);
    m_table->setRowCount(0);

    for (const auto* record : patients) {
        if (record == nullptr) {
            continue;
        }
        const QStringList haystack{
            toQt(record->id()), toQt(record->name()), toQt(record->category()),
            toQt(record->contact()), toQt(record->priorityLabel())};
        bool matches = filter.isEmpty();
        for (const auto& field : haystack) {
            if (field.contains(filter, Qt::CaseInsensitive)) {
                matches = true;
                break;
            }
        }
        if (!matches) {
            continue;
        }

        const int row = m_table->rowCount();
        m_table->insertRow(row);
        const QStringList values{
            toQt(record->id()),
            toQt(record->name()),
            QString::number(record->age()),
            toQt(record->gender()),
            toQt(record->contact()),
            toQt(record->category()),
            toQt(record->priorityLabel()),
            QString::fromStdString(scapms::utils::formatMoney(record->consultationFee()))};

        for (int column = 0; column < values.size(); ++column) {
            auto* item = new QTableWidgetItem(values.at(column));
            if (column != 5) {
                const auto alignment = (column == 2 || column == 7)
                                           ? (Qt::AlignRight | Qt::AlignVCenter)
                                           : (Qt::AlignLeft | Qt::AlignVCenter);
                item->setTextAlignment(alignment);
            }
            m_table->setItem(row, column, item);
        }
        m_table->setRowHeight(row, 34);

        if (toQt(record->id()) == selectedId) {
            m_table->selectRow(row);
        }
        ++shown;
    }

    m_table->setUpdatesEnabled(true);
    m_tableInfoLabel->setText(
        QString("Showing %1 of %2 registered patients.  Double-click a row to edit it.")
            .arg(shown)
            .arg(patients.size()));
}

QString PatientPanel::selectedPatientId() const {
    const auto rows = m_table->selectionModel() ? m_table->selectionModel()->selectedRows()
                                                : QModelIndexList{};
    if (rows.isEmpty()) {
        return {};
    }
    const auto* item = m_table->item(rows.first().row(), 0);
    return item ? item->text() : QString();
}

void PatientPanel::onSelectionChanged() {
    const QString id = selectedPatientId();
    const bool hasSelection = !id.isEmpty();
    m_updateButton->setEnabled(hasSelection);
    m_deleteButton->setEnabled(hasSelection);
    if (hasSelection && id != m_editingPatientId) {
        loadPatientIntoForm(id);
    }
}

void PatientPanel::onRowActivated(int row, int /*column*/) {
    const auto* item = m_table->item(row, 0);
    if (item != nullptr) {
        loadPatientIntoForm(item->text());
    }
}

void PatientPanel::loadPatientIntoForm(const QString& patientId) {
    const auto* record = m_manager->patient(patientId.toStdString());
    if (record == nullptr) {
        return;
    }
    m_editingPatientId = patientId;

    const QSignalBlocker blockType(m_typeCombo);
    m_idEdit->setText(toQt(record->id()));
    m_idEdit->setReadOnly(true);
    m_nameEdit->setText(toQt(record->name()));
    m_ageSpin->setValue(record->age());
    m_genderCombo->setCurrentText(toQt(record->gender()));

    if (const auto* general = dynamic_cast<const model::GeneralPatient*>(record)) {
        m_typeCombo->setCurrentIndex(0);
        m_extraStack->setCurrentIndex(0);
        m_insuranceCombo->setCurrentText(toQt(general->insuranceProvider()));
        m_chronicEdit->setText(toQt(general->chronicCondition()));
    } else if (const auto* emergency = dynamic_cast<const model::EmergencyPatient*>(record)) {
        m_typeCombo->setCurrentIndex(1);
        m_extraStack->setCurrentIndex(1);
        m_triageSpin->setValue(emergency->triageLevel());
        m_arrivalCombo->setCurrentText(toQt(emergency->arrivalMode()));
        m_admittedCheck->setChecked(emergency->admitted());
    } else if (const auto* followUp = dynamic_cast<const model::FollowUpPatient*>(record)) {
        m_typeCombo->setCurrentIndex(2);
        m_extraStack->setCurrentIndex(2);
        m_originalVisitEdit->setDate(
            QDate::fromString(toQt(followUp->originalVisitDate()), "yyyy-MM-dd"));
        m_intervalSpin->setValue(followUp->followUpIntervalDays());
        m_doctorEdit->setText(toQt(followUp->referringDoctor()));
    }

    m_registeredEdit->setDate(
        QDate::fromString(toQt(record->registeredOn()), "yyyy-MM-dd"));
    // Contact is set after the type-specific widgets so textChanged previews see
    // the final values.
    m_contactEdit->setText(toQt(record->contact()));

    m_updateButton->setEnabled(true);
    m_deleteButton->setEnabled(true);
    updateIdFieldStyle();
    updatePreview();
}

// =============================================================================
//  Form helpers
// =============================================================================
std::unique_ptr<model::PatientRecord> PatientPanel::buildPatientFromForm(
    const QString& patientId) const {
    const std::string id = patientId.toStdString();
    const std::string name = m_nameEdit->text().trimmed().toStdString();
    const std::string contact = m_contactEdit->text().trimmed().toStdString();
    const std::string gender = m_genderCombo->currentText().toStdString();
    const std::string registered = m_registeredEdit->date().toString("yyyy-MM-dd").toStdString();
    const int age = m_ageSpin->value();

    switch (m_typeCombo->currentIndex()) {
    case 1:
        return std::make_unique<model::EmergencyPatient>(
            id, name, age, contact, gender, registered, m_triageSpin->value(),
            m_arrivalCombo->currentText().toStdString(), m_admittedCheck->isChecked());
    case 2:
        return std::make_unique<model::FollowUpPatient>(
            id, name, age, contact, gender, registered,
            m_originalVisitEdit->date().toString("yyyy-MM-dd").toStdString(),
            m_intervalSpin->value(), m_doctorEdit->text().trimmed().toStdString());
    case 0:
    default:
        return std::make_unique<model::GeneralPatient>(
            id, name, age, contact, gender, registered,
            m_insuranceCombo->currentText().trimmed().toStdString(),
            m_chronicEdit->text().trimmed().toStdString());
    }
}

void PatientPanel::updatePreview() {
    const QString id = m_idEdit->text().trimmed().isEmpty() ? QStringLiteral("P-0000")
                                                            : m_idEdit->text().trimmed();
    try {
        const auto patient = buildPatientFromForm(id);
        m_previewLabel->setText(
            QString("<b>%1</b> &middot; priority %2 &middot; fee %3<br/>%4")
                .arg(toQt(patient->category()))
                .arg(toQt(patient->priorityLabel()))
                .arg(QString::fromStdString(
                    scapms::utils::formatMoney(patient->consultationFee())))
                .arg(toQt(patient->clinicalSummary())));
    } catch (const std::exception&) {
        m_previewLabel->setText("Complete the form to preview the patient summary.");
    }

    if (m_dueLabel != nullptr && m_typeCombo->currentIndex() == 2) {
        const QDate due = m_originalVisitEdit->date().addDays(m_intervalSpin->value());
        m_dueLabel->setText(QString("Review due: %1 (%2)")
                                .arg(due.toString("yyyy-MM-dd"))
                                .arg(due < QDate::currentDate() ? "overdue" : "upcoming"));
    }
}

void PatientPanel::updateIdFieldStyle() {
    const QString id = m_idEdit->text().trimmed();
    static const QRegularExpression pattern("^[A-Za-z0-9_-]{3,16}$");
    const bool valid = id.isEmpty() || pattern.match(id).hasMatch();
    m_idEdit->setStyleSheet(valid ? QString()
                                  : "border: 1px solid #DC2626; background: #FEF2F2;");
    m_idEdit->setToolTip(valid ? "Leave blank to let the system allocate the next "
                                 "patient ID."
                               : "Patient ID must be 3-16 letters, digits, '-' or '_'.");
}

// =============================================================================
//  Slots
// =============================================================================
void PatientPanel::onAddPatient() {
    QString id = m_idEdit->text().trimmed();
    if (id.isEmpty()) {
        id = toQt(m_manager->generatePatientId());
    }

    try {
        auto patient = buildPatientFromForm(id);
        const QString label = toQt(patient->name()) + " (" + id + ")";
        m_manager->registerPatient(std::move(patient));
        m_editingPatientId.clear();
        refresh();
        onClearForm();
        emit dataChanged();
        emit statusMessage("Patient " + label + " registered.");
        QMessageBox::information(this, "Patient registered",
                                 "Patient " + label + " was registered successfully.");
    } catch (const scapms::utils::ClinicException& error) {
        QMessageBox::warning(this, "Invalid patient information", error.what());
    } catch (const std::exception& error) {
        QMessageBox::critical(this, "Unexpected error", error.what());
    }
}

void PatientPanel::onUpdatePatient() {
    const QString id = m_editingPatientId.isEmpty() ? selectedPatientId() : m_editingPatientId;
    if (id.isEmpty()) {
        QMessageBox::information(this, "No patient selected",
                                 "Select a patient in the table first.");
        return;
    }
    try {
        auto updated = buildPatientFromForm(id);
        m_manager->updatePatient(id.toStdString(), std::move(updated));
        emit dataChanged();
        emit statusMessage("Patient " + id + " updated.");
        refresh();
        loadPatientIntoForm(id);
    } catch (const scapms::utils::ClinicException& error) {
        QMessageBox::warning(this, "Cannot update patient", error.what());
    } catch (const std::exception& error) {
        QMessageBox::critical(this, "Unexpected error", error.what());
    }
}

void PatientPanel::onDeletePatient() {
    const QString id = selectedPatientId();
    if (id.isEmpty()) {
        QMessageBox::information(this, "No patient selected",
                                 "Select a patient in the table first.");
        return;
    }
    const auto* record = m_manager->patient(id.toStdString());
    const QString name = record ? toQt(record->name()) : id;
    const auto answer = QMessageBox::question(
        this, "Delete patient",
        QString("Delete %1 (%2)?\n\nAny appointments for this patient are removed as "
                "well. The operation can be undone with Ctrl+Z.")
            .arg(name, id),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }

    try {
        m_manager->deletePatient(id.toStdString());
        m_editingPatientId.clear();
        refresh();
        onClearForm();
        emit dataChanged();
        emit statusMessage("Patient " + id + " deleted - Ctrl+Z undoes this.");
    } catch (const scapms::utils::ClinicException& error) {
        QMessageBox::warning(this, "Cannot delete patient", error.what());
    } catch (const std::exception& error) {
        QMessageBox::critical(this, "Unexpected error", error.what());
    }
}

void PatientPanel::onClearForm() {
    m_editingPatientId.clear();
    m_idEdit->clear();
    m_idEdit->setReadOnly(false);
    m_idEdit->setPlaceholderText("auto-generated, e.g. P-0015");
    m_nameEdit->clear();
    m_ageSpin->setValue(30);
    m_genderCombo->setCurrentIndex(0);
    m_contactEdit->clear();
    m_typeCombo->setCurrentIndex(0);
    m_extraStack->setCurrentIndex(0);
    m_insuranceCombo->setCurrentText("Self-Pay");
    m_chronicEdit->setText("None");
    m_triageSpin->setValue(3);
    m_arrivalCombo->setCurrentIndex(1);
    m_admittedCheck->setChecked(false);
    m_originalVisitEdit->setDate(QDate::currentDate().addDays(-14));
    m_intervalSpin->setValue(14);
    m_doctorEdit->setText("Dr. Adeyemi");
    m_registeredEdit->setDate(QDate::currentDate());
    m_table->clearSelection();
    m_updateButton->setEnabled(false);
    m_deleteButton->setEnabled(false);
    updateIdFieldStyle();
    updatePreview();
}

void PatientPanel::prepareNewPatient() {
    onClearForm();
    m_nameEdit->setFocus();
}

void PatientPanel::focusSearch() {
    m_filterEdit->setFocus();
    m_filterEdit->selectAll();
}

void PatientPanel::onTypeChanged(int index) {
    m_extraStack->setCurrentIndex(index);
    updatePreview();
}

void PatientPanel::onFilterChanged(const QString& /*text*/) {
    populateTable();
}

void PatientPanel::refresh() {
    populateTable();
    if (!m_editingPatientId.isEmpty()) {
        m_updateButton->setEnabled(true);
        m_deleteButton->setEnabled(true);
    }
}

} // namespace scapms::gui
