// =============================================================================
//  File   : src/model/MedicalService.h
//  Layer  : Model
//  Purpose: A service offered by the clinic (consultation, immunisation,
//           laboratory, dental, ...). Appointments reference a service code.
// =============================================================================
#ifndef SCAPMS_MODEL_MEDICALSERVICE_H
#define SCAPMS_MODEL_MEDICALSERVICE_H

#include <string>
#include <vector>

namespace scapms::model {

class MedicalService {
public:
    MedicalService() = default;

    MedicalService(std::string code,
                   std::string name,
                   std::string category,
                   int durationMinutes,
                   double baseFee)
        : m_code(std::move(code)),
          m_name(std::move(name)),
          m_category(std::move(category)),
          m_durationMinutes(durationMinutes),
          m_baseFee(baseFee) {}

    [[nodiscard]] const std::string& code() const noexcept { return m_code; }
    [[nodiscard]] const std::string& name() const noexcept { return m_name; }
    [[nodiscard]] const std::string& category() const noexcept { return m_category; }
    [[nodiscard]] int durationMinutes() const noexcept { return m_durationMinutes; }
    [[nodiscard]] double baseFee() const noexcept { return m_baseFee; }

    [[nodiscard]] std::string label() const { return m_name + " (" + m_code + ")"; }

    [[nodiscard]] std::string serialize() const;

    static std::vector<MedicalService> defaults();

private:
    std::string m_code;
    std::string m_name;
    std::string m_category;
    int m_durationMinutes = 15;
    double m_baseFee = 0.0;
};

} // namespace scapms::model

#endif // SCAPMS_MODEL_MEDICALSERVICE_H
