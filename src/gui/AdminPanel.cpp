// =============================================================================
//  File   : src/gui/AdminPanel.cpp
// =============================================================================
#include "AdminPanel.h"

#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QVBoxLayout>

#include <memory>
#include <vector>

#include "../controller/ClinicManager.h"
#include "../model/PatientFactory.h"
#include "../utils/Exceptions.h"
#include "../utils/FileHandler.h"
#include "../utils/StringUtils.h"
#include "UiHelpers.h"

namespace scapms::gui {

using scapms::controller::ClinicManager;

AdminPanel::AdminPanel(ClinicManager* manager, QWidget* parent)
    : QWidget(parent), m_manager(manager) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(10);

    auto* header = new QHBoxLayout();
    header->addWidget(createSectionTitle("Administration & Undo", this));
    header->addStretch(1);
    m_dataPathLabel = new QLabel(this);
    m_dataPathLabel->setObjectName("brandSubtitle");
    m_dataPathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    header->addWidget(m_dataPathLabel);
    root->addLayout(header);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setChildrenCollapsible(false);

    auto* leftColumn = new QWidget(splitter);
    auto* leftLayout = new QVBoxLayout(leftColumn);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(10);
    leftLayout->addWidget(createUndoBox());
    leftLayout->addWidget(createDataBox());
    leftLayout->addStretch(1);

    auto* rightColumn = new QWidget(splitter);
    auto* rightLayout = new QVBoxLayout(rightColumn);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(10);
    rightLayout->addWidget(createCacheBox());
    rightLayout->addWidget(createPolymorphismBox());
    rightLayout->addStretch(1);

    splitter->addWidget(leftColumn);
    splitter->addWidget(rightColumn);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({640, 640});
    root->addWidget(splitter, 1);

    refresh();
}

QWidget* AdminPanel::createUndoBox() {
    auto* box = new QGroupBox("Undo last administrative operation (std::stack)", this);
    auto* layout = new QVBoxLayout(box);

    auto* hint = createHintLabel(
        "Every patient deletion, record edit, appointment cancellation or removal "
        "is pushed onto an undo stack. The most recent entry is at the top; the "
        "stack is popped when it is reversed.", box);
    layout->addWidget(hint);

    m_undoList = new QListWidget(box);
    m_undoList->setMaximumHeight(220);
    m_undoList->setToolTip("Most recent operation first.");
    layout->addWidget(m_undoList);

    m_undoHint = createHintLabel("", box);
    layout->addWidget(m_undoHint);

    auto* buttons = new QHBoxLayout();
    m_undoButton = new QPushButton("Undo Last Operation", box);
    m_undoButton->setToolTip("Shortcut: Ctrl+Z. Reverses the operation at the top of "
                             "the stack.");
    auto* refreshButton = new QPushButton("Refresh", box);
    refreshButton->setObjectName("secondaryButton");
    buttons->addWidget(m_undoButton);
    buttons->addStretch(1);
    buttons->addWidget(refreshButton);
    layout->addLayout(buttons);

    connect(m_undoButton, &QPushButton::clicked, this, &AdminPanel::onUndo);
    connect(refreshButton, &QPushButton::clicked, this, &AdminPanel::refresh);
    return box;
}

QWidget* AdminPanel::createDataBox() {
    auto* box = new QGroupBox("Persistence and data files", this);
    auto* layout = new QVBoxLayout(box);

    auto* hint = createHintLabel(
        "The clinic is stored as CSV files (patients.csv, appointments.csv, "
        "services.csv). Save writes the current session; Load reads a data folder "
        "chosen with a file dialog.", box);
    layout->addWidget(hint);

    auto* firstRow = new QHBoxLayout();
    auto* saveButton = new QPushButton("Save Data (Ctrl+S)", box);
    auto* loadButton = new QPushButton("Load Data (Ctrl+O)", box);
    loadButton->setObjectName("secondaryButton");
    firstRow->addWidget(saveButton);
    firstRow->addWidget(loadButton);
    layout->addLayout(firstRow);

    auto* secondRow = new QHBoxLayout();
    auto* importButton = new QPushButton("Import Patients CSV", box);
    importButton->setObjectName("secondaryButton");
    importButton->setToolTip("Merge patient records from a CSV file into the register.");
    auto* exportButton = new QPushButton("Export Patients CSV", box);
    exportButton->setObjectName("secondaryButton");
    auto* newButton = new QPushButton("New Session", box);
    newButton->setObjectName("dangerButton");
    secondRow->addWidget(importButton);
    secondRow->addWidget(exportButton);
    secondRow->addStretch(1);
    secondRow->addWidget(newButton);
    layout->addLayout(secondRow);

    connect(saveButton, &QPushButton::clicked, this, &AdminPanel::onSave);
    connect(loadButton, &QPushButton::clicked, this, &AdminPanel::onLoad);
    connect(importButton, &QPushButton::clicked, this, &AdminPanel::onImportPatients);
    connect(exportButton, &QPushButton::clicked, this, &AdminPanel::onExportPatients);
    connect(newButton, &QPushButton::clicked, this, &AdminPanel::onNewSession);
    return box;
}

QWidget* AdminPanel::createCacheBox() {
    auto* box = new QGroupBox("Frequently accessed patients (fixed array cache)", this);
    auto* layout = new QVBoxLayout(box);

    m_cacheHint = createHintLabel(
        "PatientRecord* recentPatients[10] - the ten most recently opened records, "
        "most recent first. The array is invalidated when a record is deleted.",
        box);
    layout->addWidget(m_cacheHint);

    m_cacheList = new QListWidget(box);
    m_cacheList->setMaximumHeight(220);
    layout->addWidget(m_cacheList);
    return box;
}

QWidget* AdminPanel::createPolymorphismBox() {
    auto* box = new QGroupBox("Runtime polymorphism demonstration", this);
    auto* layout = new QVBoxLayout(box);

    auto* hint = createHintLabel(
        "Runs PatientRecord::describe() through a std::vector<PatientRecord*> - "
        "each call dispatches to the concrete subclass at run time.", box);
    layout->addWidget(hint);

    auto* button = new QPushButton("Run Polymorphism Demo", box);
    button->setObjectName("secondaryButton");
    layout->addWidget(button);
    layout->addStretch(1);

    connect(button, &QPushButton::clicked, this, &AdminPanel::onShowPolymorphism);
    return box;
}

void AdminPanel::updateUndoList() {
    m_undoList->clear();
    const auto history = m_manager->undoHistory();
    for (const auto& entry : history) {
        m_undoList->addItem(toQt(entry));
    }
    if (history.empty()) {
        m_undoList->addItem("(the undo stack is empty)");
    }
    m_undoButton->setEnabled(!history.empty());
    m_undoHint->setText(QString("Stack depth: %1 operation(s). Newest at the top.")
                            .arg(history.size()));
}

void AdminPanel::updateCacheList() {
    m_cacheList->clear();
    const auto recent = m_manager->database().recentPatients();
    int rank = 0;
    for (const auto* record : recent) {
        if (record == nullptr) {
            continue;
        }
        ++rank;
        m_cacheList->addItem(
            QString("%1. %2 - %3 (%4)").arg(rank).arg(toQt(record->id()),
                                                      toQt(record->name()),
                                                      toQt(record->category())));
    }
    if (rank == 0) {
        m_cacheList->addItem("(no patient opened yet - select a row in Patient Records)");
    }
    m_cacheHint->setText(
        QString("PatientRecord* recentPatients[10] - %1/10 slots in use, most "
                "recent first.").arg(rank));
}

void AdminPanel::onUndo() {
    try {
        const std::string description = m_manager->undoLastOperation();
        emit dataChanged();
        refresh();
        emit statusMessage("Undone: " + toQt(description));
        QMessageBox::information(this, "Operation undone",
                                 "Reversed: " + toQt(description));
    } catch (const scapms::utils::ClinicException& error) {
        QMessageBox::information(this, "Nothing to undo", error.what());
    } catch (const std::exception& error) {
        QMessageBox::critical(this, "Undo failed", error.what());
    }
}

void AdminPanel::onSave() {
    try {
        m_manager->save();
        refresh();
        emit statusMessage("Clinic data saved to " + toQt(m_manager->dataDirectory()) +
                           ".");
        QMessageBox::information(
            this, "Data saved",
            "Patient, appointment and service data were saved to:\n" +
                toQt(m_manager->dataDirectory()));
    } catch (const scapms::utils::ClinicException& error) {
        QMessageBox::warning(this, "Save failed", error.what());
    } catch (const std::exception& error) {
        QMessageBox::critical(this, "Save failed", error.what());
    }
}

void AdminPanel::onLoad() {
    const QString directory = QFileDialog::getExistingDirectory(
        this, "Choose the folder containing patients.csv", toQt(m_manager->dataDirectory()));
    if (directory.isEmpty()) {
        return;
    }
    try {
        m_manager->load(directory.toStdString());
        emit dataChanged();
        refresh();
        emit statusMessage("Clinic data loaded from " + directory + ".");
        QMessageBox::information(this, "Data loaded",
                                 QString("Loaded %1 patients and %2 appointments.")
                                     .arg(m_manager->database().patientCount())
                                     .arg(m_manager->database().appointmentCount()));
    } catch (const scapms::utils::ClinicException& error) {
        QMessageBox::warning(this, "Load failed",
                             QString("%1\n\nThe previous session is still loaded.")
                                 .arg(error.what()));
    } catch (const std::exception& error) {
        QMessageBox::critical(this, "Load failed", error.what());
    }
}

void AdminPanel::onNewSession() {
    const auto answer = QMessageBox::question(
        this, "New session",
        "Clear all patients, appointments, queues and undo history?\n\n"
        "Unsaved changes will be lost.",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }
    m_manager->newSession();
    emit dataChanged();
    refresh();
    emit statusMessage("New empty session started.");
}

void AdminPanel::onImportPatients() {
    const QString path = QFileDialog::getOpenFileName(
        this, "Import patients from CSV", toQt(m_manager->dataDirectory()),
        "CSV files (*.csv);;All files (*)");
    if (path.isEmpty()) {
        return;
    }

    int imported = 0;
    QStringList failures;
    try {
        const auto lines = scapms::utils::FileHandler::readLines(path.toStdString());
        int lineNumber = 0;
        for (const auto& line : lines) {
            ++lineNumber;
            const std::string cleaned = scapms::utils::trim(line);
            if (cleaned.empty() || cleaned.front() == '#') {
                continue;
            }
            if (scapms::utils::toLower(cleaned).find("type,id,name") == 0) {
                continue; // header row
            }
            try {
                auto patient = model::PatientFactory::fromCsvLine(cleaned);
                if (m_manager->patientIdExists(patient->id())) {
                    failures << QString("Line %1: ID %2 already exists")
                                    .arg(lineNumber)
                                    .arg(toQt(patient->id()));
                    continue;
                }
                m_manager->registerPatient(std::move(patient));
                ++imported;
            } catch (const scapms::utils::ClinicException& error) {
                failures << QString("Line %1: %2").arg(lineNumber).arg(error.what());
            }
        }
    } catch (const scapms::utils::ClinicException& error) {
        QMessageBox::warning(this, "Import failed", error.what());
        return;
    }

    emit dataChanged();
    refresh();
    emit statusMessage(QString("Imported %1 patient(s) from %2.").arg(imported).arg(path));

    QString message = QString("Imported %1 patient record(s).").arg(imported);
    if (!failures.isEmpty()) {
        message += "\n\nSkipped records:\n" + failures.mid(0, 8).join("\n");
        if (failures.size() > 8) {
            message += QString("\n... and %1 more.").arg(failures.size() - 8);
        }
    }
    QMessageBox::information(this, "Import complete", message);
}

void AdminPanel::onExportPatients() {
    const QString path = QFileDialog::getSaveFileName(
        this, "Export patients to CSV",
        QString("%1/patients-export.csv").arg(toQt(m_manager->dataDirectory())),
        "CSV files (*.csv);;All files (*)");
    if (path.isEmpty()) {
        return;
    }

    std::vector<std::string> lines;
    lines.push_back("# SCAPMS patient export");
    lines.push_back("# type,id,name,age,gender,contact,registeredOn,field1,field2,field3");
    for (const auto* record : m_manager->patients()) {
        if (record != nullptr) {
            lines.push_back(record->serialize());
        }
    }

    try {
        scapms::utils::FileHandler::writeLines(path.toStdString(), lines);
        emit statusMessage("Patients exported to " + path);
        QMessageBox::information(this, "Export complete",
                                 QString("Exported %1 patient(s) to:\n%2")
                                     .arg(m_manager->database().patientCount())
                                     .arg(path));
    } catch (const scapms::utils::ClinicException& error) {
        QMessageBox::warning(this, "Export failed", error.what());
    }
}

void AdminPanel::onShowPolymorphism() {
    const auto lines = m_manager->describeAllPatients();
    QString text = QString("PatientRecord* is the common base type. %1 virtual "
                           "call(s) dispatch to the concrete subclass:\n\n")
                       .arg(lines.size());
    for (const auto& line : lines) {
        text += toQt(line) + "\n";
    }
    if (lines.empty()) {
        text += "(no patients registered)";
    }

    QMessageBox box(this);
    box.setWindowTitle("Runtime polymorphism");
    box.setTextFormat(Qt::PlainText);
    box.setText(text);
    box.setDetailedText(
        "Each line is produced by PatientRecord::describe(), which calls the pure "
        "virtual category(), priorityLevel(), priorityLabel() and "
        "consultationFee(). The actual implementation is chosen at run time from "
        "the dynamic type of the object (GeneralPatient, EmergencyPatient or "
        "FollowUpPatient).");
    box.exec();
}

void AdminPanel::refresh() {
    m_dataPathLabel->setText("Data folder: " + toQt(m_manager->dataDirectory()));
    updateUndoList();
    updateCacheList();
}

} // namespace scapms::gui
