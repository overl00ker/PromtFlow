#pragma once

#include "NodeItem.h"

class QPushButton;
class QPlainTextEdit;
class QLabel;

struct StylePresetData
{
    QString name;
    QString styleTokens;
    QString negativeTokens;
};

class StylePresetNode : public NodeItem
{
public:
    explicit StylePresetNode(QGraphicsItem *parent = nullptr);

    int     presetIndex()    const { return m_currentPreset; }
    void    setPresetIndex(int index);

    QString presetName()     const;
    QString styleTokens()    const;
    QString negativeTokens() const;
    QString overrideText()   const;
    void    setOverrideText(const QString &text);


    QString combinedStyle()    const;
    QString combinedNegative() const;

private:
    void onPresetClicked();
    void applyPreset(int index);

    QPushButton    *m_presetBtn   = nullptr;
    QPlainTextEdit *m_overrideEdit = nullptr;
    QLabel         *m_infoLabel   = nullptr;

    int m_currentPreset = 0;

    static const QVector<StylePresetData> &presets();
};

