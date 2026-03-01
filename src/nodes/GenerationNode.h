#pragma once

#include "NodeItem.h"
#include <QPixmap>

class QLabel;
class QLineEdit;
class QPushButton;
class ClickableLabel;
class QWidget;

class GenerationNode : public NodeItem
{
public:
    explicit GenerationNode(QGraphicsItem *parent = nullptr);

    QString workName()    const;
    void    setWorkName(const QString &name);

    QString aspectRatio() const;
    void    setAspectRatio(const QString &ar);

    QString resolution()  const;
    void    setResolution(const QString &res);

    QPixmap resultPixmap() const { return m_resultPixmap; }


    void setGenerationMetadata(const QString &positive, const QString &negative,
                               const QString &style, const QString &jsonPrompt);
    QString cachedPositive()  const { return m_metaPositive; }
    QString cachedNegative()  const { return m_metaNegative; }
    QString cachedStyle()     const { return m_metaStyle; }
    QString cachedJsonPrompt() const { return m_metaJsonPrompt; }

    void showUploadingRefs(int total);
    void showUploadProgress(int done, int total);
    void showLoading();
    void showImageGenerating();
    void showGeneratedImage(const QPixmap &pixmap);
    void showApiSuccess(const QString &info);
    void showApiError(const QString &error);

private:
    void onGenerateClicked();
    void onRerunClicked();
    void onEditClicked();
    void onSaveClicked();
    void onExportClicked();
    void onPreviewClicked();

    void recalcHeight();

    QWidget *m_container = nullptr;

    QWidget     *m_setupGroup    = nullptr;
    QLineEdit   *m_nameEdit      = nullptr;
    QPushButton *m_aspectBtn     = nullptr;
    QPushButton *m_resBtn        = nullptr;
    QPushButton *m_generateBtn   = nullptr;

    QWidget        *m_resultGroup = nullptr;
    ClickableLabel *m_preview     = nullptr;
    QPushButton    *m_rerunBtn    = nullptr;
    QPushButton    *m_editBtn     = nullptr;
    QPushButton    *m_saveBtn     = nullptr;
    QPushButton    *m_exportBtn   = nullptr;

    bool m_showingResult = false;

    QPixmap m_resultPixmap;


    QString m_metaPositive;
    QString m_metaNegative;
    QString m_metaStyle;
    QString m_metaJsonPrompt;
};

