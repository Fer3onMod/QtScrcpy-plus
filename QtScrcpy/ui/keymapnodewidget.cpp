#include "keymapnodewidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPen>

static const int CLOSE_BTN_SIZE = 16;
static const int CLOSE_BTN_MARGIN = 2;

// ---------------------------------------------------------------------------
// KeymapNodeWidget (base)
// ---------------------------------------------------------------------------

KeymapNodeWidget::KeymapNodeWidget(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(60, 60);
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    setMouseTracking(true);
}

KeymapNodeWidget::~KeymapNodeWidget()
{
}

void KeymapNodeWidget::setKeyName(const QString &name)
{
    m_keyName = name;
    update();
}

QRect KeymapNodeWidget::closeButtonRect() const
{
    return QRect(width() - CLOSE_BTN_SIZE - CLOSE_BTN_MARGIN,
                 CLOSE_BTN_MARGIN,
                 CLOSE_BTN_SIZE,
                 CLOSE_BTN_SIZE);
}

void KeymapNodeWidget::paintCloseButton(QPainter &p) const
{
    QRect r = closeButtonRect();
    // Background circle
    p.setPen(Qt::NoPen);
    p.setBrush(m_closeHovered ? QColor(220, 50, 50) : QColor(180, 30, 30, 200));
    p.drawEllipse(r);
    // X mark
    p.setPen(QPen(Qt::white, 2, Qt::SolidLine, Qt::RoundCap));
    int margin = 4;
    p.drawLine(r.left() + margin, r.top() + margin,
               r.right() - margin, r.bottom() - margin);
    p.drawLine(r.right() - margin, r.top() + margin,
               r.left() + margin, r.bottom() - margin);
}

void KeymapNodeWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (closeButtonRect().contains(event->pos())) {
            emit removeRequested();
            event->accept();
            return;
        }
        m_dragStartPosition = event->pos();
        m_isDragging = true;
        raise();
    }
    event->accept();
}

void KeymapNodeWidget::mouseMoveEvent(QMouseEvent *event)
{
    bool wasHovered = m_closeHovered;
    m_closeHovered = closeButtonRect().contains(event->pos());
    if (m_closeHovered != wasHovered) update();

    if (m_isDragging && (event->buttons() & Qt::LeftButton)) {
        QPoint newPos = pos() + (event->pos() - m_dragStartPosition);
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
    f.setPointSize(8);
    f.setBold(true);
    p.setFont(f);
    // Draw label below center to leave room for X button
    QRect textRect = rect().adjusted(2, 8, -2, -2);
    p.drawText(textRect, Qt::AlignCenter,
               m_keyName.isEmpty() ? QStringLiteral("?") : m_keyName);

    paintCloseButton(p);
}

// ---------------------------------------------------------------------------
// ClickNodeWidget
// ---------------------------------------------------------------------------

ClickNodeWidget::ClickNodeWidget(QWidget *parent)
    : KeymapNodeWidget(parent)
{
    m_keyName = QStringLiteral("Key_F");
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
    if (parentWidget() && json.contains(QStringLiteral("pos"))) {
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
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Determine color by key type
    QColor ringColor(0, 200, 255);   // default cyan
    QColor fillColor(0, 80, 180, 160);
    QString display = m_keyName;

    if (m_keyName == QStringLiteral("LeftButton")) {
        ringColor = QColor(255, 160, 0);
        fillColor = QColor(160, 80, 0, 160);
        display   = QStringLiteral("FIRE");
    } else if (m_keyName == QStringLiteral("RightButton")) {
        ringColor = QColor(180, 0, 255);
        fillColor = QColor(80, 0, 160, 160);
        display   = QStringLiteral("SCOPE");
    }

    p.setPen(QPen(ringColor, 2));
    p.setBrush(fillColor);
    p.drawEllipse(rect().adjusted(2, 2, -2, -2));

    // Inner dot
    p.setPen(Qt::NoPen);
    p.setBrush(ringColor);
    p.drawEllipse(rect().center(), 5, 5);

    // Label
    p.setPen(Qt::white);
    QFont f = p.font();
    f.setPointSize(8);
    f.setBold(true);
    p.setFont(f);
    p.drawText(rect().adjusted(2, 8, -2, -2), Qt::AlignCenter, display);

    paintCloseButton(p);
}

// ---------------------------------------------------------------------------
// SteerWheelNodeWidget
// ---------------------------------------------------------------------------

SteerWheelNodeWidget::SteerWheelNodeWidget(QWidget *parent)
    : KeymapNodeWidget(parent)
{
    setFixedSize(110, 110);
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

    obj[QStringLiteral("leftKey")]   = QStringLiteral("Key_A");
    obj[QStringLiteral("rightKey")]  = QStringLiteral("Key_D");
    obj[QStringLiteral("upKey")]     = QStringLiteral("Key_W");
    obj[QStringLiteral("downKey")]   = QStringLiteral("Key_S");

    obj[QStringLiteral("leftOffset")]  = 0.05;
    obj[QStringLiteral("rightOffset")] = 0.05;
    obj[QStringLiteral("upOffset")]    = 0.05;
    obj[QStringLiteral("downOffset")]  = 0.05;

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

    // Outer ring
    p.setPen(QPen(QColor(0, 230, 100), 2, Qt::DashLine));
    p.setBrush(QColor(0, 100, 40, 110));
    p.drawEllipse(rect().adjusted(2, 2, -2, -2));

    // Crosshair lines
    p.setPen(QPen(QColor(0, 230, 100), 1));
    p.drawLine(width() / 2, 8, width() / 2, height() - 8);
    p.drawLine(8, height() / 2, width() - 8, height() / 2);

    // Key labels
    p.setPen(Qt::white);
    QFont f = p.font();
    f.setPointSize(11);
    f.setBold(true);
    p.setFont(f);
    p.drawText(rect().adjusted(0, 6,  0,  0),  Qt::AlignTop    | Qt::AlignHCenter, QStringLiteral("W"));
    p.drawText(rect().adjusted(0, 0,  0, -6),  Qt::AlignBottom | Qt::AlignHCenter, QStringLiteral("S"));
    p.drawText(rect().adjusted(6, 0,  0,  0),  Qt::AlignLeft   | Qt::AlignVCenter, QStringLiteral("A"));
    p.drawText(rect().adjusted(0, 0, -6,  0),  Qt::AlignRight  | Qt::AlignVCenter, QStringLiteral("D"));

    paintCloseButton(p);
}
