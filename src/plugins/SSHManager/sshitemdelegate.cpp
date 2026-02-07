/*
    SPDX-FileCopyrightText: 2026 Konsole Plus contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "sshitemdelegate.h"
#include "sshmanagermodel.h"

#include <QPainter>
#include <QApplication>

SshItemDelegate::SshItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void SshItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // Custom-paint only the proxy column when it has text
    if (index.column() == SSHManagerModel::ProxyColumn) {
        const QString text = index.data(Qt::DisplayRole).toString();
        if (!text.isEmpty()) {
            // Draw background (selection, hover)
            QStyleOptionViewItem opt = option;
            initStyleOption(&opt, index);
            opt.text.clear();
            QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);

            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);

            const QPalette &pal = option.palette;
            const bool selected = option.state & QStyle::State_Selected;

            QFont badgeFont = option.font;
            badgeFont.setPointSizeF(option.font.pointSizeF() * 0.8);
            badgeFont.setBold(true);
            const QFontMetrics fm(badgeFont);

            const int padH = 5;
            const int padV = 2;
            const int badgeW = fm.horizontalAdvance(text) + padH * 2;
            const int badgeH = fm.height() + padV * 2;

            const QRect badgeRect(
                option.rect.left() + (option.rect.width() - badgeW) / 2,
                option.rect.top() + (option.rect.height() - badgeH) / 2,
                badgeW,
                badgeH
            );

            const QColor badgeBg = selected ? pal.color(QPalette::HighlightedText).darker(120)
                                            : pal.color(QPalette::Highlight);
            const QColor badgeFg = selected ? pal.color(QPalette::Highlight)
                                            : pal.color(QPalette::HighlightedText);

            painter->setPen(Qt::NoPen);
            painter->setBrush(badgeBg);
            painter->drawRoundedRect(badgeRect, 4, 4);

            painter->setFont(badgeFont);
            painter->setPen(badgeFg);
            painter->drawText(badgeRect, Qt::AlignCenter, text);

            painter->restore();
            return;
        }
    }

    QStyledItemDelegate::paint(painter, option, index);
}

QSize SshItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize hint = QStyledItemDelegate::sizeHint(option, index);
    hint.setHeight(qMax(hint.height(), 28));
    return hint;
}
