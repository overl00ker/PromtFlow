#pragma once

#include <QGraphicsScene>
#include <QVector>
#include <QPixmap>
#include <QJsonArray>
#include "GeminiApiClient.h"
#include "HistoryEntry.h"

class NodeItem;
class ConnectionLine;
class GenerationNode;
class JsonPromptNode;

struct ImageRefData
{
    QString path;
    QString role;
    QString instructions;
};

class NodeScene : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit NodeScene(QObject *parent = nullptr);

    void requestGeneration(GenerationNode *genNode);
    void requestJsonOnly(JsonPromptNode *jsonNode);
    void saveToHistory(GenerationNode *genNode);

    void saveWorkflow(const QString &fileName);
    void loadWorkflow(const QString &fileName);


    void requestImageEdit(GenerationNode *genNode,
                          const QPixmap &currentImage,
                          const QString &editInstructions);

signals:
    void historyEntryReady(const HistoryEntry &entry);

protected:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent   *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent    *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
    void deleteNodeWithConnections(NodeItem *node);
    void deleteConnection(ConnectionLine *line);
    NodeItem *findParentNode(QGraphicsItem *item) const;
    NodeItem *findSourceForPin(NodeItem *node, int pinIndex) const;
    QList<ImageRefData> collectImageRefsForPin(NodeItem *node, int pinIndex) const;

    void continueWithJsonGeneration();
    void continueWithEditGeneration();

    void onFileUploaded(const QString &fileUri, const QString &mimeType,
                        const QString &role, const QString &instructions);
    void onFileUploadError(const QString &error);
    void onJsonGenerated(const QJsonObject &result);

    void onImageGenerated(const QPixmap &pixmap);
    void onNetworkError(const QString &error);

    ConnectionLine *m_dragLine  = nullptr;
    NodeItem       *m_dragSrc   = nullptr;
    int             m_dragPin   = -1;

    QVector<ConnectionLine *> m_connections;

    GeminiApiClient *m_apiClient        = nullptr;
    GenerationNode  *m_pendingGenNode   = nullptr;
    JsonPromptNode  *m_pendingJsonNode  = nullptr;

    int                        m_pendingUploads = 0;
    int                        m_totalUploads   = 0;
    QVector<UploadedFileInfo>  m_uploadedFiles;
    QString                    m_cachedPositive;
    QString                    m_cachedNegative;
    QJsonArray                 m_cachedImageRefs;
    QString                    m_cachedStylePreset;
    QString                    m_cachedNegativePreset;
    QString                    m_cachedJsonPrompt;


    bool    m_editMode = false;
    bool    m_skipJsonGeneration = false;
    QString m_editText;
};

