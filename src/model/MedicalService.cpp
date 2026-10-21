// =============================================================================
//  File   : src/model/MedicalService.cpp
// =============================================================================
#include "MedicalService.h"

#include "../utils/StringUtils.h"

namespace scapms::model {

std::string MedicalService::serialize() const {
    std::vector<std::string> fields;
    fields.push_back(utils::escapeCsv(m_code));
    fields.push_back(utils::escapeCsv(m_name));
    fields.push_back(utils::escapeCsv(m_category));
    fields.push_back(std::to_string(m_durationMinutes));
    fields.push_back(utils::formatDouble(m_baseFee, 2));
    return utils::join(fields, ",");
}

std::vector<MedicalService> MedicalService::defaults() {
    return {
        MedicalService("GEN-CON", "General Consultation", "Consultation", 20, 2500.0),
        MedicalService("EMG-CON", "Emergency Consultation", "Emergency", 15, 7500.0),
        MedicalService("FUP-CON", "Follow-Up Review", "Consultation", 15, 1500.0),
        MedicalService("IMM-ADU", "Adult Immunisation", "Immunisation", 10, 1800.0),
        MedicalService("IMM-CHD", "Child Immunisation", "Immunisation", 10, 1200.0),
        MedicalService("LAB-BLD", "Blood Panel (Laboratory)", "Laboratory", 15, 4500.0),
        MedicalService("LAB-MLR", "Malaria Test", "Laboratory", 10, 1500.0),
        MedicalService("DEN-SCL", "Dental Scaling", "Dental", 30, 6000.0),
        MedicalService("MAT-ANC", "Antenatal Check", "Maternity", 25, 3500.0),
        MedicalService("PHA-REF", "Pharmacy Refill Review", "Pharmacy", 10, 1000.0),
        MedicalService("COU-SES", "Counselling Session", "Counselling", 40, 4000.0),
        MedicalService("PHY-EXM", "Pre-Employment Physical", "Screening", 30, 5000.0),
    };
}

} // namespace scapms::model
