// =============================================================================
//  SCAPMS - Smart Clinic Appointment & Patient Management System
//  File   : src/gui/PillDelegate.h
//  Layer  : GUI
//  Purpose: Custom item rendering. Draws patient categories and appointment
//           statuses as coloured "pills" so that emergency cases and pending
//           appointments stand out at a glance (advanced GUI technique: custom
//           table presentation).
// =============================================================================
#ifndef SCAPMS_GUI_PILLDELEGATE_H
#define SCAPMS_GUI_PILLDELEGATE_H

#include <QStyledItemDelegate>

namespace scapms::gui {

class PillDelegate : public QStyledItemDelegate {
public:
    enum class Kind { Category, Status };

    explicit PillDelegate(Kind kind, QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

    [[nodiscard]] QSize sizeHint(const QStyleOptionViewItem& option,
                                 const QModelIndex& index) const override;

private:
    Kind m_kind;
};

} // namespace scapms::gui

#endif // SCAPMS_GUI_PILLDELEGATE_H
