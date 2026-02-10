/*
    SPDX-FileCopyrightText: 2018 Tomaz Canabrava <tcanabrava@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "DetachableTabBar.h"
#include "KonsoleSettings.h"
#include "widgets/ViewContainer.h"

#include <QApplication>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>

#include <KAcceleratorManager>

#include <QColor>
#include <QPainter>

namespace Konsole
{

static const QString tabMimeType = QStringLiteral("konsole/tab");
static const QString terminalMimeType = QStringLiteral("konsole/terminal_display");

DetachableTabBar::DetachableTabBar(QWidget *parent)
    : QTabBar(parent)
    , dragType(DragType::NONE)
    , _originalCursor(cursor())
    , tabId(-1)
    , _activityColor(QColor::Invalid)
{
    setAcceptDrops(true);
    setElideMode(Qt::TextElideMode::ElideLeft);
    KAcceleratorManager::setNoAccel(this);
}

void DetachableTabBar::setColor(int idx, const QColor &color)
{
    DetachableTabData data = tabData(idx).value<DetachableTabData>();
    if (data.color != color) {
        data.color = color;
        setDetachableTabData(idx, data);
        update(tabRect(idx));
    }
}

void DetachableTabBar::setActivityColor(int idx, const QColor &color)
{
    _activityColor = color;
    update();
}

void DetachableTabBar::removeColor(int idx)
{
    DetachableTabData data = tabData(idx).value<DetachableTabData>();
    if (data.color.isValid()) {
        data.color = QColor();
        setDetachableTabData(idx, data);
        update(tabRect(idx));
    }
}

void DetachableTabBar::setProgress(int idx, const std::optional<int> &progress)
{
    DetachableTabData data = tabData(idx).value<DetachableTabData>();
    if (data.progress != progress) {
        data.progress = progress;
        setDetachableTabData(idx, data);
        update(tabRect(idx));
    }
}

void DetachableTabBar::setDetachableTabData(int idx, const DetachableTabData &data)
{
    if ((data.color.isValid() && data.color.alpha() > 0) || data.progress.has_value()) {
        setTabData(idx, QVariant::fromValue(data));
    } else {
        setTabData(idx, QVariant());
    }
}

void DetachableTabBar::middleMouseButtonClickAt(const QPoint &pos)
{
    tabId = tabAt(pos);

    if (tabId != -1) {
        Q_EMIT closeTab(tabId);
    }
}

void DetachableTabBar::mousePressEvent(QMouseEvent *event)
{
    QTabBar::mousePressEvent(event);
    if (event->button() == Qt::LeftButton) {
        m_dragStartPos = event->pos();
        m_draggingTabIndex = tabAt(event->pos());
        m_dragInitiated = false;
    }
    _containers = window()->findChildren<Konsole::TabbedViewContainer *>();
}

void DetachableTabBar::mouseMoveEvent(QMouseEvent *event)
{
    if (m_draggingTabIndex >= 0 && !m_dragInitiated
        && (event->pos() - m_dragStartPos).manhattanLength() > QApplication::startDragDistance()) {
        m_dragInitiated = true;

        auto *drag = new QDrag(this);
        auto *mimeData = new QMimeData();

        // Encode: PID:tabIndex:containerPtr
        auto *container = qobject_cast<TabbedViewContainer *>(parentWidget());
        QByteArray payload;
        payload.append(QByteArray::number(qApp->applicationPid()));
        payload.append(':');
        payload.append(QByteArray::number(m_draggingTabIndex));
        payload.append(':');
        payload.append(QByteArray::number(reinterpret_cast<quintptr>(container)));

        mimeData->setData(tabMimeType, payload);
        drag->setMimeData(mimeData);

        // Use tab text as drag pixmap label
        QPixmap pixmap(tabRect(m_draggingTabIndex).size());
        pixmap.fill(Qt::transparent);
        QPainter p(&pixmap);
        p.setPen(palette().text().color());
        p.drawText(pixmap.rect(), Qt::AlignCenter, tabText(m_draggingTabIndex));
        p.end();
        drag->setPixmap(pixmap);

        Qt::DropAction result = drag->exec(Qt::MoveAction);

        // If drag was not accepted by any drop target, handle detach/move-to-window
        if (result == Qt::IgnoreAction) {
            auto globalPos = QCursor::pos();
            auto widgetAtPos = qApp->topLevelAt(globalPos);
            if (widgetAtPos == nullptr) {
                // Dropped outside any window — detach
                if (count() > 1) {
                    Q_EMIT detachTab(m_draggingTabIndex);
                }
            } else if (window() != widgetAtPos->window()) {
                // Dropped on another Konsole window
                if (_containers.size() == 1 || count() > 1) {
                    Q_EMIT moveTabToWindow(m_draggingTabIndex, widgetAtPos);
                }
            }
        }

        m_draggingTabIndex = -1;
        m_dragInitiated = false;
        setCursor(_originalCursor);
        return;
    }

    if (!m_dragInitiated) {
        QTabBar::mouseMoveEvent(event);
    }
}

void DetachableTabBar::mouseReleaseEvent(QMouseEvent *event)
{
    QTabBar::mouseReleaseEvent(event);

    switch (event->button()) {
    case Qt::MiddleButton:
        if (KonsoleSettings::closeTabOnMiddleMouseButton()) {
            middleMouseButtonClickAt(event->pos());
        }

        tabId = tabAt(event->pos());
        if (tabId == -1) {
            Q_EMIT newTabRequest();
        }
        break;
    default:
        break;
    }

    m_draggingTabIndex = -1;
    m_dragInitiated = false;
    setCursor(_originalCursor);
}

void DetachableTabBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QTabBar::mouseDoubleClickEvent(event);
    }
}

void DetachableTabBar::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat(tabMimeType) || event->mimeData()->hasFormat(terminalMimeType)) {
        // Validate same PID
        QByteArray data;
        if (event->mimeData()->hasFormat(tabMimeType)) {
            data = event->mimeData()->data(tabMimeType);
        } else {
            data = event->mimeData()->data(terminalMimeType);
        }
        auto pid = data.split(':').first().toInt();
        if (pid == qApp->applicationPid()) {
            event->acceptProposedAction();
        }
    }
}

void DetachableTabBar::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasFormat(tabMimeType)) {
        int tabIdx = tabAt(event->position().toPoint());
        if (tabIdx != -1) {
            setCurrentIndex(tabIdx);
        }
        event->acceptProposedAction();
    } else if (event->mimeData()->hasFormat(terminalMimeType)) {
        int tabIdx = tabAt(event->position().toPoint());
        if (tabIdx != -1) {
            setCurrentIndex(tabIdx);
        }
    }
}

void DetachableTabBar::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasFormat(tabMimeType)) {
        QByteArray payload = event->mimeData()->data(tabMimeType);
        auto parts = payload.split(':');
        if (parts.size() != 3) {
            return;
        }

        int sourceTabIndex = parts[1].toInt();
        auto sourceContainerPtr = reinterpret_cast<TabbedViewContainer *>(parts[2].toULongLong());

        auto *targetContainer = qobject_cast<TabbedViewContainer *>(parentWidget());

        if (sourceContainerPtr != targetContainer) {
            // Tab dropped from a different pane — move it here
            Q_EMIT tabDroppedToOtherBar(sourceTabIndex, sourceContainerPtr);
            event->acceptProposedAction();
        }
        // Same container drops are handled by QTabBar's built-in reorder
    }
}

void DetachableTabBar::paintEvent(QPaintEvent *event)
{
    QTabBar::paintEvent(event);
    if (!event->isAccepted()) {
        return; // Reduces repainting
    }

    QPainter painter(this);
    painter.setPen(Qt::NoPen);

    for (int tabIndex = 0; tabIndex < count(); tabIndex++) {
        const QVariant data = tabData(tabIndex);
        if (!data.isValid() || data.isNull()) {
            continue;
        }

        const DetachableTabData tabData = data.value<DetachableTabData>();

        const bool colorValid = tabData.color.isValid() && tabData.color.alpha() > 0;

        if (!colorValid && !tabData.progress.has_value()) {
            continue;
        }

        const QColor color = colorValid ? tabData.color : palette().highlight().color();

        painter.setBrush(color);
        QRect tRect = tabRect(tabIndex);
        tRect.setTop(painter.fontMetrics().height() + 6); // Color bar top position consider a height the font and fixed spacing of 6px
        tRect.setHeight(4);
        tRect.setLeft(tRect.left() + 6);
        tRect.setWidth(tRect.width() - 6);

        // Draw progress, if any, ontop of a faint bar.
        if (tabData.progress.has_value()) {
            painter.setOpacity(0.3);
            painter.drawRect(tRect);
            painter.setOpacity(1.0);

            tRect.setWidth(tRect.width() * tabData.progress.value() / 100.0);
            painter.drawRect(tRect);
        } else {
            painter.drawRect(tRect);
        }
    }
}

}

#include "moc_DetachableTabBar.cpp"
