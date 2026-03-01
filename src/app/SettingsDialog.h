#pragma once

#include <QDialog>

class QLineEdit;

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);

    static QString apiKey();
    static void    setApiKey(const QString &key);

private:
    QLineEdit *m_keyEdit = nullptr;
};
