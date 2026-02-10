/*
    SPDX-FileCopyrightText: 2026 Konsole Plus contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef COMPOSEBARWIDGET_H
#define COMPOSEBARWIDGET_H

#include <QWidget>

#include "konsoleprivate_export.h"

class QComboBox;
class QLineEdit;

namespace Konsole
{
class ViewManager;
class Session;

class KONSOLEPRIVATE_EXPORT ComposeBarWidget : public QWidget
{
    Q_OBJECT
public:
    enum SendTarget {
        CurrentSession = 0,
        AllSessions = 1,
    };

    explicit ComposeBarWidget(ViewManager *viewManager, QWidget *parent = nullptr);

    bool isBroadcasting() const;

Q_SIGNALS:
    void broadcastModeChanged(bool broadcasting);

private Q_SLOTS:
    void sendText();

private:
    QList<Session *> targetSessions() const;

    ViewManager *m_viewManager;
    QLineEdit *m_lineEdit;
    QComboBox *m_targetCombo;
};

}

#endif // COMPOSEBARWIDGET_H
