#include "HistoryPanel.h"

#include <QListWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDialog>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScreen>
#include <QApplication>
#include <QPlainTextEdit>
#include <QDebug>

HistoryPanel::HistoryPanel(QWidget *parent)
    : QWidget(parent)
{
    m_historyDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                   + "/history";
    QDir().mkpath(m_historyDir);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    auto *title = new QLabel("Generation History");
    title->setStyleSheet("QLabel { color: #bbb; font-weight: bold; font-size: 13px; }");
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    m_list = new QListWidget;
    m_list->setIconSize(QSize(kThumbSize, kThumbSize));
    m_list->setViewMode(QListWidget::IconMode);
    m_list->setResizeMode(QListWidget::Adjust);
    m_list->setFlow(QListWidget::TopToBottom);
    m_list->setWrapping(false);
    m_list->setSpacing(6);
    m_list->setMovement(QListWidget::Static);
    m_list->setStyleSheet(
        "QListWidget { background: #1e1e1e; border: 1px solid #333; border-radius: 4px; }"
        "QListWidget::item { background: #2a2a2a; border: 1px solid #3a3a3a; "
        "  border-radius: 4px; padding: 4px; color: #bbb; }"
        "QListWidget::item:selected { border-color: #6c5ce7; background: #2e2a3e; }"
        "QListWidget::item:hover { border-color: #555; background: #333; }");
    layout->addWidget(m_list);

    connect(m_list, &QListWidget::itemDoubleClicked,
            this, &HistoryPanel::onItemDoubleClicked);

    loadFromDisk();
}



void HistoryPanel::loadFromDisk()
{
    QDir dir(m_historyDir);
    QStringList jsonFiles = dir.entryList({"*.json"}, QDir::Files, QDir::Name);

    for (const QString &jsonFile : jsonFiles) {
        QString baseName = jsonFile;
        baseName.chop(5);

        QFile f(m_historyDir + "/" + jsonFile);
        if (!f.open(QIODevice::ReadOnly)) continue;
        QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
        f.close();
        if (!doc.isObject()) continue;

        QJsonObject obj = doc.object();
        HistoryEntry entry;
        entry.id             = baseName;
        entry.name           = obj["name"].toString();
        entry.timestamp      = QDateTime::fromString(obj["timestamp"].toString(), Qt::ISODate);
        entry.positivePrompt = obj["positivePrompt"].toString();
        entry.negativePrompt = obj["negativePrompt"].toString();
        entry.stylePreset    = obj["stylePreset"].toString();
        entry.aspectRatio    = obj["aspectRatio"].toString();
        entry.resolution     = obj["resolution"].toString();
        entry.jsonPrompt     = obj["jsonPrompt"].toString();

        QString imgPath = m_historyDir + "/" + baseName + ".png";
        if (QFile::exists(imgPath))
            entry.pixmap.load(imgPath);

        m_entries.append(entry);

        auto *item = new QListWidgetItem;
        item->setText(entry.name.isEmpty() ? entry.id : entry.name);
        if (!entry.pixmap.isNull()) {
            item->setIcon(QIcon(entry.pixmap.scaled(
                kThumbSize, kThumbSize, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
        }
        item->setToolTip(entry.timestamp.toString("yyyy-MM-dd hh:mm:ss"));
        m_list->addItem(item);
    }

    qDebug() << "[History] Loaded" << m_entries.size() << "entries from disk";
}



void HistoryPanel::addEntry(const HistoryEntry &entry)
{
    m_entries.append(entry);
    saveEntryToDisk(entry);
    enforceLimit();

    auto *item = new QListWidgetItem;
    item->setText(entry.name.isEmpty() ? entry.id : entry.name);
    if (!entry.pixmap.isNull()) {
        item->setIcon(QIcon(entry.pixmap.scaled(
            kThumbSize, kThumbSize, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
    }
    item->setToolTip(entry.timestamp.toString("yyyy-MM-dd hh:mm:ss"));
    m_list->addItem(item);
    m_list->scrollToBottom();
}



void HistoryPanel::saveEntryToDisk(const HistoryEntry &entry)
{
    if (!entry.pixmap.isNull())
        entry.pixmap.save(m_historyDir + "/" + entry.id + ".png", "PNG");

    QJsonObject obj;
    obj["name"]           = entry.name;
    obj["timestamp"]      = entry.timestamp.toString(Qt::ISODate);
    obj["positivePrompt"] = entry.positivePrompt;
    obj["negativePrompt"] = entry.negativePrompt;
    obj["stylePreset"]    = entry.stylePreset;
    obj["aspectRatio"]    = entry.aspectRatio;
    obj["resolution"]     = entry.resolution;
    obj["jsonPrompt"]     = entry.jsonPrompt;

    QFile f(m_historyDir + "/" + entry.id + ".json");
    if (f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
        f.close();
    }
}

void HistoryPanel::enforceLimit()
{
    while (m_entries.size() > kMaxEntries) {
        const HistoryEntry &oldest = m_entries.first();
        QFile::remove(m_historyDir + "/" + oldest.id + ".png");
        QFile::remove(m_historyDir + "/" + oldest.id + ".json");
        delete m_list->takeItem(0);
        m_entries.removeFirst();
    }
}



void HistoryPanel::onItemDoubleClicked(QListWidgetItem *item)
{
    int row = m_list->row(item);
    if (row >= 0 && row < m_entries.size())
        showPreview(row);
}

void HistoryPanel::showPreview(int index)
{
    const HistoryEntry &entry = m_entries[index];

    auto *dlg = new QDialog(this, Qt::Dialog | Qt::WindowCloseButtonHint);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowTitle(entry.name.isEmpty() ? "Preview" : entry.name);
    dlg->setStyleSheet("QDialog { background: #1a1a1a; }");

    QScreen *screen = QApplication::primaryScreen();
    QSize sz = screen ? screen->availableSize() : QSize(1920, 1080);
    int dlgW = sz.width()  * 2 / 3;
    int dlgH = sz.height() * 2 / 3;
    dlg->resize(dlgW, dlgH);

    auto *mainLayout = new QHBoxLayout(dlg);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);


    auto *imgLabel = new QLabel;
    imgLabel->setAlignment(Qt::AlignCenter);
    imgLabel->setStyleSheet(
        "QLabel { background: #111; border: 1px solid #333; border-radius: 4px; }");
    if (!entry.pixmap.isNull()) {
        imgLabel->setPixmap(entry.pixmap.scaled(
            dlgW * 2 / 3 - 24, dlgH - 60,
            Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        imgLabel->setText("No image");
    }
    mainLayout->addWidget(imgLabel, 3);


    auto *rightPanel = new QWidget;
    auto *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(6);

    auto *metaTitle = new QLabel("Generation Info");
    metaTitle->setStyleSheet(
        "QLabel { color: #aaa; font-weight: bold; font-size: 13px; }");
    rightLayout->addWidget(metaTitle);


    QString meta;
    meta += "Name: " + (entry.name.isEmpty() ? "(unnamed)" : entry.name) + "\n";
    meta += "Date: " + entry.timestamp.toString("yyyy-MM-dd hh:mm:ss") + "\n";
    meta += "Aspect: " + entry.aspectRatio + "\n";
    meta += "Resolution: " + entry.resolution + "\n\n";
    if (!entry.positivePrompt.isEmpty())
        meta += "Positive Prompt:\n" + entry.positivePrompt + "\n\n";
    if (!entry.negativePrompt.isEmpty())
        meta += "Negative Prompt:\n" + entry.negativePrompt + "\n\n";
    if (!entry.stylePreset.isEmpty())
        meta += "Style Preset:\n" + entry.stylePreset + "\n\n";
    if (!entry.jsonPrompt.isEmpty())
        meta += "Generated JSON:\n" + entry.jsonPrompt + "\n";

    auto *metaView = new QPlainTextEdit;
    metaView->setReadOnly(true);
    metaView->setPlainText(meta);
    metaView->setStyleSheet(
        "QPlainTextEdit { background: #1e1e1e; color: #bbb; border: 1px solid #333;"
        "  border-radius: 4px; font-size: 11px; padding: 6px; }");
    rightLayout->addWidget(metaView);


    const char *btnStyle =
        "QPushButton { background: #3c3c3c; color: #ddd; border: 1px solid #555;"
        "  border-radius: 4px; padding: 6px 14px; font-size: 12px; }"
        "QPushButton:hover { background: #505050; }";

    auto *exportImgBtn  = new QPushButton("Export Image");
    auto *exportInfoBtn = new QPushButton("Export Info (.txt)");
    auto *deleteBtn     = new QPushButton("Delete");
    auto *closeBtn      = new QPushButton("Close");

    exportImgBtn->setStyleSheet(btnStyle);
    exportInfoBtn->setStyleSheet(btnStyle);
    closeBtn->setStyleSheet(btnStyle);
    deleteBtn->setStyleSheet(
        "QPushButton { background: #4a2020; color: #e88; border: 1px solid #833;"
        "  border-radius: 4px; padding: 6px 14px; font-size: 12px; }"
        "QPushButton:hover { background: #633; }");

    rightLayout->addWidget(exportImgBtn);
    rightLayout->addWidget(exportInfoBtn);
    rightLayout->addWidget(deleteBtn);
    rightLayout->addWidget(closeBtn);

    mainLayout->addWidget(rightPanel, 1);


    QObject::connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::close);

    QPixmap exportPixmap = entry.pixmap;
    QString exportName   = entry.name;

    QObject::connect(exportImgBtn, &QPushButton::clicked, dlg, [exportPixmap, exportName]() {
        QString defName = exportName.isEmpty() ? "output.png" : exportName + ".png";
        QString path = QFileDialog::getSaveFileName(
            nullptr, "Export Image", defName,
            "PNG (*.png);;JPEG (*.jpg);;All files (*)");
        if (!path.isEmpty())
            exportPixmap.save(path);
    });

    QObject::connect(exportInfoBtn, &QPushButton::clicked, dlg, [meta]() {
        QString path = QFileDialog::getSaveFileName(
            nullptr, "Export Metadata", "generation_info.txt",
            "Text files (*.txt);;All files (*)");
        if (!path.isEmpty()) {
            QFile f(path);
            if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                f.write(meta.toUtf8());
                f.close();
            }
        }
    });

    QObject::connect(deleteBtn, &QPushButton::clicked, dlg, [this, index, dlg]() {
        if (index < 0 || index >= m_entries.size()) return;
        const HistoryEntry &e = m_entries[index];
        QFile::remove(m_historyDir + "/" + e.id + ".png");
        QFile::remove(m_historyDir + "/" + e.id + ".json");
        delete m_list->takeItem(index);
        m_entries.removeAt(index);
        dlg->close();
    });

    dlg->show();
}

