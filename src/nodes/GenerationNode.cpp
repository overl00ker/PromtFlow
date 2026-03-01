#include "GenerationNode.h"
#include "ClickableLabel.h"
#include "NodeScene.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QDialog>
#include <QScreen>
#include <QApplication>
#include <QDebug>
#include <QMenu>
#include <QInputDialog>
#include <QSizePolicy>

static constexpr int kSetupHeight    = 136;
static constexpr int kResultBaseH    = 280;

GenerationNode::GenerationNode(QGraphicsItem *parent)
    : NodeItem("Generation",
               { PinInfo{"json_data", false} },
               QColor(100, 60, 180),
               parent)
{
    m_container = new QWidget;
    auto *outerLayout = new QVBoxLayout(m_container);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    m_setupGroup = new QWidget;
    auto *setupLayout = new QVBoxLayout(m_setupGroup);
    setupLayout->setContentsMargins(0, 0, 0, 0);
    setupLayout->setSpacing(4);


    {
        auto *row = new QHBoxLayout;
        row->setSpacing(4);
        auto *label = new QLabel("Name:");
        label->setFixedWidth(50);
        row->addWidget(label);

        m_nameEdit = new QLineEdit;
        m_nameEdit->setPlaceholderText("Work name (optional)");
        m_nameEdit->setStyleSheet(
            "QLineEdit { background: #1e1e1e; color: #ccc; border: 1px solid #555;"
            "  border-radius: 3px; padding: 3px 6px; }"
            "QLineEdit:focus { border-color: #6c5ce7; }");
        row->addWidget(m_nameEdit);
        setupLayout->addLayout(row);
    }


    {
        auto *row = new QHBoxLayout;
        row->setSpacing(4);
        auto *label = new QLabel("Aspect:");
        label->setFixedWidth(50);
        row->addWidget(label);

        m_aspectBtn = new QPushButton("1:1");
        m_aspectBtn->setStyleSheet(
            "QPushButton { background: #1e1e1e; color: #ccc; border: 1px solid #555;"
            "  border-radius: 3px; padding: 3px 8px; text-align: left; }"
            "QPushButton:hover { background: #2a2a2a; border-color: #888; }");
        row->addWidget(m_aspectBtn);
        setupLayout->addLayout(row);
    }


    {
        auto *row = new QHBoxLayout;
        row->setSpacing(4);
        auto *label = new QLabel("Res:");
        label->setFixedWidth(50);
        row->addWidget(label);

        m_resBtn = new QPushButton("FHD (1920x1080)");
        m_resBtn->setStyleSheet(
            "QPushButton { background: #1e1e1e; color: #ccc; border: 1px solid #555;"
            "  border-radius: 3px; padding: 3px 8px; text-align: left; }"
            "QPushButton:hover { background: #2a2a2a; border-color: #888; }");
        row->addWidget(m_resBtn);
        setupLayout->addLayout(row);
    }

    m_generateBtn = new QPushButton("Generate");
    m_generateBtn->setStyleSheet(
        "QPushButton { background: #3c78b5; color: white; border: none;"
        "  border-radius: 4px; padding: 7px 14px; font-weight: bold; font-size: 13px; }"
        "QPushButton:hover { background: #4a8ed0; }");
    setupLayout->addWidget(m_generateBtn);

    outerLayout->addWidget(m_setupGroup);


    m_resultGroup = new QWidget;
    auto *resultLayout = new QVBoxLayout(m_resultGroup);
    resultLayout->setContentsMargins(0, 0, 0, 0);
    resultLayout->setSpacing(4);

    m_preview = new ClickableLabel;
    m_preview->setMinimumHeight(100);
    m_preview->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    m_preview->setAlignment(Qt::AlignCenter);
    m_preview->setStyleSheet(
        "QLabel { background: #1e1e1e; border: 1px solid #3c78b5;"
        "  border-radius: 4px; color: #9bb8d8; }");
    resultLayout->addWidget(m_preview);

    {
        auto *row = new QHBoxLayout;
        row->setSpacing(3);

        const char *smallBtn =
            "QPushButton { background: #3c3c3c; color: #ddd; border: 1px solid #555;"
            "  border-radius: 3px; padding: 4px 6px; font-size: 11px; }"
            "QPushButton:hover { background: #505050; }";

        m_rerunBtn = new QPushButton("Rerun");
        m_rerunBtn->setStyleSheet(smallBtn);
        row->addWidget(m_rerunBtn);

        m_editBtn = new QPushButton("Edit");
        m_editBtn->setStyleSheet(
            "QPushButton { background: #3c6c8c; color: #ddd; border: 1px solid #4a8ab0;"
            "  border-radius: 3px; padding: 4px 6px; font-size: 11px; font-weight: bold; }"
            "QPushButton:hover { background: #4a8ab0; }");
        m_editBtn->setEnabled(true);
        row->addWidget(m_editBtn);

        m_saveBtn = new QPushButton("Save");
        m_saveBtn->setStyleSheet(
            "QPushButton { background: #2d7d46; color: white; border: none;"
            "  border-radius: 3px; padding: 4px 6px; font-size: 11px; font-weight: bold; }"
            "QPushButton:hover { background: #389654; }");
        m_saveBtn->setToolTip("Save to app history");
        row->addWidget(m_saveBtn);

        m_exportBtn = new QPushButton("Export");
        m_exportBtn->setStyleSheet(
            "QPushButton { background: #5a4a8a; color: white; border: none;"
            "  border-radius: 3px; padding: 4px 6px; font-size: 11px; font-weight: bold; }"
            "QPushButton:hover { background: #6c5ce7; }");
        m_exportBtn->setToolTip("Export image to device");
        row->addWidget(m_exportBtn);

        resultLayout->addLayout(row);
    }

    m_resultGroup->hide();
    outerLayout->addWidget(m_resultGroup);

    m_container->setFixedHeight(kSetupHeight);
    setEmbeddedWidget(m_container);

    QObject::connect(m_generateBtn, &QPushButton::clicked, [this]() { onGenerateClicked(); });
    QObject::connect(m_rerunBtn,    &QPushButton::clicked, [this]() { onRerunClicked(); });
    QObject::connect(m_editBtn,     &QPushButton::clicked, [this]() { onEditClicked(); });
    QObject::connect(m_saveBtn,     &QPushButton::clicked, [this]() { onSaveClicked(); });
    QObject::connect(m_exportBtn,   &QPushButton::clicked, [this]() { onExportClicked(); });
    QObject::connect(m_preview,     &ClickableLabel::clicked, [this]() { onPreviewClicked(); });


    QObject::connect(m_aspectBtn, &QPushButton::clicked, [this]() {
        QMenu menu;
        menu.setStyleSheet(
            "QMenu { background: #2b2b2b; color: #ccc; border: 1px solid #555; padding: 4px; }"
            "QMenu::item { padding: 6px 24px; }"
            "QMenu::item:selected { background: #3c78b5; color: white; }"
            "QMenu::separator { height: 1px; background: #555; margin: 4px 8px; }");
        menu.addAction("1:1");
        menu.addAction("3:4");
        menu.addAction("4:3");
        menu.addSeparator();
        menu.addAction("9:16");
        menu.addAction("16:9");
        menu.addSeparator();
        menu.addAction("9:19.5");
        menu.addAction("19.5:9");
        menu.addAction("21:9");
        menu.addSeparator();
        QAction *custom = menu.addAction("Custom...");
        QAction *chosen = menu.exec(QCursor::pos());
        if (chosen == custom) {
            bool ok = false;
            QString val = QInputDialog::getText(
                nullptr, "Custom Aspect Ratio",
                "Enter aspect ratio (e.g. 3:2, 32:9):",
                QLineEdit::Normal, m_aspectBtn->text(), &ok);
            if (ok && !val.trimmed().isEmpty())
                m_aspectBtn->setText(val.trimmed());
        } else if (chosen) {
            m_aspectBtn->setText(chosen->text());
        }
    });


    QObject::connect(m_resBtn, &QPushButton::clicked, [this]() {
        QMenu menu;
        menu.setStyleSheet(
            "QMenu { background: #2b2b2b; color: #ccc; border: 1px solid #555; padding: 4px; }"
            "QMenu::item { padding: 6px 24px; }"
            "QMenu::item:selected { background: #3c78b5; color: white; }"
            "QMenu::separator { height: 1px; background: #555; margin: 4px 8px; }");
        menu.addAction("FHD (1920x1080)");
        menu.addAction("QHD (2560x1440)");
        menu.addAction("UHD (3840x2160)");
        menu.addSeparator();
        QAction *custom = menu.addAction("Custom...");
        QAction *chosen = menu.exec(QCursor::pos());
        if (chosen == custom) {
            bool ok = false;
            QString val = QInputDialog::getText(
                nullptr, "Custom Resolution",
                "Enter resolution (e.g. 5120x1440, 1920x1080):",
                QLineEdit::Normal, m_resBtn->text(), &ok);
            if (ok && !val.trimmed().isEmpty())
                m_resBtn->setText(val.trimmed());
        } else if (chosen) {
            m_resBtn->setText(chosen->text());
        }
    });
}



QString GenerationNode::workName() const
{
    return m_nameEdit ? m_nameEdit->text().trimmed() : QString();
}

void GenerationNode::setWorkName(const QString &name)
{
    if (m_nameEdit)
        m_nameEdit->setText(name);
}

QString GenerationNode::aspectRatio() const
{
    return m_aspectBtn ? m_aspectBtn->text() : QString();
}

void GenerationNode::setAspectRatio(const QString &ar)
{
    if (m_aspectBtn)
        m_aspectBtn->setText(ar);
}

QString GenerationNode::resolution() const
{
    return m_resBtn ? m_resBtn->text() : QString();
}

void GenerationNode::setResolution(const QString &res)
{
    if (m_resBtn)
        m_resBtn->setText(res);
}

void GenerationNode::setGenerationMetadata(const QString &positive,
                                           const QString &negative,
                                           const QString &style,
                                           const QString &jsonPrompt)
{
    m_metaPositive   = positive;
    m_metaNegative   = negative;
    m_metaStyle      = style;
    m_metaJsonPrompt = jsonPrompt;
}



void GenerationNode::showUploadingRefs(int total)
{
    m_showingResult = true;
    m_setupGroup->hide();
    m_resultGroup->show();
    m_preview->setPixmap(QPixmap());
    m_preview->setStyleSheet(
        "QLabel { background: #1e1e2e; border: 1px solid #e8a838;"
        "  border-radius: 4px; color: #e8c868; }");
    m_preview->setText(QString::fromUtf8("\xf0\x9f\x93\xa4 Uploading %1 reference image(s)...")
                           .arg(total));
    recalcHeight();
}

void GenerationNode::showUploadProgress(int done, int total)
{
    m_preview->setText(QString::fromUtf8("\xf0\x9f\x93\xa4 Uploading reference images... (%1/%2)")
                           .arg(done).arg(total));
}

void GenerationNode::showLoading()
{
    m_preview->setPixmap(QPixmap());
    m_preview->setStyleSheet(
        "QLabel { background: #1e1e1e; border: 1px solid #3c78b5;"
        "  border-radius: 4px; color: #9bb8d8; }");
    m_preview->setText(QString::fromUtf8("\xe2\x8f\xb3 Step 1/2: Generating JSON prompt..."));
}

void GenerationNode::showImageGenerating()
{
    m_showingResult = true;
    m_setupGroup->hide();
    m_resultGroup->show();
    m_preview->setPixmap(QPixmap());
    m_preview->setStyleSheet(
        "QLabel { background: #1a1a2e; border: 1px solid #6c5ce7;"
        "  border-radius: 4px; color: #a29bfe; }");
    m_preview->setText(QString::fromUtf8("\xf0\x9f\x8e\xa8 Step 2/2: Rendering image..."));
    recalcHeight();
}

void GenerationNode::showGeneratedImage(const QPixmap &pixmap)
{
    m_resultPixmap = pixmap;
    if (!pixmap.isNull()) {
        m_preview->setText(QString());
        m_preview->setStyleSheet(
            "QLabel { background: #1e1e1e; border: 1px solid #4a8a4a;"
            "  border-radius: 4px; }");
        m_preview->setPixmap(pixmap.scaled(
            m_preview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

void GenerationNode::showApiSuccess(const QString &info)
{
    m_preview->setPixmap(QPixmap());
    m_preview->setStyleSheet(
        "QLabel { background: #1a2e1a; border: 1px solid #4a8a4a;"
        "  border-radius: 4px; color: #8cc88c; }");
    m_preview->setText(QString::fromUtf8("\xe2\x9c\x85 ") + info);
}

void GenerationNode::showApiError(const QString &error)
{
    m_showingResult = true;
    m_setupGroup->hide();
    m_resultGroup->show();
    m_preview->setPixmap(QPixmap());
    m_preview->setStyleSheet(
        "QLabel { background: #2e1a1a; border: 1px solid #a44444;"
        "  border-radius: 4px; color: #cc8888; }");
    m_preview->setText(QString::fromUtf8("\xe2\x9d\x8c ") + error);
    recalcHeight();
}



void GenerationNode::onGenerateClicked()
{
    auto *ns = dynamic_cast<NodeScene *>(scene());
    if (ns)
        ns->requestGeneration(this);
}

void GenerationNode::onRerunClicked()
{
    m_showingResult = false;
    m_resultGroup->hide();
    m_setupGroup->show();
    m_resultPixmap = QPixmap();
    recalcHeight();
}

void GenerationNode::onEditClicked()
{
    if (m_resultPixmap.isNull()) {
        QMessageBox::information(nullptr, "Edit Image",
                                 "No generated image to edit yet.\nGenerate an image first.");
        return;
    }

    bool ok = false;
    QString instructions = QInputDialog::getMultiLineText(
        nullptr, "Edit Image", "What would you like to change?",
        QString(), &ok);

    if (!ok || instructions.trimmed().isEmpty())
        return;

    auto *ns = dynamic_cast<NodeScene *>(scene());
    if (ns)
        ns->requestImageEdit(this, m_resultPixmap, instructions.trimmed());
}

void GenerationNode::onSaveClicked()
{
    if (m_resultPixmap.isNull()) {
        QMessageBox::information(nullptr, "Save", "No generated image to save yet.");
        return;
    }

    auto *ns = dynamic_cast<NodeScene *>(scene());
    if (ns)
        ns->saveToHistory(this);
}

void GenerationNode::onExportClicked()
{
    if (m_resultPixmap.isNull()) {
        QMessageBox::information(nullptr, "Export Image",
                                 "No generated image to export yet.");
        return;
    }

    QString defaultName = workName().isEmpty() ? "output.png" : workName() + ".png";
    QString path = QFileDialog::getSaveFileName(
        nullptr, "Export Generated Image", defaultName,
        "PNG (*.png);;JPEG (*.jpg);;All files (*)");

    if (!path.isEmpty())
        m_resultPixmap.save(path);
}

void GenerationNode::onPreviewClicked()
{
    auto *dlg = new QDialog(nullptr, Qt::Dialog | Qt::WindowCloseButtonHint);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowTitle("Image Preview");
    dlg->setStyleSheet("QDialog { background: #1a1a1a; }");

    QScreen *screen = QApplication::primaryScreen();
    QSize screenSize = screen ? screen->availableSize() : QSize(1920, 1080);
    int dlgW = screenSize.width()  * 2 / 3;
    int dlgH = screenSize.height() * 2 / 3;
    dlg->resize(dlgW, dlgH);

    auto *layout = new QVBoxLayout(dlg);
    layout->setContentsMargins(8, 8, 8, 8);

    auto *imgLabel = new QLabel;
    imgLabel->setAlignment(Qt::AlignCenter);
    imgLabel->setStyleSheet("QLabel { background: transparent; color: #666; font-size: 16px; }");

    if (m_resultPixmap.isNull()) {
        imgLabel->setText("No image generated yet.\nPress Generate first.");
    } else {
        imgLabel->setPixmap(m_resultPixmap.scaled(
            dlgW - 16, dlgH - 80,
            Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    layout->addWidget(imgLabel);

    const char *btnStyle =
        "QPushButton { background: #3c3c3c; color: #ddd; border: 1px solid #555;"
        "  border-radius: 4px; padding: 6px 20px; font-size: 13px; }"
        "QPushButton:hover { background: #505050; }";

    auto *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();

    auto *exportBtn = new QPushButton("Export");
    exportBtn->setStyleSheet(btnStyle);
    btnLayout->addWidget(exportBtn);

    auto *closeBtn = new QPushButton("Close");
    closeBtn->setStyleSheet(btnStyle);
    btnLayout->addWidget(closeBtn);

    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    QPixmap pix = m_resultPixmap;
    QString name = workName();
    QObject::connect(exportBtn, &QPushButton::clicked, dlg, [pix, name]() {
        QString defName = name.isEmpty() ? "output.png" : name + ".png";
        QString path = QFileDialog::getSaveFileName(
            nullptr, "Export Image", defName,
            "PNG (*.png);;JPEG (*.jpg);;All files (*)");
        if (!path.isEmpty())
            pix.save(path);
    });

    QObject::connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::close);
    dlg->show();
}

void GenerationNode::recalcHeight()
{
    int h = m_showingResult ? kResultBaseH : kSetupHeight;
    updateEmbeddedSize(h);
}

