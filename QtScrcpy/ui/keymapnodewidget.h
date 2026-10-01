#ifndef KEYMAPNODEWIDGET_H
#define KEYMAPNODEWIDGET_H

#include <QWidget>
#include <QJsonObject>

/**
 * @brief Base class for all keymap overlay nodes.
 *
 * Each node represents one mapped key/action that will be
 * translated to a touch event on the device.
 *
 * Nodes have an X button in the top-right corner to remove them.
 * They can be dragged freely within the parent widget.
 * Double-clicking a node opens the key-capture dialog to reassign it.
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

    /** The key label shown on the node (e.g. "Key_A", "LeftButton"). */
    const QString &keyName() const { return m_keyName; }
    void setKeyName(const QString &name);

signals:
    /** Emitted when the user clicks the X button to request removal. */
    void removeRequested();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

    // Draws the X close button — call from subclass paintEvent
    void paintCloseButton(QPainter &p) const;

    // Returns the rect for the X button (top-right corner)
    QRect closeButtonRect() const;

    // Opens key-capture dialog and sets m_keyName to result
    void captureKey();

    QPoint  m_dragStartPosition;
    bool    m_isDragging = false;
    QString m_keyName;

private:
    bool m_closeHovered = false;
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
// SteerWheelNodeWidget — WASD movement mapping
//   Double-click opens 4-key assignment dialog (Up/Down/Left/Right)
// ---------------------------------------------------------------------------
class SteerWheelNodeWidget : public KeymapNodeWidget
{
    Q_OBJECT
public:
    explicit SteerWheelNodeWidget(QWidget *parent = nullptr);

    QJsonObject toJson()  const override;
    void fromJson(const QJsonObject &json) override;

    // Individual key getters
    const QString &upKey()    const { return m_upKey;    }
    const QString &downKey()  const { return m_downKey;  }
    const QString &leftKey()  const { return m_leftKey;  }
    const QString &rightKey() const { return m_rightKey; }

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    QString m_upKey    = QStringLiteral("Key_W");
    QString m_downKey  = QStringLiteral("Key_S");
    QString m_leftKey  = QStringLiteral("Key_A");
    QString m_rightKey = QStringLiteral("Key_D");
};

// ---------------------------------------------------------------------------
// MouseMoveNodeWidget — Camera Look / Mouse Move mapping
//   Has a configurable toggle (switch) key shown in the widget.
// ---------------------------------------------------------------------------
class MouseMoveNodeWidget : public KeymapNodeWidget
{
    Q_OBJECT
public:
    explicit MouseMoveNodeWidget(QWidget *parent = nullptr);

    QJsonObject toJson()  const override;
    void fromJson(const QJsonObject &json) override;

    const QString &switchKey() const { return m_switchKey; }
    void setSwitchKey(const QString &key);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    // Key that toggles mouse-aim on/off (default: backtick/tilde)
    QString m_switchKey = QStringLiteral("Key_QuoteLeft");
    double  m_speedRatioX = 3.0;
    double  m_speedRatioY = 3.0;
};

#endif // KEYMAPNODEWIDGET_H
