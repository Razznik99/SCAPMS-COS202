// =============================================================================
//  File   : src/gui/PillDelegate.cpp
// =============================================================================
#include "PillDelegate.h"

#include <QApplication>
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QWidget>

#include "UiHelpers.h"

namespace scapms::gui {

PillDelegate::PillDelegate(Kind kind, QObject* parent)
    : QStyledItemDelegate(parent), m_kind(kind) {}

void PillDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                         const QModelIndex& index) const {
    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);
    opt.text.clear(); // the base style paints the background/selection only

    const QWidget* widget = option.widget;
    QStyle* style = widget ? widget->style() : nullptr;
    if (style != nullptr) {
        style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);
    }

    const QString text = index.data(Qt::DisplayRole).toString();
    if (text.isEmpty()) {
        return;
    }

    const QColor background = m_kind == Kind::Category ? categoryColor(text)
                                                       : statusColor(text);
    const QColor foreground = m_kind == Kind::Category ? categoryTextColor(text)
                                                       : statusTextColor(text);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    QFont font = option.font;
    font.setBold(true);
    font.setPointSizeF(option.font.pointSizeF() > 0 ? option.font.pointSizeF() - 0.5
                                                    : 8.5);
    painter->setFont(font);

    const QFontMetrics metrics(font);
    const int textWidth = metrics.horizontalAdvance(text);
    const int pillWidth = qMin(textWidth + 18, option.rect.width() - 8);
    const int pillHeight = qMin(metrics.height() + 6, option.rect.height() - 4);
    const QRect pill(option.rect.left() + 4,
                     option.rect.top() + (option.rect.height() - pillHeight) / 2,
                     qMax(pillWidth, 24), pillHeight);

    painter->setPen(Qt::NoPen);
    painter->setBrush(background);
    painter->drawRoundedRect(pill, pillHeight / 2.0, pillHeight / 2.0);

    painter->setPen(foreground);
    painter->drawText(pill, Qt::AlignCenter,
                      metrics.elidedText(text, Qt::ElideRight, pill.width() - 10));
    painter->restore();
}

QSize PillDelegate::sizeHint(const QStyleOptionViewItem& option,
                             const QModelIndex& index) const {
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    size.setHeight(qMax(size.height(), 30));
    size.setWidth(qMax(size.width() + 16, 90));
    return size;
}

} // namespace scapms::gui
