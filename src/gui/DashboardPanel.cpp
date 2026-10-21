// =============================================================================
//  File   : src/gui/DashboardPanel.cpp
//  =============================================================================
#include "DashboardPanel.h"

#include <QDate>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QTableWidget>
#include <QTime>
#include <QVBoxLayout>

#include <algorithm>
#include <vector>

#include "../controller/ClinicManager.h"
#include "../utils/DateUtils.h"
#include "../utils/StringUtils.h"
#include "UiHelpers.h"

namespace scapms::gui {

using scapms::controller::ClinicManager;

DashboardPanel::DashboardPanel(ClinicManager* manager, QWidget* parent)
    : QWidget(parent), m_manager(manager) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    auto* header = new QHBoxLayout();
    auto* titleBlock = new QVBoxLayout();
    titleBlock->setSpacing(0);
    titleBlock->addWidget(createSectionTitle("Clinic Dashboard", this));
    m_dateLabel = createHintLabel("", this);
    titleBlock->addWidget(m_dateLabel);
    header->addLayout(titleBlock);
    header->addStretch(1);
    auto* refreshButton = new QPushButton("Refresh", this);
    refreshButton->setObjectName("secondaryButton");
    refreshButton->setToolTip("Recompute every statistic from the live database.");
    header->addWidget(refreshButton);
    root->addLayout(header);

    m_bannerLabel = new QLabel(this);
    m_bannerLabel->setObjectName("bannerLabel");
    m_bannerLabel->setWordWrap(true);
    root->addWidget(m_bannerLabel);

    auto* cards = new QGridLayout();
    cards->setSpacing(10);
    cards->addWidget(createStatCard("Total patients", "#2563EB", m_totalPatientsValue, this),
                     0, 0);
    cards->addWidget(createStatCard("Appointments today", "#7C3AED", m_todayValue, this),
                     0, 1);
    cards->addWidget(createStatCard("Pending appointments", "#D97706", m_pendingValue, this),
                     0, 2);
    cards->addWidget(createStatCard("Completed", "#16A34A", m_completedValue, this), 0, 3);
    cards->addWidget(createStatCard("Waiting queue", "#0891B2", m_queueValue, this), 1, 0);
    cards->addWidget(createStatCard("Emergency patients", "#DC2626", m_emergencyValue, this),
                     1, 1);
    cards->addWidget(createStatCard("Fees collected", "#059669", m_revenueValue, this), 1, 2);
    cards->addWidget(createStatCard("Undo operations", "#475569", m_undoValue, this), 1, 3);
    root->addLayout(cards);

    auto* lower = new QHBoxLayout();
    lower->setSpacing(10);

    auto* upcomingBox = new QGroupBox("Next appointments", this);
    auto* upcomingLayout = new QVBoxLayout(upcomingBox);
    m_upcomingTable = new QTableWidget(0, 5, upcomingBox);
    m_upcomingTable->setHorizontalHeaderLabels({"Date", "Time", "Patient", "Service",
                                                "Clinician"});
    m_upcomingTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_upcomingTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_upcomingTable->setAlternatingRowColors(true);
    m_upcomingTable->verticalHeader()->setVisible(false);
    m_upcomingTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_upcomingTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_upcomingTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    upcomingLayout->addWidget(m_upcomingTable);
    lower->addWidget(upcomingBox, 3);

    auto* side = new QVBoxLayout();
    side->setSpacing(10);

    auto* breakdownBox = new QGroupBox("Patient categories", this);
    auto* breakdownLayout = new QVBoxLayout(breakdownBox);
    m_breakdownLabel = new QLabel(breakdownBox);
    m_breakdownLabel->setWordWrap(true);
    m_breakdownLabel->setTextFormat(Qt::RichText);
    breakdownLayout->addWidget(m_breakdownLabel);
    side->addWidget(breakdownBox);

    auto* recentBox = new QGroupBox("Recently accessed patients (cache)", this);
    auto* recentLayout = new QVBoxLayout(recentBox);
    m_recentList = new QListWidget(recentBox);
    m_recentList->setMaximumHeight(190);
    recentLayout->addWidget(m_recentList);
    m_recentHint = createHintLabel("", recentBox);
    recentLayout->addWidget(m_recentHint);
    side->addWidget(recentBox);
    side->addStretch(1);

    lower->addLayout(side, 2);
    root->addLayout(lower, 1);

    connect(refreshButton, &QPushButton::clicked, this, &DashboardPanel::refresh);
    refresh();
}

void DashboardPanel::updateCards() {
    const auto stats = m_manager->stats();
    m_totalPatientsValue->setText(QString::number(stats.totalPatients));
    m_todayValue->setText(QString::number(stats.todayAppointments));
    m_pendingValue->setText(QString::number(stats.pendingAppointments));
    m_completedValue->setText(QString::number(stats.completedAppointments));
    m_queueValue->setText(QString::number(stats.queueLength));
    m_emergencyValue->setText(QString::number(stats.emergencyPatients));
    m_revenueValue->setText(
        QString::fromStdString(scapms::utils::formatMoney(stats.revenueCollected)));
    m_undoValue->setText(QString::number(stats.undoDepth));

    m_dateLabel->setText(
        QDate::currentDate().toString("dddd, d MMMM yyyy") + "  |  " +
        QString("%1 patients  |  %2 appointments  |  %3 waiting")
            .arg(stats.totalPatients)
            .arg(stats.totalAppointments)
            .arg(stats.queueLength));
}

void DashboardPanel::updateUpcoming() {
    std::vector<model::Appointment> upcoming;
    const std::string today = scapms::utils::todayIso();
    for (const auto& appointment : m_manager->database().appointments()) {
        if (appointment.isPending() && appointment.date() >= today) {
            upcoming.push_back(appointment);
        }
    }
    // Hand-written insertion sort by date then time.
    for (std::size_t i = 1; i < upcoming.size(); ++i) {
        const model::Appointment current = upcoming[i];
        std::size_t j = i;
        while (j > 0 && current.startKey() < upcoming[j - 1].startKey()) {
            upcoming[j] = upcoming[j - 1];
            --j;
        }
        upcoming[j] = current;
    }

    m_upcomingTable->setRowCount(0);
    int shown = 0;
    for (const auto& appointment : upcoming) {
        if (shown >= 6) {
            break;
        }
        const auto* patient =
            m_manager->database().findPatientById(appointment.patientId());
        const int row = m_upcomingTable->rowCount();
        m_upcomingTable->insertRow(row);
        const QStringList values{
            toQt(appointment.date()), toQt(appointment.time()),
            patient ? toQt(patient->name()) : QString("(deleted)"),
            toQt(appointment.serviceCode()), toQt(appointment.clinician())};
        for (int column = 0; column < values.size(); ++column) {
            m_upcomingTable->setItem(row, column, new QTableWidgetItem(values.at(column)));
        }
        m_upcomingTable->setRowHeight(row, 30);
        ++shown;
    }
    if (shown == 0) {
        m_upcomingTable->insertRow(0);
        auto* item = new QTableWidgetItem("No pending upcoming appointments.");
        m_upcomingTable->setItem(0, 0, item);
        m_upcomingTable->setSpan(0, 0, 1, 5);
    }
}

void DashboardPanel::updateBreakdown() {
    const auto stats = m_manager->stats();
    const auto row = [](const QString& name, std::size_t count, const QString& colour) {
        return QString("<div style='margin:3px 0;'><span style='color:%1;'>&#9679;</span>"
                       "  <b>%2</b> &nbsp; %3</div>")
            .arg(colour, name)
            .arg(count);
    };
    m_breakdownLabel->setText(
        row("General patients", stats.generalPatients, "#2563EB") +
        row("Emergency patients", stats.emergencyPatients, "#DC2626") +
        row("Follow-up patients", stats.followUpPatients, "#D97706") +
        QString("<hr/><div style='color:#64748B;'>Patients with pending appointments: "
                "<b>%1</b></div>")
            .arg(stats.patientsWithPending));
}

void DashboardPanel::updateRecent() {
    m_recentList->clear();
    const auto recent = m_manager->database().recentPatients();
    int shown = 0;
    for (const auto* record : recent) {
        if (record == nullptr || shown >= 5) {
            continue;
        }
        ++shown;
        m_recentList->addItem(QString("%1 - %2 (%3)")
                                  .arg(toQt(record->id()), toQt(record->name()),
                                       toQt(record->category())));
    }
    m_recentHint->setText(
        shown == 0
            ? "Open a patient record to populate the fixed-size cache."
            : QString("Showing %1 of %2 slots from PatientRecord*[10].")
                  .arg(shown)
                  .arg(m_manager->database().recentCount()));
}

void DashboardPanel::updateBanner() {
    const std::string today = scapms::utils::todayIso();
    const auto entries = m_manager->queueSnapshot();

    QString banner;
    if (!entries.empty()) {
        banner = QString("<b>%1 patient(s) waiting</b> - next: %2 %3 (%4). ")
                     .arg(entries.size())
                     .arg(toQt(entries.front().ticket), toQt(entries.front().patientName),
                          toQt(entries.front().category));
    } else {
        banner = "<b>No patient is waiting.</b> ";
    }

    // Appointment reminder: next pending appointment of today.
    const QTime now = QTime::currentTime();
    QString reminder;
    for (const auto& appointment : m_manager->database().appointments()) {
        if (!appointment.isPending() || appointment.date() != today) {
            continue;
        }
        const QTime slot = QTime::fromString(toQt(appointment.time()), "HH:mm");
        if (!slot.isValid()) {
            continue;
        }
        const int minutes = static_cast<int>(now.secsTo(slot)) / 60;
        if (minutes >= -30 && minutes <= 120) {
            const auto* patient =
                m_manager->database().findPatientById(appointment.patientId());
            reminder = QString("Reminder: %1 at %2 (%3). ")
                           .arg(patient ? toQt(patient->name()) : QString("Patient"),
                                toQt(appointment.time()),
                                minutes >= 0 ? QString("in %1 min").arg(minutes)
                                             : QString("started %1 min ago").arg(-minutes));
            break;
        }
    }

    banner += reminder;
    banner += QString("<span style='color:#64748B;'>Use Save (Ctrl+S) to persist, "
                      "Ctrl+Z to undo, and the tabs above to manage records.</span>");
    m_bannerLabel->setText(banner);
}

void DashboardPanel::refresh() {
    updateCards();
    updateUpcoming();
    updateBreakdown();
    updateRecent();
    updateBanner();
}

} // namespace scapms::gui
