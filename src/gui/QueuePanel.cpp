// =============================================================================
//  File   : src/gui/QueuePanel.cpp
// =============================================================================
#include "QueuePanel.h"

#include <QComboBox>
#include <QDateTime>
#include <QFont>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QStringList>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>

#include "../controller/ClinicManager.h"
#include "../utils/Exceptions.h"
#include "UiHelpers.h"

namespace scapms::gui {

using scapms::controller::ClinicManager;
using scapms::controller::QueueEntry;

QueuePanel::QueuePanel(ClinicManager* manager, QWidget* parent)
    : QWidget(parent), m_manager(manager) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(10);

    auto* header = new QHBoxLayout();
    header->addWidget(createSectionTitle("Waiting Queue", this));
    header->addStretch(1);
    m_statsLabel = new QLabel(this);
    m_statsLabel->setObjectName("brandSubtitle");
    header->addWidget(m_statsLabel);
    root->addLayout(header);

    auto* policy = createHintLabel(
        "Queue policy: patients are served first-in first-out. Emergency (triage) "
        "patients jump ahead of the routine queue, as they do in a real clinic. The "
        "waiting line is a std::queue, so entries can only leave from the front.",
        this);
    root->addWidget(policy);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setChildrenCollapsible(false);
    splitter->addWidget(createQueueBox());
    splitter->addWidget(createAddBox());
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({780, 320});
    root->addWidget(splitter, 1);

    // Timer event: refresh the waiting times and raise the long-wait alert.
    m_timer = new QTimer(this);
    m_timer->setInterval(15000);
    connect(m_timer, &QTimer::timeout, this, &QueuePanel::onTick);
    m_timer->start();

    refresh();
}

QWidget* QueuePanel::createQueueBox() {
    auto* box = new QGroupBox("Patients waiting for consultation", this);
    auto* layout = new QVBoxLayout(box);

    m_queueTable = new QTableWidget(0, 6, box);
    m_queueTable->setHorizontalHeaderLabels(
        {"#", "Ticket", "Patient", "Category", "Priority", "Waiting"});
    m_queueTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_queueTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_queueTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_queueTable->setAlternatingRowColors(true);
    m_queueTable->verticalHeader()->setVisible(false);
    m_queueTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_queueTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_queueTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_queueTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_queueTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_queueTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_queueTable->setToolTip("Priority patients are listed first, in arrival order.");
    layout->addWidget(m_queueTable, 1);

    auto* actions = new QHBoxLayout();
    auto* refreshButton = new QPushButton("Refresh", box);
    refreshButton->setObjectName("secondaryButton");
    auto* removeButton = new QPushButton("Remove Selected", box);
    removeButton->setObjectName("dangerButton");
    m_callNextButton = new QPushButton("Call Next Patient", box);
    m_callNextButton->setToolTip("Pops the highest-priority patient from the queue.");
    actions->addWidget(m_callNextButton);
    actions->addWidget(removeButton);
    actions->addStretch(1);
    actions->addWidget(refreshButton);
    layout->addLayout(actions);

    m_alertLabel = new QLabel(box);
    m_alertLabel->setObjectName("warningLabel");
    m_alertLabel->setWordWrap(true);
    m_alertLabel->setVisible(false);
    layout->addWidget(m_alertLabel);

    connect(m_callNextButton, &QPushButton::clicked, this, &QueuePanel::onCallNext);
    connect(removeButton, &QPushButton::clicked, this, &QueuePanel::onRemoveSelected);
    connect(refreshButton, &QPushButton::clicked, this, &QueuePanel::refresh);
    return box;
}

QWidget* QueuePanel::createAddBox() {
    auto* box = new QGroupBox("Add a patient to the queue", this);
    auto* layout = new QVBoxLayout(box);

    auto* hint = createHintLabel(
        "Walk-in patients (including emergency cases) join the queue here. "
        "Patients already waiting are hidden from the list.", box);
    layout->addWidget(hint);

    m_patientCombo = new QComboBox(box);
    m_patientCombo->setMaxVisibleItems(18);
    layout->addWidget(m_patientCombo);

    auto* addButton = new QPushButton("Add to Queue", box);
    addButton->setToolTip("Issue a queue ticket and place the patient in the "
                          "appropriate queue.");
    layout->addWidget(addButton);

    auto* legend = createHintLabel(
        "Priority = 1 (emergency, triage level 1-2) is served before routine "
        "patients. All other patients wait their turn.", box);
    layout->addWidget(legend);
    layout->addStretch(1);

    connect(addButton, &QPushButton::clicked, this, &QueuePanel::onAddToQueue);
    return box;
}

int QueuePanel::waitingMinutes(const QString& joinedAt) const {
    const QDateTime joined = QDateTime::fromString(joinedAt, "yyyy-MM-dd HH:mm:ss");
    if (!joined.isValid()) {
        return 0;
    }
    return static_cast<int>(joined.secsTo(QDateTime::currentDateTime()) / 60);
}

void QueuePanel::populateCandidates() {
    const QString previous = m_patientCombo->currentData().toString();
    m_patientCombo->clear();
    for (const auto* record : m_manager->patients()) {
        if (record == nullptr || m_manager->isInQueue(record->id())) {
            continue;
        }
        m_patientCombo->addItem(
            QString("%1 - %2 (%3)").arg(toQt(record->id()), toQt(record->name()),
                                        toQt(record->category())),
            toQt(record->id()));
    }
    if (m_patientCombo->count() == 0) {
        m_patientCombo->addItem("(every patient is already waiting)", QString());
    }
    if (!previous.isEmpty()) {
        const int index = m_patientCombo->findData(previous);
        if (index >= 0) {
            m_patientCombo->setCurrentIndex(index);
        }
    }
}

void QueuePanel::populateQueue() {
    m_queueTable->setUpdatesEnabled(false);
    m_queueTable->setRowCount(0);

    const auto entries = m_manager->queueSnapshot();
    int longestWait = 0;

    for (std::size_t i = 0; i < entries.size(); ++i) {
        const auto& entry = entries[i];
        const int minutes = waitingMinutes(toQt(entry.joinedAt));
        longestWait = std::max(longestWait, minutes);

        const int row = m_queueTable->rowCount();
        m_queueTable->insertRow(row);
        const QStringList values{
            QString::number(i + 1),
            toQt(entry.ticket),
            toQt(entry.patientName),
            toQt(entry.category),
            entry.priorityCase ? "PRIORITY" : "Routine",
            QString("%1 min").arg(minutes)};

        for (int column = 0; column < values.size(); ++column) {
            auto* item = new QTableWidgetItem(values.at(column));
            item->setData(Qt::UserRole, toQt(entry.patientId));
            if (entry.priorityCase) {
                item->setForeground(QColor("#B91C1C"));
                QFont font = item->font();
                font.setBold(true);
                item->setFont(font);
            }
            if (column == 0 || column == 5) {
                item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            }
            m_queueTable->setItem(row, column, item);
        }
        m_queueTable->setRowHeight(row, 30);
    }

    if (entries.empty()) {
        m_queueTable->insertRow(0);
        auto* item = new QTableWidgetItem(
            "The queue is empty - all patients have been attended to.");
        item->setForeground(QColor("#64748B"));
        m_queueTable->setItem(0, 0, item);
        m_queueTable->setSpan(0, 0, 1, 6);
    }
    m_queueTable->clearSelection();
    m_queueTable->setUpdatesEnabled(true);

    m_statsLabel->setText(QString("Waiting: %1  |  Priority: %2  |  Longest wait: %3 min")
                              .arg(entries.size())
                              .arg(m_manager->priorityQueueLength())
                              .arg(longestWait));

    if (longestWait >= 20) {
        m_alertLabel->setText(
            QString("Waiting-time alert: a patient has been queued for %1 minutes. "
                    "Please call the next patient.")
                .arg(longestWait));
        m_alertLabel->setVisible(true);
    } else {
        m_alertLabel->setVisible(false);
    }
}

QString QueuePanel::selectedPatientId() const {
    const auto rows = m_queueTable->selectionModel() != nullptr
                          ? m_queueTable->selectionModel()->selectedRows()
                          : QModelIndexList{};
    if (rows.isEmpty()) {
        return {};
    }
    const auto* item = m_queueTable->item(rows.first().row(), 0);
    return item != nullptr ? item->data(Qt::UserRole).toString() : QString();
}

void QueuePanel::onAddToQueue() {
    const QString patientId = m_patientCombo->currentData().toString();
    if (patientId.isEmpty()) {
        QMessageBox::information(this, "No patient available",
                                 "Every registered patient is already in the queue.");
        return;
    }
    try {
        const QueueEntry entry = m_manager->joinQueue(patientId.toStdString());
        refresh();
        emit dataChanged();
        emit statusMessage(toQt(entry.patientName) + " joined the queue as " +
                           toQt(entry.ticket) +
                           (entry.priorityCase ? " (priority)." : "."));
    } catch (const scapms::utils::ClinicException& error) {
        QMessageBox::warning(this, "Cannot add to queue", error.what());
    }
}

void QueuePanel::onCallNext() {
    QueueEntry served;
    if (!m_manager->callNextPatient(served)) {
        QMessageBox::information(this, "Queue empty",
                                 "There is no patient waiting to be called.");
        return;
    }
    refresh();
    emit dataChanged();
    emit statusMessage("Called " + toQt(served.ticket) + " - " +
                       toQt(served.patientName) + " (" + toQt(served.category) + ").");
    QMessageBox::information(
        this, "Next patient",
        QString("Now serving %1\n\n%2\n%3 patient\nTicket issued at %4")
            .arg(toQt(served.ticket), toQt(served.patientName), toQt(served.category),
                 toQt(served.joinedAt)));
}

void QueuePanel::onRemoveSelected() {
    const QString patientId = selectedPatientId();
    if (patientId.isEmpty()) {
        QMessageBox::information(this, "No patient selected",
                                 "Select a waiting patient first.");
        return;
    }
    if (!m_manager->removeFromQueue(patientId.toStdString())) {
        QMessageBox::warning(this, "Not in queue", "That patient is no longer waiting.");
        return;
    }
    refresh();
    emit dataChanged();
    emit statusMessage("Patient " + patientId + " removed from the queue.");
}

void QueuePanel::onTick() {
    // Timer event: keep the waiting times fresh and warn the staff.
    populateQueue();
    for (const auto& entry : m_manager->queueSnapshot()) {
        if (waitingMinutes(toQt(entry.joinedAt)) >= 20) {
            emit statusMessage("Queue alert: " + toQt(entry.patientName) +
                               " has been waiting 20+ minutes.");
            break;
        }
    }
}

void QueuePanel::refresh() {
    populateCandidates();
    populateQueue();
    m_callNextButton->setEnabled(m_manager->queueLength() > 0);
}

} // namespace scapms::gui
