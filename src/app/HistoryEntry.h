#pragma once

#include <QString>
#include <QPixmap>
#include <QDateTime>

struct HistoryEntry
{
    QString    id;
    QString    name;
    QDateTime  timestamp;
    QPixmap    pixmap;
    QString    positivePrompt;
    QString    negativePrompt;
    QString    stylePreset;
    QString    aspectRatio;
    QString    resolution;
    QString    jsonPrompt;
};

Q_DECLARE_METATYPE(HistoryEntry)
