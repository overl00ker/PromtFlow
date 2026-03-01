#pragma once

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QPixmap>
#include <QVector>

class QNetworkAccessManager;
class QNetworkReply;

struct UploadedFileInfo
{
    QString fileUri;
    QString mimeType;
    QString role;
    QString instructions;
};

class GeminiApiClient : public QObject
{
    Q_OBJECT

public:
    explicit GeminiApiClient(QObject *parent = nullptr);

    void requestFileUpload(const QString &filePath,
                           const QString &role,
                           const QString &instructions);


    void requestJsonPrompt(const QString &positive,
                           const QString &negative,
                           const QJsonArray &imageRefsMeta,
                           const QVector<UploadedFileInfo> &uploadedFiles,
                           const QString &stylePreset = QString(),
                           const QString &negativePreset = QString());


    void requestImageGeneration(const QString &jsonPrompt,
                                const QString &aspectRatio,
                                const QString &resolution,
                                const QVector<UploadedFileInfo> &files);

signals:
    void fileUploaded(QString fileUri, QString mimeType, QString role, QString instructions);
    void fileUploadError(QString errorDetails);
    void jsonGenerated(QJsonObject result);
    void thoughtsGenerated(QString thoughts);
    void imageGenerated(QPixmap image);
    void networkError(QString errorDetails);

private:
    void onJsonReplyFinished(QNetworkReply *reply);
    void onImageReplyFinished(QNetworkReply *reply, const QString &aspectRatio, const QString &resolution);
    QString loadApiKey();

    QNetworkAccessManager *m_manager = nullptr;
};

