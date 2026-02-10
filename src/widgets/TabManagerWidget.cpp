/*
    SPDX-FileCopyrightText: 2026 Konsole Plus contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "TabManagerWidget.h"

#include <QHeaderView>
#include <QStandardItemModel>
#include <QTreeView>
#include <QVBoxLayout>

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

    auto *container = m_viewManager->activeContainer();
    if (container) {
        connect(container, &TabbedViewContainer::viewAdded, this, &TabManagerWidget::onViewAdded);
        connect(container, &TabbedViewContainer::viewRemoved, this, &TabManagerWidget::onViewRemoved);
        connect(container, &TabbedViewContainer::activeViewChanged, this, &TabManagerWidget::onActiveViewChanged);
        connect(container, &QTabWidget::tabBarDoubleClicked, this, [this](int) {
            refresh();
        });
    }

    connect(m_treeView, &QTreeView::clicked, this, &TabManagerWidget::onItemClicked);

    refresh();
}

void TabManagerWidget::refresh()
{
    m_model->clear();

    auto *container = m_viewManager->activeContainer();
    if (!container) {
        return;
    }

    for (int i = 0; i < container->count(); i++) {
        auto *splitter = container->viewSplitterAt(i);
        if (!splitter) {
            continue;
        }

        auto *tabItem = new QStandardItem(container->tabIcon(i), container->tabText(i));
        tabItem->setData(i, TabIndexRole);
        tabItem->setData(true, IsTabRole);

        auto terminals = splitter->findChildren<TerminalDisplay *>();

        // Only show child items if the tab has multiple terminals (splits)
        if (terminals.count() > 1) {
            for (auto *terminal : terminals) {
                auto *controller = terminal->sessionController();
                if (!controller) {
                    continue;
                }

                auto *childItem = new QStandardItem(controller->icon(), controller->title());
                childItem->setData(terminal->id(), TerminalIdRole);
                childItem->setData(false, IsTabRole);
                tabItem->appendRow(childItem);

                connect(controller, &ViewProperties::titleChanged, this, &TabManagerWidget::onTitleChanged, Qt::UniqueConnection);
                connect(controller, &ViewProperties::iconChanged, this, &TabManagerWidget::onIconChanged, Qt::UniqueConnection);
            }
        } else if (terminals.count() == 1) {
            // Single terminal — connect signals for title/icon updates on the tab itself
            auto *controller = terminals.first()->sessionController();
            if (controller) {
                connect(controller, &ViewProperties::titleChanged, this, &TabManagerWidget::onTitleChanged, Qt::UniqueConnection);
                connect(controller, &ViewProperties::iconChanged, this, &TabManagerWidget::onIconChanged, Qt::UniqueConnection);
            }
        }

        m_model->appendRow(tabItem);
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
    auto *container = m_viewManager->activeContainer();
    if (!container) {
        return;
    }

    // Find which tab/terminal this property belongs to and update it
    auto *controller = qobject_cast<SessionController *>(properties);
    if (!controller || !controller->view()) {
        return;
    }

    int terminalId = controller->view()->id();

    // Check if it's a child terminal item
    auto *childItem = findItemForTerminal(terminalId);
    if (childItem) {
        childItem->setText(properties->title());
        return;
    }

    // It might be a single-terminal tab — find by tab index
    for (int i = 0; i < container->count(); i++) {
        auto *splitter = container->viewSplitterAt(i);
        if (!splitter) {
            continue;
        }
        auto terminals = splitter->findChildren<TerminalDisplay *>();
        if (terminals.count() == 1 && terminals.first()->id() == terminalId) {
            // Update tab text in container (the tab widget itself)
            // The tree item text comes from container->tabText(), so refresh
            if (i < m_model->rowCount()) {
                m_model->item(i)->setText(container->tabText(i));
            }
            return;
        }
    }
}

void TabManagerWidget::onIconChanged(ViewProperties *properties)
{
    auto *container = m_viewManager->activeContainer();
    if (!container) {
        return;
    }

    auto *controller = qobject_cast<SessionController *>(properties);
    if (!controller || !controller->view()) {
        return;
    }

    int terminalId = controller->view()->id();

    auto *childItem = findItemForTerminal(terminalId);
    if (childItem) {
        childItem->setIcon(properties->icon());
        return;
    }

    // Single-terminal tab — update from container icon
    for (int i = 0; i < container->count(); i++) {
        auto *splitter = container->viewSplitterAt(i);
        if (!splitter) {
            continue;
        }
        auto terminals = splitter->findChildren<TerminalDisplay *>();
        if (terminals.count() == 1 && terminals.first()->id() == terminalId) {
            if (i < m_model->rowCount()) {
                m_model->item(i)->setIcon(container->tabIcon(i));
            }
            return;
        }
    }
}

void TabManagerWidget::onItemClicked(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }

    m_updatingSelection = true;

    bool isTab = index.data(IsTabRole).toBool();

    auto *container = m_viewManager->activeContainer();
    if (!container) {
        m_updatingSelection = false;
        return;
    }

    if (isTab) {
        int tabIndex = index.data(TabIndexRole).toInt();
        container->setCurrentIndex(tabIndex);
    } else {
        int terminalId = index.data(TerminalIdRole).toInt();
        // Find the terminal by walking all terminals in the container
        const auto terminals = container->findChildren<TerminalDisplay *>();
        for (auto *terminal : terminals) {
            if (terminal->id() == terminalId) {
                // Switch to the tab containing this terminal
                auto *splitter = qobject_cast<ViewSplitter *>(terminal->parentWidget());
                if (splitter) {
                    container->setCurrentWidget(splitter->getToplevelSplitter());
                }
                terminal->setFocus();
                break;
            }
        }
    }

    m_updatingSelection = false;
}

void TabManagerWidget::highlightActiveTab()
{
    if (m_updatingSelection) {
        return;
    }

    auto *container = m_viewManager->activeContainer();
    if (!container) {
        return;
    }

    int currentIdx = container->currentIndex();
    if (currentIdx < 0 || currentIdx >= m_model->rowCount()) {
        return;
    }

    m_updatingSelection = true;
    QModelIndex modelIndex = m_model->index(currentIdx, 0);
    m_treeView->setCurrentIndex(modelIndex);
    m_updatingSelection = false;
}

QStandardItem *TabManagerWidget::findItemForTerminal(int terminalId) const
{
    for (int i = 0; i < m_model->rowCount(); i++) {
        auto *tabItem = m_model->item(i);
        for (int j = 0; j < tabItem->rowCount(); j++) {
            auto *childItem = tabItem->child(j);
            if (childItem && childItem->data(TerminalIdRole).toInt() == terminalId) {
                return childItem;
            }
        }
    }
    return nullptr;
}
