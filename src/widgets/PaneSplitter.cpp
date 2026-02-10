/*
    SPDX-FileCopyrightText: 2026 Konsole Plus contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "PaneSplitter.h"
#include "ViewContainer.h"

using namespace Konsole;

PaneSplitter::PaneSplitter(QWidget *parent)
    : QSplitter(parent)
{
    setChildrenCollapsible(false);
}

void PaneSplitter::addContainer(TabbedViewContainer *container, TabbedViewContainer *relativeTo, Qt::Orientation splitOrientation)
{
    if (count() == 0) {
        // First pane — just add it
        addWidget(container);
        return;
    }

    if (count() == 1 || orientation() == splitOrientation) {
        // Same orientation or only one child — add directly
        setOrientation(splitOrientation);
        if (relativeTo) {
            int idx = indexOf(relativeTo);
            insertWidget(idx + 1, container);
        } else {
            addWidget(container);
        }
        // Equalize sizes
        QList<int> sizes;
        int totalSize = (splitOrientation == Qt::Horizontal) ? width() : height();
        int perChild = totalSize / count();
        for (int i = 0; i < count(); i++) {
            sizes << perChild;
        }
        setSizes(sizes);
        return;
    }

    // Different orientation — wrap relativeTo + new container in a nested PaneSplitter
    if (relativeTo) {
        int idx = indexOf(relativeTo);
        QList<int> oldSizes = sizes();

        auto *nested = new PaneSplitter();
        nested->setOrientation(splitOrientation);

        // Replace relativeTo with the nested splitter
        nested->addWidget(relativeTo);
        nested->addWidget(container);
        insertWidget(idx, nested);

        // Restore sizes (the nested splitter takes relativeTo's slot)
        setSizes(oldSizes);

        // Equalize within nested
        int nestedTotal = (splitOrientation == Qt::Horizontal) ? nested->width() : nested->height();
        nested->setSizes({nestedTotal / 2, nestedTotal / 2});
    } else {
        addWidget(container);
    }
}

void PaneSplitter::removeContainer(TabbedViewContainer *container)
{
    // Find which splitter directly holds this container
    auto *parentSplitter = qobject_cast<PaneSplitter *>(container->parentWidget());
    if (!parentSplitter) {
        return;
    }

    container->setParent(nullptr);
    container->deleteLater();

    // If the parent splitter is nested and now has only one child, unwrap it
    if (parentSplitter != this && parentSplitter->count() == 1) {
        QWidget *remaining = parentSplitter->widget(0);
        auto *grandparent = qobject_cast<PaneSplitter *>(parentSplitter->parentWidget());
        if (grandparent) {
            int idx = grandparent->indexOf(parentSplitter);
            QList<int> oldSizes = grandparent->sizes();
            remaining->setParent(nullptr);
            grandparent->insertWidget(idx, remaining);
            parentSplitter->deleteLater();
            grandparent->setSizes(oldSizes);
        }
    }
}

QList<TabbedViewContainer *> PaneSplitter::containers() const
{
    QList<TabbedViewContainer *> result;
    for (int i = 0; i < count(); i++) {
        auto *container = qobject_cast<TabbedViewContainer *>(widget(i));
        if (container) {
            result.append(container);
        } else {
            auto *nested = qobject_cast<PaneSplitter *>(widget(i));
            if (nested) {
                result.append(nested->containers());
            }
        }
    }
    return result;
}

TabbedViewContainer *PaneSplitter::containerInDirection(TabbedViewContainer *current, Qt::Orientation orient, int direction) const
{
    // Use geometry to find the adjacent container
    if (!current) {
        return nullptr;
    }

    QPoint center = current->mapTo(this, QPoint(current->width() / 2, current->height() / 2));

    int probeX = center.x();
    int probeY = center.y();
    if (orient == Qt::Horizontal) {
        probeX = (direction > 0) ? current->mapTo(this, QPoint(current->width() + 5, 0)).x()
                                 : current->mapTo(this, QPoint(-5, 0)).x();
    } else {
        probeY = (direction > 0) ? current->mapTo(this, QPoint(0, current->height() + 5)).y()
                                 : current->mapTo(this, QPoint(0, -5)).y();
    }

    QWidget *hit = childAt(probeX, probeY);
    if (!hit) {
        return nullptr;
    }

    // Walk up from the hit widget to find a TabbedViewContainer
    QWidget *w = hit;
    while (w && w != this) {
        auto *container = qobject_cast<TabbedViewContainer *>(w);
        if (container && container != current) {
            return container;
        }
        w = w->parentWidget();
    }

    return nullptr;
}
