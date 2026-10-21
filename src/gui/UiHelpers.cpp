// =============================================================================
//  File   : src/gui/UiHelpers.cpp
// =============================================================================
#include "UiHelpers.h"

#include <QFile>
#include <QVBoxLayout>

namespace scapms::gui {

QString appStyleSheet() {
    QFile file(":/style.qss");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString::fromUtf8(file.readAll());
    }
    return {};
}

QColor categoryColor(const QString& category) {
    if (category == "Emergency") {
        return QColor("#FEE2E2");
    }
    if (category == "Follow-Up") {
        return QColor("#FEF3C7");
    }
    return QColor("#DBEAFE");
}

QColor categoryTextColor(const QString& category) {
    if (category == "Emergency") {
        return QColor("#B91C1C");
    }
    if (category == "Follow-Up") {
        return QColor("#92400E");
    }
    return QColor("#1D4ED8");
}

QColor statusColor(const QString& status) {
    if (status == "Completed") {
        return QColor("#DCFCE7");
    }
    if (status == "Cancelled") {
        return QColor("#E2E8F0");
    }
    return QColor("#FEF3C7"); // Pending
}

QColor statusTextColor(const QString& status) {
    if (status == "Completed") {
        return QColor("#15803D");
    }
    if (status == "Cancelled") {
        return QColor("#475569");
    }
    return QColor("#B45309");
}

QString toQt(const std::string& text) {
    return QString::fromStdString(text);
}

QFrame* createStatCard(const QString& title, const QString& accent, QLabel*& valueLabel,
                       QWidget* parent) {
    auto* card = new QFrame(parent);
    card->setObjectName("statCard");
    card->setFrameShape(QFrame::NoFrame);
    card->setStyleSheet(QString("#statCard { border-left: 4px solid %1; }").arg(accent));

    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(14, 12, 14, 12);
    layout->setSpacing(2);

    auto* caption = new QLabel(title.toUpper(), card);
    caption->setObjectName("statTitle");

    auto* value = new QLabel("0", card);
    value->setObjectName("statValue");

    layout->addWidget(caption);
    layout->addWidget(value);
    valueLabel = value;
    return card;
}

QLabel* createSectionTitle(const QString& text, QWidget* parent) {
    auto* label = new QLabel(text, parent);
    label->setObjectName("sectionTitle");
    return label;
}

QLabel* createHintLabel(const QString& text, QWidget* parent) {
    auto* label = new QLabel(text, parent);
    label->setObjectName("hintLabel");
    label->setWordWrap(true);
    return label;
}

QString sortAlgorithmLabel(scapms::controller::SortAlgorithm algorithm) {
    return QString::fromStdString(
        scapms::controller::SortEngine::algorithmName(algorithm));
}

} // namespace scapms::gui
