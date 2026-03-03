#include "NodeScene.h"
#include "NodeItem.h"
#include "ConnectionLine.h"
#include "TextPromptNode.h"
#include "ImageRefNode.h"
#include "JsonPromptNode.h"
#include "GenerationNode.h"
#include "StylePresetNode.h"

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsProxyWidget>
#include <QKeyEvent>
#include <QMenu>
#include <QColorDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDir>
#include <QFile>
#include <QMap>

static const char *kMenuStyle =
    "QMenu { background: #2b2b2b; color: #ccc; border: 1px solid #555; padding: 4px; }"
    "QMenu::item { padding: 6px 24px; }"
    "QMenu::item:selected { background: #3c78b5; color: white; }"
    "QMenu::separator { height: 1px; background: #555; margin: 4px 8px; }";

NodeScene::NodeScene(QObject *parent)
    : QGraphicsScene(parent)
{
    setSceneRect(-2000, -2000, 4000, 4000);

    m_apiClient = new GeminiApiClient(this);
    connect(m_apiClient, &GeminiApiClient::fileUploaded,      this, &NodeScene::onFileUploaded);
    connect(m_apiClient, &GeminiApiClient::fileUploadError,   this, &NodeScene::onFileUploadError);
    connect(m_apiClient, &GeminiApiClient::jsonGenerated,     this, &NodeScene::onJsonGenerated);

    connect(m_apiClient, &GeminiApiClient::imageGenerated,    this, &NodeScene::onImageGenerated);
    connect(m_apiClient, &GeminiApiClient::networkError,      this, &NodeScene::onNetworkError);
}

NodeItem *NodeScene::findParentNode(QGraphicsItem *item) const
{
    while (item) {
        auto *node = dynamic_cast<NodeItem *>(item);
        if (node) return node;
        item = item->parentItem();
    }
    return nullptr;
}

NodeItem *NodeScene::findSourceForPin(NodeItem *node, int pinIndex) const
{
    for (auto *line : node->connections()) {
        if (line->targetNode() == node && line->targetPin() == pinIndex)
            return line->sourceNode();
    }
    return nullptr;
}



QList<ImageRefData> NodeScene::collectImageRefsForPin(NodeItem *node, int pinIndex) const
{
    QList<ImageRefData> result;

    for (auto *line : node->connections()) {
        if (line->targetNode() != node || line->targetPin() != pinIndex)
            continue;

        auto *imgNode = dynamic_cast<ImageRefNode *>(line->sourceNode());
        if (!imgNode)
            continue;

        ImageRefData data;
        data.path         = imgNode->imagePath();
        data.role         = imgNode->role();
        data.instructions = imgNode->instructions();
        result.append(data);
    }

    return result;
}



void NodeScene::requestJsonOnly(JsonPromptNode *jsonNode)
{
    m_editMode = false;
    m_skipJsonGeneration = false;
    m_editText.clear();

    QString positive;
    auto *posSource = dynamic_cast<TextPromptNode *>(findSourceForPin(jsonNode, 0));
    if (posSource) positive = posSource->promptText();

    QString negative;
    auto *negSource = dynamic_cast<TextPromptNode *>(findSourceForPin(jsonNode, 1));
    if (negSource) negative = negSource->promptText();

    m_cachedStylePreset.clear();
    m_cachedNegativePreset.clear();
    auto *styleSource = dynamic_cast<StylePresetNode *>(findSourceForPin(jsonNode, 3));
    if (styleSource) {
        m_cachedStylePreset    = styleSource->combinedStyle();
        m_cachedNegativePreset = styleSource->combinedNegative();
    }

    m_pendingGenNode  = nullptr;
    m_pendingJsonNode = jsonNode;
    m_cachedPositive  = positive;
    m_cachedNegative  = negative;
    m_cachedImageRefs = QJsonArray();
    m_uploadedFiles.clear();
    m_pendingUploads = 0;
    m_totalUploads   = 0;

    QList<ImageRefData> imageRefDataList = collectImageRefsForPin(jsonNode, 2);
    QList<ImageRefData> imagesToUpload;
    for (const auto &ref : imageRefDataList) {
        if (!ref.path.isEmpty()) imagesToUpload.append(ref);
    }

    if (positive.isEmpty() && negative.isEmpty() && imagesToUpload.isEmpty()) {
        jsonNode->showApiError("Nothing connected.\nConnect at least a TextPrompt or Image Reference.");
        m_pendingJsonNode = nullptr;
        return;
    }

    jsonNode->showLoading();

    if (imagesToUpload.isEmpty()) {
        continueWithJsonGeneration();
        return;
    }

    m_totalUploads   = imagesToUpload.size();
    m_pendingUploads = imagesToUpload.size();

    for (const auto &refData : imagesToUpload) {
        QJsonObject ref;
        ref["role"]         = refData.role;
        ref["instructions"] = refData.instructions;
        m_cachedImageRefs.append(ref);

        m_apiClient->requestFileUpload(refData.path, refData.role, refData.instructions);
    }
}

void NodeScene::requestGeneration(GenerationNode *genNode)
{
    m_editMode = false;
    m_skipJsonGeneration = false;
    m_editText.clear();

    auto *jsonNodeRaw = findSourceForPin(genNode, 0);
    auto *jsonNode = dynamic_cast<JsonPromptNode *>(jsonNodeRaw);

    if (!jsonNode) {
        genNode->showApiError("No JSON Builder connected.\nConnect JsonBuilder -> Generation first.");
        return;
    }

    QString positive;
    auto *posSource = dynamic_cast<TextPromptNode *>(findSourceForPin(jsonNode, 0));
    if (posSource)
        positive = posSource->promptText();

    QString negative;
    auto *negSource = dynamic_cast<TextPromptNode *>(findSourceForPin(jsonNode, 1));
    if (negSource)
        negative = negSource->promptText();


    m_cachedStylePreset.clear();
    m_cachedNegativePreset.clear();
    auto *styleSource = dynamic_cast<StylePresetNode *>(findSourceForPin(jsonNode, 3));
    if (styleSource) {
        m_cachedStylePreset    = styleSource->combinedStyle();
        m_cachedNegativePreset = styleSource->combinedNegative();
    }

    m_pendingGenNode  = genNode;
    m_pendingJsonNode = jsonNode;
    m_cachedPositive  = positive;
    m_cachedNegative  = negative;
    m_cachedImageRefs = QJsonArray();
    m_uploadedFiles.clear();
    m_pendingUploads = 0;
    m_totalUploads   = 0;


    QList<ImageRefData> imageRefDataList = collectImageRefsForPin(jsonNode, 2);


    QList<ImageRefData> imagesToUpload;
    for (const auto &ref : imageRefDataList) {
        if (!ref.path.isEmpty())
            imagesToUpload.append(ref);
    }


    if (positive.isEmpty() && negative.isEmpty() && imagesToUpload.isEmpty()) {
        genNode->showApiError("Nothing connected.\nConnect at least a TextPrompt or Image Reference to JsonBuilder.");
        return;
    }

    if (jsonNode->isApiGenerated()) {
        m_skipJsonGeneration = true;
        m_cachedJsonPrompt = jsonNode->jsonData();
        if (imagesToUpload.isEmpty()) {
            if (m_pendingGenNode) m_pendingGenNode->showImageGenerating();
            m_apiClient->requestImageGeneration(
                m_cachedJsonPrompt,
                m_pendingGenNode->aspectRatio(),
                m_pendingGenNode->resolution(),
                m_uploadedFiles
            );
            return;
        }
    }

    if (imagesToUpload.isEmpty()) {
        continueWithJsonGeneration();
        return;
    }

    m_totalUploads   = imagesToUpload.size();
    m_pendingUploads = imagesToUpload.size();

    genNode->showUploadingRefs(m_totalUploads);

    for (const auto &refData : imagesToUpload) {
        QJsonObject ref;
        ref["role"]         = refData.role;
        ref["instructions"] = refData.instructions;
        m_cachedImageRefs.append(ref);

        m_apiClient->requestFileUpload(
            refData.path,
            refData.role,
            refData.instructions);
    }
}



void NodeScene::saveToHistory(GenerationNode *genNode)
{
    if (!genNode || genNode->resultPixmap().isNull())
        return;

    HistoryEntry entry;
    entry.id             = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz");
    entry.name           = genNode->workName();
    entry.timestamp      = QDateTime::currentDateTime();
    entry.pixmap         = genNode->resultPixmap();
    entry.positivePrompt = genNode->cachedPositive();
    entry.negativePrompt = genNode->cachedNegative();
    entry.stylePreset    = genNode->cachedStyle();
    entry.aspectRatio    = genNode->aspectRatio();
    entry.resolution     = genNode->resolution();
    entry.jsonPrompt     = genNode->cachedJsonPrompt();

    emit historyEntryReady(entry);
}



void NodeScene::requestImageEdit(GenerationNode *genNode,
                                 const QPixmap &currentImage,
                                 const QString &editInstructions)
{
    m_editMode = true;
    m_skipJsonGeneration = true;
    m_editText = editInstructions;

    m_pendingGenNode  = genNode;
    m_pendingJsonNode = nullptr;
    m_uploadedFiles.clear();
    m_cachedImageRefs = QJsonArray();
    m_cachedPositive.clear();
    m_cachedNegative.clear();

    QString tempPath = QDir::tempPath() + "/promtflow_edit_base.png";
    if (!currentImage.save(tempPath, "PNG")) {
        genNode->showApiError("Failed to save image to temporary file.");
        return;
    }

    qDebug() << "[NodeScene] Edit: saved base image to" << tempPath;


    m_totalUploads   = 1;
    m_pendingUploads = 1;

    genNode->showUploadingRefs(1);

    QJsonObject ref;
    ref["role"]         = QStringLiteral("Main_Base");
    ref["instructions"] = editInstructions;
    m_cachedImageRefs.append(ref);

    m_apiClient->requestFileUpload(tempPath, "Main_Base", editInstructions);

}



void NodeScene::onFileUploaded(const QString &fileUri, const QString &mimeType,
                               const QString &role, const QString &instructions)
{
    UploadedFileInfo info;
    info.fileUri      = fileUri;
    info.mimeType     = mimeType;
    info.role         = role;
    info.instructions = instructions;
    m_uploadedFiles.append(info);

    --m_pendingUploads;

    int done = m_totalUploads - m_pendingUploads;
    qDebug() << "[NodeScene] File upload" << done << "/" << m_totalUploads << "complete";

    if (m_pendingGenNode)
        m_pendingGenNode->showUploadProgress(done, m_totalUploads);

    if (m_pendingUploads <= 0) {
        if (m_editMode) {
            continueWithEditGeneration();
        } else if (m_skipJsonGeneration) {
            if (m_pendingGenNode) m_pendingGenNode->showImageGenerating();
            m_apiClient->requestImageGeneration(
                m_cachedJsonPrompt,
                m_pendingGenNode ? m_pendingGenNode->aspectRatio() : QStringLiteral("1:1"),
                m_pendingGenNode ? m_pendingGenNode->resolution() : QStringLiteral("FHD (1920x1080)"),
                m_uploadedFiles
            );
        } else {
            continueWithJsonGeneration();
        }
    }
}

void NodeScene::onFileUploadError(const QString &error)
{
    m_pendingUploads = 0;
    m_uploadedFiles.clear();
    m_editMode = false;
    m_skipJsonGeneration = false;

    if (m_pendingGenNode)
        m_pendingGenNode->showApiError("File upload failed:\n" + error);
    else if (m_pendingJsonNode)
        m_pendingJsonNode->showApiError("File upload failed:\n" + error);

    m_pendingGenNode  = nullptr;
    m_pendingJsonNode = nullptr;
}



void NodeScene::continueWithJsonGeneration()
{
    if (m_pendingGenNode)
        m_pendingGenNode->showLoading();


    m_apiClient->requestJsonPrompt(
        m_cachedPositive,
        m_cachedNegative,
        m_cachedImageRefs,
        m_uploadedFiles,
        m_cachedStylePreset,
        m_cachedNegativePreset);
}



void NodeScene::continueWithEditGeneration()
{
    if (m_pendingGenNode)
        m_pendingGenNode->showImageGenerating();


    QString editPrompt = QStringLiteral(
        "Edit the provided base image according to the user's instructions.\n"
        "User wants: %1").arg(m_editText);

    m_apiClient->requestImageGeneration(
        editPrompt,
        m_pendingGenNode ? m_pendingGenNode->aspectRatio() : QStringLiteral("1:1"),
        m_pendingGenNode ? m_pendingGenNode->resolution()  : QStringLiteral("FHD (1920x1080)"),
        m_uploadedFiles);

    m_editMode = false;
    m_editText.clear();
}



void NodeScene::onJsonGenerated(const QJsonObject &result)
{
    if (m_pendingJsonNode)
        m_pendingJsonNode->displayApiResult(result);

    if (m_pendingGenNode) {
        QJsonDocument doc(result);
        QString jsonText = doc.toJson(QJsonDocument::Indented);
        m_cachedJsonPrompt = jsonText;

        m_pendingGenNode->showImageGenerating();

        m_apiClient->requestImageGeneration(
            jsonText,
            m_pendingGenNode->aspectRatio(),
            m_pendingGenNode->resolution(),
            m_uploadedFiles);
    } else {
        m_pendingJsonNode = nullptr;
    }
}



void NodeScene::onImageGenerated(const QPixmap &pixmap)
{
    if (m_pendingGenNode) {
        m_pendingGenNode->showGeneratedImage(pixmap);
        m_pendingGenNode->setGenerationMetadata(
            m_cachedPositive, m_cachedNegative,
            m_cachedStylePreset, m_cachedJsonPrompt);
    }

    m_pendingGenNode  = nullptr;
    m_pendingJsonNode = nullptr;
    m_uploadedFiles.clear();
}

void NodeScene::onNetworkError(const QString &error)
{
    if (m_pendingGenNode)
        m_pendingGenNode->showApiError(error);
    else if (m_pendingJsonNode)
        m_pendingJsonNode->showApiError(error);

    m_pendingGenNode  = nullptr;
    m_pendingJsonNode = nullptr;
    m_uploadedFiles.clear();
    m_editMode = false;
    m_skipJsonGeneration = false;
}



void NodeScene::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    QGraphicsItem *itemUnder = itemAt(event->scenePos(), QTransform());
    NodeItem *node = itemUnder ? findParentNode(itemUnder) : nullptr;
    ConnectionLine *line = dynamic_cast<ConnectionLine *>(itemUnder);

    if (node) {
        QMenu menu;
        menu.setStyleSheet(kMenuStyle);

        QAction *changeColor = menu.addAction(QString::fromUtf8("\xf0\x9f\x8e\xa8  Change Color"));
        menu.addSeparator();
        QAction *deleteNode  = menu.addAction(QString::fromUtf8("\xf0\x9f\x97\x91  Delete Node"));

        QAction *chosen = menu.exec(event->screenPos());

        if (chosen == changeColor) {
            QColor color = QColorDialog::getColor(
                node->headerColor(), nullptr, "Choose Header Color");
            if (color.isValid())
                node->setHeaderColor(color);
        } else if (chosen == deleteNode) {
            deleteNodeWithConnections(node);
        }

        event->accept();
        return;
    }

    if (line) {
        QMenu menu;
        menu.setStyleSheet(kMenuStyle);

        QAction *deleteLine = menu.addAction(QString::fromUtf8("\xf0\x9f\x97\x91  Delete Connection"));
        QAction *chosen = menu.exec(event->screenPos());

        if (chosen == deleteLine)
            deleteConnection(line);

        event->accept();
        return;
    }

    QMenu menu;
    menu.setStyleSheet(kMenuStyle);

    QAction *addPrompt   = menu.addAction(QString::fromUtf8("\xf0\x9f\x93\x9d  Add Text Prompt"));
    menu.addSeparator();
    QAction *addImageRef = menu.addAction(QString::fromUtf8("\xf0\x9f\x96\xbc  Add Image Reference"));
    QAction *addStyle    = menu.addAction(QString::fromUtf8("\xf0\x9f\x8e\xad  Add Style Preset"));
    menu.addSeparator();
    QAction *addJson     = menu.addAction(QString::fromUtf8("\xf0\x9f\x93\x8b  Add JSON Builder"));
    QAction *addGen      = menu.addAction(QString::fromUtf8("\xf0\x9f\x8e\xa8  Add Generation"));

    const QPointF spawnPos = event->scenePos();
    QAction *chosen = menu.exec(event->screenPos());

    NodeItem *newNode = nullptr;
    if      (chosen == addPrompt)   newNode = new TextPromptNode;
    else if (chosen == addImageRef) newNode = new ImageRefNode;
    else if (chosen == addStyle)    newNode = new StylePresetNode;
    else if (chosen == addJson)     newNode = new JsonPromptNode;
    else if (chosen == addGen)      newNode = new GenerationNode;

    if (newNode) {
        newNode->setPos(spawnPos);
        addItem(newNode);
    }

    event->accept();
}



void NodeScene::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        QGraphicsItem *focused = focusItem();
        if (focused && dynamic_cast<QGraphicsProxyWidget *>(focused)) {
            QGraphicsScene::keyPressEvent(event);
            return;
        }

        QList<NodeItem *> nodesToDelete;
        QList<ConnectionLine *> linesToDelete;

        for (auto *item : selectedItems()) {
            if (auto *node = dynamic_cast<NodeItem *>(item))
                nodesToDelete.append(node);
            else if (auto *line = dynamic_cast<ConnectionLine *>(item))
                linesToDelete.append(line);
        }

        for (auto *node : nodesToDelete)
            deleteNodeWithConnections(node);

        for (auto *line : linesToDelete)
            deleteConnection(line);

        event->accept();
        return;
    }

    QGraphicsScene::keyPressEvent(event);
}

void NodeScene::deleteNodeWithConnections(NodeItem *node)
{
    if (m_pendingGenNode == node)   m_pendingGenNode  = nullptr;
    if (m_pendingJsonNode == node)  m_pendingJsonNode = nullptr;

    auto conns = node->connections();
    for (auto *line : conns)
        deleteConnection(line);

    removeItem(node);
    delete node;
}

void NodeScene::deleteConnection(ConnectionLine *line)
{
    if (line->sourceNode())
        line->sourceNode()->removeConnection(line);
    if (line->targetNode())
        line->targetNode()->removeConnection(line);

    m_connections.removeAll(line);

    removeItem(line);
    delete line;
}



void NodeScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        const QPointF sp = event->scenePos();
        for (auto *item : items(sp)) {
            auto *node = dynamic_cast<NodeItem *>(item);
            if (!node) continue;

            int idx = node->pinIndexAt(sp);
            if (idx < 0) continue;

            const PinInfo &pin = node->pinAt(idx);
            if (!pin.isOutput) continue;

            m_dragSrc = node;
            m_dragPin = idx;

            m_dragLine = new ConnectionLine();
            m_dragLine->setSource(node, idx);
            m_dragLine->setDragEnd(sp);
            addItem(m_dragLine);

            event->accept();
            return;
        }
    }

    QGraphicsScene::mousePressEvent(event);
}

void NodeScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_dragLine) {
        m_dragLine->setDragEnd(event->scenePos());
        event->accept();
        return;
    }

    QGraphicsScene::mouseMoveEvent(event);
}

void NodeScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_dragLine) {
        const QPointF sp = event->scenePos();
        bool connected = false;

        for (auto *item : items(sp)) {
            auto *node = dynamic_cast<NodeItem *>(item);
            if (!node || node == m_dragSrc) continue;

            int idx = node->pinIndexAt(sp);
            if (idx < 0) continue;

            const PinInfo &targetPin = node->pinAt(idx);
            if (targetPin.isOutput) continue;

            const PinInfo &sourcePin = m_dragSrc->pinAt(m_dragPin);
            if (sourcePin.type != PinType::Any && targetPin.type != PinType::Any && sourcePin.type != targetPin.type) {
                continue;
            }

            if (!targetPin.multiInput && node->inputPinHasConnection(idx))
                continue;

            m_dragLine->setTarget(node, idx);
            m_connections.append(m_dragLine);
            connected = true;
            break;
        }

        if (!connected) {
            removeItem(m_dragLine);
            if (m_dragSrc)
                m_dragSrc->removeConnection(m_dragLine);
            delete m_dragLine;
        }

        m_dragLine = nullptr;
        m_dragSrc  = nullptr;
        m_dragPin  = -1;

        event->accept();
        return;
    }

    QGraphicsScene::mouseReleaseEvent(event);
}



void NodeScene::saveWorkflow(const QString &fileName)
{
    QJsonArray nodesArr;
    QMap<NodeItem*, int> nodeIds;

    int idCounter = 0;
    for (QGraphicsItem *itm : items()) {
        if (auto *node = dynamic_cast<NodeItem *>(itm)) {
            nodeIds[node] = idCounter;
            QJsonObject nObj;
            nObj["id"] = idCounter++;
            nObj["x"] = node->x();
            nObj["y"] = node->y();
            nObj["width"] = node->currentWidth();
            nObj["widgetHeight"] = node->currentWidgetHeight();

            if (auto *tp = dynamic_cast<TextPromptNode*>(node)) {
                nObj["type"] = "TextPrompt";
                nObj["promptText"] = tp->promptText();
            } else if (auto *ir = dynamic_cast<ImageRefNode*>(node)) {
                nObj["type"] = "ImageRef";
                nObj["imagePath"] = ir->imagePath();
                nObj["role"] = ir->role();
                nObj["instructions"] = ir->instructions();
            } else if (auto *sp = dynamic_cast<StylePresetNode*>(node)) {
                nObj["type"] = "StylePreset";
                nObj["presetIndex"] = sp->presetIndex();
                nObj["overrideText"] = sp->overrideText();
            } else if (dynamic_cast<JsonPromptNode*>(node)) {
                nObj["type"] = "JsonPrompt";
            } else if (auto *gn = dynamic_cast<GenerationNode*>(node)) {
                nObj["type"] = "Generation";
                nObj["workName"] = gn->workName();
                nObj["aspectRatio"] = gn->aspectRatio();
                nObj["resolution"] = gn->resolution();
            }
            nodesArr.append(nObj);
        }
    }

    QJsonArray connsArr;
    for (ConnectionLine *line : m_connections) {
        if (!line->sourceNode() || !line->targetNode()) continue;
        QJsonObject cObj;
        cObj["srcId"] = nodeIds[line->sourceNode()];
        cObj["srcPin"] = line->sourcePin();
        cObj["dstId"] = nodeIds[line->targetNode()];
        cObj["dstPin"] = line->targetPin();
        connsArr.append(cObj);
    }

    QJsonObject root;
    root["nodes"] = nodesArr;
    root["connections"] = connsArr;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) return;
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

void NodeScene::loadWorkflow(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) return;


    clear();
    m_connections.clear();
    m_pendingGenNode = nullptr;
    m_pendingJsonNode = nullptr;
    m_dragLine = nullptr;

    QJsonObject root = doc.object();
    QJsonArray nodesArr = root["nodes"].toArray();
    QMap<int, NodeItem*> idToNode;

    for (int i = 0; i < nodesArr.size(); ++i) {
        QJsonObject nObj = nodesArr[i].toObject();
        QString type = nObj["type"].toString();
        int id = nObj["id"].toInt();
        qreal x = nObj["x"].toDouble();
        qreal y = nObj["y"].toDouble();

        NodeItem *node = nullptr;
        if (type == "TextPrompt") {
            auto *tp = new TextPromptNode();
            tp->setPromptText(nObj["promptText"].toString());
            node = tp;
        } else if (type == "ImageRef") {
            auto *ir = new ImageRefNode();
            ir->setImagePath(nObj["imagePath"].toString());
            ir->setRole(nObj["role"].toString());
            ir->setInstructions(nObj["instructions"].toString());
            node = ir;
        } else if (type == "StylePreset") {
            auto *sp = new StylePresetNode();
            sp->setPresetIndex(nObj["presetIndex"].toInt());
            sp->setOverrideText(nObj["overrideText"].toString());
            node = sp;
        } else if (type == "JsonPrompt") {
            node = new JsonPromptNode();
        } else if (type == "Generation") {
            auto *gn = new GenerationNode();
            gn->setWorkName(nObj["workName"].toString());
            gn->setAspectRatio(nObj["aspectRatio"].toString());
            gn->setResolution(nObj["resolution"].toString());
            node = gn;
        }

        if (node) {
            node->setPos(x, y);
            if (nObj.contains("width") && nObj.contains("widgetHeight")) {
                node->resizeNode(nObj["width"].toDouble(), nObj["widgetHeight"].toDouble());
            }
            addItem(node);
            idToNode[id] = node;
        }
    }

    QJsonArray connsArr = root["connections"].toArray();
    for (int i = 0; i < connsArr.size(); ++i) {
        QJsonObject cObj = connsArr[i].toObject();
        NodeItem *srcNode = idToNode.value(cObj["srcId"].toInt());
        NodeItem *dstNode = idToNode.value(cObj["dstId"].toInt());
        if (srcNode && dstNode) {
            auto *line = new ConnectionLine();
            addItem(line);
            line->setSource(srcNode, cObj["srcPin"].toInt());
            line->setTarget(dstNode, cObj["dstPin"].toInt());
            m_connections.append(line);
        }
    }
}

