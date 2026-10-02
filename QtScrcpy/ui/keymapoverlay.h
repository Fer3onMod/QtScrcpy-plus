#ifndef KEYMAPOVERLAY_H
#define KEYMAPOVERLAY_H

#include <QWidget>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QPair>

class KeymapNodeWidget;
class QResizeEvent;

/**
 * @brief Transparent overlay that displays keymap nodes over the video widget.
 *
 * In edit mode the overlay accepts mouse input so the user can:
 *  - Right-click to get a preset menu (Fire, Scope, WASD, Custom)
 *  - Double-click to add a custom tap button
 *  - Drag nodes to reposition them
 *  - Click the red X on a node to remove it
 *
 * When edit mode is turned off the overlay becomes fully transparent to
 * mouse events so it doesn't interfere with normal game input.
 */
class KeymapOverlay : public QWidget
{
    Q_OBJECT
public:
    explicit KeymapOverlay(QWidget *parent = nullptr);
    ~KeymapOverlay();

    void loadKeymap(const QString &jsonFilePath);
    bool saveKeymap(const QString &jsonFilePath);

    void setEditMode(bool edit);
    bool isEditMode() const { return m_editMode; }

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    void addNodeAt(KeymapNodeWidget *node, const QPoint &pos);
    void registerNode(KeymapNodeWidget *node, const QJsonValue &preservedValue = QJsonValue());

    bool m_editMode = false;
    QList<KeymapNodeWidget *> m_nodes;
    QList<QPair<KeymapNodeWidget *, QJsonValue>> m_nodeEntries;
    QJsonObject m_document;
    bool m_loadedMouseMoveMap = false;
};

#endif // KEYMAPOVERLAY_H
