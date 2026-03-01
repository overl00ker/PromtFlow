#pragma once

#include "NodeItem.h"
#include <QStringList>
#include <QJsonObject>

class QPlainTextEdit;
class QPushButton;

class JsonPromptNode : public NodeItem
{
public:
    explicit JsonPromptNode(QGraphicsItem *parent = nullptr);

    void    setPositivePrompt(const QString &text);
    void    setNegativePrompt(const QString &text);
    void    setImageRefs(const QStringList &refs);

    void    displayApiResult(const QJsonObject &json);

    QString jsonData() const;

    bool isApiGenerated() const;
    void setApiGenerated(bool val);
    void showLoading();
    void showApiError(const QString &err);

private:
    void rebuildJson();

    QPushButton *m_btnMakeJson = nullptr;
    bool m_isApiGenerated = false;
    
    QPlainTextEdit *m_jsonView = nullptr;
    QString     m_positivePrompt;
    QString     m_negativePrompt;
    QStringList m_imageRefs;
};
