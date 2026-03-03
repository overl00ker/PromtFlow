#include "StylePresetNode.h"

#include <QPushButton>
#include <QPlainTextEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QMenu>
#include <QSizePolicy>



const QVector<StylePresetData> &StylePresetNode::presets()
{
    static const QVector<StylePresetData> data = {
        {
            "Pro Photo (85mm)",
            "photorealistic, 85mm lens, f/1.4, professional DSLR camera, Hasselblad, "
            "ultra-detailed, subsurface scattering, cinematic lighting, sharp focus, 8k resolution",
            "3d, cartoon, anime, painted, blurry, bad anatomy, deformed, oversaturated"
        },
        {
            "Studio Portrait",
            "studio portrait, professional studio lighting, softbox, color accurate, "
            "high contrast, sharp details, medium format camera, phase one, flawless",
            "candid, outdoor, natural light, low quality, selfie, webcam, noisy"
        },
        {
            "Smartphone Casual",
            "candid photo taken on latest flagship smartphone, natural lighting, "
            "computational photography, HDR, sharp details, real life, 4k",
            "studio, professional, stylized, cartoon, overly smooth, CGI"
        },
        {
            "Vintage Phone (2010s)",
            "vintage digital camera, early 2010s smartphone photo, iPhone X era, "
            "slight digital noise, lower dynamic range, real world imperfections",
            "4k, 8k, ultra detailed, professional studio lighting, DSLR, fake"
        },
        {
            "Modern Anime",
            "anime artwork, modern anime studio, Kyoto Animation style, highly detailed, "
            "vibrant colors, beautiful lighting, cel shaded, anime masterpiece, trending on pixiv",
            "photo, realism, 3d, western cartoon, sketch, ugly, deformed"
        },
        {
            "Retro Anime (90s)",
            "retro anime aesthetic, 80s or 90s anime style, classic cell animation, "
            "dark fantasy anime vibe, muted colors, vintage anime masterpiece, VHS quality",
            "modern anime, 3d, CGI, photo, photorealistic, overly bright, digital art"
        },
        {
            "Oil Painting",
            "classical oil painting, visible impasto brush strokes, detailed texture, "
            "chiaroscuro lighting, museum quality masterpiece, traditional art",
            "photo, digital, flat colors, 3d, vector art, smooth, blurry"
        },
        {
            "Alcohol Marker",
            "A direct, full-frame scan of a traditional mixed media illustration on textured paper. The image fills the entire canvas without borders. Drawn with alcohol markers and fine liner pens. "
            "Strongly visible paper grain and texture across the whole image surface. Sharp, precise, thin line art. Characteristic marker textures, streaks, and layering visible on clothes and shading. Cool color palette. Hand-drawn anime style, analog look, non-digital finish",
            "photo, 3d, CGI, digital painting, smooth gradients, flat solid colors, border, frame, white margins"
        },
        {
            "Watercolor",
            "delicate watercolor painting, artistic, soft color washes, paper texture, "
            "wet-on-wet technique, ethereal, pastel colors",
            "photo, realism, thick paint, sharp lines, CGI, 3d render"
        },
        {
            "Pencil Sketch",
            "detailed pencil sketch, graphite pencil drawing, hatching, cross-hatching, "
            "monochrome, paper texture, traditional sketching techniques",
            "vibrant colors, photo, 3d, painting, digital art, smooth"
        },
        {
            "Game Engine (2015)",
            "2015-era game graphics, Xbox One / PS4 era rendering, real-time lighting, "
            "bloom effect, slightly dated textures, screen space reflections, game screenshot",
            "photorealism, raytracing, unreal engine 5, nanite, flat 2d, painting"
        },
        {
            "Next-Gen Engine (UE5)",
            "Unreal Engine 5 render, path tracing, Lumen lighting, Nanite geometry, "
            "hyperrealistic 3D, Quixel Megascans, real-time ray tracing, cinematic game art, 8k",
            "2d, painting, anime, sketch, low poly, 2015 graphics, flat"
        }
    };
    return data;
}



StylePresetNode::StylePresetNode(QGraphicsItem *parent)
    : NodeItem("Style Preset",
               { PinInfo{"style_out", true, false, PinType::Style} },
               QColor(180, 80, 160),
               parent)
{
    auto *container = new QWidget;
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);


    m_presetBtn = new QPushButton(presets().at(0).name);
    m_presetBtn->setStyleSheet(
        "QPushButton { background: #2a1a2e; color: #e0b0e0; border: 1px solid #8a4a8a;"
        "  border-radius: 4px; padding: 5px 10px; font-weight: bold; text-align: left; }"
        "QPushButton:hover { background: #3a2a3e; border-color: #bb6abb; }");
    layout->addWidget(m_presetBtn);


    m_infoLabel = new QLabel;
    m_infoLabel->setWordWrap(true);
    m_infoLabel->setStyleSheet(
        "QLabel { color: #999; font-size: 10px; padding: 2px 4px; }");
    m_infoLabel->setMaximumHeight(36);
    layout->addWidget(m_infoLabel);


    m_overrideEdit = new QPlainTextEdit;
    m_overrideEdit->setPlaceholderText("Optional: override / add tokens...");
    m_overrideEdit->setMinimumHeight(30);
    m_overrideEdit->setMaximumHeight(50);
    m_overrideEdit->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_overrideEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_overrideEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_overrideEdit->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    layout->addWidget(m_overrideEdit);

    container->setFixedHeight(130);
    setEmbeddedWidget(container);

    applyPreset(0);

    QObject::connect(m_presetBtn, &QPushButton::clicked, [this]() { onPresetClicked(); });
}



void StylePresetNode::onPresetClicked()
{
    QMenu menu;
    menu.setStyleSheet(
        "QMenu { background: #2b2b2b; color: #ccc; border: 1px solid #555; padding: 4px; }"
        "QMenu::item { padding: 6px 24px; }"
        "QMenu::item:selected { background: #8a4a8a; color: white; }"
        "QMenu::separator { height: 1px; background: #555; margin: 4px 8px; }");

    const auto &p = presets();
    for (int i = 0; i < p.size(); ++i) {
        QAction *a = menu.addAction(p[i].name);
        if (i == m_currentPreset)
            a->setCheckable(true), a->setChecked(true);
    }

    QAction *chosen = menu.exec(QCursor::pos());
    if (!chosen) return;

    for (int i = 0; i < p.size(); ++i) {
        if (chosen->text() == p[i].name) {
            applyPreset(i);
            break;
        }
    }
}

void StylePresetNode::setPresetIndex(int index)
{
    applyPreset(index);
}

void StylePresetNode::applyPreset(int index)
{
    if (index < 0 || index >= presets().size()) return;

    m_currentPreset = index;
    const auto &p = presets()[index];

    m_presetBtn->setText(p.name);


    QString preview = p.styleTokens;
    if (preview.length() > 80)
        preview = preview.left(77) + "...";
    m_infoLabel->setText(preview);
}



QString StylePresetNode::presetName() const
{
    return presets().at(m_currentPreset).name;
}

QString StylePresetNode::styleTokens() const
{
    return presets().at(m_currentPreset).styleTokens;
}

QString StylePresetNode::negativeTokens() const
{
    return presets().at(m_currentPreset).negativeTokens;
}

QString StylePresetNode::overrideText() const
{
    return m_overrideEdit ? m_overrideEdit->toPlainText().trimmed() : QString();
}

void StylePresetNode::setOverrideText(const QString &text)
{
    if (m_overrideEdit)
        m_overrideEdit->setPlainText(text);
}

QString StylePresetNode::combinedStyle() const
{
    QString base = styleTokens();
    QString ovr  = overrideText();
    if (!ovr.isEmpty())
        base += ", " + ovr;
    return base;
}

QString StylePresetNode::combinedNegative() const
{
    return negativeTokens();
}

