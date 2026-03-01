/*  This file was part of the KDE libraries

    SPDX-FileCopyrightText: 2021 Tomaz Canabrava <tcanabrava@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "sshmanagerplugin.h"

#include "sshmanagermodel.h"
#include "sshmanagerpluginwidget.h"

#include "ProcessInfo.h"
#include "konsoledebug.h"
#include "session/SessionController.h"
#include "session/Session.h"
#include "widgets/ViewContainer.h"
#include "widgets/ViewSplitter.h"

#include <QDockWidget>
#include <QListView>
#include <QMainWindow>
#include <QMenuBar>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QTimer>

#include <KActionCollection>
#include <KCommandBar>
#include <KCrash>
#include <KLocalizedString>
#include <KMessageBox>
#include <QSettings>
#include <QStandardPaths>
#include <kcommandbar.h>
#include <QDir>
#include <QProcess>
#include <QUuid>
#include <QTextStream>

#include "MainWindow.h"
#include "ViewManager.h"
#include "terminalDisplay/TerminalDisplay.h"

#include <KIO/AuthInfo>
#include <KIO/JobUiDelegateFactory>
#include <KIO/OpenUrlJob>
#include <KPasswdServerClient>

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

K_PLUGIN_CLASS_WITH_JSON(SSHManagerPlugin, "konsole_sshmanager.json")

// Find a free local TCP port by binding to port 0 and letting the OS assign one.
static int findFreePort()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return 0;
    }
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    if (bind(sock, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0) {
        ::close(sock);
        return 0;
    }
    socklen_t len = sizeof(addr);
    getsockname(sock, reinterpret_cast<struct sockaddr *>(&addr), &len);
    int port = ntohs(addr.sin_port);
    ::close(sock);
    return port;
}

struct SSHManagerPluginPrivate {
    SSHManagerModel model;

    QMap<Konsole::MainWindow *, SSHManagerTreeWidget *> widgetForWindow;
    QMap<Konsole::MainWindow *, QDockWidget *> dockForWindow;
    QAction *showQuickAccess = nullptr;

    QPointer<Konsole::MainWindow> currentMainWindow;

    // Track which sessions were connected via the SSH Manager, so we can duplicate them.
    QHash<Konsole::Session*, SSHConfigurationData> activeSessionData;

    // Track per-session SSH state so we know if reconnect is possible.
    QHash<Konsole::Session*, int> sessionSshState;

    // SFTP proxy tunnels: running QProcess and local port per session for cleanup.
    QHash<Konsole::Session *, QProcess *> sftpTunnelProcesses;
    QHash<Konsole::Session *, int> sftpTunnelPorts;
};

SSHManagerPlugin::SSHManagerPlugin(QObject *object, const QVariantList &args)
    : Konsole::IKonsolePlugin(object, args)
    , d(std::make_unique<SSHManagerPluginPrivate>())
{
    d->showQuickAccess = new QAction();

    setName(QStringLiteral("SshManager"));
    KCrash::initialize();
}

SSHManagerPlugin::~SSHManagerPlugin()
{
}

void SSHManagerPlugin::createWidgetsForMainWindow(Konsole::MainWindow *mainWindow)
{
    auto *sshDockWidget = new QDockWidget(mainWindow);
    auto *managerWidget = new SSHManagerTreeWidget();
    managerWidget->setModel(&d->model);
    sshDockWidget->setWidget(managerWidget);
    sshDockWidget->setWindowTitle(i18n("SSH Manager"));
    sshDockWidget->setObjectName(QStringLiteral("SSHManagerDock"));
    sshDockWidget->setVisible(false);
    sshDockWidget->setAllowedAreas(Qt::DockWidgetArea::LeftDockWidgetArea | Qt::DockWidgetArea::RightDockWidgetArea);

    mainWindow->addDockWidget(Qt::LeftDockWidgetArea, sshDockWidget);

    d->widgetForWindow[mainWindow] = managerWidget;
    d->dockForWindow[mainWindow] = sshDockWidget;
    d->currentMainWindow = mainWindow;

    connect(managerWidget, &SSHManagerTreeWidget::requestNewTab, this, [mainWindow] {
        mainWindow->newTab();
    });
    
    connect(managerWidget, &SSHManagerTreeWidget::requestConnection, this, &SSHManagerPlugin::requestConnection);
    connect(managerWidget, &SSHManagerTreeWidget::requestQuickConnection, this, &SSHManagerPlugin::handleQuickConnection);

    connect(managerWidget, &SSHManagerTreeWidget::quickAccessShortcutChanged, this, [this, mainWindow](QKeySequence s) {
        mainWindow->actionCollection()->setDefaultShortcut(d->showQuickAccess, s);

        QString sequenceText = s.toString();
        QSettings settings;
        settings.beginGroup(QStringLiteral("plugins"));
        settings.beginGroup(QStringLiteral("sshplugin"));
        settings.setValue(QStringLiteral("ssh_shortcut"), sequenceText);
        settings.sync();
    });
}

QList<QAction *> SSHManagerPlugin::menuBarActions(Konsole::MainWindow *mainWindow) const
{
    Q_UNUSED(mainWindow)

    QAction *toggleVisibilityAction = new QAction(i18n("Show SSH Manager"), mainWindow);
    toggleVisibilityAction->setCheckable(true);
    mainWindow->actionCollection()->setDefaultShortcut(toggleVisibilityAction, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_F2));
    connect(toggleVisibilityAction, &QAction::triggered, d->dockForWindow[mainWindow], &QDockWidget::setVisible);
    connect(d->dockForWindow[mainWindow], &QDockWidget::visibilityChanged, toggleVisibilityAction, &QAction::setChecked);

    return {toggleVisibilityAction};
}

void SSHManagerPlugin::activeViewChanged(Konsole::SessionController *controller, Konsole::MainWindow *mainWindow)
{
    Q_ASSERT(controller);
    Q_ASSERT(mainWindow);

    auto terminalDisplay = controller->view();

    d->showQuickAccess->deleteLater();
    d->showQuickAccess = new QAction(i18n("Show Quick Access for SSH Actions"));

    QSettings settings;
    settings.beginGroup(QStringLiteral("plugins"));
    settings.beginGroup(QStringLiteral("sshplugin"));

    const QKeySequence def(Qt::CTRL | Qt::ALT | Qt::Key_H);
    const QString defText = def.toString();
    const QString entry = settings.value(QStringLiteral("ssh_shortcut"), defText).toString();
    const QKeySequence shortcutEntry(entry);

    mainWindow->actionCollection()->setDefaultShortcut(d->showQuickAccess, shortcutEntry);
    terminalDisplay->addAction(d->showQuickAccess);

    connect(d->showQuickAccess, &QAction::triggered, this, [this, terminalDisplay, controller] {
        auto bar = new KCommandBar(terminalDisplay->topLevelWidget());
        QList<QAction *> actions;
        for (int i = 0; i < d->model.rowCount(); i++) {
            QModelIndex folder = d->model.index(i, 0);
            for (int e = 0; e < d->model.rowCount(folder); e++) {
                QModelIndex idx = d->model.index(e, 0, folder);
                QAction *act = new QAction(idx.data().toString());
                connect(act, &QAction::triggered, this, [this, idx, controller] {
                    requestConnection(idx, controller);
                });
                actions.append(act);
            }
        }

        if (actions.isEmpty()) // no ssh config found, must give feedback to the user about that
        {
            const QString feedbackMessage = i18n("No saved SSH config found. You can add one on Plugins -> SSH Manager");
            const QString feedbackTitle = i18n("Plugins - SSH Manager");
            KMessageBox::error(terminalDisplay->topLevelWidget(), feedbackMessage, feedbackTitle);
            return;
        }

        QVector<KCommandBar::ActionGroup> groups;
        groups.push_back(KCommandBar::ActionGroup{i18n("SSH Entries"), actions});
        bar->setActions(groups);
        bar->show();
    });

    if (mainWindow) {
        d->widgetForWindow[mainWindow]->setCurrentController(controller);
        d->currentMainWindow = mainWindow;
    }
    
}

void SSHManagerPlugin::requestConnection(const QModelIndex &idx, Konsole::SessionController *controller)
{
    if (!controller) {
        return;
    }
    
    // Index should already be from source model
    if (idx.parent() == d->model.invisibleRootItem()->index()) {
        return;
    }

    auto item = d->model.itemFromIndex(idx);
    auto data = item->data(SSHManagerModel::SSHRole).value<SSHConfigurationData>();

#ifndef Q_OS_WIN
    // Check if the current shell is idle (running a known shell process)
    bool shellIsIdle = false;
    Konsole::ProcessInfo *info = controller->session()->getProcessInfo();
    bool ok = false;
    if (info) {
        QString processName = info->name(&ok);
        if (ok) {
            const QStringList shells = {QStringLiteral("fish"),
                                        QStringLiteral("bash"),
                                        QStringLiteral("dash"),
                                        QStringLiteral("sh"),
                                        QStringLiteral("csh"),
                                        QStringLiteral("ksh"),
                                        QStringLiteral("zsh")};
            shellIsIdle = shells.contains(processName);
        }
    }

    if (!shellIsIdle) {
        // Shell is busy (running vim, another ssh, etc.) or PTY not ready.
        // Open a new tab and connect there once the session's shell is ready.
        Konsole::MainWindow *mainWindow = d->currentMainWindow;
        if (!mainWindow && controller->view()) {
            mainWindow = qobject_cast<Konsole::MainWindow *>(controller->view()->window());
        }
        if (mainWindow) {
            // Create the new tab — this triggers activeViewChanged, but we do NOT
            // use pendingConnectionIndex. Instead we connect to the new session's
            // started() signal so we wait for the PTY to be ready.
            mainWindow->newTab();

            // The new tab's controller is now the active one
            auto *viewManager = mainWindow->viewManager();
            auto *newController = viewManager->activeViewController();
            if (newController && newController != controller) {
                // Wait for the session's shell to actually start before sending commands
                connect(newController->session(), &Konsole::Session::started, this,
                        [this, data, newController]() {
                            startConnection(data, newController);
                        }, Qt::SingleShotConnection);
            }
        }
        return;
    }
#else
    // FIXME: Can we do this on windows?
#endif

    startConnection(data, controller);
}

void SSHManagerPlugin::handleQuickConnection(const SSHConfigurationData &data, Konsole::SessionController *controller)
{
    if (!controller) {
        return;
    }

#ifndef Q_OS_WIN
    bool shellIsIdle = false;
    Konsole::ProcessInfo *info = controller->session()->getProcessInfo();
    bool ok = false;
    if (info) {
        QString processName = info->name(&ok);
        if (ok) {
            const QStringList shells = {QStringLiteral("fish"),
                                        QStringLiteral("bash"),
                                        QStringLiteral("dash"),
                                        QStringLiteral("sh"),
                                        QStringLiteral("csh"),
                                        QStringLiteral("ksh"),
                                        QStringLiteral("zsh")};
            shellIsIdle = shells.contains(processName);
        }
    }

    if (!shellIsIdle) {
        Konsole::MainWindow *mainWindow = d->currentMainWindow;
        if (!mainWindow && controller->view()) {
            mainWindow = qobject_cast<Konsole::MainWindow *>(controller->view()->window());
        }
        if (mainWindow) {
            mainWindow->newTab();
            auto *viewManager = mainWindow->viewManager();
            auto *newController = viewManager->activeViewController();
            if (newController && newController != controller) {
                connect(newController->session(), &Konsole::Session::started, this,
                        [this, data, newController]() {
                            startConnection(data, newController);
                        }, Qt::SingleShotConnection);
            }
        }
        return;
    }
#endif

    startConnection(data, controller);
}

void SSHManagerPlugin::startConnection(const SSHConfigurationData &data, Konsole::SessionController *controller)
{
    if (!controller || !controller->session()) {
        return;
    }

    QString sshCommand = QStringLiteral("ssh ");
    if (data.useSshConfig) {
        sshCommand += data.name;
    } else {
        if (!data.password.isEmpty()) {
            sshCommand = QStringLiteral("sshpass -p '%1' ").arg(data.password) + sshCommand;
        } else if (!data.sshKeyPassphrase.isEmpty()) {
            // Use sshpass with -P to match the "Enter passphrase" prompt from ssh
            sshCommand = QStringLiteral("sshpass -P 'passphrase' -p '%1' ").arg(data.sshKeyPassphrase) + sshCommand;
        }

        sshCommand += QStringLiteral("-o ConnectTimeout=15 ");

        if (data.autoAcceptKeys) {
             sshCommand += QStringLiteral("-o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null ");
        }

        if (data.useProxy && !data.proxyIp.isEmpty() && !data.proxyPort.isEmpty()) {
             QString proxyCmd = QStringLiteral("ncat --proxy-type socks5 ");
             if (!data.proxyUsername.isEmpty()) {
                 proxyCmd += QStringLiteral("--proxy-auth %1:%2 ").arg(data.proxyUsername, data.proxyPassword);
             }
             proxyCmd += QStringLiteral("--proxy %1:%2 %h %p").arg(data.proxyIp, data.proxyPort);
             
             sshCommand += QStringLiteral("-o ProxyCommand='%1' ").arg(proxyCmd);
        }

        if (data.sshKey.length()) {
            // Ensure the key file has correct permissions (600) so SSH doesn't reject it
            QFile::setPermissions(data.sshKey, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
            sshCommand += QStringLiteral("-i %1 ").arg(data.sshKey);
        }

        if (data.port.length()) {
            sshCommand += QStringLiteral("-p %1 ").arg(data.port);
        }

        if (!data.username.isEmpty()) {
            sshCommand += data.username + QLatin1Char('@');
        }
        
        if (!data.host.isEmpty()) {
            sshCommand += data.host;
        }
    }

    // Set tab title to the SSH identifier, or hostname if no name was set
    const QString tabTitle = data.name.isEmpty() ? data.host : data.name;
    controller->session()->setTitle(Konsole::Session::NameRole, tabTitle);
    controller->session()->setTabTitleFormat(Konsole::Session::LocalTabTitle, tabTitle);
    controller->session()->setTabTitleFormat(Konsole::Session::RemoteTabTitle, tabTitle);
    controller->session()->tabTitleSetByUser(true);

    // SSH -E logs errors to a file without redirecting stderr, so the
    // password prompt appears normally. On failure, the log is parsed
    // for a specific reason. Leading space keeps it out of shell history.
    const QString sshErrLog = QStringLiteral("/tmp/konsole_ssh_err_%1.log")
        .arg(QUuid::createUuid().toString(QUuid::Id128));

    // Status file: the script writes "connected", "disconnected", or "failed"
    // so C++ can poll and update the tab indicator.
    const QString sshStatusFile = QStringLiteral("/tmp/konsole_ssh_status_%1")
        .arg(QUuid::createUuid().toString(QUuid::Id128));

    const QString greenOk = QStringLiteral("printf '\\033[32mOK\\033[0m\\n'; echo connected > '%1'").arg(sshStatusFile);
    const QString localCmdOpts = QStringLiteral("-o PermitLocalCommand=yes -o LocalCommand=\"%1\" ").arg(greenOk);

    const QString sshOpts = QStringLiteral("-E '%1' ").arg(sshErrLog) + localCmdOpts;
    int sshPos = sshCommand.lastIndexOf(QStringLiteral("ssh "));
    if (sshPos >= 0) {
        sshCommand.insert(sshPos + 4, sshOpts);
    }

    // Write the command to a temp script so only a short ". /tmp/..." is sent
    // through the PTY — prevents the long command from leaking through echo.
    const QString scriptPath = QStringLiteral("/tmp/konsole_ssh_cmd_%1.sh")
        .arg(QUuid::createUuid().toString(QUuid::Id128));

    QString script;
    script += QStringLiteral("clear; printf 'Connecting to %1...\\n'; ").arg(tabTitle);
    script += sshCommand;
    script += QStringLiteral(" || { printf ' \\033[31mFAILED\\033[0m\\n';");
    script += QStringLiteral(" _e=$(cat '%1');").arg(sshErrLog);
    script += QStringLiteral(
        " case \"$_e\" in"
        "  *'Permission denied'*)      _r='Authentication failed (wrong password or key)';;"
        "  *'Connection refused'*)      _r='Connection refused (host is not accepting SSH)';;"
        "  *'Connection timed out'*)    _r='Connection timed out';;"
        "  *'No route to host'*)        _r='No route to host (network unreachable)';;"
        "  *'Could not resolve'*)       _r='Could not resolve hostname';;"
        "  *'Host key verification'*)   _r='Host key verification failed';;"
        "  *'Connection reset'*)        _r='Connection reset by remote host';;"
        "  *'Network is unreachable'*)  _r='Network is unreachable';;"
        "  *'Connection closed'*)       _r='Connection closed by remote host';;"
        "  *'incorrect password'*|*'Wrong passphrase'*) _r='Incorrect password or passphrase';;"
        "  *)                           _r=\"$_e\";;"
        " esac;");
    script += QStringLiteral(" [ -n \"$_r\" ] && echo -e '  \\033[33m'\"$_r\"'\\033[0m';");
    script += QStringLiteral(" echo failed > '%1';").arg(sshStatusFile);
    script += QStringLiteral(" rm -f '%1' '%2'; exec bash; };").arg(sshErrLog, scriptPath);
    // SSH exited normally (user typed 'exit' or connection dropped) — mark disconnected.
    script += QStringLiteral(" echo disconnected > '%1';").arg(sshStatusFile);
    script += QStringLiteral(" rm -f '%1' '%2'\n").arg(sshErrLog, scriptPath);

    QFile scriptFile(scriptPath);
    if (scriptFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        scriptFile.write(script.toUtf8());
        scriptFile.close();
        scriptFile.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    }

    const QString wrappedCommand = QStringLiteral(" . %1").arg(scriptPath);

    QPointer<Konsole::Session> session = controller->session();
    session->setEchoEnabled(false);
    session->sendTextToTerminal(wrappedCommand, QLatin1Char('\r'));

    // Do NOT use a timer to re-enable echo. SSH puts the PTY into raw mode
    // itself once it connects. Re-enabling echo while SSH is active causes
    // double echo. Instead, re-enable echo only when SSH exits (disconnect
    // or failure), detected by the status polling timer below.

    // SSH status indicator: poll the status file to detect connect/disconnect/fail.
    d->sessionSshState[controller->session()] = IKonsolePlugin::SshConnecting;
    Q_EMIT sshStateChanged(controller->session(), IKonsolePlugin::SshConnecting);

    auto *statusTimer = new QTimer(controller->session());
    statusTimer->setInterval(500);
    auto lastState = std::make_shared<int>(IKonsolePlugin::SshConnecting);
    connect(statusTimer, &QTimer::timeout, this, [this, session, statusTimer, sshStatusFile, lastState]() {
        QFile f(sshStatusFile);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return; // File doesn't exist yet — still connecting
        }
        const QString status = QString::fromUtf8(f.readAll()).trimmed();
        f.close();
        if (status == QLatin1String("connected")) {
            if (*lastState != IKonsolePlugin::SshConnected) {
                *lastState = IKonsolePlugin::SshConnected;
                if (session) {
                    d->sessionSshState[session] = IKonsolePlugin::SshConnected;
                    Q_EMIT sshStateChanged(session, IKonsolePlugin::SshConnected);
                }
            }
            // Keep polling — need to detect disconnect later
        } else if (status == QLatin1String("disconnected") || status == QLatin1String("failed")) {
            statusTimer->stop();
            // Re-enable echo now that SSH has exited and the user is
            // back in a local shell.
            if (session) {
                session->setEchoEnabled(true);
                d->sessionSshState[session] = IKonsolePlugin::SshDisconnected;
                Q_EMIT sshStateChanged(session, IKonsolePlugin::SshDisconnected);
            }
            QFile::remove(sshStatusFile);
        }
    });
    connect(controller->session(), &Konsole::Session::finished, this, [this, session, statusTimer, sshStatusFile]() {
        statusTimer->stop();
        QFile::remove(sshStatusFile);
        if (session) {
            session->setEchoEnabled(true);
            d->sessionSshState.remove(session);
            Q_EMIT sshStateChanged(session, IKonsolePlugin::SshDisconnected);
        }
    });
    statusTimer->start();

    // Track this session so it can be duplicated from the tab context menu.
    d->activeSessionData[controller->session()] = data;
    connect(controller->session(), &Konsole::Session::finished, this, [this, session]() {
        if (session) {
            d->activeSessionData.remove(session);
        }
    });

    // Apply custom tab icon and color from the SSH profile
    if (controller->view()) {
        Konsole::TabbedViewContainer *container = nullptr;
        // Walk parent chain to find the owning container
        QWidget *w = controller->view()->parentWidget();
        while (w) {
            if (auto *c = qobject_cast<Konsole::TabbedViewContainer *>(w)) {
                container = c;
                break;
            }
            w = w->parentWidget();
        }
        if (container) {
            auto *splitter = qobject_cast<Konsole::ViewSplitter *>(controller->view()->parentWidget());
            if (splitter) {
                int tabIdx = container->indexOf(splitter->getToplevelSplitter());
                if (tabIdx >= 0) {
                    if (!data.tabIcon.isEmpty()) {
                        container->setTabCustomIcon(tabIdx, QIcon::fromTheme(data.tabIcon));
                    }
                    if (!data.tabColor.isEmpty()) {
                        container->setTabColorByIndex(tabIdx, QColor(data.tabColor));
                    }
                }
            }
        }
    }

    if (controller->session()->views().count()) {
        controller->session()->views().at(0)->setFocus();
    }
}

bool SSHManagerPlugin::canDuplicateSession(Konsole::Session *session) const
{
    return session && d->activeSessionData.contains(session);
}

void SSHManagerPlugin::duplicateSession(Konsole::Session *session, Konsole::MainWindow *mainWindow)
{
    if (!session || !mainWindow) {
        return;
    }

    auto it = d->activeSessionData.find(session);
    if (it == d->activeSessionData.end()) {
        return;
    }

    SSHConfigurationData data = it.value();
    mainWindow->newTab();

    auto *newController = mainWindow->viewManager()->activeViewController();
    if (!newController) {
        return;
    }

    Konsole::Session *newSession = newController->session();
    if (newSession->isRunning()) {
        startConnection(data, newController);
    } else {
        connect(newSession, &Konsole::Session::started, this,
                [this, data, newController]() {
                    startConnection(data, newController);
                }, Qt::SingleShotConnection);
    }
}

bool SSHManagerPlugin::canReconnectSession(Konsole::Session *session) const
{
    if (!session || !d->activeSessionData.contains(session)) {
        return false;
    }
    int state = d->sessionSshState.value(session, IKonsolePlugin::NoSsh);
    return state == IKonsolePlugin::SshConnecting || state == IKonsolePlugin::SshConnected;
}

void SSHManagerPlugin::reconnectSession(Konsole::Session *session, Konsole::MainWindow *mainWindow)
{
    Q_UNUSED(mainWindow)

    if (!session) {
        return;
    }

    auto it = d->activeSessionData.find(session);
    if (it == d->activeSessionData.end()) {
        return;
    }

    SSHConfigurationData data = it.value();

    // Find the SessionController for this session
    Konsole::SessionController *controller = nullptr;
    const auto views = session->views();
    for (auto *view : views) {
        if (view->sessionController() && view->sessionController()->session() == session) {
            controller = view->sessionController();
            break;
        }
    }
    if (!controller) {
        return;
    }

    // Send exit to terminate any active SSH, then reconnect after a short delay
    session->sendTextToTerminal(QStringLiteral("exit"), QLatin1Char('\r'));

    QTimer::singleShot(300, this, [this, data, controller]() {
        if (controller && controller->session()) {
            startConnection(data, controller);
        }
    });
}

Konsole::SshSessionData SSHManagerPlugin::getSessionSshData(Konsole::Session *session) const
{
    if (!session) {
        return {};
    }
    auto it = d->activeSessionData.find(session);
    if (it == d->activeSessionData.end()) {
        return {};
    }
    const auto &cfg = it.value();
    Konsole::SshSessionData data;
    data.valid = true;
    data.host = cfg.host;
    data.port = cfg.port;
    data.username = cfg.username;
    data.password = cfg.password;
    data.sshKey = cfg.sshKey;
    data.sshKeyPassphrase = cfg.sshKeyPassphrase;
    return data;
}

bool SSHManagerPlugin::canOpenSftp(Konsole::Session *session) const
{
    if (!session) {
        return false;
    }
    return d->activeSessionData.contains(session);
}

// Helper: build and open the sftp:// URL in the default file manager.
static void launchSftpUrl(const QString &host, int port, const QString &username, const QString &password, Konsole::MainWindow *mainWindow)
{
    QUrl sftpUrl;
    sftpUrl.setScheme(QStringLiteral("sftp"));
    sftpUrl.setHost(host);
    sftpUrl.setPort(port);
    sftpUrl.setUserName(username);
    if (!password.isEmpty()) {
        sftpUrl.setPassword(password);
    }
    if (username == QLatin1String("root")) {
        sftpUrl.setPath(QStringLiteral("/root"));
    } else {
        sftpUrl.setPath(QStringLiteral("/home/") + username);
    }

    // Pre-cache via KPasswdServer as backup
    if (!password.isEmpty()) {
        KIO::AuthInfo authInfo;
        authInfo.url = sftpUrl;
        authInfo.username = username;
        authInfo.password = password;
        authInfo.keepPassword = true;

        KPasswdServerClient passwdClient;
        passwdClient.addAuthInfo(authInfo, mainWindow ? mainWindow->winId() : 0);
    }

    auto *job = new KIO::OpenUrlJob(sftpUrl);
    job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, mainWindow));
    job->start();
}

void SSHManagerPlugin::openSftp(Konsole::Session *session, Konsole::MainWindow *mainWindow)
{
    if (!session) {
        return;
    }

    auto it = d->activeSessionData.find(session);
    if (it == d->activeSessionData.end()) {
        return;
    }
    const auto &cfg = it.value();

    int remotePort = cfg.port.isEmpty() ? 22 : cfg.port.toInt();
    bool needsProxy = cfg.useProxy && !cfg.proxyIp.isEmpty() && !cfg.proxyPort.isEmpty();

    if (!needsProxy) {
        launchSftpUrl(cfg.host, remotePort, cfg.username, cfg.password, mainWindow);
        return;
    }

    // Reuse an existing tunnel if the process is still running.
    if (d->sftpTunnelProcesses.contains(session)) {
        auto *existing = d->sftpTunnelProcesses.value(session);
        if (existing && existing->state() == QProcess::Running) {
            int existingPort = d->sftpTunnelPorts.value(session);
            launchSftpUrl(QStringLiteral("localhost"), existingPort, cfg.username, cfg.password, mainWindow);
            return;
        }
        // Tunnel died — clean up
        if (existing) {
            existing->deleteLater();
        }
        d->sftpTunnelProcesses.remove(session);
        d->sftpTunnelPorts.remove(session);
    }

    int localPort = findFreePort();
    if (localPort == 0) {
        launchSftpUrl(cfg.host, remotePort, cfg.username, cfg.password, mainWindow);
        return;
    }

    // Build SSH args directly (no shell quoting needed — QProcess passes
    // each argument separately when using the QStringList overload).
    QStringList sshArgs;
    sshArgs << QStringLiteral("-N");
    sshArgs << QStringLiteral("-o") << QStringLiteral("ExitOnForwardFailure=yes");
    sshArgs << QStringLiteral("-o") << QStringLiteral("ConnectTimeout=15");
    sshArgs << QStringLiteral("-o") << QStringLiteral("ServerAliveInterval=60");
    // Always bypass host key checks — this is a non-interactive background
    // process with no TTY for prompts.
    sshArgs << QStringLiteral("-o") << QStringLiteral("StrictHostKeyChecking=no");
    sshArgs << QStringLiteral("-o") << QStringLiteral("UserKnownHostsFile=/dev/null");

    // ProxyCommand: same ncat SOCKS5 command as startConnection()
    QString proxyCmd = QStringLiteral("ncat --proxy-type socks5 ");
    if (!cfg.proxyUsername.isEmpty()) {
        proxyCmd += QStringLiteral("--proxy-auth %1:%2 ").arg(cfg.proxyUsername, cfg.proxyPassword);
    }
    proxyCmd += QStringLiteral("--proxy %1:%2 %h %p").arg(cfg.proxyIp, cfg.proxyPort);
    sshArgs << QStringLiteral("-o") << QStringLiteral("ProxyCommand=%1").arg(proxyCmd);

    if (!cfg.sshKey.isEmpty()) {
        sshArgs << QStringLiteral("-i") << cfg.sshKey;
    }

    // Local forward: localhost:localPort → remote's localhost:sshPort
    sshArgs << QStringLiteral("-L") << QStringLiteral("%1:localhost:%2").arg(localPort).arg(remotePort);

    if (!cfg.port.isEmpty()) {
        sshArgs << QStringLiteral("-p") << cfg.port;
    }

    QString destination;
    if (!cfg.username.isEmpty()) {
        destination = cfg.username + QLatin1Char('@');
    }
    destination += cfg.host;
    sshArgs << destination;

    // Determine the executable: sshpass wrapping ssh, or bare ssh.
    QString program;
    QStringList fullArgs;
    if (!cfg.password.isEmpty()) {
        program = QStringLiteral("sshpass");
        fullArgs << QStringLiteral("-e") << QStringLiteral("ssh") << sshArgs;
    } else if (!cfg.sshKeyPassphrase.isEmpty()) {
        program = QStringLiteral("sshpass");
        fullArgs << QStringLiteral("-P") << QStringLiteral("passphrase") << QStringLiteral("-e") << QStringLiteral("ssh") << sshArgs;
    } else {
        program = QStringLiteral("ssh");
        fullArgs = sshArgs;
    }

    auto *process = new QProcess(mainWindow);

    // Pass password via SSHPASS env var (avoids shell quoting issues).
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (!cfg.password.isEmpty()) {
        env.insert(QStringLiteral("SSHPASS"), cfg.password);
    } else if (!cfg.sshKeyPassphrase.isEmpty()) {
        env.insert(QStringLiteral("SSHPASS"), cfg.sshKeyPassphrase);
    }
    process->setProcessEnvironment(env);
    process->setProcessChannelMode(QProcess::MergedChannels);
    process->start(program, fullArgs);

    qDebug() << "SFTP tunnel:" << program << fullArgs;

    // Capture values for the lambdas.
    QString username = cfg.username;
    QString password = cfg.password;
    QPointer<Konsole::Session> sessionPtr = session;
    QPointer<QProcess> processPtr = process;

    // ssh -N stays in the foreground.  We poll the local port to detect
    // when the tunnel is ready, rather than relying on ssh -f + fork.
    auto *readyTimer = new QTimer(process);
    readyTimer->setInterval(300);

    auto *timeoutTimer = new QTimer(process);
    timeoutTimer->setSingleShot(true);
    timeoutTimer->setInterval(15000);

    connect(readyTimer, &QTimer::timeout, mainWindow, [=, this]() {
        // Try connecting to the local forwarded port.
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            return;
        }
        struct sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = htons(static_cast<uint16_t>(localPort));
        bool connected = (::connect(sock, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) == 0);
        ::close(sock);

        if (!connected) {
            return;
        }
        // Tunnel is ready.
        readyTimer->stop();
        timeoutTimer->stop();
        if (sessionPtr) {
            d->sftpTunnelProcesses[sessionPtr] = processPtr;
            d->sftpTunnelPorts[sessionPtr] = localPort;
        }
        launchSftpUrl(QStringLiteral("localhost"), localPort, username, password, mainWindow);
    });

    connect(timeoutTimer, &QTimer::timeout, mainWindow, [=]() {
        readyTimer->stop();
        if (processPtr && processPtr->state() != QProcess::NotRunning) {
            processPtr->kill();
        }
        KMessageBox::error(mainWindow, i18n("Timed out creating SFTP tunnel through proxy."), i18n("SFTP Error"));
    });

    // If ssh exits before the tunnel is ready, it failed.
    connect(process, &QProcess::finished, mainWindow, [=, this](int exitCode) {
        if (readyTimer->isActive()) {
            readyTimer->stop();
            timeoutTimer->stop();
            QString output = QString::fromUtf8(process->readAll()).trimmed();
            qWarning() << "SFTP proxy tunnel exited early (code" << exitCode << ")" << output;
            KMessageBox::error(mainWindow, i18n("Failed to create SFTP tunnel through proxy (exit code %1).\n\n%2", exitCode, output), i18n("SFTP Error"));
        }
        process->deleteLater();
        if (sessionPtr) {
            d->sftpTunnelProcesses.remove(sessionPtr);
            d->sftpTunnelPorts.remove(sessionPtr);
        }
    });

    readyTimer->start();
    timeoutTimer->start();

    // Clean up the tunnel when the session ends.
    connect(session, &Konsole::Session::finished, this, [this, sessionPtr]() {
        if (!sessionPtr) {
            return;
        }
        auto *proc = d->sftpTunnelProcesses.take(sessionPtr);
        d->sftpTunnelPorts.remove(sessionPtr);
        if (proc && proc->state() != QProcess::NotRunning) {
            proc->terminate();
        }
    });
}

#include "moc_sshmanagerplugin.cpp"
#include "sshmanagerplugin.moc"
