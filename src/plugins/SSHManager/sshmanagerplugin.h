/*  This file was part of the KDE libraries

    SPDX-FileCopyrightText: 2021 Tomaz Canabrava <tcanabrava@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef SSHMANAGERPLUGIN_H
#define SSHMANAGERPLUGIN_H

#include <pluginsystem/IKonsolePlugin.h>

#include <memory>

#include "sshconfigurationdata.h"

namespace Konsole
{
class Session;
class SessionController;
class MainWindow;
}

class QSortFilterProxyModel;
class QStandardItemModel;

struct SSHManagerPluginPrivate;

class SSHManagerPlugin : public Konsole::IKonsolePlugin
{
    Q_OBJECT
public:
    SSHManagerPlugin(QObject *object, const QVariantList &args);
    ~SSHManagerPlugin() override;

    void createWidgetsForMainWindow(Konsole::MainWindow *mainWindow) override;
    void activeViewChanged(Konsole::SessionController *controller, Konsole::MainWindow *mainWindow) override;
    QList<QAction *> menuBarActions(Konsole::MainWindow *mainWindow) const override;

    void requestConnection(const QModelIndex &idx, Konsole::SessionController *controller);
    void handleQuickConnection(const SSHConfigurationData &data, Konsole::SessionController *controller);

    bool canDuplicateSession(Konsole::Session *session) const override;
    void duplicateSession(Konsole::Session *session, Konsole::MainWindow *mainWindow) override;
    bool canReconnectSession(Konsole::Session *session) const override;
    void reconnectSession(Konsole::Session *session, Konsole::MainWindow *mainWindow) override;

    Konsole::SshSessionData getSessionSshData(Konsole::Session *session) const override;

    bool canOpenSftp(Konsole::Session *session) const override;
    void openSftp(Konsole::Session *session, Konsole::MainWindow *mainWindow) override;

private:
    void startConnection(const SSHConfigurationData &data, Konsole::SessionController *controller);

    std::unique_ptr<SSHManagerPluginPrivate> d;
};

#endif
