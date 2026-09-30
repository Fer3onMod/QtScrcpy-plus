#include "keymapoverlay.h"
#include "keymapnodewidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>

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
                     tr("Edit Mode: Double-click to add button | Right-click node to remove | Drag to move"));
}

void KeymapOverlay::mousePressEvent(QMouseEvent *event)
{
    QWidget::mousePressEvent(event);
}

void KeymapOverlay::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (m_editMode && event->button() == Qt::LeftButton) {
        auto *node = new ClickNodeWidget(this);
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

    if (!doc.isArray()) {
        qWarning() << "KeymapOverlay: JSON root is not array";
        return;
    }

    // Clear existing nodes
    qDeleteAll(m_nodes);
    m_nodes.clear();

    const QJsonArray arr = doc.array();
    for (const QJsonValue &val : arr) {
        if (!val.isObject()) continue;
        QJsonObject obj = val.toObject();
        QString type = obj.value("type").toString();

        KeymapNodeWidget *node = nullptr;
        if (type == "click") {
            node = new ClickNodeWidget(this);
        }
        // Add more types here as needed

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

    QJsonDocument doc(arr);

    QFile file(jsonFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "KeymapOverlay: Cannot open file for writing:" << jsonFilePath;
        return;
    }
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
}
