#include "JsonPromptNode.h"
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QSizePolicy>
#include <QPushButton>
#include <QApplication>
#include <QClipboard>
#include "NodeScene.h"

JsonPromptNode::JsonPromptNode(QGraphicsItem *parent)
    : NodeItem("JSON Builder",
               { PinInfo{"positive_prompt", false, false, PinType::Text},
                 PinInfo{"negative_prompt", false, false, PinType::Text},
                 PinInfo{"image_refs",      false, true,  PinType::Image},
                 PinInfo{"style_preset",    false, false, PinType::Style},
                 PinInfo{"json_data",       true,  false, PinType::Json} },
               QColor(180, 160, 50),
               parent)
{
    auto *container = new QWidget;
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_jsonView = new QPlainTextEdit;
    m_jsonView->setReadOnly(true);
    m_jsonView->setMinimumHeight(60);
    m_jsonView->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    m_jsonView->setPlaceholderText("{ JSON will appear here }");
    layout->addWidget(m_jsonView);

    auto *btnLayout = new QHBoxLayout;
    btnLayout->setContentsMargins(0, 0, 0, 0);
    btnLayout->setSpacing(2);

    m_btnMakeJson = new QPushButton("Make JSON");
    btnLayout->addWidget(m_btnMakeJson);
    QObject::connect(m_btnMakeJson, &QPushButton::clicked, [this]() {
        auto *ns = dynamic_cast<NodeScene*>(scene());
        if (ns) ns->requestJsonOnly(this);
    });

    auto *btnCopyJson = new QPushButton("Copy");
    btnCopyJson->setToolTip("Copy JSON to clipboard");
    btnCopyJson->setFixedWidth(60);
    btnLayout->addWidget(btnCopyJson);
    QObject::connect(btnCopyJson, &QPushButton::clicked, [this]() {
        if (m_jsonView) {
            QApplication::clipboard()->setText(m_jsonView->toPlainText());
        }
    });

    auto *btnResetJson = new QPushButton("Reset");
    btnResetJson->setToolTip("Reset JSON to connected inputs");
    btnResetJson->setFixedWidth(60);
    btnLayout->addWidget(btnResetJson);
    QObject::connect(btnResetJson, &QPushButton::clicked, [this]() {
        rebuildJson();
    });

    layout->addLayout(btnLayout);

    container->setFixedHeight(170);
    setEmbeddedWidget(container);

    rebuildJson();
}

void JsonPromptNode::setPositivePrompt(const QString &text)
{
    m_positivePrompt = text;
    rebuildJson();
}

void JsonPromptNode::setNegativePrompt(const QString &text)
{
    m_negativePrompt = text;
    rebuildJson();
}

void JsonPromptNode::setImageRefs(const QStringList &refs)
{
    m_imageRefs = refs;
    rebuildJson();
}

void JsonPromptNode::displayApiResult(const QJsonObject &json)
{
    m_isApiGenerated = true;
    if (m_btnMakeJson) {
        m_btnMakeJson->setText("Ready");
        m_btnMakeJson->setStyleSheet("background-color: #2e7d32; color: white;");
        m_btnMakeJson->setEnabled(true);
        m_btnMakeJson->setToolTip("");
    }
    if (!m_jsonView) return;
    QJsonDocument doc(json);
    m_jsonView->setPlainText(doc.toJson(QJsonDocument::Indented));
}

QString JsonPromptNode::jsonData() const
{
    return m_jsonView ? m_jsonView->toPlainText() : QString();
}

void JsonPromptNode::rebuildJson()
{
    m_isApiGenerated = false;
    if (m_btnMakeJson) {
        m_btnMakeJson->setText("Make JSON");
        m_btnMakeJson->setEnabled(true);
        m_btnMakeJson->setStyleSheet("");
        m_btnMakeJson->setToolTip("");
    }

    if (!m_jsonView) return;

    QString refsArray;
    if (m_imageRefs.isEmpty()) {
        refsArray = QStringLiteral("[]");
    } else {
        QStringList entries;
        for (const auto &ref : m_imageRefs) {
            QStringList parts = ref.split('|');
            QString path  = parts.value(0, "(none)");
            QString role  = parts.value(1, "General");
            QString instr = parts.value(2, "");

            entries << QStringLiteral(
                "    {\n"
                "      \"path\": \"%1\",\n"
                "      \"role\": \"%2\",\n"
                "      \"instructions\": \"%3\"\n"
                "    }")
                .arg(path, role, instr);
        }
        refsArray = QStringLiteral("[\n%1\n  ]").arg(entries.join(",\n"));
    }

    QString json = QStringLiteral(
        "{\n"
        "  \"positive_prompt\": \"%1\",\n"
        "  \"negative_prompt\": \"%2\",\n"
        "  \"image_refs\": %3\n"
        "}")
        .arg(m_positivePrompt.isEmpty() ? "(empty)" : m_positivePrompt)
        .arg(m_negativePrompt.isEmpty() ? "(none)"  : m_negativePrompt)
        .arg(refsArray);

    m_jsonView->setPlainText(json);
}

bool JsonPromptNode::isApiGenerated() const { return m_isApiGenerated; }
void JsonPromptNode::setApiGenerated(bool val) { m_isApiGenerated = val; }

void JsonPromptNode::showLoading()
{
    if (m_btnMakeJson) {
        m_btnMakeJson->setEnabled(false);
        m_btnMakeJson->setText("Generating JSON...");
        m_btnMakeJson->setStyleSheet("background-color: #7b1fa2; color: white;");
        m_btnMakeJson->setToolTip("");
    }
}

void JsonPromptNode::showApiError(const QString &err)
{
    if (m_btnMakeJson) {
        m_btnMakeJson->setEnabled(true);
        m_btnMakeJson->setText("Error (Hover)");
        m_btnMakeJson->setToolTip(err);
        m_btnMakeJson->setStyleSheet("background-color: #c62828; color: white;");
    }
}
