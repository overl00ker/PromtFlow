#pragma once

#include "NodeItem.h"
#include <QPixmap>

class QLabel;
class QPushButton;
class QLineEdit;

class ImageRefNode : public NodeItem
{
public:
    explicit ImageRefNode(QGraphicsItem *parent = nullptr);

    QString imagePath()    const { return m_imagePath; }
    void    setImagePath(const QString &path);

    QString role()         const { return m_role; }
    void    setRole(const QString &role);

    QString instructions() const;
    void    setInstructions(const QString &inst);

    QPixmap pixmap()       const { return m_pixmap; }

    QString structuredOutput() const;

private:
    void onLoadClicked();
    void onRoleClicked();

    QPushButton *m_roleBtn      = nullptr;
    QPushButton *m_loadBtn      = nullptr;
    QLabel      *m_preview      = nullptr;
    QLineEdit   *m_instrEdit    = nullptr;
    QString      m_imagePath;
    QString      m_role = "General";
    QPixmap      m_pixmap;
};
