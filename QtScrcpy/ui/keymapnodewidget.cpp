#include "keymapnodewidget.h"

#include <QInputDialog>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>

// ---------------------------------------------------------------------------
// KeymapNodeWidget (base)
// ---------------------------------------------------------------------------

KeymapNodeWidget::KeymapNodeWidget(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(54, 54);
    // Nodes must raise their own mouse events; don't suppress them.
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
}

KeymapNodeWidget::~KeymapNodeWidget()
{
}

void KeymapNodeWidget::setKeyName(const QString &name)
{
    m_keyName = name;
    update();
}

void KeymapNodeWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragStartPosition = event->pos();
        m_isDragging = true;
        raise(); // bring to front while dragging
    } else if (event->button() == Qt::RightButton) {
        emit removeRequested();
    }
    // Don't propagate — stop the overlay from catching this
    event->accept();
}

void KeymapNodeWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isDragging && (event->buttons() & Qt::LeftButton)) {
        QPoint newPos = pos() + (event->pos() - m_dragStartPosition);
        // Clamp inside parent
        if (parentWidget()) {
            int maxX = parentWidget()->width()  - width();
            int maxY = parentWidget()->height() - height();
            newPos.setX(qBound(0, newPos.x(), maxX));
            newPos.setY(qBound(0, newPos.y(), maxY));
        }
        move(newPos);
    }
    event->accept();
}

void KeymapNodeWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = false;
    }
    event->accept();
}

void KeymapNodeWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor(0, 200, 255), 2));
    p.setBrush(QColor(0, 120, 200, 160));
    p.drawEllipse(rect().adjusted(2, 2, -2, -2));

    p.setPen(Qt::white);
    QFont f = p.font();
    f.setPointSize(9);
    f.setBold(true);
    p.setFont(f);
    p.drawText(rect(), Qt::AlignCenter,
               m_keyName.isEmpty() ? QStringLiteral("?") : m_keyName);
}

// ---------------------------------------------------------------------------
// ClickNodeWidget
// ---------------------------------------------------------------------------

ClickNodeWidget::ClickNodeWidget(QWidget *parent)
    : KeymapNodeWidget(parent)
{
    m_keyName = QStringLiteral("TAP");
}

QJsonObject ClickNodeWidget::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("type")] = QStringLiteral("click");
    // Store position as fraction of parent size so it's resolution-independent
    if (parentWidget()) {
        obj[QStringLiteral("xRatio")] = static_cast<double>(pos().x() + width()  / 2) / parentWidget()->width();
        obj[QStringLiteral("yRatio")] = static_cast<double>(pos().y() + height() / 2) / parentWidget()->height();
    } else {
        obj[QStringLiteral("xRatio")] = 0.5;
        obj[QStringLiteral("yRatio")] = 0.5;
    }
    obj[QStringLiteral("key")] = m_keyName;
    return obj;
}

void ClickNodeWidget::fromJson(const QJsonObject &json)
{
    if (json.contains(QStringLiteral("key"))) {
        m_keyName = json[QStringLiteral("key")].toString();
    }
    // Position will be resolved by the overlay after the widget has a parent and size
    if (parentWidget() &&
        json.contains(QStringLiteral("xRatio")) &&
        json.contains(QStringLiteral("yRatio")))
    {
        double xR = json[QStringLiteral("xRatio")].toDouble();
        double yR = json[QStringLiteral("yRatio")].toDouble();
        int cx = static_cast<int>(xR * parentWidget()->width());
        int cy = static_cast<int>(yR * parentWidget()->height());
        move(cx - width() / 2, cy - height() / 2);
    }
}

void ClickNodeWidget::paintEvent(QPaintEvent *event)
{
    // Draw the base circle from the parent
    KeymapNodeWidget::paintEvent(event);

    // Add a small red dot in the center to distinguish click nodes
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(QColor(255, 80, 80));
    p.setPen(Qt::NoPen);
    p.drawEllipse(rect().center(), 5, 5);
}
