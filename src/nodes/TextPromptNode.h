#pragma once

#include "NodeItem.h"

class QPlainTextEdit;

class TextPromptNode : public NodeItem
{
public:
    explicit TextPromptNode(QGraphicsItem *parent = nullptr);

    QString promptText() const;
    void setPromptText(const QString &text);

private:
    QPlainTextEdit *m_textEdit = nullptr;
};
