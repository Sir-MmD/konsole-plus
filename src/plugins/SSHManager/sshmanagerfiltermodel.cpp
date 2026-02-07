/*  This file was part of the KDE libraries

    SPDX-FileCopyrightText: 2021 Tomaz Canabrava <tcanabrava@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "sshmanagerfiltermodel.h"
#include "sshconfigurationdata.h"
#include "sshmanagermodel.h"

SSHManagerFilterModel::SSHManagerFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setSortCaseSensitivity(Qt::CaseInsensitive);
}

bool SSHManagerFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    auto text = filterRegularExpression().pattern();
    if (text.isEmpty()) {
        return true;
    }

    const QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
    if (sourceModel()->rowCount(idx) != 0) {
        return true;
    }

    const QString lowerText = text.toLower();
    bool result = idx.data(Qt::DisplayRole).toString().toLower().contains(lowerText);

    // Also match against the hostname
    if (!result) {
        const QVariant sshData = idx.data(SSHManagerModel::SSHRole);
        if (sshData.isValid()) {
            const auto data = sshData.value<SSHConfigurationData>();
            result = data.host.toLower().contains(lowerText);
        }
    }

    return m_invertFilter == false ? result : !result;
}

void SSHManagerFilterModel::setInvertFilter(bool invert)
{
    m_invertFilter = invert;
    invalidateFilter();
}

bool SSHManagerFilterModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    const QString leftText = left.data(Qt::DisplayRole).toString();
    const QString rightText = right.data(Qt::DisplayRole).toString();
    return QString::localeAwareCompare(leftText, rightText) < 0;
}

#include "moc_sshmanagerfiltermodel.cpp"
