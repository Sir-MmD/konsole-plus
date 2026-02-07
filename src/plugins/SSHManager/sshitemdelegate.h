/*
    SPDX-FileCopyrightText: 2026 Konsole Plus contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef SSHITEMDELEGATE_H
#define SSHITEMDELEGATE_H

#include <QStyledItemDelegate>

class SshItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit SshItemDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

#endif
