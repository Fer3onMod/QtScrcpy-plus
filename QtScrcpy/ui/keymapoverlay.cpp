#include "keymapoverlay.h"
#include "keymapnodewidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QDebug>
#include <QMenu>
#include <QAction>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QInputDialog>

KeymapOverlay::KeymapOverlay(QWidget *parent)
    : QWidget(parent), m_editMode(false)
{
    // Transparent, stays on top of video widget
    setAttribute(Qt::WA_TranslucentBackground);
    // Don't consume mouse events when not editing
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
}

KeymapOverlay::~KeymapOverlay()
{
    qDeleteAll(m_nodes);
    m_nodes.clear();
}

void KeymapOverlay::setEditMode(bool edit)
{
    m_editMode = edit;
    setAttribute(Qt::WA_TransparentForMouseEvents, !m_editMode);

    // Show or hide all nodes
    for (auto *node : m_nodes) {
        node->setVisible(m_editMode);
    }

    update();
}

void KeymapOverlay::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    if (!m_editMode) {
        return;
    }

    QPainter painter(this);
    painter.fillRect(rect(), QColor(0, 0, 0, 80));

    // Green border to show edit mode is active
    painter.setPen(QPen(QColor(0, 230, 0), 2));
    painter.drawRect(rect().adjusted(1, 1, -1, -1));

    // Instructions text
    painter.setPen(QColor(255, 255, 255, 200));
    QFont f = painter.font();
    f.setPointSize(10);
    painter.setFont(f);
    painter.drawText(rect().adjusted(8, 8, -8, -8),
                     Qt::AlignTop | Qt::AlignLeft,
                     tr("Edit Mode: Double-click or Right-click to add nodes | Right-click node to remove | Drag to move"));
}

void KeymapOverlay::mousePressEvent(QMouseEvent *event)
{
    if (m_editMode && event->button() == Qt::RightButton) {
        QMenu menu(this);
        QAction *addTap = menu.addAction(tr("Add Tap Button (click)"));
        QAction *addWasd = menu.addAction(tr("Add WASD Movement"));
        
        QAction *result = menu.exec(event->globalPos());
        if (result == addTap) {
            bool ok;
            QString key = QInputDialog::getText(this, tr("Assign Key"),
                tr("Enter key name (e.g. Key_F, Key_1, LeftButton):"),
                QLineEdit::Normal, QStringLiteral("Key_F"), &ok);
            if (!ok || key.trimmed().isEmpty()) return;
            auto *node = new ClickNodeWidget(this);
            node->setKeyName(key.trimmed());
            node->move(event->pos() - QPoint(node->width() / 2, node->height() / 2));
            node->show();
            m_nodes.append(node);
            connect(node, &KeymapNodeWidget::removeRequested, this, [this, node]() {
                m_nodes.removeAll(node);
                node->deleteLater();
            });
        } else if (result == addWasd) {
            auto *node = new SteerWheelNodeWidget(this);
            node->move(event->pos() - QPoint(node->width() / 2, node->height() / 2));
            node->show();
            m_nodes.append(node);
            connect(node, &KeymapNodeWidget::removeRequested, this, [this, node]() {
                m_nodes.removeAll(node);
                node->deleteLater();
            });
        }
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void KeymapOverlay::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (m_editMode && event->button() == Qt::LeftButton) {
        bool ok;
        QString key = QInputDialog::getText(this, tr("Assign Key"),
            tr("Enter key name (e.g. Key_F, Key_1, LeftButton):"),
            QLineEdit::Normal, QStringLiteral("Key_F"), &ok);
        if (!ok || key.trimmed().isEmpty()) {
            QWidget::mouseDoubleClickEvent(event);
            return;
        }
        auto *node = new ClickNodeWidget(this);
        node->setKeyName(key.trimmed());
        node->move(event->pos() - QPoint(node->width() / 2, node->height() / 2));
        node->show();
        m_nodes.append(node);

        // Connect right-click delete signal
        connect(node, &KeymapNodeWidget::removeRequested, this, [this, node]() {
            m_nodes.removeAll(node);
            node->deleteLater();
        });
    }
    QWidget::mouseDoubleClickEvent(event);
}

void KeymapOverlay::loadKeymap(const QString &jsonFilePath)
{
    QFile file(jsonFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "KeymapOverlay: Cannot open file for reading:" << jsonFilePath;
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) {
        qWarning() << "KeymapOverlay: JSON root is not object";
        return;
    }

    // Clear existing nodes
    qDeleteAll(m_nodes);
    m_nodes.clear();

    QJsonObject root = doc.object();
    QJsonArray arr = root["keyMapNodes"].toArray();
    for (const QJsonValue &val : arr) {
        if (!val.isObject()) continue;
        QJsonObject obj = val.toObject();
        QString type = obj.value("type").toString();

        KeymapNodeWidget *node = nullptr;
        if (type == "KMT_CLICK" || type == "click") {
            node = new ClickNodeWidget(this);
        } else if (type == "KMT_STEER_WHEEL") {
            node = new SteerWheelNodeWidget(this);
        }

        if (node) {
            node->fromJson(obj);
            node->setVisible(m_editMode);
            connect(node, &KeymapNodeWidget::removeRequested, this, [this, node]() {
                m_nodes.removeAll(node);
                node->deleteLater();
            });
            m_nodes.append(node);
        }
    }
}

void KeymapOverlay::saveKeymap(const QString &jsonFilePath)
{
    QJsonArray arr;
    for (const auto *node : m_nodes) {
        arr.append(node->toJson());
    }

    QJsonObject root;
    root[QStringLiteral("switchKey")] = QStringLiteral("Key_QuoteLeft");
    
    QJsonObject mouseMoveMap;
    QJsonObject startPos;
    startPos[QStringLiteral("x")] = 0.5;
    startPos[QStringLiteral("y")] = 0.5;
    mouseMoveMap[QStringLiteral("startPos")] = startPos;
    mouseMoveMap[QStringLiteral("speedRatio")] = 5;
    
    QJsonObject smallEyes;
    smallEyes[QStringLiteral("type")] = QStringLiteral("KMT_CLICK");
    smallEyes[QStringLiteral("key")] = QStringLiteral("Key_Alt");
    QJsonObject smallEyesPos;
    smallEyesPos[QStringLiteral("x")] = 0.5;
    smallEyesPos[QStringLiteral("y")] = 0.5;
    smallEyes[QStringLiteral("pos")] = smallEyesPos;
    smallEyes[QStringLiteral("switchMap")] = true;
    mouseMoveMap[QStringLiteral("smallEyes")] = smallEyes;
    
    root[QStringLiteral("mouseMoveMap")] = mouseMoveMap;
    root[QStringLiteral("keyMapNodes")] = arr;

    QJsonDocument doc(root);

    QFile file(jsonFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "KeymapOverlay: Cannot open file for writing:" << jsonFilePath;
        return;
    }
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
}
