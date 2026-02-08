/*  This file was part of the KDE libraries

    SPDX-FileCopyrightText: 2021 Tomaz Canabrava <tcanabrava@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef IKONSOLEPLUGIN_H
#define IKONSOLEPLUGIN_H

#include <QList>
#include <QObject>

#include <terminalDisplay/TerminalDisplay.h>

#include <KPluginFactory>

#include <memory>

#include "konsoleapp_export.h"

namespace Konsole
{
class MainWindow;
class Session;

class KONSOLEAPP_EXPORT IKonsolePlugin : public QObject
{
    Q_OBJECT
public:
    IKonsolePlugin(QObject *parent, const QVariantList &args);
    ~IKonsolePlugin() override;

    QString name() const;

    // Usable only from PluginManager, please don't use.
    void addMainWindow(Konsole::MainWindow *mainWindow);
    void removeMainWindow(Konsole::MainWindow *mainWindow);

    virtual void createWidgetsForMainWindow(Konsole::MainWindow *mainWindow) = 0;
    virtual void activeViewChanged(Konsole::SessionController *controller, Konsole::MainWindow *mainWindow) = 0;

    virtual QList<QAction *> menuBarActions(Konsole::MainWindow *mainWindow) const
    {
        Q_UNUSED(mainWindow)
        return {};
    }

    /** Returns true if the plugin can duplicate the given session (e.g. re-connect SSH). */
    virtual bool canDuplicateSession(Konsole::Session *session) const
    {
        Q_UNUSED(session)
        return false;
    }

    /** Duplicate the session by opening a new tab with the same connection. */
    virtual void duplicateSession(Konsole::Session *session, Konsole::MainWindow *mainWindow)
    {
        Q_UNUSED(session)
        Q_UNUSED(mainWindow)
    }

protected:
    void setName(const QString &pluginName);

private:
    struct Private;
    std::unique_ptr<Private> d;
};

}
#endif
