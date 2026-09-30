#ifndef KEYMAPOVERLAY_H
#define KEYMAPOVERLAY_H

#include <QWidget>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

class KeymapNodeWidget;

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
    void saveKeymap(const QString &jsonFilePath);

    void setEditMode(bool edit);
    bool isEditMode() const { return m_editMode; }

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    void addNodeAt(KeymapNodeWidget *node, const QPoint &pos);

    bool m_editMode = false;
    QList<KeymapNodeWidget *> m_nodes;
};

#endif // KEYMAPOVERLAY_H
