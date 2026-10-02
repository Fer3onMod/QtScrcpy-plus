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
#include <QJsonParseError>
#include <QMessageBox>
#include <QResizeEvent>
#include <QSaveFile>

KeymapOverlay::KeymapOverlay(QWidget *parent)
    : QWidget(parent), m_editMode(false)
{
    setAttribute(Qt::WA_TranslucentBackground);
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
    painter.fillRect(rect(), QColor(0, 0, 0, 70));
    painter.setPen(QPen(QColor(0, 230, 0), 2));
    painter.drawRect(rect().adjusted(1, 1, -1, -1));
    painter.setPen(QColor(255, 255, 255, 210));
    QFont f = painter.font();
    f.setPointSize(9);
    painter.setFont(f);
    painter.drawText(rect().adjusted(8, 8, -8, -8),
                     Qt::AlignTop | Qt::AlignLeft,
                     tr("Edit Mode  |  Right-click: add node  |  Double-click: custom key  |  [X] to remove  |  Drag to move"));
}

void KeymapOverlay::resizeEvent(QResizeEvent *event)
{
    const QSize oldSize = event->oldSize();
    const QSize newSize = event->size();
    if (!oldSize.isEmpty() && !newSize.isEmpty()) {
        for (auto *node : m_nodes) {
            const QPoint oldCenter = node->geometry().center();
            const double xRatio = static_cast<double>(oldCenter.x()) / oldSize.width();
            const double yRatio = static_cast<double>(oldCenter.y()) / oldSize.height();
            const int x = qRound(xRatio * newSize.width() - node->width() / 2.0);
            const int y = qRound(yRatio * newSize.height() - node->height() / 2.0);
            node->move(qBound(0, x, qMax(0, newSize.width() - node->width())),
                       qBound(0, y, qMax(0, newSize.height() - node->height())));
        }
    }
    QWidget::resizeEvent(event);
}

void KeymapOverlay::addNodeAt(KeymapNodeWidget *node, const QPoint &pos)
{
    const QPoint topLeft = pos - QPoint(node->width() / 2, node->height() / 2);
    node->move(qBound(0, topLeft.x(), qMax(0, width() - node->width())),
               qBound(0, topLeft.y(), qMax(0, height() - node->height())));
    node->show();
    registerNode(node);
}

void KeymapOverlay::registerNode(KeymapNodeWidget *node, const QJsonValue &preservedValue)
{
    m_nodes.append(node);
    if (!qobject_cast<MouseMoveNodeWidget *>(node)) {
        m_nodeEntries.append(qMakePair(node, preservedValue));
    }
    connect(node, &KeymapNodeWidget::removeRequested, this, [this, node]() {
        m_nodes.removeAll(node);
        for (int i = m_nodeEntries.size() - 1; i >= 0; --i) {
            if (m_nodeEntries.at(i).first == node) {
                m_nodeEntries.removeAt(i);
            }
        }
        node->deleteLater();
    });
}

void KeymapOverlay::mousePressEvent(QMouseEvent *event)
{
    if (m_editMode && event->button() == Qt::RightButton) {
        QMenu menu(this);
        QAction *addFire  = menu.addAction(tr("🔥  Fire Button  (Left Click)"));
        QAction *addScope = menu.addAction(tr("🎯  Scope Button  (Right Click)"));
        menu.addSeparator();
        QAction *addWasd  = menu.addAction(tr("🕹  WASD Movement"));
        QAction *addCamera = menu.addAction(tr("👀  Camera Look (Mouse Move)"));
        menu.addSeparator();
        QAction *addCustom = menu.addAction(tr("⌨  Custom Key…"));
        QAction *addDoubleTap = menu.addAction(tr("Double Tap"));
        QAction *result = menu.exec(
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            event->globalPosition().toPoint()
#else
            event->globalPos()
#endif
        );
        if (result == addFire) {
            auto *node = new ClickNodeWidget(this);
            node->setKeyName(QStringLiteral("LeftButton"));
            addNodeAt(node, event->pos());
        } else if (result == addScope) {
            auto *node = new ClickNodeWidget(this);
            node->setKeyName(QStringLiteral("RightButton"));
            addNodeAt(node, event->pos());
        } else if (result == addWasd) {
            auto *node = new SteerWheelNodeWidget(this);
            addNodeAt(node, event->pos());
        } else if (result == addCamera) {
            for (auto *existingNode : m_nodes) {
                if (qobject_cast<MouseMoveNodeWidget *>(existingNode)) {
                    return;
                }
            }
            auto *node = new MouseMoveNodeWidget(this);
            addNodeAt(node, event->pos());
        } else if (result == addCustom) {
            bool ok;
            QString key = QInputDialog::getText(this, tr("Custom Key"),
                tr("Qt key name (e.g.  Key_F,  Key_1,  Key_Space):"),
                QLineEdit::Normal, QStringLiteral("Key_F"), &ok);
            if (ok && !key.trimmed().isEmpty()) {
                auto *node = new ClickNodeWidget(this);
                node->setKeyName(key.trimmed());
                addNodeAt(node, event->pos());
            }
        } else if (result == addDoubleTap) {
            bool ok;
            QString key = QInputDialog::getText(this, tr("Double Tap Key"),
                tr("Qt key name (e.g.  Key_Q,  Key_E):"),
                QLineEdit::Normal, QStringLiteral("Key_Q"), &ok);
            if (ok && !key.trimmed().isEmpty()) {
                auto *node = new ClickNodeWidget(this);
                node->setActionType(QStringLiteral("KMT_CLICK_TWICE"));
                node->setKeyName(key.trimmed());
                addNodeAt(node, event->pos());
            }
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
        QString key = QInputDialog::getText(this, tr("Custom Key"),
            tr("Qt key name (e.g.  Key_F,  Key_1,  Key_Space):"),
            QLineEdit::Normal, QStringLiteral("Key_F"), &ok);
        if (ok && !key.trimmed().isEmpty()) {
            auto *node = new ClickNodeWidget(this);
            node->setKeyName(key.trimmed());
            addNodeAt(node, event->pos());
        }
    }
    QWidget::mouseDoubleClickEvent(event);
}

void KeymapOverlay::loadKeymap(const QString &jsonFilePath)
{
    QFile file(jsonFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (QFile::exists(jsonFilePath)) {
            qWarning() << "KeymapOverlay: cannot open keymap:" << jsonFilePath << file.errorString();
        }
        return;
    }
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "KeymapOverlay: invalid JSON keymap:" << jsonFilePath
                   << parseError.errorString() << "at offset" << parseError.offset;
        return;
    }
    const QJsonObject parsedRoot = doc.object();
    if (parsedRoot.contains(QStringLiteral("keyMapNodes"))
        && !parsedRoot.value(QStringLiteral("keyMapNodes")).isArray()) {
        qWarning() << "KeymapOverlay: keyMapNodes is not an array:" << jsonFilePath;
        return;
    }
    m_document = parsedRoot;
    m_nodeEntries.clear();
    m_loadedMouseMoveMap = false;
    qDeleteAll(m_nodes);
    m_nodes.clear();
    QJsonObject root = m_document;
    QJsonArray arr = root[QStringLiteral("keyMapNodes")].toArray();
    for (const QJsonValue &val : arr) {
        if (!val.isObject()) {
            m_nodeEntries.append(qMakePair(static_cast<KeymapNodeWidget *>(nullptr), val));
            continue;
        }
        QJsonObject obj = val.toObject();
        QString type = obj.value(QStringLiteral("type")).toString();
        KeymapNodeWidget *node = nullptr;
        if (type == QStringLiteral("KMT_CLICK") || type == QStringLiteral("KMT_CLICK_TWICE")
            || type == QStringLiteral("click")) {
            node = new ClickNodeWidget(this);
        } else if (type == QStringLiteral("KMT_STEER_WHEEL")) {
            node = new SteerWheelNodeWidget(this);
        }
        if (!node) {
            m_nodeEntries.append(qMakePair(static_cast<KeymapNodeWidget *>(nullptr), val));
            continue;
        }
        node->fromJson(obj);
        node->setVisible(m_editMode);
        registerNode(node);
    }
    if (root.value(QStringLiteral("mouseMoveMap")).isObject()) {
        m_loadedMouseMoveMap = true;
        QJsonObject mouseMoveObj = root.value(QStringLiteral("mouseMoveMap")).toObject();
        auto *node = new MouseMoveNodeWidget(this);
        node->fromJson(mouseMoveObj);
        node->setVisible(m_editMode);
        registerNode(node);
    }
}

bool KeymapOverlay::saveKeymap(const QString &jsonFilePath)
{
    QJsonObject root = m_document;
    QJsonArray arr;
    bool hasMouseMoveMap = false;
    if (m_loadedMouseMoveMap) {
        root.remove(QStringLiteral("mouseMoveMap"));
    }
    for (const auto *node : m_nodes) {
        if (const auto *mouseMoveNode = qobject_cast<const MouseMoveNodeWidget *>(node)) {
            root[QStringLiteral("mouseMoveMap")] = mouseMoveNode->toJson();
            hasMouseMoveMap = true;
        }
    }
    if (m_loadedMouseMoveMap && !hasMouseMoveMap) {
        root.remove(QStringLiteral("mouseMoveMap"));
    }
    for (const auto &entry : m_nodeEntries) {
        arr.append(entry.first ? entry.first->toJson() : entry.second);
    }
    if (!root.contains(QStringLiteral("switchKey"))) {
        root[QStringLiteral("switchKey")] = QStringLiteral("Key_QuoteLeft");
    }
    root[QStringLiteral("keyMapNodes")] = arr;
    QJsonDocument doc(root);
    QSaveFile file(jsonFilePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "KeymapOverlay: cannot write keymap to:" << jsonFilePath << file.errorString();
        return false;
    }
    const QByteArray data = doc.toJson(QJsonDocument::Indented);
    if (file.write(data) != data.size()) {
        qWarning() << "KeymapOverlay: incomplete keymap write to:" << jsonFilePath << file.errorString();
        file.cancelWriting();
        return false;
    }
    if (!file.commit()) {
        qWarning() << "KeymapOverlay: cannot commit keymap to:" << jsonFilePath << file.errorString();
        return false;
    }
    m_document = root;
    m_loadedMouseMoveMap = hasMouseMoveMap;
    return true;
}
