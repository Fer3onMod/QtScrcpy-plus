#ifndef KEYMAPOVERLAY_H
#define KEYMAPOVERLAY_H

#include <QWidget>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

class KeymapNodeWidget;

class KeymapOverlay : public QWidget
{
    Q_OBJECT
public:
    explicit KeymapOverlay(QWidget *parent = nullptr);
    ~KeymapOverlay();

    void loadKeymap(const QString& jsonFilePath);
    void saveKeymap(const QString& jsonFilePath);
    
    // Switch between edit mode and normal mode
    void setEditMode(bool edit);
    bool isEditMode() const { return m_editMode; }

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    
private:
    bool m_editMode = false;
    QList<KeymapNodeWidget*> m_nodes;
};

#endif // KEYMAPOVERLAY_H
