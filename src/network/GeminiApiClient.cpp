#include "GeminiApiClient.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QRegularExpression>
#include <QtMath>

static const QString kJsonModelUrl =
    QStringLiteral("https://generativelanguage.googleapis.com/v1beta/models/gemini-3.1-pro-preview:generateContent?key=");

static const QString kImageModelUrl =
    QStringLiteral("https://generativelanguage.googleapis.com/v1beta/models/gemini-3.1-flash-image-preview:generateContent?key=");

static const QString kUploadUrl =
    QStringLiteral("https://generativelanguage.googleapis.com/upload/v1beta/files?key=");


static const QString kSystemPrompt =
    QStringLiteral(
        "You are an Elite Art Director and Prompt Engineer for a state-of-the-art "
        "AI image generator. Your goal is to synthesize user text and image "
        "references into ONE visually breathtaking, cohesive scene description.\n\n"

        "=== MEDIUM & AESTHETICS ('Anti-Plastic' Rule) ===\n"
        "1. PHOTOGRAPHY: Force realism â€” 'RAW photo, 85mm lens, ultra-detailed "
        "skin texture, visible pores, subsurface scattering, film grain'. "
        "NEVER output smooth/plastic/CGI/airbrushed looks.\n"
        "2. ANIME/2D: Force authentic 2D â€” 'flat anime cel shading, masterpiece "
        "illustration'. NEVER output 2.5D or uncanny 3D renders.\n"
        "3. NO STYLE SPECIFIED: Default to hyper-realistic cinematic photography.\n"
        "4. STYLE PRESET: If a style preset is provided, use its exact tokens as-is "
        "and integrate them into the 'style' field. Do not override preset tokens.\n\n"

        "=== REFERENCE IMAGE ROLES ===\n"
        "Each attached image has a 'role'. Strictly follow these rules:\n"
        "- Character: Extract ONLY the person's identity, face, body type, and "
        "features. Ignore their original background and pose.\n"
        "- Style: Extract ONLY the artistic style, color palette, mood, medium, "
        "and rendering technique. Ignore the subject.\n"
        "- Pose: Extract ONLY the body pose, gesture, and framing. Ignore identity.\n"
        "NEVER mix attributes across roles.\n"
        "If multiple Character refs exist, merge their best features unless "
        "Instructions say otherwise.\n\n"

        "=== INSTRUCTIONS (HIGHEST PRIORITY) ===\n"
        "Each reference may include user 'Instructions' (e.g., 'change hair to "
        "black'). These ALWAYS override original image features. Treat them as "
        "mandatory edits.\n\n"

        "=== INPUT MODES ===\n"
        "- Text + Images: Use text as primary guide, images as visual references.\n"
        "- Images only (no text): Analyze all references and infer the scene.\n"
        "- Text only: Expand the text into a detailed scene description.\n\n"

        "=== OUTPUT FORMAT ===\n"
        "Return ONLY valid JSON (no markdown fences, no commentary). "
        "Use exactly these keys, each filled with rich descriptive English:\n"
        "  subject         - Detailed physical description of the main subject.\n"
        "  apparel         - Clothing, fabrics, textures, accessories.\n"
        "  environment     - Background, setting, atmosphere, props.\n"
        "  lighting        - Cinematic lighting setup.\n"
        "  camera          - Angle, lens, depth of field.\n"
        "  style           - Art medium and aesthetic triggers.\n"
        "  negative_prompt - Anti-bad tags merged with any user-provided negative "
        "prompt. For photos: '3d, plastic, CGI, airbrushed, deformed'. "
        "For anime: '3d, photorealistic, bad anatomy'."
    );

GeminiApiClient::GeminiApiClient(QObject *parent)
    : QObject(parent)
    , m_manager(new QNetworkAccessManager(this))
{
}

QString GeminiApiClient::loadApiKey()
{
    QSettings s;
    return s.value(QStringLiteral("api/key")).toString().trimmed();
}

static QString guessMimeType(const QString &path)
{
    QString lower = path.toLower();
    if (lower.endsWith(".png"))  return QStringLiteral("image/png");
    if (lower.endsWith(".webp")) return QStringLiteral("image/webp");
    if (lower.endsWith(".bmp"))  return QStringLiteral("image/bmp");
    if (lower.endsWith(".gif"))  return QStringLiteral("image/gif");
    return QStringLiteral("image/jpeg");
}



void GeminiApiClient::requestFileUpload(const QString &filePath,
                                        const QString &role,
                                        const QString &instructions)
{
    QString apiKey = loadApiKey();
    if (apiKey.isEmpty()) {
        emit fileUploadError("API key is not set.");
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit fileUploadError("Cannot open file: " + filePath);
        return;
    }
    QByteArray fileData = file.readAll();
    file.close();

    QString mimeType = guessMimeType(filePath);
    QString displayName = QFileInfo(filePath).fileName();

    qDebug() << "[GeminiAPI] Uploading file:" << displayName
             << "size:" << fileData.size() << "mime:" << mimeType;

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::RelatedType);

    QHttpPart jsonPart;
    jsonPart.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=UTF-8");
    QJsonObject meta;
    QJsonObject fileObj;
    fileObj["displayName"] = displayName;
    meta["file"] = fileObj;
    jsonPart.setBody(QJsonDocument(meta).toJson(QJsonDocument::Compact));
    multiPart->append(jsonPart);

    QHttpPart dataPart;
    dataPart.setHeader(QNetworkRequest::ContentTypeHeader, mimeType);
    dataPart.setBody(fileData);
    multiPart->append(dataPart);

    QNetworkRequest request;
    request.setUrl(QUrl(kUploadUrl + apiKey));
    request.setRawHeader("X-Goog-Upload-Protocol", "multipart");

    QNetworkReply *reply = m_manager->post(request, multiPart);
    multiPart->setParent(reply);

    connect(reply, &QNetworkReply::finished, this,
            [this, reply, mimeType, role, instructions]() {
        reply->deleteLater();
        QByteArray responseBody = reply->readAll();

        if (reply->error() != QNetworkReply::NoError) {
            QString details = reply->errorString() + " | " + QString::fromUtf8(responseBody);
            qDebug() << "[GeminiAPI] Upload error:" << details;
            emit fileUploadError(details);
            return;
        }

        qDebug() << "[GeminiAPI] Upload response:" << QString::fromUtf8(responseBody).left(500);

        QJsonParseError parseErr;
        QJsonDocument doc = QJsonDocument::fromJson(responseBody, &parseErr);
        if (parseErr.error != QJsonParseError::NoError) {
            emit fileUploadError("Failed to parse upload response: " + parseErr.errorString());
            return;
        }

        QJsonObject root = doc.object();
        QJsonObject fileInfo = root["file"].toObject();
        QString fileUri = fileInfo["uri"].toString();

        if (fileUri.isEmpty()) {
            emit fileUploadError("No URI in upload response: " + QString::fromUtf8(responseBody).left(300));
            return;
        }

        qDebug() << "[GeminiAPI] File uploaded, URI:" << fileUri;
        emit fileUploaded(fileUri, mimeType, role, instructions);
    });
}



void GeminiApiClient::requestJsonPrompt(const QString &positive,
                                        const QString &negative,
                                        const QJsonArray &imageRefsMeta,
                                        const QVector<UploadedFileInfo> &uploadedFiles,
                                        const QString &stylePreset,
                                        const QString &negativePreset)
{
    QString apiKey = loadApiKey();
    if (apiKey.isEmpty()) {
        emit networkError("API key is not set. Go to File -> Settings to enter your key.");
        return;
    }

    bool hasText = !positive.isEmpty() || !negative.isEmpty();
    bool hasRefs = !uploadedFiles.isEmpty();


    QJsonArray userParts;


    for (int i = 0; i < uploadedFiles.size(); ++i) {
        const auto &f = uploadedFiles[i];


        QJsonObject fileDataInner;
        fileDataInner["fileUri"]  = f.fileUri;
        fileDataInner["mimeType"] = f.mimeType;

        QJsonObject fileDataPart;
        fileDataPart["fileData"] = fileDataInner;
        userParts.append(fileDataPart);


        QString desc = QStringLiteral("[Image %1 â€” role: %2]").arg(i + 1).arg(f.role);
        if (!f.instructions.isEmpty())
            desc += QStringLiteral(" Instructions: %1").arg(f.instructions);

        QJsonObject descPart;
        descPart["text"] = desc;
        userParts.append(descPart);
    }


    QString userText;
    if (hasText) {
        if (!positive.isEmpty())
            userText += QStringLiteral("Positive prompt: %1\n").arg(positive);
        if (!negative.isEmpty())
            userText += QStringLiteral("Negative prompt (avoid these): %1\n").arg(negative);
    } else if (hasRefs) {

        userText += QStringLiteral(
            "No text description was provided. "
            "Analyze the attached reference images above. "
            "Combine them into a single harmonious scene. "
            "Infer subject, apparel, environment, lighting, and camera from the images.\n");
    }


    if (!imageRefsMeta.isEmpty()) {
        userText += QStringLiteral("\nReference summary:\n");
        for (int i = 0; i < imageRefsMeta.size(); ++i) {
            QJsonObject ref = imageRefsMeta[i].toObject();
            QString instrText = ref["instructions"].toString();
            userText += QStringLiteral("  %1) role: %2%3\n")
                .arg(i + 1)
                .arg(ref["role"].toString(),
                     instrText.isEmpty() ? QString() : QStringLiteral(", instructions: %1").arg(instrText));
        }
    }


    if (!stylePreset.isEmpty()) {
        userText += QStringLiteral("\nStyle Preset (use these exact tokens in the 'style' field): %1\n").arg(stylePreset);
    }
    if (!negativePreset.isEmpty()) {
        userText += QStringLiteral("Preset negative tags (merge with negative_prompt): %1\n").arg(negativePreset);
    }

    QJsonObject textPart;
    textPart["text"] = userText;
    userParts.append(textPart);


    QJsonObject systemTextPart;
    systemTextPart["text"] = kSystemPrompt;

    QJsonObject systemInstruction;
    systemInstruction["parts"] = QJsonArray{ systemTextPart };


    QJsonObject userContent;
    userContent["role"] = QStringLiteral("user");
    userContent["parts"] = userParts;

    QJsonObject generationConfig;
    generationConfig["response_mime_type"] = QStringLiteral("application/json");

    QJsonObject body;
    body["system_instruction"] = systemInstruction;
    body["contents"] = QJsonArray{ userContent };
    body["generationConfig"] = generationConfig;

    QJsonDocument doc(body);
    QByteArray payload = doc.toJson(QJsonDocument::Compact);

    qDebug() << "[GeminiAPI] JSON prompt request with" << uploadedFiles.size()
             << "images, payload size:" << payload.size();

    QNetworkRequest request;
    request.setUrl(QUrl(kJsonModelUrl + apiKey));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = m_manager->post(request, payload);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onJsonReplyFinished(reply);
    });
}



void GeminiApiClient::requestImageGeneration(const QString &jsonPrompt,
                                             const QString &aspectRatio,
                                             const QString &resolution,
                                             const QVector<UploadedFileInfo> &files)
{
    QString apiKey = loadApiKey();
    if (apiKey.isEmpty()) {
        emit networkError("API key is not set. Go to File -> Settings to enter your key.");
        return;
    }

    QJsonArray parts;

    for (const auto &f : files) {
        QJsonObject fileDataInner;
        fileDataInner["fileUri"]  = f.fileUri;
        fileDataInner["mimeType"] = f.mimeType;

        QJsonObject fileDataPart;
        fileDataPart["fileData"] = fileDataInner;
        parts.append(fileDataPart);

        QString refDescription = QStringLiteral("[%1 reference]").arg(f.role);
        if (!f.instructions.isEmpty())
            refDescription += " Instructions: " + f.instructions;

        QJsonObject refTextPart;
        refTextPart["text"] = refDescription;
        parts.append(refTextPart);
    }

    QString mainPrompt = QStringLiteral(
        "Generate an image based on the following JSON parameters. "
        "Aspect Ratio: %1. Resolution: %2.\n\n%3")
        .arg(aspectRatio, resolution, jsonPrompt);

    QJsonObject mainTextPart;
    mainTextPart["text"] = mainPrompt;
    parts.append(mainTextPart);

    QJsonObject userContent;
    userContent["role"] = QStringLiteral("user");
    userContent["parts"] = parts;

    QJsonObject generationConfig;
    generationConfig["responseModalities"] = QJsonArray{ QStringLiteral("TEXT"), QStringLiteral("IMAGE") };

    // New imageConfig for Flash Image
    QJsonObject imageConfig;
    if (!aspectRatio.isEmpty() && aspectRatio != "Auto") {
        if (aspectRatio == "9:19.5") imageConfig["aspectRatio"] = "9:16"; // fallback nearest
        else if (aspectRatio == "19.5:9") imageConfig["aspectRatio"] = "16:9"; 
        else imageConfig["aspectRatio"] = aspectRatio;
    }
    
    if (resolution.contains("1024") || resolution.contains("1K") || resolution.contains("FHD") || resolution.contains("1920")) {
        imageConfig["imageSize"] = "1K";
    } else if (resolution.contains("2048") || resolution.contains("2K") || resolution.contains("QHD") || resolution.contains("2560")) {
        imageConfig["imageSize"] = "2K";
    } else if (resolution.contains("4096") || resolution.contains("4K") || resolution.contains("UHD") || resolution.contains("3840")) {
        imageConfig["imageSize"] = "4K";
    } else if (resolution.contains("512")) {
        imageConfig["imageSize"] = "0.5K";
    } else {
        imageConfig["imageSize"] = "1K"; // Default fallback
    }

    if (!imageConfig.isEmpty()) {
        generationConfig["imageConfig"] = imageConfig;
    }

    QJsonObject body;
    body["contents"] = QJsonArray{ userContent };
    body["generationConfig"] = generationConfig;

    QJsonDocument doc(body);
    QByteArray payload = doc.toJson(QJsonDocument::Compact);

    qDebug() << "[GeminiAPI] Image generation request with" << files.size()
             << "file refs, payload size:" << payload.size();

    QNetworkRequest request;
    request.setUrl(QUrl(kImageModelUrl + apiKey));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = m_manager->post(request, payload);
    connect(reply, &QNetworkReply::finished, this, [this, reply, aspectRatio, resolution]() {
        onImageReplyFinished(reply, aspectRatio, resolution);
    });
}



void GeminiApiClient::onJsonReplyFinished(QNetworkReply *reply)
{
    reply->deleteLater();
    QByteArray responseBody = reply->readAll();

    if (reply->error() != QNetworkReply::NoError) {
        QString details = reply->errorString() + " | " + QString::fromUtf8(responseBody);
        qDebug() << "[GeminiAPI] JSON request error:" << details;
        emit networkError(details);
        return;
    }

    QJsonParseError parseErr;
    QJsonDocument responseDoc = QJsonDocument::fromJson(responseBody, &parseErr);
    if (parseErr.error != QJsonParseError::NoError) {
        emit networkError("Failed to parse API response: " + parseErr.errorString());
        return;
    }

    QJsonObject root = responseDoc.object();
    QJsonArray candidates = root["candidates"].toArray();
    if (candidates.isEmpty()) {
        QString reason = root.contains("error")
            ? root["error"].toObject()["message"].toString()
            : "No candidates in response";
        emit networkError("API returned no candidates: " + reason);
        return;
    }

    QJsonObject firstCandidate = candidates[0].toObject();
    QJsonObject content = firstCandidate["content"].toObject();
    QJsonArray parts = content["parts"].toArray();
    if (parts.isEmpty()) {
        emit networkError("API response has no parts in candidate.");
        return;
    }

    QString generatedText = parts[0].toObject()["text"].toString();
    if (generatedText.isEmpty()) {
        emit networkError("API returned empty text in response.");
        return;
    }

    QJsonDocument innerDoc = QJsonDocument::fromJson(generatedText.toUtf8(), &parseErr);
    if (parseErr.error != QJsonParseError::NoError) {
        QJsonObject fallback;
        fallback["raw_response"] = generatedText;
        emit jsonGenerated(fallback);
        return;
    }

    emit jsonGenerated(innerDoc.object());
}



void GeminiApiClient::onImageReplyFinished(QNetworkReply *reply, const QString &aspectRatio, const QString &resolution)
{
    reply->deleteLater();
    QByteArray responseBody = reply->readAll();

    if (reply->error() != QNetworkReply::NoError) {
        QString details = reply->errorString() + " | " + QString::fromUtf8(responseBody);
        qDebug() << "[GeminiAPI] Image request error:" << details;
        emit networkError(details);
        return;
    }

    QJsonParseError parseErr;
    QJsonDocument responseDoc = QJsonDocument::fromJson(responseBody, &parseErr);
    if (parseErr.error != QJsonParseError::NoError) {
        emit networkError("Failed to parse image API response: " + parseErr.errorString());
        return;
    }

    QJsonObject root = responseDoc.object();
    QJsonArray candidates = root["candidates"].toArray();
    if (candidates.isEmpty()) {
        QString reason = root.contains("error")
            ? root["error"].toObject()["message"].toString()
            : "No candidates in response";
        emit networkError("Image API returned no candidates: " + reason);
        return;
    }

    QJsonObject firstCandidate = candidates[0].toObject();
    QJsonObject content = firstCandidate["content"].toObject();
    QJsonArray parts = content["parts"].toArray();


    QString thoughts;
    QPixmap resultPixmap;
    bool foundImage = false;

    for (int i = 0; i < parts.size(); ++i) {
        QJsonObject part = parts[i].toObject();

        if (part.contains("text")) {
            QString text = part["text"].toString();
            if (!thoughts.isEmpty())
                thoughts += "\n\n";
            thoughts += text;
        }

        if (part.contains("inlineData")) {
            QJsonObject inlineData = part["inlineData"].toObject();
            QString base64Data = inlineData["data"].toString();
            QString mimeType   = inlineData["mimeType"].toString();

            qDebug() << "[GeminiAPI] Found inlineData, mimeType:" << mimeType
                     << ", base64 size:" << base64Data.size();

            QByteArray imageBytes = QByteArray::fromBase64(base64Data.toUtf8());
            if (resultPixmap.loadFromData(imageBytes)) {
                qDebug() << "[GeminiAPI] Image decoded:" << resultPixmap.width() << "x" << resultPixmap.height();
                foundImage = true;
            }
        }
    }


    if (!thoughts.isEmpty()) {
        qDebug() << "[GeminiAPI] Model thoughts:" << thoughts.left(200);
        emit thoughtsGenerated(thoughts);
    }

    if (foundImage && !resultPixmap.isNull()) {
        int targetW = 1024, targetH = 1024;
        
        QRegularExpression rxRes("(\\d+)x(\\d+)");
        QRegularExpressionMatch matchRes = rxRes.match(resolution);
        int baseLongEdge = 1024;
        if (matchRes.hasMatch()) {
            baseLongEdge = qMax(matchRes.captured(1).toInt(), matchRes.captured(2).toInt());
        } else {
            if (resolution.contains("1024") || resolution.contains("FHD") || resolution.contains("1920")) baseLongEdge = 1024;
            else if (resolution.contains("2048") || resolution.contains("QHD") || resolution.contains("2560")) baseLongEdge = 2048;
            else if (resolution.contains("4096") || resolution.contains("UHD") || resolution.contains("3840")) baseLongEdge = 4096;
        }

        double ar = 1.0;
        QRegularExpression rxAr("([\\d\\.]+):([\\d\\.]+)");
        QRegularExpressionMatch matchAr = rxAr.match(aspectRatio);
        if (matchAr.hasMatch()) {
            double wRatio = matchAr.captured(1).toDouble();
            double hRatio = matchAr.captured(2).toDouble();
            if (hRatio > 0.0001) ar = wRatio / hRatio;
        }

        if (ar >= 1.0) {
            targetW = baseLongEdge;
            targetH = qRound(targetW / ar);
        } else {
            targetH = baseLongEdge;
            targetW = qRound(targetH * ar);
        }

        QPixmap scaledPix = resultPixmap.scaled(targetW, targetH, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        
        int x = (scaledPix.width() - targetW) / 2;
        int y = (scaledPix.height() - targetH) / 2;
        resultPixmap = scaledPix.copy(x, y, targetW, targetH);
        
        emit imageGenerated(resultPixmap);
    } else if (!thoughts.isEmpty()) {
        emit networkError("Model returned text instead of image:\n" + thoughts.left(500));
    } else {
        emit networkError("No image data found in API response.");
    }
}

