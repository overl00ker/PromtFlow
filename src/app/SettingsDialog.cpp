#include "SettingsDialog.h"

#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSettings>

static const QString kSettingsKey = QStringLiteral("api/key");

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Settings");
    setMinimumWidth(420);

    setStyleSheet(
        "QDialog   { background: #2b2b2b; color: #ddd; }"
        "QLabel    { color: #ccc; font-size: 13px; }"
        "QLineEdit { background: #1e1e1e; color: #ccc; border: 1px solid #555;"
        "            border-radius: 4px; padding: 6px; font-size: 13px; }"
        "QPushButton { background: #3c78b5; color: white; border: none;"
        "              border-radius: 4px; padding: 6px 20px; font-size: 13px; font-weight: bold; }"
        "QPushButton:hover { background: #4a8ed0; }"
        "QPushButton#cancelBtn { background: #555; }"
        "QPushButton#cancelBtn:hover { background: #666; }"
    );

    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(12);

    layout->addWidget(new QLabel("Enter your API key:"));

    m_keyEdit = new QLineEdit;
    m_keyEdit->setEchoMode(QLineEdit::Password);
    m_keyEdit->setPlaceholderText("sk-...");
    m_keyEdit->setText(apiKey());
    layout->addWidget(m_keyEdit);

    auto *btnRow = new QHBoxLayout;
    btnRow->addStretch();

    auto *cancelBtn = new QPushButton("Cancel");
    cancelBtn->setObjectName("cancelBtn");
    btnRow->addWidget(cancelBtn);

    auto *saveBtn = new QPushButton("Save");
    btnRow->addWidget(saveBtn);

    layout->addLayout(btnRow);

    connect(saveBtn, &QPushButton::clicked, this, [this]() {
        setApiKey(m_keyEdit->text().trimmed());
        accept();
    });
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

QString SettingsDialog::apiKey()
{
    QSettings s;
    return s.value(kSettingsKey).toString();
}

void SettingsDialog::setApiKey(const QString &key)
{
    QSettings s;
    s.setValue(kSettingsKey, key);
}
