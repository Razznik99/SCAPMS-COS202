// =============================================================================
//  File   : src/controller/DemoData.cpp
// =============================================================================
#include "DemoData.h"

#include <memory>
#include <string>

#include "../model/Appointable.h"
#include "../model/Appointment.h"
#include "../model/EmergencyPatient.h"
#include "../model/FollowUpPatient.h"
#include "../model/GeneralPatient.h"
#include "../utils/DateUtils.h"
#include "ClinicManager.h"

namespace scapms::controller {

namespace {

using scapms::model::Appointment;
using scapms::model::AppointmentStatus;
using scapms::model::EmergencyPatient;
using scapms::model::FollowUpPatient;
using scapms::model::GeneralPatient;
using scapms::utils::addDays;
using scapms::utils::todayIso;

std::string clinicianFor(int index) {
    switch (index % 4) {
    case 0:
        return "Dr. Adeyemi";
    case 1:
        return "Dr. Balogun";
    case 2:
        return "Dr. Eze";
    default:
        return "Nurse Okafor";
    }
}

} // namespace

void DemoData::seed(ClinicManager& manager) {
    auto& database = manager.database();
    if (!database.isEmpty()) {
        return;
    }

    const std::string today = todayIso();

    const auto addGeneral = [&database, &today](const std::string& id, const std::string& name,
                                        int age, const std::string& gender,
                                        const std::string& contact,
                                        const std::string& insurance,
                                        const std::string& chronic) {
        auto patient = std::make_unique<GeneralPatient>(id, name, age, contact, gender,
                                                        today, insurance, chronic);
        patient->validate();
        database.addPatient(std::move(patient));
    };

    const auto addEmergency = [&database, &today](const std::string& id, const std::string& name,
                                          int age, const std::string& gender,
                                          const std::string& contact, int triage,
                                          const std::string& arrival, bool admitted) {
        auto patient = std::make_unique<EmergencyPatient>(
            id, name, age, contact, gender, today, triage, arrival, admitted);
        patient->validate();
        database.addPatient(std::move(patient));
    };

    const auto addFollowUp = [&database, &today](const std::string& id,
                                                 const std::string& name, int age,
                                                 const std::string& gender,
                                                 const std::string& contact,
                                                 int visitDaysAgo, int interval,
                                                 const std::string& doctor) {
        auto patient = std::make_unique<FollowUpPatient>(
            id, name, age, contact, gender, today, addDays(today, -visitDaysAgo),
            interval, doctor);
        patient->validate();
        database.addPatient(std::move(patient));
    };

    // ---- 14 patients across the three categories ---------------------------
    addGeneral("P-0001", "Amina Bello", 34, "Female", "+234 803 555 0101", "NHIS",
               "Hypertension");
    addEmergency("P-0002", "Chinedu Okafor", 41, "Male", "+234 805 555 0102", 2,
                 "Ambulance", true);
    addFollowUp("P-0003", "Fatima Yusuf", 28, "Female", "+234 807 555 0103", 30, 14,
                "Dr. Adeyemi");
    addGeneral("P-0004", "Tunde Adebayo", 52, "Male", "+234 809 555 0104", "Self-Pay",
               "Type 2 Diabetes");
    addGeneral("P-0005", "Ngozi Eze", 19, "Female", "+234 802 555 0105",
               "AXA Mansard", "None");
    addEmergency("P-0006", "Ibrahim Musa", 67, "Male", "+234 806 555 0106", 1,
                 "Referral", true);
    addFollowUp("P-0007", "Blessing Okon", 36, "Female", "+234 808 555 0107", 21, 21,
                "Dr. Balogun");
    addGeneral("P-0008", "Emeka Nwosu", 45, "Male", "+234 810 555 0108", "Self-Pay",
               "Asthma");
    addGeneral("P-0009", "Halima Sani", 8, "Female", "+234 803 555 0109", "NHIS",
               "None");
    addEmergency("P-0010", "Segun Ojo", 73, "Male", "+234 805 555 0110", 3, "Walk-in",
                 false);
    addFollowUp("P-0011", "Adaeze Umeh", 31, "Female", "+234 807 555 0111", 10, 10,
                "Dr. Adeyemi");
    addGeneral("P-0012", "Yusuf Lawal", 26, "Male", "+234 809 555 0112",
               "Reliance HMO", "Peptic ulcer");
    addGeneral("P-0013", "Chiamaka Obi", 58, "Female", "+234 802 555 0113", "NHIS",
               "Arthritis");
    addEmergency("P-0014", "Bala Abubakar", 39, "Male", "+234 806 555 0114", 4,
                 "Walk-in", false);

    // ---- appointments relative to today ------------------------------------
    const auto book = [&database, &today](const std::string& id,
                                          const std::string& patientId, int dayOffset,
                                          const std::string& time,
                                          const std::string& serviceCode,
                                          const std::string& clinician,
                                          AppointmentStatus status,
                                          const std::string& notes) {
        const auto* patient = database.findPatientById(patientId);
        const auto* service = database.findService(serviceCode);
        if (patient == nullptr || service == nullptr) {
            return;
        }
        double fee = service->baseFee();
        if (service->category() == "Consultation") {
            fee = patient->consultationFee();
        }
        Appointment appointment(id, patientId, addDays(today, dayOffset), time,
                                serviceCode, clinician, fee, notes, status);
        appointment.validate();
        database.addAppointment(appointment);
        if (auto* appointable = dynamic_cast<scapms::model::Appointable*>(
                database.findPatientById(patientId))) {
            appointable->scheduleAppointment(appointment);
        }
    };

    // Past, completed visits.
    book("APT-0001", "P-0001", -7, "09:00", "GEN-CON", clinicianFor(0),
         AppointmentStatus::Completed, "Blood pressure review");
    book("APT-0002", "P-0004", -7, "10:00", "GEN-CON", clinicianFor(0),
         AppointmentStatus::Completed, "Diabetes medication review");
    book("APT-0003", "P-0003", -6, "11:00", "FUP-CON", clinicianFor(1),
         AppointmentStatus::Completed, "Wound check");
    book("APT-0004", "P-0005", -5, "09:30", "IMM-ADU", clinicianFor(3),
         AppointmentStatus::Completed, "Hepatitis B booster");
    book("APT-0005", "P-0001", -4, "14:00", "LAB-BLD", clinicianFor(0),
         AppointmentStatus::Completed, "Full blood count");
    book("APT-0006", "P-0008", -4, "15:00", "GEN-CON", clinicianFor(1),
         AppointmentStatus::Cancelled, "Patient travelled");
    book("APT-0007", "P-0009", -3, "10:00", "IMM-CHD", clinicianFor(3),
         AppointmentStatus::Completed, "Routine immunisation");
    book("APT-0008", "P-0012", -2, "12:00", "GEN-CON", clinicianFor(0),
         AppointmentStatus::Completed, "Ulcer follow-up");
    book("APT-0009", "P-0013", -1, "09:00", "GEN-CON", clinicianFor(1),
         AppointmentStatus::Completed, "Joint pain assessment");
    book("APT-0022", "P-0007", -8, "09:00", "FUP-CON", clinicianFor(1),
         AppointmentStatus::Completed, "Post-treatment review");
    book("APT-0023", "P-0003", -12, "10:00", "FUP-CON", clinicianFor(0),
         AppointmentStatus::Completed, "Initial follow-up");
    book("APT-0024", "P-0005", -9, "11:30", "GEN-CON", clinicianFor(1),
         AppointmentStatus::Completed, "General consultation");
    book("APT-0025", "P-0013", -15, "09:30", "GEN-CON", clinicianFor(0),
         AppointmentStatus::Completed, "Arthritis review");

    // Today's clinic.
    book("APT-0010", "P-0001", 0, "09:00", "GEN-CON", clinicianFor(0),
         AppointmentStatus::Pending, "Hypertension review");
    book("APT-0011", "P-0011", 0, "09:30", "FUP-CON", clinicianFor(1),
         AppointmentStatus::Pending, "Review of test results");
    book("APT-0012", "P-0004", 0, "10:00", "LAB-MLR", clinicianFor(0),
         AppointmentStatus::Completed, "Malaria test - negative");
    book("APT-0013", "P-0005", 0, "11:00", "DEN-SCL", clinicianFor(2),
         AppointmentStatus::Pending, "Dental scaling");
    book("APT-0014", "P-0007", 0, "12:00", "FUP-CON", clinicianFor(1),
         AppointmentStatus::Cancelled, "Rescheduled by patient");

    // The coming week.
    book("APT-0015", "P-0003", 1, "09:00", "FUP-CON", clinicianFor(0),
         AppointmentStatus::Pending, "Follow-up review");
    book("APT-0016", "P-0008", 1, "10:30", "GEN-CON", clinicianFor(1),
         AppointmentStatus::Pending, "Asthma review");
    book("APT-0017", "P-0009", 2, "09:30", "IMM-CHD", clinicianFor(3),
         AppointmentStatus::Pending, "Second dose");
    book("APT-0018", "P-0012", 2, "11:00", "PHY-EXM", clinicianFor(2),
         AppointmentStatus::Pending, "Pre-employment screening");
    book("APT-0019", "P-0013", 3, "15:00", "GEN-CON", clinicianFor(0),
         AppointmentStatus::Pending, "Medication review");
    book("APT-0020", "P-0005", 5, "09:00", "COU-SES", clinicianFor(1),
         AppointmentStatus::Pending, "Counselling session");
    book("APT-0021", "P-0011", 6, "10:00", "FUP-CON", clinicianFor(0),
         AppointmentStatus::Pending, "Follow-up review");

    // ---- a live waiting queue ---------------------------------------------
    for (const char* id : {"P-0002", "P-0005", "P-0006", "P-0008", "P-0014"}) {
        try {
            manager.joinQueue(id);
        } catch (const std::exception&) {
            // A duplicate queue entry is not fatal for the demonstration seed.
        }
    }
}

} // namespace scapms::controller
