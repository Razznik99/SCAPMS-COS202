// =============================================================================
//  File   : src/gui/AppointmentPanel.cpp
// =============================================================================
#include "AppointmentPanel.h"

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
#include <QSplitter>
#include <QStringList>
#include <QStyle>
#include <QTableWidget>
#include <QTime>
#include <QTimeEdit>
#include <QVBoxLayout>

#include <algorithm>

#include "../controller/ClinicManager.h"
#include "../utils/Exceptions.h"
#include "../utils/StringUtils.h"
#include "PillDelegate.h"
#include "UiHelpers.h"

namespace scapms::gui {

using scapms::controller::ClinicManager;

AppointmentPanel::AppointmentPanel(ClinicManager* manager, QWidget* parent)
    : QWidget(parent), m_manager(manager) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(10);

    auto* header = new QHBoxLayout();
    header->addWidget(createSectionTitle("Appointment Management", this));
    header->addStretch(1);
    header->addWidget(new QLabel("Show:", this));
    m_statusFilter = new QComboBox(this);
    m_statusFilter->addItems({"All statuses", "Pending", "Completed", "Cancelled"});
    m_statusFilter->setToolTip("Filter the appointment list by status.");
    header->addWidget(m_statusFilter);
    root->addLayout(header);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setChildrenCollapsible(false);

    // ---- schedule form -----------------------------------------------------
    auto* formBox = new QGroupBox("Schedule an appointment", splitter);
    auto* formLayout = new QVBoxLayout(formBox);
    formLayout->setSpacing(8);

    auto* form = new QFormLayout();
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    m_patientCombo = new QComboBox(formBox);
    m_patientCombo->setToolTip("Every registered patient is listed; the controller "
                               "rejects types that cannot be booked.");
    m_patientCombo->setMaxVisibleItems(18);
    form->addRow("Patient *", m_patientCombo);

    m_dateEdit = new QDateEdit(QDate::currentDate(), formBox);
    m_dateEdit->setCalendarPopup(true);
    m_dateEdit->setDisplayFormat("yyyy-MM-dd");
    m_dateEdit->setMinimumDate(QDate::currentDate());
    m_dateEdit->setToolTip("Past dates are rejected by the appointment controller.");
    form->addRow("Date *", m_dateEdit);

    m_timeEdit = new QTimeEdit(QTime(9, 0), formBox);
    m_timeEdit->setDisplayFormat("HH:mm");
    form->addRow("Time *", m_timeEdit);

    m_serviceCombo = new QComboBox(formBox);
    m_serviceCombo->setToolTip("Medical service; the fee is taken from the catalogue "
                               "or from the patient type for consultations.");
    form->addRow("Service *", m_serviceCombo);

    m_clinicianEdit = new QLineEdit("Dr. Adeyemi", formBox);
    form->addRow("Clinician *", m_clinicianEdit);

    m_notesEdit = new QLineEdit(formBox);
    m_notesEdit->setPlaceholderText("Optional notes for the clinician");
    form->addRow("Notes", m_notesEdit);

    formLayout->addLayout(form);

    m_eligibilityLabel = createHintLabel("", formBox);
    m_eligibilityLabel->setWordWrap(true);
    formLayout->addWidget(m_eligibilityLabel);

    m_feePreviewLabel = createHintLabel("", formBox);
    formLayout->addWidget(m_feePreviewLabel);

    auto* scheduleButton = new QPushButton("Schedule Appointment", formBox);
    scheduleButton->setToolTip("Book the appointment (validates date, time, clinician "
                               "and double-booking).");
    formLayout->addWidget(scheduleButton);
    formLayout->addStretch(1);

    // ---- table -------------------------------------------------------------
    auto* tableBox = new QWidget(splitter);
    auto* tableLayout = new QVBoxLayout(tableBox);
    tableLayout->setContentsMargins(0, 0, 0, 0);
    tableLayout->setSpacing(6);

    m_table = new QTableWidget(0, 8, tableBox);
    m_table->setHorizontalHeaderLabels({"Appointment", "Patient", "Date", "Time", "Service",
                                        "Clinician", "Status", "Fee"});
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(7, QHeaderView::ResizeToContents);
    m_table->setItemDelegateForColumn(6, new PillDelegate(PillDelegate::Kind::Status, this));
    tableLayout->addWidget(m_table, 1);

    auto* actions = new QHBoxLayout();
    m_completeButton = new QPushButton("Mark Completed", tableBox);
    m_completeButton->setToolTip("Move the selected pending appointment to Completed.");
    m_cancelButton = new QPushButton("Cancel Appointment", tableBox);
    m_cancelButton->setObjectName("secondaryButton");
    m_cancelButton->setToolTip("Cancel the selected pending appointment.");
    m_removeButton = new QPushButton("Delete", tableBox);
    m_removeButton->setObjectName("dangerButton");
    m_removeButton->setToolTip("Permanently remove the appointment (undoable).");
    actions->addWidget(m_completeButton);
    actions->addWidget(m_cancelButton);
    actions->addWidget(m_removeButton);
    actions->addStretch(1);
    tableLayout->addLayout(actions);

    m_infoLabel = createHintLabel("", tableBox);
    tableLayout->addWidget(m_infoLabel);

    splitter->addWidget(formBox);
    splitter->addWidget(tableBox);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 3);
    splitter->setSizes({420, 780});
    root->addWidget(splitter, 1);

    populateServices();
    populatePatients();

    connect(scheduleButton, &QPushButton::clicked, this, &AppointmentPanel::onSchedule);
    connect(m_completeButton, &QPushButton::clicked, this, &AppointmentPanel::onComplete);
    connect(m_cancelButton, &QPushButton::clicked, this, &AppointmentPanel::onCancel);
    connect(m_removeButton, &QPushButton::clicked, this, &AppointmentPanel::onRemove);
    connect(m_patientCombo, &QComboBox::currentIndexChanged, this,
            &AppointmentPanel::onPatientChanged);
    connect(m_serviceCombo, &QComboBox::currentIndexChanged, this,
            &AppointmentPanel::onServiceChanged);
    connect(m_statusFilter, &QComboBox::currentIndexChanged, this,
            &AppointmentPanel::onStatusFilterChanged);

    refresh();
}

void AppointmentPanel::populateServices() {
    m_serviceCombo->clear();
    for (const auto& service : m_manager->services()) {
        m_serviceCombo->addItem(
            QString("%1 - %2 (%3 min)")
                .arg(toQt(service.code()), toQt(service.name()))
                .arg(service.durationMinutes()),
            toQt(service.code()));
    }
}

void AppointmentPanel::populatePatients() {
    const QString previous = m_patientCombo->currentData().toString();
    m_patientCombo->clear();
    for (const auto* record : m_manager->patients()) {
        if (record == nullptr) {
            continue;
        }
        m_patientCombo->addItem(
            QString("%1 - %2 (%3)").arg(toQt(record->id()), toQt(record->name()),
                                        toQt(record->category())),
            toQt(record->id()));
    }
    if (!previous.isEmpty()) {
        const int index = m_patientCombo->findData(previous);
        if (index >= 0) {
            m_patientCombo->setCurrentIndex(index);
        }
    }
    updateEligibility();
}

void AppointmentPanel::updateEligibility() {
    if (m_patientCombo->currentIndex() < 0) {
        m_eligibilityLabel->setText("Register a patient before scheduling.");
        return;
    }
    const auto* record =
        m_manager->patient(m_patientCombo->currentData().toString().toStdString());
    if (record == nullptr) {
        m_eligibilityLabel->setText("Select a patient.");
        return;
    }

    const bool appointable = controller::AppointmentController::isAppointable(*record);
    m_eligibilityLabel->setText(
        toQt(controller::AppointmentController::eligibilityMessage(*record)));
    m_eligibilityLabel->setObjectName(appointable ? "successLabel" : "warningLabel");
    m_eligibilityLabel->setStyleSheet({});
    m_eligibilityLabel->style()->unpolish(m_eligibilityLabel);
    m_eligibilityLabel->style()->polish(m_eligibilityLabel);

    const auto* service = m_manager->database().findService(
        m_serviceCombo->currentData().toString().toStdString());
    if (service != nullptr) {
        double fee = service->baseFee();
        if (service->category() == "Consultation") {
            fee = record->consultationFee();
        }
        m_feePreviewLabel->setText(
            QString("Estimated fee: %1  |  %2 minute slot")
                .arg(QString::fromStdString(scapms::utils::formatMoney(fee)))
                .arg(service->durationMinutes()));
    }
}

void AppointmentPanel::onServiceChanged(int /*index*/) {
    updateEligibility();
}

void AppointmentPanel::onPatientChanged(int /*index*/) {
    updateEligibility();
}

void AppointmentPanel::populateTable() {
    const QString selectedId = selectedAppointmentId();
    const int filter = m_statusFilter->currentIndex();

    m_display.clear();
    for (const auto& appointment : m_manager->database().appointments()) {
        if (filter == 1 && !appointment.isPending()) {
            continue;
        }
        if (filter == 2 && !appointment.isCompleted()) {
            continue;
        }
        if (filter == 3 && !appointment.isCancelled()) {
            continue;
        }
        m_display.push_back(appointment);
    }

    // Hand-written insertion sort keeps the list in chronological order.
    for (std::size_t i = 1; i < m_display.size(); ++i) {
        const model::Appointment current = m_display[i];
        std::size_t j = i;
        while (j > 0 && current.startKey() < m_display[j - 1].startKey()) {
            m_display[j] = m_display[j - 1];
            --j;
        }
        m_display[j] = current;
    }

    m_table->setUpdatesEnabled(false);
    m_table->setRowCount(0);
    for (const auto& appointment : m_display) {
        const auto* patient =
            m_manager->database().findPatientById(appointment.patientId());
        const int row = m_table->rowCount();
        m_table->insertRow(row);

        const QStringList values{
            toQt(appointment.id()),
            patient ? toQt(patient->name()) : QString("(deleted patient)"),
            toQt(appointment.date()),
            toQt(appointment.time()),
            toQt(appointment.serviceCode()),
            toQt(appointment.clinician()),
            toQt(appointment.statusText()),
            QString::fromStdString(scapms::utils::formatMoney(appointment.fee()))};

        for (int column = 0; column < values.size(); ++column) {
            auto* item = new QTableWidgetItem(values.at(column));
            if (column == 7) {
                item->setTextAlignment((Qt::AlignRight | Qt::AlignVCenter));
            }
            m_table->setItem(row, column, item);
        }
        m_table->setRowHeight(row, 34);
        if (toQt(appointment.id()) == selectedId) {
            m_table->selectRow(row);
        }
    }
    m_table->setUpdatesEnabled(true);

    const auto stats = m_manager->stats();
    m_infoLabel->setText(
        QString("Showing %1 of %2 appointments  |  Pending: %3  Completed: %4  "
                "Cancelled: %5  |  Today: %6")
            .arg(m_display.size())
            .arg(stats.totalAppointments)
            .arg(stats.pendingAppointments)
            .arg(stats.completedAppointments)
            .arg(stats.cancelledAppointments)
            .arg(stats.todayAppointments));
}

QString AppointmentPanel::selectedAppointmentId() const {
    if (m_table->selectionModel() == nullptr) {
        return {};
    }
    const auto rows = m_table->selectionModel()->selectedRows();
    if (rows.isEmpty()) {
        return {};
    }
    const auto* item = m_table->item(rows.first().row(), 0);
    return item ? item->text() : QString();
}

void AppointmentPanel::onSchedule() {
    if (m_patientCombo->currentIndex() < 0) {
        QMessageBox::information(this, "No patient", "Register a patient first.");
        return;
    }
    const std::string patientId = m_patientCombo->currentData().toString().toStdString();
    const std::string date = m_dateEdit->date().toString("yyyy-MM-dd").toStdString();
    const std::string time = m_timeEdit->time().toString("HH:mm").toStdString();
    const std::string service = m_serviceCombo->currentData().toString().toStdString();
    const std::string clinician = m_clinicianEdit->text().trimmed().toStdString();
    const std::string notes = m_notesEdit->text().trimmed().toStdString();

    try {
        const auto appointment = m_manager->scheduleAppointment(patientId, date, time,
                                                                service, clinician, notes);
        emit dataChanged();
        emit statusMessage("Appointment " + toQt(appointment.id()) + " scheduled for " +
                           toQt(appointment.date()) + " at " + toQt(appointment.time()) +
                           ".");
        refresh();
        QMessageBox::information(
            this, "Appointment scheduled",
            QString("Appointment %1 booked for %2 at %3.")
                .arg(toQt(appointment.id()), toQt(appointment.date()),
                     toQt(appointment.time())));
    } catch (const scapms::utils::ClinicException& error) {
        QMessageBox::warning(this, "Cannot schedule appointment", error.what());
    } catch (const std::exception& error) {
        QMessageBox::critical(this, "Unexpected error", error.what());
    }
}

void AppointmentPanel::onComplete() {
    const QString id = selectedAppointmentId();
    if (id.isEmpty()) {
        QMessageBox::information(this, "No appointment selected",
                                 "Select an appointment in the table first.");
        return;
    }
    try {
        m_manager->completeAppointment(id.toStdString());
        emit dataChanged();
        emit statusMessage("Appointment " + id + " marked completed (Ctrl+Z undoes this).");
        refresh();
    } catch (const scapms::utils::ClinicException& error) {
        QMessageBox::warning(this, "Cannot complete appointment", error.what());
    }
}

void AppointmentPanel::onCancel() {
    const QString id = selectedAppointmentId();
    if (id.isEmpty()) {
        QMessageBox::information(this, "No appointment selected",
                                 "Select an appointment in the table first.");
        return;
    }
    const auto answer = QMessageBox::question(
        this, "Cancel appointment",
        QString("Cancel appointment %1? The record is kept with status Cancelled and "
                "the action can be undone with Ctrl+Z.")
            .arg(id),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }
    try {
        m_manager->cancelAppointment(id.toStdString());
        emit dataChanged();
        emit statusMessage("Appointment " + id + " cancelled.");
        refresh();
    } catch (const scapms::utils::ClinicException& error) {
        QMessageBox::warning(this, "Cannot cancel appointment", error.what());
    }
}

void AppointmentPanel::onRemove() {
    const QString id = selectedAppointmentId();
    if (id.isEmpty()) {
        QMessageBox::information(this, "No appointment selected",
                                 "Select an appointment in the table first.");
        return;
    }
    const auto answer = QMessageBox::question(
        this, "Delete appointment",
        QString("Permanently delete appointment %1?\n\nUse Ctrl+Z to restore it.").arg(id),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }
    try {
        m_manager->removeAppointment(id.toStdString());
        emit dataChanged();
        emit statusMessage("Appointment " + id + " deleted - Ctrl+Z restores it.");
        refresh();
    } catch (const scapms::utils::ClinicException& error) {
        QMessageBox::warning(this, "Cannot delete appointment", error.what());
    }
}

void AppointmentPanel::onStatusFilterChanged(int /*index*/) {
    populateTable();
}

void AppointmentPanel::refresh() {
    populatePatients();
    populateTable();
}

void AppointmentPanel::focusScheduling() {
    m_patientCombo->setFocus();
}

} // namespace scapms::gui
