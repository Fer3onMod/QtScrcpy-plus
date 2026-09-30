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
    obj[QStringLiteral("type")] = QStringLiteral("KMT_CLICK");
    
    QJsonObject posObj;
    if (parentWidget()) {
        posObj[QStringLiteral("x")] = static_cast<double>(pos().x() + width()  / 2) / parentWidget()->width();
        posObj[QStringLiteral("y")] = static_cast<double>(pos().y() + height() / 2) / parentWidget()->height();
    } else {
        posObj[QStringLiteral("x")] = 0.5;
        posObj[QStringLiteral("y")] = 0.5;
    }
    obj[QStringLiteral("pos")] = posObj;
    obj[QStringLiteral("key")] = m_keyName;
    obj[QStringLiteral("switchMap")] = false;
    return obj;
}

void ClickNodeWidget::fromJson(const QJsonObject &json)
{
    if (json.contains(QStringLiteral("key"))) {
        m_keyName = json[QStringLiteral("key")].toString();
    }
    // Position will be resolved by the overlay after the widget has a parent and size
    if (parentWidget() && json.contains(QStringLiteral("pos")))
    {
        QJsonObject posObj = json[QStringLiteral("pos")].toObject();
        double xR = posObj[QStringLiteral("x")].toDouble();
        double yR = posObj[QStringLiteral("y")].toDouble();
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

// ---------------------------------------------------------------------------
// SteerWheelNodeWidget
// ---------------------------------------------------------------------------

SteerWheelNodeWidget::SteerWheelNodeWidget(QWidget *parent)
    : KeymapNodeWidget(parent)
{
    setFixedSize(100, 100);
    m_keyName = QStringLiteral("WASD");
}

QJsonObject SteerWheelNodeWidget::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("type")] = QStringLiteral("KMT_STEER_WHEEL");
    
    QJsonObject posObj;
    if (parentWidget()) {
        posObj[QStringLiteral("x")] = static_cast<double>(pos().x() + width()  / 2) / parentWidget()->width();
        posObj[QStringLiteral("y")] = static_cast<double>(pos().y() + height() / 2) / parentWidget()->height();
    } else {
        posObj[QStringLiteral("x")] = 0.5;
        posObj[QStringLiteral("y")] = 0.5;
    }
    obj[QStringLiteral("centerPos")] = posObj;
    
    obj[QStringLiteral("leftKey")] = QStringLiteral("Key_A");
    obj[QStringLiteral("rightKey")] = QStringLiteral("Key_D");
    obj[QStringLiteral("upKey")] = QStringLiteral("Key_W");
    obj[QStringLiteral("downKey")] = QStringLiteral("Key_S");
    
    obj[QStringLiteral("leftOffset")] = 0.05;
    obj[QStringLiteral("rightOffset")] = 0.05;
    obj[QStringLiteral("upOffset")] = 0.05;
    obj[QStringLiteral("downOffset")] = 0.05;

    return obj;
}

void SteerWheelNodeWidget::fromJson(const QJsonObject &json)
{
    if (parentWidget() && json.contains(QStringLiteral("centerPos"))) {
        QJsonObject posObj = json[QStringLiteral("centerPos")].toObject();
        double xR = posObj[QStringLiteral("x")].toDouble();
        double yR = posObj[QStringLiteral("y")].toDouble();
        int cx = static_cast<int>(xR * parentWidget()->width());
        int cy = static_cast<int>(yR * parentWidget()->height());
        move(cx - width() / 2, cy - height() / 2);
    }
}

void SteerWheelNodeWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    
    // Outer circle
    p.setPen(QPen(QColor(0, 255, 100), 2, Qt::DashLine));
    p.setBrush(QColor(0, 120, 50, 100));
    p.drawEllipse(rect().adjusted(2, 2, -2, -2));
    
    // Crosshair
    p.setPen(QPen(QColor(0, 255, 100), 1));
    p.drawLine(width() / 2, 0, width() / 2, height());
    p.drawLine(0, height() / 2, width(), height() / 2);
    
    // Labels
    p.setPen(Qt::white);
    QFont f = p.font();
    f.setPointSize(10);
    f.setBold(true);
    p.setFont(f);
    
    p.drawText(rect().adjusted(0, 5, 0, 0), Qt::AlignTop | Qt::AlignHCenter, "W");
    p.drawText(rect().adjusted(0, 0, 0, -5), Qt::AlignBottom | Qt::AlignHCenter, "S");
    p.drawText(rect().adjusted(5, 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter, "A");
    p.drawText(rect().adjusted(0, 0, -5, 0), Qt::AlignRight | Qt::AlignVCenter, "D");
}

