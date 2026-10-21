// =============================================================================
//  SCAPMS - Smart Clinic Appointment & Patient Management System
//  File   : src/gui/UiHelpers.h
//  Layer  : GUI
//  Purpose: Small presentation helpers shared by every panel (stylesheet,
//           category/status colours, label factories) so that styling stays in
//           one place instead of being repeated in each widget.
// =============================================================================
#ifndef SCAPMS_GUI_UIHELPERS_H
#define SCAPMS_GUI_UIHELPERS_H

#include <QColor>
#include <QFrame>
#include <QLabel>
#include <QString>
#include <QWidget>

#include "../controller/SortEngine.h"

namespace scapms::gui {

/// Application-wide stylesheet (loaded from the Qt resource file).
QString appStyleSheet();

/// Accent colour used for a patient category ("General", "Emergency", ...).
QColor categoryColor(const QString& category);
QColor categoryTextColor(const QString& category);

/// Accent colour used for an appointment status.
QColor statusColor(const QString& status);
QColor statusTextColor(const QString& status);

/// "P-0001" -> "P-0001"; converts std::string safely.
QString toQt(const std::string& text);

/// Creates a dashboard statistic card. The returned frame owns the value label,
/// which is written through `valueLabel`.
QFrame* createStatCard(const QString& title, const QString& accent, QLabel*& valueLabel,
                       QWidget* parent = nullptr);

/// Section heading used above groups of widgets.
QLabel* createSectionTitle(const QString& text, QWidget* parent = nullptr);

/// Grey hint label.
QLabel* createHintLabel(const QString& text, QWidget* parent = nullptr);

/// Human label for a sort algorithm (kept here so panels do not repeat switch
/// statements).
QString sortAlgorithmLabel(scapms::controller::SortAlgorithm algorithm);

} // namespace scapms::gui

#endif // SCAPMS_GUI_UIHELPERS_H
