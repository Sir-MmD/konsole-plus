/*
    SPDX-FileCopyrightText: 2026 Konsole Plus contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "ComposeBarWidget.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>

#include <KLocalizedString>

#include "ViewManager.h"
#include "session/Session.h"
#include "session/SessionController.h"

using namespace Konsole;

ComposeBarWidget::ComposeBarWidget(ViewManager *viewManager, QWidget *parent)
    : QWidget(parent)
    , m_viewManager(viewManager)
    , m_lineEdit(new QLineEdit(this))
    , m_targetCombo(new QComboBox(this))
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 2, 4, 2);

    auto *label = new QLabel(i18n("Send:"), this);
    layout->addWidget(label);

    m_targetCombo->addItem(i18n("Current Session"), CurrentSession);
    m_targetCombo->addItem(i18n("All Sessions"), AllSessions);
    m_targetCombo->setToolTip(i18n("Choose which sessions receive the command"));
    layout->addWidget(m_targetCombo);

    m_lineEdit->setPlaceholderText(i18n("Type command and press Enter to send..."));
    m_lineEdit->setClearButtonEnabled(true);
    layout->addWidget(m_lineEdit, 1);

    connect(m_lineEdit, &QLineEdit::returnPressed, this, &ComposeBarWidget::sendText);

    connect(m_targetCombo, &QComboBox::currentIndexChanged, this, [this] {
        Q_EMIT broadcastModeChanged(isBroadcasting());
    });
}

bool ComposeBarWidget::isBroadcasting() const
{
    return isVisible() && m_targetCombo->currentData().toInt() == AllSessions;
}

void ComposeBarWidget::sendText()
{
    const QString text = m_lineEdit->text();
    if (text.isEmpty()) {
        return;
    }

    const auto sessions = targetSessions();
    for (auto *session : sessions) {
        if (session && !session->isReadOnly()) {
            session->sendTextToTerminal(text, QLatin1Char('\r'));
        }
    }

    m_lineEdit->clear();
}

QList<Session *> ComposeBarWidget::targetSessions() const
{
    int target = m_targetCombo->currentData().toInt();

    if (target == AllSessions) {
        return m_viewManager->sessions();
    }

    auto *controller = m_viewManager->activeViewController();
    if (controller && controller->session()) {
        return {controller->session()};
    }

    return {};
}
