#include "keymapnodewidget.h"
#include "keycapturedlg.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

static const int CLOSE_BTN_SIZE   = 18;
static const int CLOSE_BTN_MARGIN = 3;

// ===========================================================================
// KeymapNodeWidget (base)
// ===========================================================================

KeymapNodeWidget::KeymapNodeWidget(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(64, 64);
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
    p.setPen(Qt::NoPen);
    p.setBrush(m_closeHovered ? QColor(220, 50, 50) : QColor(180, 30, 30, 200));
    p.drawEllipse(r);
    p.setPen(QPen(Qt::white, 2, Qt::SolidLine, Qt::RoundCap));
    int margin = 4;
    p.drawLine(r.left() + margin, r.top() + margin, r.right() - margin, r.bottom() - margin);
    p.drawLine(r.right() - margin, r.top() + margin, r.left() + margin, r.bottom() - margin);
}

void KeymapNodeWidget::captureKey()
{
    KeyCaptureDlg dlg(this);
    if (dlg.exec() == QDialog::Accepted && !dlg.capturedKey().isEmpty()) {
        setKeyName(dlg.capturedKey());
    }
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

void KeymapNodeWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && !closeButtonRect().contains(event->pos())) {
        captureKey();
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
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
    QRect textRect = rect().adjusted(2, 8, -2, -2);
    p.drawText(textRect, Qt::AlignCenter,
               m_keyName.isEmpty() ? QStringLiteral("?") : m_keyName);

    paintCloseButton(p);
}

// ===========================================================================
// ClickNodeWidget
// ===========================================================================

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
    obj[QStringLiteral("pos")]       = posObj;
    obj[QStringLiteral("key")]       = m_keyName;
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

    QColor ringColor(0, 200, 255);
    QColor fillColor(0, 80, 180, 160);
    QString display = m_keyName;

    if (m_keyName == QStringLiteral("LeftButton")) {
        ringColor = QColor(255, 160, 0);
        fillColor = QColor(160, 80, 0, 160);
        display   = QStringLiteral("FIRE\n(LClick)");
    } else if (m_keyName == QStringLiteral("RightButton")) {
        ringColor = QColor(180, 0, 255);
        fillColor = QColor(80, 0, 160, 160);
        display   = QStringLiteral("SCOPE\n(RClick)");
    } else {
        // Show clean key name (strip "Key_" prefix)
        display = m_keyName;
        if (display.startsWith(QStringLiteral("Key_")))
            display = display.mid(4);
    }

    p.setPen(QPen(ringColor, 2));
    p.setBrush(fillColor);
    p.drawEllipse(rect().adjusted(2, 2, -2, -2));

    p.setPen(Qt::NoPen);
    p.setBrush(ringColor);
    p.drawEllipse(rect().center(), 4, 4);

    p.setPen(Qt::white);
    QFont f = p.font();
    f.setPointSize(7);
    f.setBold(true);
    p.setFont(f);
    p.drawText(rect().adjusted(2, 6, -2, -4), Qt::AlignCenter, display);

    paintCloseButton(p);
}

// ===========================================================================
// SteerWheelNodeWidget
// ===========================================================================

SteerWheelNodeWidget::SteerWheelNodeWidget(QWidget *parent)
    : KeymapNodeWidget(parent)
{
    setFixedSize(120, 120);
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
    obj[QStringLiteral("centerPos")]   = posObj;
    obj[QStringLiteral("leftKey")]     = m_leftKey;
    obj[QStringLiteral("rightKey")]    = m_rightKey;
    obj[QStringLiteral("upKey")]       = m_upKey;
    obj[QStringLiteral("downKey")]     = m_downKey;
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
    if (json.contains(QStringLiteral("upKey")))    m_upKey    = json[QStringLiteral("upKey")].toString();
    if (json.contains(QStringLiteral("downKey")))  m_downKey  = json[QStringLiteral("downKey")].toString();
    if (json.contains(QStringLiteral("leftKey")))  m_leftKey  = json[QStringLiteral("leftKey")].toString();
    if (json.contains(QStringLiteral("rightKey"))) m_rightKey = json[QStringLiteral("rightKey")].toString();
}

void SteerWheelNodeWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    // Don't open generic capture — open 4-key direction dialog
    if (event->button() != Qt::LeftButton || closeButtonRect().contains(event->pos())) {
        QWidget::mouseDoubleClickEvent(event);
        return;
    }

    // A small dialog with 4 capture buttons
    QDialog dlg(this);
    dlg.setWindowTitle(tr("WASD Keys — Double-click a direction to change"));
    dlg.setFixedSize(300, 240);

    auto *vlay = new QVBoxLayout(&dlg);
    vlay->addWidget(new QLabel(tr("Double-click a direction button to reassign it:"), &dlg));

    struct DirInfo { QString *key; const char *label; };
    DirInfo dirs[] = {
        { &m_upKey,    "Up  (↑)" },
        { &m_downKey,  "Down (↓)" },
        { &m_leftKey,  "Left (←)" },
        { &m_rightKey, "Right (→)" },
    };

    for (auto &d : dirs) {
        auto *row = new QHBoxLayout();
        auto *lbl = new QLabel(tr(d.label), &dlg);
        lbl->setFixedWidth(80);
        auto *btn = new QPushButton(*d.key, &dlg);
        btn->setFixedWidth(180);

        // Capture on click
        QString *keyPtr = d.key;
        QPushButton *btnPtr = btn;
        connect(btn, &QPushButton::clicked, [this, keyPtr, btnPtr, &dlg]() {
            KeyCaptureDlg kd(&dlg);
            if (kd.exec() == QDialog::Accepted && !kd.capturedKey().isEmpty()) {
                *keyPtr = kd.capturedKey();
                btnPtr->setText(*keyPtr);
                update();
            }
        });

        row->addWidget(lbl);
        row->addWidget(btn);
        vlay->addLayout(row);
    }

    auto *ok = new QPushButton(tr("Done"), &dlg);
    connect(ok, &QPushButton::clicked, &dlg, &QDialog::accept);
    vlay->addWidget(ok);

    dlg.exec();
    event->accept();
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

    // Crosshair
    p.setPen(QPen(QColor(0, 230, 100), 1));
    p.drawLine(width() / 2, 10, width() / 2, height() - 10);
    p.drawLine(10, height() / 2, width() - 10, height() / 2);

    // Direction labels (strip "Key_" prefix for display)
    auto shortKey = [](const QString &k) {
        return k.startsWith("Key_") ? k.mid(4) : k;
    };

    p.setPen(Qt::white);
    QFont f = p.font();
    f.setPointSize(10);
    f.setBold(true);
    p.setFont(f);
    p.drawText(rect().adjusted(0, 6, 0, 0),  Qt::AlignTop    | Qt::AlignHCenter, shortKey(m_upKey));
    p.drawText(rect().adjusted(0, 0, 0, -6), Qt::AlignBottom | Qt::AlignHCenter, shortKey(m_downKey));
    p.drawText(rect().adjusted(6, 0, 0, 0),  Qt::AlignLeft   | Qt::AlignVCenter, shortKey(m_leftKey));
    p.drawText(rect().adjusted(0, 0, -6, 0), Qt::AlignRight  | Qt::AlignVCenter, shortKey(m_rightKey));

    // "WASD" hint in center
    f.setPointSize(7);
    f.setBold(false);
    p.setFont(f);
    p.setPen(QColor(180, 255, 180, 160));
    p.drawText(rect(), Qt::AlignCenter, tr("dbl-click\nto edit"));

    paintCloseButton(p);
}

// ===========================================================================
// MouseMoveNodeWidget
// ===========================================================================

MouseMoveNodeWidget::MouseMoveNodeWidget(QWidget *parent)
    : KeymapNodeWidget(parent)
{
    setFixedSize(150, 110);
    m_keyName = QStringLiteral("Camera Look");
}

void MouseMoveNodeWidget::setSwitchKey(const QString &key)
{
    m_switchKey = key;
    update();
}

QJsonObject MouseMoveNodeWidget::toJson() const
{
    QJsonObject obj;

    QJsonObject posObj;
    if (parentWidget()) {
        posObj[QStringLiteral("x")] = static_cast<double>(pos().x() + width()  / 2) / parentWidget()->width();
        posObj[QStringLiteral("y")] = static_cast<double>(pos().y() + height() / 2) / parentWidget()->height();
    } else {
        posObj[QStringLiteral("x")] = 0.5;
        posObj[QStringLiteral("y")] = 0.5;
    }
    obj[QStringLiteral("startPos")]    = posObj;
    obj[QStringLiteral("speedRatioX")] = m_speedRatioX;
    obj[QStringLiteral("speedRatioY")] = m_speedRatioY;

    // Small eyes = the toggle key  (On/Off)
    QJsonObject smallEyes;
    smallEyes[QStringLiteral("type")]      = QStringLiteral("KMT_CLICK");
    smallEyes[QStringLiteral("key")]       = m_switchKey;
    QJsonObject eyePos;
    eyePos[QStringLiteral("x")] = 0.5;
    eyePos[QStringLiteral("y")] = 0.5;
    smallEyes[QStringLiteral("pos")]       = eyePos;
    smallEyes[QStringLiteral("switchMap")] = true;
    obj[QStringLiteral("smallEyes")]       = smallEyes;

    return obj;
}

void MouseMoveNodeWidget::fromJson(const QJsonObject &json)
{
    if (json.contains(QStringLiteral("startPos"))) {
        QJsonObject posObj = json[QStringLiteral("startPos")].toObject();
        double x = posObj[QStringLiteral("x")].toDouble(0.5);
        double y = posObj[QStringLiteral("y")].toDouble(0.5);
        if (parentWidget()) {
            move(x * parentWidget()->width()  - width()  / 2,
                 y * parentWidget()->height() - height() / 2);
        }
    }
    if (json.contains(QStringLiteral("speedRatioX")))
        m_speedRatioX = json[QStringLiteral("speedRatioX")].toDouble(3.0);
    if (json.contains(QStringLiteral("speedRatioY")))
        m_speedRatioY = json[QStringLiteral("speedRatioY")].toDouble(3.0);

    // Load switch key from smallEyes
    if (json.contains(QStringLiteral("smallEyes"))) {
        QJsonObject se = json[QStringLiteral("smallEyes")].toObject();
        if (se.contains(QStringLiteral("key")))
            m_switchKey = se[QStringLiteral("key")].toString();
    }
}

void MouseMoveNodeWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || closeButtonRect().contains(event->pos())) {
        QWidget::mouseDoubleClickEvent(event);
        return;
    }

    // Dialog to reassign toggle key
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Camera Look Settings"));
    dlg.setFixedSize(340, 200);

    auto *vlay = new QVBoxLayout(&dlg);
    vlay->addWidget(new QLabel(tr("Toggle Key (shows/hides mouse cursor):"), &dlg));

    auto *switchBtn = new QPushButton(m_switchKey, &dlg);
    switchBtn->setFixedHeight(40);
    vlay->addWidget(switchBtn);

    vlay->addWidget(new QLabel(tr("Click button to capture new toggle key."), &dlg));

    // Speed ratio slider row
    auto *speedRow = new QHBoxLayout();
    speedRow->addWidget(new QLabel(tr("Speed:"), &dlg));

    connect(switchBtn, &QPushButton::clicked, [this, switchBtn, &dlg]() {
        KeyCaptureDlg kd(&dlg);
        if (kd.exec() == QDialog::Accepted && !kd.capturedKey().isEmpty()) {
            m_switchKey = kd.capturedKey();
            switchBtn->setText(m_switchKey);
        }
    });

    auto *ok = new QPushButton(tr("Done"), &dlg);
    connect(ok, &QPushButton::clicked, &dlg, &QDialog::accept);
    vlay->addLayout(speedRow);
    vlay->addWidget(ok);

    dlg.exec();
    update();
    event->accept();
}

void MouseMoveNodeWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Background
    p.setPen(QPen(QColor(100, 150, 255), 2));
    p.setBrush(QColor(30, 60, 180, 120));
    p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 10, 10);

    // Title
    p.setPen(Qt::white);
    QFont f = p.font();
    f.setBold(true);
    f.setPointSize(10);
    p.setFont(f);
    p.drawText(QRect(0, 10, width(), 22), Qt::AlignCenter, tr("🎯 Camera Look"));

    // Toggle key info
    f.setPointSize(8);
    f.setBold(false);
    p.setFont(f);
    p.setPen(QColor(180, 210, 255));

    QString shortSwitch = m_switchKey.startsWith("Key_") ? m_switchKey.mid(4) : m_switchKey;
    p.drawText(QRect(0, 38, width(), 20), Qt::AlignCenter,
               tr("Toggle: ") + shortSwitch);
    p.drawText(QRect(0, 58, width(), 20), Qt::AlignCenter,
               tr("Speed: ×%1").arg(m_speedRatioX, 0, 'f', 1));

    // Help hint
    p.setPen(QColor(140, 170, 255, 160));
    f.setPointSize(7);
    p.setFont(f);
    p.drawText(QRect(0, 80, width(), 20), Qt::AlignCenter, tr("dbl-click to configure"));

    paintCloseButton(p);
}