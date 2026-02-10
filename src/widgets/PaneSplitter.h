/*
    SPDX-FileCopyrightText: 2026 Konsole Plus contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef PANESPLITTER_H
#define PANESPLITTER_H

#include <QSplitter>

#include "konsoleprivate_export.h"

namespace Konsole
{
class TabbedViewContainer;

/**
 * A top-level splitter that holds one or more TabbedViewContainer panes.
 * Each pane has its own independent tab bar (Xshell-like split model).
 *
 * When there is only one pane, this widget is transparent — the single
 * TabbedViewContainer fills the entire area.
 */
class KONSOLEPRIVATE_EXPORT PaneSplitter : public QSplitter
{
    Q_OBJECT

public:
    explicit PaneSplitter(QWidget *parent = nullptr);

    /**
     * Add a container pane next to @p relativeTo.
     * If @p relativeTo is null, appends at the end.
     * If the orientation differs from the current splitter orientation and
     * there are already 2+ children, wraps in a nested PaneSplitter.
     */
    void addContainer(TabbedViewContainer *container, TabbedViewContainer *relativeTo, Qt::Orientation splitOrientation);

    /**
     * Remove a container pane. Cleans up empty nested PaneSplitters.
     */
    void removeContainer(TabbedViewContainer *container);

    /**
     * Returns all TabbedViewContainer instances (recursively through nested PaneSplitters).
     */
    QList<TabbedViewContainer *> containers() const;

    /**
     * Find the TabbedViewContainer adjacent to @p current in the given direction.
     * Returns nullptr if there is no adjacent container in that direction.
     */
    TabbedViewContainer *containerInDirection(TabbedViewContainer *current, Qt::Orientation orientation, int direction) const;
};

}

#endif // PANESPLITTER_H
