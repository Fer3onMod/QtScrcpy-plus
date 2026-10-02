#ifndef KEYMAPNODEWIDGET_H
#define KEYMAPNODEWIDGET_H

#include <QWidget>
#include <QJsonObject>

class KeymapNodeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit KeymapNodeWidget(QWidget *parent = nullptr);
    virtual ~KeymapNodeWidget();
    virtual QJsonObject toJson() const = 0;
    virtual void fromJson(const QJsonObject &json) = 0;
    const QString &keyName() const { return m_keyName; }
    void setKeyName(const QString &name);

signals:
    void removeRequested();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void paintCloseButton(QPainter &p) const;
    QRect closeButtonRect() const;
    void captureKey();

    QPoint  m_dragStartPosition;
    bool    m_isDragging = false;
    QString m_keyName;
    QJsonObject m_originalJson;

private:
    bool m_closeHovered = false;
};

class ClickNodeWidget : public KeymapNodeWidget
{
    Q_OBJECT
public:
    explicit ClickNodeWidget(QWidget *parent = nullptr);
    void setActionType(const QString &type);
    QJsonObject toJson()  const override;
    void fromJson(const QJsonObject &json) override;

protected:
    void paintEvent(QPaintEvent *event) override;
};

class SteerWheelNodeWidget : public KeymapNodeWidget
{
    Q_OBJECT
public:
    explicit SteerWheelNodeWidget(QWidget *parent = nullptr);
    QJsonObject toJson()  const override;
    void fromJson(const QJsonObject &json) override;
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
    QString m_switchKey = QStringLiteral("Key_QuoteLeft");
    double  m_speedRatioX = 3.0;
    double  m_speedRatioY = 3.0;
};

#endif // KEYMAPNODEWIDGET_H
