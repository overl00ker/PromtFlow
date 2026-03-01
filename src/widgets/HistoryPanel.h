#pragma once

#include <QWidget>
#include "HistoryEntry.h"

class QListWidget;
class QListWidgetItem;

class HistoryPanel : public QWidget
{
    Q_OBJECT

public:
    explicit HistoryPanel(QWidget *parent = nullptr);

public slots:
    void addEntry(const HistoryEntry &entry);

private:
    void loadFromDisk();
    void saveEntryToDisk(const HistoryEntry &entry);
    void enforceLimit();
    void onItemDoubleClicked(QListWidgetItem *item);
    void showPreview(int index);

    QListWidget *m_list = nullptr;
    QVector<HistoryEntry> m_entries;
    QString m_historyDir;

    static constexpr int kMaxEntries = 75;
    static constexpr int kThumbSize  = 150;
};
