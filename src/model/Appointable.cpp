// =============================================================================
//  File   : src/model/Appointable.cpp
// =============================================================================
#include "Appointable.h"

#include <cstddef>

namespace scapms::model {

bool AppointableImpl::scheduleAppointment(const Appointment& appointment) {
    const std::string& id = appointment.id();
    if (id.empty()) {
        return false;
    }
    // Hand-written linear scan - no standard search algorithm is used here.
    for (const auto& existing : m_appointmentIds) {
        if (existing == id) {
            return false;
        }
    }
    m_appointmentIds.push_back(id);
    return true;
}

bool AppointableImpl::cancelAppointment(const std::string& appointmentId) {
    for (std::size_t i = 0; i < m_appointmentIds.size(); ++i) {
        if (m_appointmentIds[i] == appointmentId) {
            m_appointmentIds.erase(m_appointmentIds.begin() +
                                   static_cast<std::ptrdiff_t>(i));
            return true;
        }
    }
    return false;
}

std::vector<std::string> AppointableImpl::appointmentIds() const {
    return m_appointmentIds;
}

std::size_t AppointableImpl::appointmentCount() const {
    return m_appointmentIds.size();
}

} // namespace scapms::model
