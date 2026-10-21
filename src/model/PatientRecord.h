// =============================================================================
//  SCAPMS - Smart Clinic Appointment & Patient Management System
//  File   : src/model/PatientRecord.h
//  Layer  : Model
//  Purpose: Abstract base class of the patient hierarchy. Holds the fields
//           common to every patient and declares the pure virtual operations
//           that give each patient type its own behaviour (runtime
//           polymorphism through PatientRecord*).
// =============================================================================
#ifndef SCAPMS_MODEL_PATIENTRECORD_H
#define SCAPMS_MODEL_PATIENTRECORD_H

#include <memory>
#include <string>
#include <vector>

namespace scapms::model {

class PatientRecord {
public:
    PatientRecord(std::string id,
                  std::string name,
                  int age,
                  std::string contact,
                  std::string gender,
                  std::string registeredOn);

    virtual ~PatientRecord() = default;

    // ---- common attributes ------------------------------------------------
    [[nodiscard]] const std::string& id() const noexcept { return m_id; }
    [[nodiscard]] const std::string& name() const noexcept { return m_name; }
    [[nodiscard]] int age() const noexcept { return m_age; }
    [[nodiscard]] const std::string& contact() const noexcept { return m_contact; }
    [[nodiscard]] const std::string& gender() const noexcept { return m_gender; }
    [[nodiscard]] const std::string& registeredOn() const noexcept { return m_registeredOn; }

    void setName(std::string name) { m_name = std::move(name); }
    void setAge(int age) noexcept { m_age = age; }
    void setContact(std::string contact) { m_contact = std::move(contact); }
    void setGender(std::string gender) { m_gender = std::move(gender); }
    void setRegisteredOn(std::string date) { m_registeredOn = std::move(date); }

    // ---- pure virtual operations (abstract class contract) ----------------
    /// Human-readable category shown in the GUI ("General", "Emergency"...).
    [[nodiscard]] virtual std::string category() const = 0;

    /// 1 = highest clinical priority. Used by triage and by the waiting queue.
    [[nodiscard]] virtual int priorityLevel() const = 0;

    /// Priority caption derived from priorityLevel().
    [[nodiscard]] virtual std::string priorityLabel() const = 0;

    /// Fee charged for one consultation of this patient type.
    [[nodiscard]] virtual double consultationFee() const = 0;

    /// One-line clinical description rendered below the form.
    [[nodiscard]] virtual std::string clinicalSummary() const = 0;

    /// Tag stored in the first CSV column and used by the factory.
    [[nodiscard]] virtual std::string typeTag() const = 0;

    /// Deep copy used by the undo stack and by record editing.
    [[nodiscard]] virtual std::unique_ptr<PatientRecord> clone() const = 0;

    // ---- shared behaviour --------------------------------------------------
    /// Throws ValidationException describing the first invalid field.
    virtual void validate() const;

    [[nodiscard]] bool isValid() const noexcept;

    /// CSV row: type,id,name,age,gender,contact,registeredOn,<type fields...>
    [[nodiscard]] virtual std::string serialize() const;

    /// Number of virtuals reachable through the base pointer (used by tests).
    [[nodiscard]] std::string describe() const;

    /// Restores type-specific columns produced by extraFields().
    /// Malformed input throws ValidationException.
    virtual void loadExtraFields(const std::vector<std::string>& extras) = 0;

protected:
    /// Type-specific columns appended to the CSV row.
    [[nodiscard]] virtual std::vector<std::string> extraFields() const = 0;

private:
    std::string m_id;
    std::string m_name;
    std::string m_contact;
    std::string m_gender;
    std::string m_registeredOn;
    int m_age = 0;
};

} // namespace scapms::model

#endif // SCAPMS_MODEL_PATIENTRECORD_H
