// =============================================================================
//  File   : src/gui/ReportPanel.cpp
// =============================================================================
#include "ReportPanel.h"

#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QDateTime>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QStringList>
#include <QTableWidget>
#include <QVBoxLayout>

#include <vector>

#include "../controller/ClinicManager.h"
#include "../utils/Exceptions.h"
#include "../utils/FileHandler.h"
#include "../utils/StringUtils.h"
#include "UiHelpers.h"

namespace scapms::gui {

using scapms::controller::ClinicManager;
using scapms::controller::ReportGenerator;
using scapms::controller::ReportResult;
using scapms::controller::ReportType;

ReportPanel::ReportPanel(ClinicManager* manager, QWidget* parent)
    : QWidget(parent), m_manager(manager) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(10);

    auto* header = new QHBoxLayout();
    header->addWidget(createSectionTitle("Clinic Reports", this));
    header->addStretch(1);

    header->addWidget(new QLabel("Report:", this));
    m_typeCombo = new QComboBox(this);
    for (const auto type : ReportGenerator::all()) {
        m_typeCombo->addItem(toQt(ReportGenerator::name(type)), static_cast<int>(type));
    }
    m_typeCombo->setMinimumWidth(260);
    header->addWidget(m_typeCombo);

    header->addWidget(new QLabel("Date:", this));
    m_dateEdit = new QDateEdit(QDate::currentDate(), this);
    m_dateEdit->setCalendarPopup(true);
    m_dateEdit->setDisplayFormat("yyyy-MM-dd");
    m_dateEdit->setEnabled(false);
    m_dateEdit->setToolTip("Only the Daily Appointment Summary uses a date.");
    header->addWidget(m_dateEdit);

    auto* generateButton = new QPushButton("Generate Report", this);
    generateButton->setToolTip("Build the selected report from live clinic data.");
    header->addWidget(generateButton);

    m_exportButton = new QPushButton("Export CSV", this);
    m_exportButton->setObjectName("secondaryButton");
    m_exportButton->setToolTip("Save the current report with a file dialog.");
    header->addWidget(m_exportButton);
    root->addLayout(header);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setObjectName("brandTitle");
    root->addWidget(m_titleLabel);

    m_descriptionLabel = createHintLabel("", this);
    root->addWidget(m_descriptionLabel);

    m_table = new QTableWidget(0, 0, this);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    root->addWidget(m_table, 1);

    m_summaryLabel = new QLabel(this);
    m_summaryLabel->setObjectName("bannerLabel");
    m_summaryLabel->setWordWrap(true);
    root->addWidget(m_summaryLabel);

    connect(generateButton, &QPushButton::clicked, this, &ReportPanel::onGenerate);
    connect(m_exportButton, &QPushButton::clicked, this, &ReportPanel::onExportCsv);
    connect(m_typeCombo, &QComboBox::currentIndexChanged, this,
            &ReportPanel::onReportTypeChanged);
    connect(m_dateEdit, &QDateEdit::dateChanged, this, [this](const QDate&) {
        if (m_generated && currentType() == ReportType::DailySummary) {
            onGenerate();
        }
    });

    m_exportButton->setEnabled(false);
    generate(false);
}

ReportType ReportPanel::currentType() const {
    return static_cast<ReportType>(m_typeCombo->currentData().toInt());
}

void ReportPanel::onReportTypeChanged(int /*index*/) {
    m_dateEdit->setEnabled(currentType() == ReportType::DailySummary);
    onGenerate();
}

void ReportPanel::onGenerate() {
    generate(true);
}

void ReportPanel::generate(bool notify) {
    try {
        const std::string date =
            currentType() == ReportType::DailySummary
                ? m_dateEdit->date().toString("yyyy-MM-dd").toStdString()
                : std::string();
        const ReportResult report = m_manager->report(currentType(), date);
        m_generated = true;
        render(report);
        if (notify) {
            emit statusMessage("Report generated: " + toQt(report.title));
        }
    } catch (const std::exception& error) {
        QMessageBox::critical(this, "Report generation failed", error.what());
    }
}

void ReportPanel::render(const ReportResult& report) {
    m_currentReport = report;
    m_titleLabel->setText(toQt(report.title));
    m_descriptionLabel->setText(toQt(report.description));

    m_table->setUpdatesEnabled(false);
    m_table->clear();
    m_table->setColumnCount(static_cast<int>(report.headers.size()));
    QStringList headers;
    for (const auto& header : report.headers) {
        headers << toQt(header);
    }
    m_table->setHorizontalHeaderLabels(headers);
    m_table->setRowCount(static_cast<int>(report.rows.size()));

    for (std::size_t row = 0; row < report.rows.size(); ++row) {
        for (std::size_t column = 0; column < report.rows[row].size(); ++column) {
            auto* item = new QTableWidgetItem(toQt(report.rows[row][column]));
            item->setTextAlignment((Qt::AlignLeft | Qt::AlignVCenter));
            m_table->setItem(static_cast<int>(row), static_cast<int>(column), item);
        }
        m_table->setRowHeight(static_cast<int>(row), 32);
    }
    m_table->setUpdatesEnabled(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    QStringList summaryLines;
    for (const auto& line : report.summary) {
        summaryLines << "  \u2022 " + toQt(line);
    }
    m_summaryLabel->setText(summaryLines.join("\n"));
    m_exportButton->setEnabled(true);
}

void ReportPanel::onExportCsv() {
    if (!m_generated) {
        QMessageBox::information(this, "No report", "Generate a report first.");
        return;
    }
    const QString suggested =
        QString("scapms-report-%1.csv")
            .arg(QDateTime::currentDateTime().toString("yyyyMMdd-HHmm"));
    const QString path = QFileDialog::getSaveFileName(
        this, "Export report to CSV", suggested, "CSV files (*.csv);;All files (*)");
    if (path.isEmpty()) {
        return;
    }

    try {
        const std::string csv = m_currentReport.toCsv();
        std::vector<std::string> lines;
        std::string current;
        for (const char c : csv) {
            if (c == '\n') {
                lines.push_back(current);
                current.clear();
            } else if (c != '\r') {
                current.push_back(c);
            }
        }
        if (!current.empty()) {
            lines.push_back(current);
        }
        scapms::utils::FileHandler::writeLines(path.toStdString(), lines);
        emit statusMessage("Report exported to " + path);
        QMessageBox::information(this, "Report exported",
                                 "The report was exported to:\n" + path);
    } catch (const scapms::utils::ClinicException& error) {
        QMessageBox::warning(this, "Export failed", error.what());
    } catch (const std::exception& error) {
        QMessageBox::critical(this, "Export failed", error.what());
    }
}

void ReportPanel::refresh() {
    if (m_generated) {
        generate(false);
    }
}

} // namespace scapms::gui
