#include "ImageRefNode.h"
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMenu>
#include <QSizePolicy>

ImageRefNode::ImageRefNode(QGraphicsItem *parent)
    : NodeItem("Image Reference",
               { PinInfo{"image_ref", true, false, PinType::Image} },
               QColor(200, 100, 60),
               parent)
{
    auto *container = new QWidget;
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    {
        auto *row = new QHBoxLayout;
        row->setSpacing(4);
        auto *label = new QLabel("Role:");
        label->setFixedWidth(42);
        row->addWidget(label);

        m_roleBtn = new QPushButton("General");
        m_roleBtn->setStyleSheet(
            "QPushButton { background: #1e1e1e; color: #ccc; border: 1px solid #555;"
            "  border-radius: 3px; padding: 3px 8px; text-align: left; }"
            "QPushButton:hover { background: #2a2a2a; border-color: #888; }");
        row->addWidget(m_roleBtn);
        layout->addLayout(row);
    }

    m_loadBtn = new QPushButton("Load Image...");
    layout->addWidget(m_loadBtn);

    m_preview = new QLabel;
    m_preview->setMinimumHeight(60);
    m_preview->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    m_preview->setAlignment(Qt::AlignCenter);
    m_preview->setStyleSheet("QLabel { background: #1e1e1e; border: 1px dashed #555; border-radius: 3px; color: #777; }");
    m_preview->setText("No image loaded");
    layout->addWidget(m_preview);

    m_instrEdit = new QLineEdit;
    m_instrEdit->setPlaceholderText("Additional instructions (e.g. change hair color)...");
    layout->addWidget(m_instrEdit);

    container->setFixedHeight(192);
    setEmbeddedWidget(container);

    QObject::connect(m_loadBtn, &QPushButton::clicked, [this]() { onLoadClicked(); });
    QObject::connect(m_roleBtn, &QPushButton::clicked, [this]() { onRoleClicked(); });
}

void ImageRefNode::onRoleClicked()
{
    QMenu menu;
    menu.setStyleSheet(
        "QMenu { background: #2b2b2b; color: #ccc; border: 1px solid #555; padding: 4px; }"
        "QMenu::item { padding: 6px 24px; }"
        "QMenu::item:selected { background: #3c78b5; color: white; }");

    QAction *aGeneral    = menu.addAction("General");
    QAction *aCharacter  = menu.addAction("Character");
    QAction *aBackground = menu.addAction("Background");
    QAction *aStyle      = menu.addAction("Style");
    QAction *aPose       = menu.addAction("Pose");

    QAction *chosen = menu.exec(QCursor::pos());

    if (chosen) {
        m_role = chosen->text();
        m_roleBtn->setText(m_role);
    }
}

QString ImageRefNode::instructions() const
{
    return m_instrEdit ? m_instrEdit->text() : QString();
}

void ImageRefNode::setInstructions(const QString &inst)
{
    if (m_instrEdit)
        m_instrEdit->setText(inst);
}

void ImageRefNode::setRole(const QString &role)
{
    m_role = role;
    if (m_roleBtn)
        m_roleBtn->setText(role);
}

void ImageRefNode::setImagePath(const QString &path)
{
    m_imagePath = path;
    if (path.isEmpty()) {
        m_pixmap = QPixmap();
        m_preview->setText("No image loaded");
    } else {
        m_pixmap = QPixmap(path);
        if (!m_pixmap.isNull()) {
            m_preview->setPixmap(m_pixmap.scaled(
                m_preview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            m_preview->setText(QString());
        } else {
            m_preview->setText("Failed to load");
        }
    }
}

QString ImageRefNode::structuredOutput() const
{
    return QString("%1|%2|%3")
        .arg(m_imagePath.isEmpty() ? "(none)" : m_imagePath,
             m_role,
             instructions());
}

void ImageRefNode::onLoadClicked()
{
    QString path = QFileDialog::getOpenFileName(
        nullptr,
        "Select Reference Image",
        QString(),
        "Images (*.png *.jpg *.jpeg *.bmp *.webp);;All files (*)");

    if (path.isEmpty()) return;

    m_imagePath = path;
    m_pixmap = QPixmap(path);

    if (!m_pixmap.isNull()) {
        m_preview->setPixmap(m_pixmap.scaled(
            m_preview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        m_preview->setText(QString());
    }
}
