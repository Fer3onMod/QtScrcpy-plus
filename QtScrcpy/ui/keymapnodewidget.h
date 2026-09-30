#ifndef KEYMAPNODEWIDGET_H
#define KEYMAPNODEWIDGET_H

#include <QWidget>
#include <QJsonObject>

/**
 * @brief Base class for all keymap overlay nodes.
 *
 * Each node represents one mapped key/action that will be
 * translated to a touch event on the device.
 */
class KeymapNodeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit KeymapNodeWidget(QWidget *parent = nullptr);
    virtual ~KeymapNodeWidget();

    /** Serialize this node to a JSON object for saving. */
    virtual QJsonObject toJson() const = 0;

    /** Deserialize from a JSON object when loading a keymap. */
    virtual void fromJson(const QJsonObject &json) = 0;

    /** The key label shown on the node (e.g. "A", "W", "LMB"). */
    const QString &keyName() const { return m_keyName; }
    void setKeyName(const QString &name);

signals:
    /** Emitted when the user right-clicks to request removal. */
    void removeRequested();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

    QPoint  m_dragStartPosition;
    bool    m_isDragging = false;
    QString m_keyName;
};

// ---------------------------------------------------------------------------
// ClickNodeWidget — single tap / click mapping
// ---------------------------------------------------------------------------
class ClickNodeWidget : public KeymapNodeWidget
{
    Q_OBJECT
public:
    explicit ClickNodeWidget(QWidget *parent = nullptr);

    QJsonObject toJson()  const override;
    void fromJson(const QJsonObject &json) override;

protected:
    void paintEvent(QPaintEvent *event) override;
};

// ---------------------------------------------------------------------------
// SteerWheelNodeWidget — WASD mapping
// ---------------------------------------------------------------------------
class SteerWheelNodeWidget : public KeymapNodeWidget
{
    Q_OBJECT
public:
    explicit SteerWheelNodeWidget(QWidget *parent = nullptr);

    QJsonObject toJson()  const override;
    void fromJson(const QJsonObject &json) override;

protected:
    void paintEvent(QPaintEvent *event) override;
};

#endif // KEYMAPNODEWIDGET_H
