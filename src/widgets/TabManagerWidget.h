/*
    SPDX-FileCopyrightText: 2026 Konsole Plus contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef TABMANAGERWIDGET_H
#define TABMANAGERWIDGET_H

#include <QWidget>

#include "konsoleprivate_export.h"

class QTreeView;
class QStandardItemModel;
class QStandardItem;

namespace Konsole
{
class ViewManager;
class TerminalDisplay;
class ViewProperties;
class TabbedViewContainer;

class KONSOLEPRIVATE_EXPORT TabManagerWidget : public QWidget
{
    Q_OBJECT
public:
    enum Role {
        TabIndexRole = Qt::UserRole + 1,
        TerminalIdRole,
        IsTabRole,
    };

    explicit TabManagerWidget(ViewManager *viewManager, QWidget *parent = nullptr);

    void refresh();

private Q_SLOTS:
    void onViewAdded(TerminalDisplay *view);
    void onViewRemoved();
    void onActiveViewChanged(TerminalDisplay *view);
    void onTitleChanged(ViewProperties *properties);
    void onIconChanged(ViewProperties *properties);
    void onItemClicked(const QModelIndex &index);

private:
    void highlightActiveTab();
    QStandardItem *findItemForTerminal(int terminalId) const;

    ViewManager *m_viewManager;
    QTreeView *m_treeView;
    QStandardItemModel *m_model;
    bool m_updatingSelection = false;
};

}

#endif // TABMANAGERWIDGET_H
