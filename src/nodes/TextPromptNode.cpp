#include "TextPromptNode.h"
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QScrollBar>
#include <QSizePolicy>

TextPromptNode::TextPromptNode(QGraphicsItem *parent)
    : NodeItem("Text Prompt",
               { PinInfo{"text", true} },
               QColor(70, 130, 210),
               parent)
{
    auto *container = new QWidget;
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_textEdit = new QPlainTextEdit;
    m_textEdit->setPlaceholderText("Enter text here...\n(connect to positive or negative prompt)");
    m_textEdit->setMinimumHeight(40);
    m_textEdit->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);


    m_textEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_textEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_textEdit->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);

    layout->addWidget(m_textEdit);

    container->setFixedHeight(80);
    setEmbeddedWidget(container);
}

QString TextPromptNode::promptText() const
{
    return m_textEdit ? m_textEdit->toPlainText() : QString();
}

void TextPromptNode::setPromptText(const QString &text)
{
    if (m_textEdit)
        m_textEdit->setPlainText(text);
}

