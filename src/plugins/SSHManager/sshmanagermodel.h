/*  This file was part of the KDE libraries

    SPDX-FileCopyrightText: 2021 Tomaz Canabrava <tcanabrava@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef SSHMANAGERMODEL_H
#define SSHMANAGERMODEL_H

#include <QFileSystemWatcher>
#include <QJsonDocument>
#include <QStandardItemModel>

#include <memory>
#include <optional>

namespace Konsole
{
class SessionController;
class Session;
}

class SSHConfigurationData;

class SSHManagerModel : public QStandardItemModel
{
    Q_OBJECT
public:
    enum Roles {
        SSHRole = Qt::UserRole + 1,
    };

    enum Column {
        NameColumn = 0,
        HostColumn = 1,
        ProxyColumn = 2,
        ColumnCount = 3,
    };

    explicit SSHManagerModel(QObject *parent = nullptr);
    ~SSHManagerModel() override;

    void setSessionController(Konsole::SessionController *controller);

    /** Connected to Session::hostnameChanged, tries to set the profile to
     * the current configured profile for the specified SSH host
     */
    Q_SLOT void triggerProfileChange(const QString &sshHost);

    QStandardItem *addTopLevelItem(const QString &toplevel);
    void addChildItem(const SSHConfigurationData &config, const QString &parentName);
    void editChildItem(const SSHConfigurationData &config, const QModelIndex &idx, const QString &newFolder = {});
    void removeIndex(const QModelIndex &idx);
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QStringList folders() const;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void startImportFromSshConfig();
    void importFromSshConfigFile(const QString &file);
    void load();
    void save();

    bool hasHost(const QString &hostName) const;
    std::optional<QString> profileForHost(const QString &host) const;
    void setManageProfile(bool manage);
    bool getManageProfile();

    // Encryption
    void setMasterPassword(const QString &password);
    bool hasMasterPassword() const;
    bool verifyMasterPassword(const QString &password) const;
    void enableEncryption(const QString &password);
    void disableEncryption();
    bool isEncryptionEnabled() const;
    void decryptAll();

    // Import/Export
    QJsonDocument exportToJson(const QString &exportPassword = {}) const;
    bool importFromJson(const QJsonDocument &doc, const QString &importPassword = {});

private:
    QString maybeEncrypt(const QString &value) const;
    QString maybeDecrypt(const QString &value) const;

    QStandardItem *m_sshConfigTopLevelItem = nullptr;
    QFileSystemWatcher m_sshConfigWatcher;
    Konsole::Session *m_session = nullptr;

    QHash<Konsole::Session *, QString> m_sessionToProfileName;

    bool manageProfile = false;

    // Encryption state (in-memory only, never persisted directly)
    QString m_masterPassword;
    bool m_encryptionEnabled = false;
    QString m_encryptionSalt;
    QString m_encryptionVerifier;
};

#endif
