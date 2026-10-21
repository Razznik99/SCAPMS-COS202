// =============================================================================
//  File   : tests/SelfTest.cpp
//  Purpose: Console test suite (no GUI). Every check prints its result; the exit
//           code is the number of failures.
// =============================================================================
#include "SelfTest.h"

#include <QCoreApplication>
#include <QDir>
#include <QString>
#include <QStringList>
#include <QTextStream>

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include "../src/controller/AppointmentController.h"
#include "../src/controller/ClinicManager.h"
#include "../src/controller/DemoData.h"
#include "../src/controller/ReportGenerator.h"
#include "../src/controller/SearchEngine.h"
#include "../src/controller/SortEngine.h"
#include "../src/model/Appointable.h"
#include "../src/model/EmergencyPatient.h"
#include "../src/model/FollowUpPatient.h"
#include "../src/model/GeneralPatient.h"
#include "../src/utils/DateUtils.h"
#include "../src/utils/Exceptions.h"
#include "../src/utils/FileHandler.h"
#include "../src/utils/IDGenerator.h"
#include "../src/utils/StringUtils.h"

namespace scapms::tests {

namespace {

using scapms::controller::ClinicManager;
using scapms::controller::SearchEngine;
using scapms::controller::SortEngine;
using scapms::model::Appointable;
using scapms::model::Appointment;
using scapms::model::AppointmentStatus;
using scapms::model::EmergencyPatient;
using scapms::model::FollowUpPatient;
using scapms::model::GeneralPatient;
using scapms::model::PatientRecord;

int g_passed = 0;
int g_failed = 0;
QStringList g_failures;
QString g_section = "general";

QTextStream& out() {
    static QTextStream stream(stdout);
    return stream;
}

void section(const QString& name) {
    g_section = name;
    out() << "\n-- " << name << " --\n";
    out().flush();
}

void check(bool condition, const QString& label) {
    if (condition) {
        ++g_passed;
    } else {
        ++g_failed;
        g_failures << QString("[%1] %2").arg(g_section, label);
    }
}

std::unique_ptr<GeneralPatient> general(const std::string& id, const std::string& name,
                                        int age = 30,
                                        const std::string& contact = "+234 803 555 0100") {
    return std::make_unique<GeneralPatient>(id, name, age, contact, "Female",
                                           scapms::utils::todayIso());
}

std::unique_ptr<EmergencyPatient> emergency(const std::string& id,
                                            const std::string& name, int triage = 2) {
    return std::make_unique<EmergencyPatient>(
        id, name, 40, "+234 805 555 0101", "Male", scapms::utils::todayIso(), triage,
        "Ambulance", true);
}

std::unique_ptr<FollowUpPatient> followUp(const std::string& id, const std::string& name) {
    return std::make_unique<FollowUpPatient>(
        id, name, 35, "+234 807 555 0102", "Female", scapms::utils::todayIso(),
        scapms::utils::addDays(scapms::utils::todayIso(), -10), 14, "Dr. Adeyemi");
}

} // namespace

#define SC_CHECK(expr) ::scapms::tests::check((expr), QString::fromUtf8(#expr))
#define SC_CHECK_MSG(expr, message) \
    ::scapms::tests::check((expr), QString::fromUtf8(message))
#define SC_CHECK_THROWS(expr, ExceptionType)                                       \
    do {                                                                           \
        bool caught = false;                                                       \
        try {                                                                      \
            (void)(expr);                                                          \
        } catch (const ExceptionType&) {                                           \
            caught = true;                                                         \
        } catch (...) {                                                            \
        }                                                                          \
        ::scapms::tests::check(caught, QString("throws " #ExceptionType ": " #expr)); \
    } while (false)

int runAllTests() {
    using namespace scapms::utils;

    const QString tempRoot = QDir::tempPath() + "/scapms_selftest_" +
                             QString::number(QCoreApplication::applicationPid());
    QDir(tempRoot).removeRecursively();
    QDir().mkpath(tempRoot);

    out() << "SCAPMS self-test suite\n";
    out() << "Data directory for persistence tests: " << tempRoot << "\n";

    // =========================================================================
    section("1. Patient hierarchy, validation and polymorphism");
    // =========================================================================
    {
        auto patient = general("P-9001", "Amina Bello", 34);
        SC_CHECK(patient->isValid());
        SC_CHECK(patient->category() == "General");
        SC_CHECK(patient->priorityLevel() == 3);
        SC_CHECK(patient->priorityLabel() == "Normal");
        SC_CHECK(std::abs(patient->consultationFee() - 2500.0) < 0.001);
        SC_CHECK(patient->typeTag() == "GENERAL");

        auto clone = patient->clone();
        SC_CHECK(clone != nullptr);
        SC_CHECK(clone->id() == patient->id());
        SC_CHECK(clone->describe() == patient->describe());
        SC_CHECK(dynamic_cast<Appointable*>(patient.get()) != nullptr);

        auto emergencyPatient = emergency("P-9002", "Chinedu Okafor", 1);
        SC_CHECK(emergencyPatient->category() == "Emergency");
        SC_CHECK(emergencyPatient->priorityLevel() == 1);
        SC_CHECK(emergencyPatient->consultationFee() > patient->consultationFee());
        SC_CHECK(dynamic_cast<Appointable*>(emergencyPatient.get()) == nullptr);

        auto followUpPatient = followUp("P-9003", "Fatima Yusuf");
        SC_CHECK(followUpPatient->category() == "Follow-Up");
        SC_CHECK(followUpPatient->priorityLevel() == 2);
        SC_CHECK(dynamic_cast<Appointable*>(followUpPatient.get()) != nullptr);
        SC_CHECK(followUpPatient->dueDate() ==
                 scapms::utils::addDays(followUpPatient->originalVisitDate(), 14));

        // The base-class pointer must dispatch to the concrete overrides.
        std::vector<std::unique_ptr<PatientRecord>> records;
        records.push_back(general("P-9101", "Grace Ade"));
        records.push_back(emergency("P-9102", "Musa Ibrahim"));
        records.push_back(followUp("P-9103", "Ngozi Eze"));
        std::vector<std::string> categories;
        for (const auto& record : records) {
            categories.push_back(record->category());
            SC_CHECK(record->consultationFee() > 0.0);
            SC_CHECK(!record->clinicalSummary().empty());
            SC_CHECK(!record->describe().empty());
        }
        SC_CHECK(categories.size() == 3);
        SC_CHECK(categories[0] == "General" && categories[1] == "Emergency" &&
                 categories[2] == "Follow-Up");

        // Validation failures.
        auto badAge = general("P-9201", "Bad Age", -1);
        SC_CHECK_THROWS(badAge->validate(), ValidationException);
        auto oldAge = general("P-9202", "Old Age", 130);
        SC_CHECK_THROWS(oldAge->validate(), ValidationException);
        auto emptyName = general("P-9203", "A");
        SC_CHECK_THROWS(emptyName->validate(), ValidationException);
        auto digitName = general("P-9204", "Patient 7");
        SC_CHECK_THROWS(digitName->validate(), ValidationException);
        auto badContact = general("P-9205", "Ada Bello", 30, "12");
        SC_CHECK_THROWS(badContact->validate(), ValidationException);
        auto badTriage = emergency("P-9206", "Eve Okafor", 9);
        SC_CHECK_THROWS(badTriage->validate(), ValidationException);
        auto futureVisit = std::make_unique<FollowUpPatient>(
            "P-9207", "Ada Obi", 30, "+234 807 555 0102", "Female",
            scapms::utils::todayIso(), scapms::utils::addDays(scapms::utils::todayIso(), 5),
            14, "Dr. Adeyemi");
        SC_CHECK_THROWS(futureVisit->validate(), ValidationException);

        // Custom exception hierarchy.
        bool hierarchyOk = false;
        try {
            throw DuplicateIdException("P-9001");
        } catch (const ClinicException& error) {
            hierarchyOk = error.code() == "DUPLICATE_ID";
        }
        SC_CHECK(hierarchyOk);
    }

    // =========================================================================
    section("2. Appointable interface and appointment rules");
    // =========================================================================
    {
        ClinicManager manager((tempRoot + "/apt").toStdString());
        manager.registerPatient(general("P-1001", "Amina Bello"));
        manager.registerPatient(general("P-1002", "Bola Chan"));
        manager.registerPatient(emergency("P-1003", "Chidi Okeke"));

        const std::string tomorrow = scapms::utils::addDays(scapms::utils::todayIso(), 1);
        const auto appointment =
            manager.scheduleAppointment("P-1001", tomorrow, "09:00", "GEN-CON",
                                        "Dr. Test", "first visit");
        SC_CHECK(!appointment.id().empty());
        SC_CHECK(appointment.isPending());
        SC_CHECK(appointment.fee() > 0.0);
        SC_CHECK(manager.database().appointmentCount() == 1);

        auto* patient = manager.patient("P-1001");
        auto* appointable = dynamic_cast<Appointable*>(patient);
        SC_CHECK(appointable != nullptr);
        SC_CHECK(appointable != nullptr && appointable->appointmentCount() == 1);

        // Clinician double-booking.
        SC_CHECK_THROWS(
            manager.scheduleAppointment("P-1002", tomorrow, "09:00", "GEN-CON",
                                        "Dr. Test", ""),
            AppointmentException);
        // Same patient, same slot, different clinician.
        SC_CHECK_THROWS(
            manager.scheduleAppointment("P-1001", tomorrow, "09:00", "LAB-BLD",
                                        "Dr. Other", ""),
            AppointmentException);
        // Past date.
        SC_CHECK_THROWS(
            manager.scheduleAppointment("P-1002",
                                        scapms::utils::addDays(scapms::utils::todayIso(), -1),
                                        "09:00", "GEN-CON", "Dr. Test", ""),
            AppointmentException);
        // Emergency patients are walk-in only.
        SC_CHECK_THROWS(
            manager.scheduleAppointment("P-1003", tomorrow, "10:00", "GEN-CON",
                                        "Dr. Test", ""),
            AppointmentException);
        // Unknown patient / service.
        SC_CHECK_THROWS(
            manager.scheduleAppointment("P-9999", tomorrow, "10:00", "GEN-CON",
                                        "Dr. Test", ""),
            NotFoundException);
        SC_CHECK_THROWS(
            manager.scheduleAppointment("P-1002", tomorrow, "11:00", "NOPE",
                                        "Dr. Test", ""),
            NotFoundException);
        // Malformed date / time.
        SC_CHECK_THROWS(
            manager.scheduleAppointment("P-1002", "2026-02-31", "09:00", "GEN-CON",
                                        "Dr. Test", ""),
            AppointmentException);
        SC_CHECK_THROWS(
            manager.scheduleAppointment("P-1002", tomorrow, "25:00", "GEN-CON",
                                        "Dr. Test", ""),
            AppointmentException);

        // Status transitions + undo of a cancellation.
        manager.cancelAppointment(appointment.id());
        SC_CHECK(manager.database().findAppointmentById(appointment.id())->isCancelled());
        manager.undoLastOperation();
        SC_CHECK(manager.database().findAppointmentById(appointment.id())->isPending());

        manager.completeAppointment(appointment.id());
        SC_CHECK(manager.database().findAppointmentById(appointment.id())->isCompleted());
        SC_CHECK_THROWS(manager.cancelAppointment(appointment.id()), AppointmentException);

        // Deleting removes the appointment from the patient association.
        manager.removeAppointment(appointment.id());
        SC_CHECK(manager.database().appointmentCount() == 0);
        auto* reloadedPatient = manager.patient("P-1001");
        auto* reloadedAppointable = dynamic_cast<Appointable*>(reloadedPatient);
        SC_CHECK(reloadedAppointable != nullptr &&
                 reloadedAppointable->appointmentCount() == 0);
        manager.undoLastOperation();
        SC_CHECK(manager.database().appointmentCount() == 1);
    }

    // =========================================================================
    section("3. Searching algorithms");
    // =========================================================================
    {
        std::vector<std::unique_ptr<PatientRecord>> owned;
        const std::vector<std::string> names{"Zara Ahmed", "Bola Chan", "Chidi Eze",
                                             "Damilola Ojo", "Emeka Obi", "Fatima Sani",
                                             "Grace Umeh", "Halima Yusuf"};
        for (std::size_t i = 0; i < names.size(); ++i) {
            owned.push_back(general("P-" + std::to_string(2000 + i), names[i],
                                    static_cast<int>(20 + i)));
        }
        std::vector<PatientRecord*> records;
        for (const auto& record : owned) {
            records.push_back(record.get());
        }

        const auto linear = SearchEngine::linearSearchById(records, "P-2004");
        SC_CHECK(linear.matches.size() == 1);
        SC_CHECK(linear.stats.comparisons == 5);
        SC_CHECK(linear.matches.size() == 1 && linear.matches.front()->id() == "P-2004");

        const auto missing = SearchEngine::linearSearchById(records, "P-9999");
        SC_CHECK(missing.matches.empty());
        SC_CHECK(missing.stats.comparisons == records.size());
        SC_CHECK_THROWS(SearchEngine::linearSearchById(records, "   "), ValidationException);

        // Binary search refuses unsorted input (technique depends on data state).
        SC_CHECK(!SearchEngine::isSortedByName(records));
        SC_CHECK_THROWS(SearchEngine::binarySearchByName(records, "Bola Chan"),
                        ValidationException);

        SortEngine::sort(records, controller::SortField::PatientName,
                         controller::SortAlgorithm::Merge);
        SC_CHECK(SearchEngine::isSortedByName(records));

        const auto binary = SearchEngine::binarySearchByName(records, "bola chan");
        SC_CHECK(binary.matches.size() == 1);
        SC_CHECK(binary.matches.front()->name() == "Bola Chan");
        SC_CHECK(binary.usedSortedData);
        SC_CHECK(binary.stats.comparisons <= 6); // log2(8) + 1

        const auto binaryMissing = SearchEngine::binarySearchByName(records, "Nobody Here");
        SC_CHECK(binaryMissing.matches.empty());

        const auto recursive = SearchEngine::recursiveSearchByName(records, "Obi");
        SC_CHECK(recursive.matches.size() == 1);
        SC_CHECK(recursive.stats.comparisons == records.size());
        const auto recursiveMany = SearchEngine::recursiveSearchByName(records, "a");
        SC_CHECK(recursiveMany.matches.size() >= 5);
        SC_CHECK_THROWS(SearchEngine::recursiveSearchByName(records, " "),
                        ValidationException);

        const auto category = SearchEngine::searchByCategory(records, "General");
        SC_CHECK(category.matches.size() == records.size());
        const auto categoryEmpty = SearchEngine::searchByCategory(records, "Emergency");
        SC_CHECK(categoryEmpty.matches.empty());

        // Auto mode: sorted input -> binary, unsorted -> linear.
        const auto automatic = SearchEngine::autoSearch(records, "Grace Umeh");
        SC_CHECK(automatic.matches.size() == 1);
        SC_CHECK(automatic.usedSortedData);

        const auto automaticById = SearchEngine::autoSearch(records, "P-2001");
        SC_CHECK(automaticById.matches.size() == 1);
        SC_CHECK(!automaticById.matches.empty() &&
                 automaticById.matches.front()->id() == "P-2001");
    }

    // =========================================================================
    section("4. Sorting algorithms (four algorithms, four keys)");
    // =========================================================================
    {
        const std::vector<std::string> firstNames{"Ada", "Bola", "Chidi", "Dami",
                                                  "Emeka", "Fola", "Grace", "Halima",
                                                  "Ife", "Jide", "Kemi", "Lola"};
        const std::vector<std::string> lastNames{"Ahmed", "Bello", "Chan", "Eze",
                                                 "Obi", "Sani"};

        std::vector<std::unique_ptr<PatientRecord>> owned;
        int index = 0;
        for (const auto& first : firstNames) {
            for (const auto& last : lastNames) {
                const std::string id = "P-" + std::to_string(3000 + index);
                owned.push_back(general(id, first + " " + last,
                                        (index * 7) % 80 + 1));
                ++index;
            }
        }

        std::vector<PatientRecord*> base;
        for (const auto& record : owned) {
            base.push_back(record.get());
        }
        const std::size_t count = base.size();
        SC_CHECK(count == 72);

        const auto lookup = [&owned](const std::string& patientId) {
            for (std::size_t i = 0; i < owned.size(); ++i) {
                if (owned[i]->id() == patientId) {
                    return scapms::utils::addDays(scapms::utils::todayIso(),
                                                  -static_cast<int>(i % 9));
                }
            }
            return std::string();
        };

        const std::vector<controller::SortAlgorithm> algorithms{
            controller::SortAlgorithm::Selection, controller::SortAlgorithm::Insertion,
            controller::SortAlgorithm::Merge, controller::SortAlgorithm::Quick};
        const std::vector<controller::SortField> fields{
            controller::SortField::PatientName, controller::SortField::PatientId,
            controller::SortField::Age, controller::SortField::AppointmentDate};

        std::vector<std::string> referenceOrder;
        bool firstRun = true;
        for (const auto algorithm : algorithms) {
            for (const auto field : fields) {
                std::vector<PatientRecord*> records = base;
                const auto outcome = SortEngine::sort(records, field, algorithm, lookup);
                SC_CHECK(outcome.stats.comparisons > 0);
                SC_CHECK(outcome.itemCount == records.size());

                bool sorted = true;
                for (std::size_t i = 1; i < records.size(); ++i) {
                    if (SortEngine::less(records[i], records[i - 1], field, lookup)) {
                        sorted = false;
                        break;
                    }
                }
                SC_CHECK_MSG(sorted, "records are ordered after sorting");
                SC_CHECK(records.size() == count);

                std::vector<std::string> order;
                order.reserve(records.size());
                for (const auto* record : records) {
                    order.push_back(record->id());
                }
                if (field == controller::SortField::PatientName && firstRun) {
                    referenceOrder = order;
                    firstRun = false;
                }
                if (field == controller::SortField::PatientName) {
                    SC_CHECK(order == referenceOrder); // all algorithms agree
                }
            }
        }

        // Insertion sort is adaptive: already sorted data needs fewer comparisons.
        std::vector<PatientRecord*> preSorted = base;
        SortEngine::sort(preSorted, controller::SortField::PatientName,
                         controller::SortAlgorithm::Merge);
        const auto adaptive = SortEngine::sort(preSorted,
                                               controller::SortField::PatientName,
                                               controller::SortAlgorithm::Insertion);
        SC_CHECK(adaptive.stats.comparisons < count * 2);

        // Sorting a single record and an empty vector must not crash.
        std::vector<PatientRecord*> single{base.front()};
        SC_CHECK(SortEngine::sort(single, controller::SortField::Age,
                                  controller::SortAlgorithm::Quick)
                     .stats.comparisons == 0);
        std::vector<PatientRecord*> empty;
        SC_CHECK(SortEngine::sort(empty, controller::SortField::Age,
                                  controller::SortAlgorithm::Merge)
                     .stats.moves == 0);
    }

    // =========================================================================
    section("5. Waiting queue (std::queue)");
    // =========================================================================
    {
        ClinicManager manager((tempRoot + "/queue").toStdString());
        manager.registerPatient(general("P-4001", "Amina Bello"));
        manager.registerPatient(general("P-4002", "Bola Chan"));
        manager.registerPatient(general("P-4003", "Chidi Eze"));
        manager.registerPatient(emergency("P-4004", "Musa Ibrahim"));

        manager.joinQueue("P-4001");
        manager.joinQueue("P-4002");
        manager.joinQueue("P-4004");
        manager.joinQueue("P-4003");
        SC_CHECK(manager.queueLength() == 4);
        SC_CHECK(manager.priorityQueueLength() == 1);
        SC_CHECK(manager.isInQueue("P-4001"));
        SC_CHECK_THROWS(manager.joinQueue("P-4001"), ValidationException);

        const auto snapshot = manager.queueSnapshot();
        SC_CHECK(snapshot.size() == 4);
        SC_CHECK(snapshot.front().patientId == "P-4004"); // emergency first
        SC_CHECK(snapshot[1].patientId == "P-4001");      // then FIFO
        SC_CHECK(snapshot[2].patientId == "P-4002");
        SC_CHECK(snapshot[3].patientId == "P-4003");
        SC_CHECK(snapshot.front().priorityCase);

        controller::QueueEntry served;
        SC_CHECK(manager.callNextPatient(served));
        SC_CHECK(served.patientId == "P-4004");
        SC_CHECK(manager.callNextPatient(served));
        SC_CHECK(served.patientId == "P-4001");
        SC_CHECK(manager.queueLength() == 2);

        SC_CHECK(manager.removeFromQueue("P-4002"));
        SC_CHECK(!manager.isInQueue("P-4002"));
        SC_CHECK(!manager.removeFromQueue("P-4002"));

        // Deleting a patient removes them from the queue.
        SC_CHECK(manager.isInQueue("P-4003"));
        manager.deletePatient("P-4003");
        SC_CHECK(!manager.isInQueue("P-4003"));

        // Every remaining patient has been served or removed.
        SC_CHECK(manager.queueLength() == 0);
        SC_CHECK(!manager.callNextPatient(served));
    }

    // =========================================================================
    section("6. Undo stack for administrative operations");
    // =========================================================================
    {
        ClinicManager manager((tempRoot + "/undo").toStdString());
        manager.registerPatient(general("P-5001", "Amina Bello"));
        manager.registerPatient(general("P-5002", "Bola Chan"));
        const std::string tomorrow = scapms::utils::addDays(scapms::utils::todayIso(), 1);
        manager.scheduleAppointment("P-5001", tomorrow, "09:00", "GEN-CON", "Dr. Undo",
                                    "");

        SC_CHECK(!manager.canUndo());
        SC_CHECK_THROWS(manager.undoLastOperation(), ValidationException);

        // 1. delete patient (cascades to the appointment) then undo.
        manager.deletePatient("P-5001");
        SC_CHECK(manager.database().patientCount() == 1);
        SC_CHECK(manager.database().appointmentCount() == 0);
        SC_CHECK(manager.canUndo());
        SC_CHECK(manager.undoDepth() == 1);
        const std::string description = manager.undoLastOperation();
        SC_CHECK(description.find("Delete patient") != std::string::npos);
        SC_CHECK(manager.database().patientCount() == 2);
        SC_CHECK(manager.database().appointmentCount() == 1);
        SC_CHECK(manager.database().findPatientById("P-5001") != nullptr);
        auto* restored = dynamic_cast<Appointable*>(manager.patient("P-5001"));
        SC_CHECK(restored != nullptr && restored->appointmentCount() == 1);
        SC_CHECK(!manager.canUndo());

        // 2. modify patient then undo restores the previous version.
        auto updated = general("P-5001", "Amina Bello-Updated", 50);
        manager.updatePatient("P-5001", std::move(updated));
        SC_CHECK(manager.database().findPatientById("P-5001")->name() == "Amina Bello-Updated");
        SC_CHECK(manager.undoLastOperation().find("Modify patient") != std::string::npos);
        SC_CHECK(manager.database().findPatientById("P-5001")->name() == "Amina Bello");
        SC_CHECK(manager.database().findPatientById("P-5001")->age() == 30);

        // 3. cancel appointment then undo restores Pending.
        const std::string appointmentId =
            manager.database().appointments().front().id();
        manager.cancelAppointment(appointmentId);
        SC_CHECK(manager.database().findAppointmentById(appointmentId)->isCancelled());
        manager.undoLastOperation();
        SC_CHECK(manager.database().findAppointmentById(appointmentId)->isPending());

        // 4. duplicate IDs are rejected before touching the stack.
        SC_CHECK_THROWS(manager.registerPatient(general("P-5002", "Copy Cat")),
                        DuplicateIdException);
        SC_CHECK(manager.undoDepth() == 0);
    }

    // =========================================================================
    section("7. Frequently-accessed patient cache (fixed array)");
    // =========================================================================
    {
        ClinicManager manager((tempRoot + "/cache").toStdString());
        for (int i = 0; i < 12; ++i) {
            manager.registerPatient(general(
                "P-" + std::to_string(6000 + i),
                "Cache Patient" + std::string(1, static_cast<char>('A' + i))));
        }
        for (int i = 0; i < 12; ++i) {
            (void)manager.patient("P-" + std::to_string(6000 + i));
        }
        SC_CHECK(manager.database().recentCount() == 10);
        const auto recent = manager.database().recentPatients();
        SC_CHECK(recent.size() == 10);
        SC_CHECK(recent.front() != nullptr && recent.front()->id() == "P-6011");
        SC_CHECK(recent.back() != nullptr && recent.back()->id() == "P-6002");

        // Re-touching an existing entry moves it to the front without growing.
        (void)manager.patient("P-6005");
        SC_CHECK(manager.database().recentCount() == 10);
        SC_CHECK(manager.database().recentPatients().front()->id() == "P-6005");

        // Deleting invalidates the cached pointer.
        manager.deletePatient("P-6005");
        SC_CHECK(manager.database().recentCount() == 9);
        for (const auto* record : manager.database().recentPatients()) {
            SC_CHECK(record == nullptr || record->id() != "P-6005");
        }
    }

    // =========================================================================
    section("8. Persistence (CSV save / load, failure handling)");
    // =========================================================================
    {
        const QString dataDir = tempRoot + "/persist";
        ClinicManager manager(dataDir.toStdString());
        controller::DemoData::seed(manager);
        const auto originalStats = manager.stats();
        SC_CHECK(originalStats.totalPatients == 14);
        SC_CHECK(originalStats.totalAppointments == 25);

        manager.save();
        SC_CHECK(FileHandler::exists((dataDir + "/patients.csv").toStdString()));
        SC_CHECK(FileHandler::exists((dataDir + "/appointments.csv").toStdString()));
        SC_CHECK(FileHandler::exists((dataDir + "/services.csv").toStdString()));

        ClinicManager reloaded(dataDir.toStdString());
        reloaded.load();
        const auto reloadedStats = reloaded.stats();
        SC_CHECK(reloadedStats.totalPatients == originalStats.totalPatients);
        SC_CHECK(reloadedStats.totalAppointments == originalStats.totalAppointments);
        SC_CHECK(reloadedStats.pendingAppointments == originalStats.pendingAppointments);
        SC_CHECK(reloadedStats.completedAppointments == originalStats.completedAppointments);
        SC_CHECK(reloadedStats.emergencyPatients == originalStats.emergencyPatients);
        SC_CHECK(reloaded.database().findPatientById("P-0001") != nullptr);
        SC_CHECK(reloaded.database().findPatientById("P-0001")->name() == "Amina Bello");
        SC_CHECK(reloaded.database().appointments().size() == 25);

        // Associations are rebuilt on load.
        auto* appointable = dynamic_cast<Appointable*>(reloaded.patient("P-0001"));
        SC_CHECK(appointable != nullptr && appointable->appointmentCount() >= 2);

        // Round-trip a patient edit.
        auto* before = reloaded.database().findPatientById("P-0005");
        const double feeBefore = before != nullptr ? before->consultationFee() : 0.0;
        SC_CHECK(feeBefore > 0.0);

        // Missing folder -> FileException.
        ClinicManager missing((tempRoot + "/does-not-exist").toStdString());
        SC_CHECK_THROWS(missing.load(), FileException);

        // Corrupt record -> FileException mentioning the line.
        const QString badDir = tempRoot + "/corrupt";
        QDir().mkpath(badDir);
        FileHandler::writeLines((badDir + "/patients.csv").toStdString(),
                                {"# test", "NOT_A_TYPE,JUNK,1"});
        FileHandler::writeLines((badDir + "/appointments.csv").toStdString(), {"# none"});
        ClinicManager corrupt(badDir.toStdString());
        SC_CHECK_THROWS(corrupt.load(), FileException);

        // A cancelled appointment survives the round trip.
        bool foundCancelled = false;
        for (const auto& appointment : reloaded.database().appointments()) {
            if (appointment.isCancelled()) {
                foundCancelled = true;
                break;
            }
        }
        SC_CHECK(foundCancelled);
    }

    // =========================================================================
    section("9. Reports");
    // =========================================================================
    {
        ClinicManager manager((tempRoot + "/reports").toStdString());
        controller::DemoData::seed(manager);
        const auto stats = manager.stats();

        const auto attendance = manager.report(controller::ReportType::FrequentAttendees);
        SC_CHECK(!attendance.rows.empty());
        SC_CHECK(attendance.headers.size() == 6);
        SC_CHECK(!attendance.toPlainText().empty());
        SC_CHECK(!attendance.toCsv().empty());
        int totalCompleted = 0;
        for (const auto& row : attendance.rows) {
            totalCompleted += std::stoi(row[4]);
        }
        SC_CHECK(totalCompleted == static_cast<int>(stats.completedAppointments));

        const auto pending = manager.report(controller::ReportType::PendingAppointments);
        SC_CHECK(pending.rows.size() == stats.pendingAppointments);

        const auto distribution =
            manager.report(controller::ReportType::CategoryDistribution);
        SC_CHECK(distribution.rows.size() == 3);
        int patientsFromCategories = 0;
        for (const auto& row : distribution.rows) {
            patientsFromCategories += std::stoi(row[1]);
        }
        SC_CHECK(patientsFromCategories == static_cast<int>(stats.totalPatients));

        const auto daily = manager.report(controller::ReportType::DailySummary,
                                          scapms::utils::todayIso());
        SC_CHECK(daily.rows.size() == stats.todayAppointments);
        int completed = 0;
        int pendingCount = 0;
        int cancelled = 0;
        for (const auto& row : daily.rows) {
            if (row[5] == "Completed") {
                ++completed;
            } else if (row[5] == "Pending") {
                ++pendingCount;
            } else if (row[5] == "Cancelled") {
                ++cancelled;
            }
        }
        SC_CHECK(completed + pendingCount + cancelled ==
                 static_cast<int>(daily.rows.size()));
        SC_CHECK(daily.summary.size() >= 6);

        const auto revenue = manager.report(controller::ReportType::RevenueByService);
        SC_CHECK(!revenue.rows.empty());
        SC_CHECK(revenue.headers.size() == 6);
    }

    // =========================================================================
    section("10. Statistics and polymorphic description");
    // =========================================================================
    {
        ClinicManager manager((tempRoot + "/stats").toStdString());
        controller::DemoData::seed(manager);
        const auto stats = manager.stats();
        SC_CHECK(stats.totalPatients ==
                 stats.generalPatients + stats.emergencyPatients + stats.followUpPatients);
        SC_CHECK(stats.totalAppointments == stats.pendingAppointments +
                                                stats.completedAppointments +
                                                stats.cancelledAppointments);
        SC_CHECK(stats.queueLength > 0);
        SC_CHECK(stats.revenueCollected > 0.0);
        SC_CHECK(stats.patientsWithPending > 0);

        const auto descriptions = manager.describeAllPatients();
        SC_CHECK(descriptions.size() == stats.totalPatients);
        if (!descriptions.empty()) {
            SC_CHECK(descriptions.front().find("priority") != std::string::npos);
        }
    }

    // =========================================================================
    section("11. String / date utilities");
    // =========================================================================
    {
        SC_CHECK(scapms::utils::trim("  hello \t") == "hello");
        SC_CHECK(scapms::utils::toLower("AbC") == "abc");
        SC_CHECK(scapms::utils::containsIgnoreCase("Amina Bello", "bello"));
        SC_CHECK(scapms::utils::startsWithIgnoreCase("Emergency", "emer"));
        SC_CHECK(scapms::utils::split("a,b,c", ',').size() == 3);
        SC_CHECK(scapms::utils::join({"a", "b"}, "-") == "a-b");
        SC_CHECK(scapms::utils::escapeCsv("a,b") == "\"a,b\"");

        const auto fields = scapms::utils::parseCsvLine("1,\"Smith, John\",x,\"say \"\"hi\"\"\"");
        SC_CHECK(fields.size() == 4);
        SC_CHECK(fields[1] == "Smith, John");
        SC_CHECK(fields[3] == "say \"hi\"");

        const std::string today = scapms::utils::todayIso();
        SC_CHECK(scapms::utils::isValidIsoDate(today));
        SC_CHECK(!scapms::utils::isValidIsoDate("2026-02-30"));
        SC_CHECK(!scapms::utils::isValidIsoDate("2026-13-01"));
        SC_CHECK(scapms::utils::isValidIsoDate("2024-02-29"));
        SC_CHECK(!scapms::utils::isValidIsoDate("2025-02-29"));
        SC_CHECK(scapms::utils::isValidIsoTime("23:59"));
        SC_CHECK(!scapms::utils::isValidIsoTime("24:00"));
        SC_CHECK(scapms::utils::addDays("2026-01-31", 1) == "2026-02-01");
        SC_CHECK(scapms::utils::addDays("2026-03-01", -1) == "2026-02-28");
        SC_CHECK(scapms::utils::daysBetween("2026-01-01", "2026-01-31") == 30);
        SC_CHECK(scapms::utils::isPastDate(scapms::utils::addDays(today, -1)));
        SC_CHECK(!scapms::utils::isPastDate(today));
        SC_CHECK(scapms::utils::formatMoney(1234567.5) == "1,234,567.50");
        SC_CHECK(scapms::utils::isAllDigits("0123"));
        SC_CHECK(!scapms::utils::isAllDigits("01a"));
    }

    // =========================================================================
    section("12. Identifier generator");
    // =========================================================================
    {
        scapms::utils::IDGenerator& ids = scapms::utils::IDGenerator::instance();
        ids.reset();
        SC_CHECK(ids.nextId("P") == "P-0001");
        SC_CHECK(ids.nextId("P") == "P-0002");
        ids.observeExistingId("P-0042");
        SC_CHECK(ids.nextId("P") == "P-0043");
        SC_CHECK(ids.nextId("APT") == "APT-0001");
        ids.reset();
    }

    // =========================================================================
    //  Summary
    // =========================================================================
    QDir(tempRoot).removeRecursively();

    out() << "\n=============================================\n";
    out() << "Passed: " << g_passed << "   Failed: " << g_failed << "\n";
    if (!g_failures.isEmpty()) {
        out() << "\nFailures:\n";
        for (const auto& failure : g_failures) {
            out() << "  - " << failure << "\n";
        }
    }
    out() << (g_failed == 0 ? "RESULT: ALL CHECKS PASSED\n" : "RESULT: FAILURES DETECTED\n");
    out() << "=============================================\n";
    out().flush();
    return g_failed;
}

} // namespace scapms::tests
