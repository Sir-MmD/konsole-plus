/*
    SPDX-FileCopyrightText: 2026 Konsole Plus contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "TabManagerWidget.h"

#include <QHeaderView>
#include <QStandardItemModel>
#include <QTreeView>
#include <QVBoxLayout>

#include <KLocalizedString>

#include "ViewContainer.h"
#include "ViewManager.h"
#include "ViewSplitter.h"
#include "session/SessionController.h"
#include "terminalDisplay/TerminalDisplay.h"

using namespace Konsole;

TabManagerWidget::TabManagerWidget(ViewManager *viewManager, QWidget *parent)
    : QWidget(parent)
    , m_viewManager(viewManager)
    , m_treeView(new QTreeView(this))
    , m_model(new QStandardItemModel(this))
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_treeView);

    m_treeView->setModel(m_model);
    m_treeView->setHeaderHidden(true);
    m_treeView->setRootIsDecorated(true);
    m_treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_treeView->setIndentation(16);

    // Connect to all existing containers
    const auto containers = m_viewManager->containers();
    for (auto *container : containers) {
        connectContainer(container);
    }

    connect(m_viewManager, &ViewManager::containerAdded, this, &TabManagerWidget::onContainerAdded);
    connect(m_viewManager, &ViewManager::containerRemoved, this, &TabManagerWidget::onContainerRemoved);

    connect(m_treeView, &QTreeView::clicked, this, &TabManagerWidget::onItemClicked);

    refresh();
}

void TabManagerWidget::connectContainer(TabbedViewContainer *container)
{
    connect(container, &TabbedViewContainer::viewAdded, this, &TabManagerWidget::onViewAdded, Qt::UniqueConnection);
    connect(container, &TabbedViewContainer::viewRemoved, this, &TabManagerWidget::onViewRemoved, Qt::UniqueConnection);
    connect(container, &TabbedViewContainer::activeViewChanged, this, &TabManagerWidget::onActiveViewChanged, Qt::UniqueConnection);
    connect(container, &QTabWidget::tabBarDoubleClicked, this, [this](int) {
        refresh();
    });
}

void TabManagerWidget::onContainerAdded(TabbedViewContainer *container)
{
    connectContainer(container);
    refresh();
}

void TabManagerWidget::onContainerRemoved(TabbedViewContainer *container)
{
    Q_UNUSED(container)
    refresh();
}

void TabManagerWidget::refresh()
{
    m_model->clear();

    const auto containers = m_viewManager->containers();
    const bool multiPane = containers.count() > 1;

    for (int ci = 0; ci < containers.count(); ci++) {
        auto *container = containers[ci];
        if (!container) continue;

        QStandardItem *paneItem = nullptr;

        // Only show pane-level items when there are multiple panes
        if (multiPane) {
            paneItem = new QStandardItem(i18n("Pane %1", ci + 1));
            paneItem->setData(ci, ContainerIndexRole);
            paneItem->setData(false, IsTabRole);
            m_model->appendRow(paneItem);
        }

        for (int i = 0; i < container->count(); i++) {
            auto *splitter = container->viewSplitterAt(i);
            if (!splitter) continue;

            auto *tabItem = new QStandardItem(container->tabIcon(i), container->tabText(i));
            tabItem->setData(i, TabIndexRole);
            tabItem->setData(ci, ContainerIndexRole);
            tabItem->setData(true, IsTabRole);

            auto terminals = splitter->findChildren<TerminalDisplay *>();
            if (terminals.count() == 1) {
                auto *controller = terminals.first()->sessionController();
                if (controller) {
                    connect(controller, &ViewProperties::titleChanged, this, &TabManagerWidget::onTitleChanged, Qt::UniqueConnection);
                    connect(controller, &ViewProperties::iconChanged, this, &TabManagerWidget::onIconChanged, Qt::UniqueConnection);
                }
            }

            if (paneItem) {
                paneItem->appendRow(tabItem);
            } else {
                m_model->appendRow(tabItem);
            }
        }
    }

    m_treeView->expandAll();
    highlightActiveTab();
}

void TabManagerWidget::onViewAdded(TerminalDisplay *view)
{
    Q_UNUSED(view)
    refresh();
}

void TabManagerWidget::onViewRemoved()
{
    refresh();
}

void TabManagerWidget::onActiveViewChanged(TerminalDisplay *view)
{
    Q_UNUSED(view)
    highlightActiveTab();
}

void TabManagerWidget::onTitleChanged(ViewProperties *properties)
{
    Q_UNUSED(properties)
    refresh();
}

void TabManagerWidget::onIconChanged(ViewProperties *properties)
{
    Q_UNUSED(properties)
    refresh();
}

void TabManagerWidget::onItemClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;

    m_updatingSelection = true;

    bool isTab = index.data(IsTabRole).toBool();
    int containerIdx = index.data(ContainerIndexRole).toInt();

    const auto containers = m_viewManager->containers();
    if (containerIdx < 0 || containerIdx >= containers.count()) {
        m_updatingSelection = false;
        return;
    }

    auto *container = containers[containerIdx];

    if (isTab) {
        int tabIndex = index.data(TabIndexRole).toInt();
        container->setCurrentIndex(tabIndex);
        // Focus a terminal in that tab
        auto *splitter = container->viewSplitterAt(tabIndex);
        if (splitter) {
            auto *td = splitter->activeTerminalDisplay();
            if (td) td->setFocus();
        }
    } else {
        // Clicked on a pane item — focus the active terminal in that pane
        auto *splitter = container->activeViewSplitter();
        if (splitter) {
            auto *td = splitter->activeTerminalDisplay();
            if (td) td->setFocus();
        }
    }

    m_updatingSelection = false;
}

void TabManagerWidget::highlightActiveTab()
{
    if (m_updatingSelection) return;

    auto *activeContainer = m_viewManager->activeContainer();
    if (!activeContainer) return;

    const auto containers = m_viewManager->containers();
    int containerIdx = containers.indexOf(activeContainer);
    int currentTabIdx = activeContainer->currentIndex();

    if (containerIdx < 0 || currentTabIdx < 0) return;

    m_updatingSelection = true;

    // Find the matching model item
    const bool multiPane = containers.count() > 1;
    if (multiPane) {
        // Pane items are top-level, tabs are children
        if (containerIdx < m_model->rowCount()) {
            auto *paneItem = m_model->item(containerIdx);
            if (paneItem && currentTabIdx < paneItem->rowCount()) {
                QModelIndex modelIndex = paneItem->child(currentTabIdx)->index();
                m_treeView->setCurrentIndex(modelIndex);
            }
        }
    } else {
        // Tabs are top-level
        if (currentTabIdx < m_model->rowCount()) {
            QModelIndex modelIndex = m_model->index(currentTabIdx, 0);
            m_treeView->setCurrentIndex(modelIndex);
        }
    }

    m_updatingSelection = false;
}

QStandardItem *TabManagerWidget::findItemForTerminal(int terminalId) const
{
    for (int i = 0; i < m_model->rowCount(); i++) {
        auto *item = m_model->item(i);
        for (int j = 0; j < item->rowCount(); j++) {
            auto *childItem = item->child(j);
            if (childItem && childItem->data(TerminalIdRole).toInt() == terminalId) {
                return childItem;
            }
        }
    }
    return nullptr;
}
