#include "MessageDialog.h"

#include <QAbstractButton>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace
{
    QString kindGlyph(MessageDialog::Kind kind)
    {
        switch (kind) {
        case MessageDialog::Kind::Question: return QStringLiteral("❓");          // ❓
        case MessageDialog::Kind::Warning: return QStringLiteral("⚠️");     // ⚠️
        case MessageDialog::Kind::Critical: return QStringLiteral("⛔");          // ⛔
        case MessageDialog::Kind::Information: break;
        }
        return QStringLiteral("ℹ️");                                         // ℹ️
    }
}

MessageDialog::MessageDialog(Kind kind, const QString &title, const QString &text, QWidget *parent)
    : FramelessDialog(parent)
{
    setWindowTitle(title);

    auto *icon = new QLabel(kindGlyph(kind), this);
    icon->setStyleSheet(QStringLiteral("font-size: 22pt;"));
    icon->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    auto *heading = new QLabel(title, this);
    heading->setObjectName(QStringLiteral("pageTitle"));
    heading->setStyleSheet(QStringLiteral("font-size: 13pt;"));
    heading->setWordWrap(true);

    auto *message = new QLabel(text, this);
    message->setWordWrap(true);
    message->setTextInteractionFlags(Qt::TextSelectableByMouse);

    m_informative = new QLabel(this);
    m_informative->setObjectName(QStringLiteral("mutedLabel"));
    m_informative->setWordWrap(true);
    m_informative->hide();

    auto *textColumn = new QVBoxLayout;
    textColumn->setSpacing(6);
    textColumn->addWidget(heading);
    textColumn->addWidget(message);
    textColumn->addWidget(m_informative);

    auto *body = new QHBoxLayout;
    body->setSpacing(14);
    body->addWidget(icon, 0, Qt::AlignTop);
    body->addLayout(textColumn, 1);

    m_buttons = new QDialogButtonBox(this);
    connect(m_buttons, &QDialogButtonBox::clicked, this, [this](QAbstractButton *button) {
        m_clicked = button;
        const QDialogButtonBox::ButtonRole role = m_buttons->buttonRole(button);
        if (role == QDialogButtonBox::RejectRole || role == QDialogButtonBox::NoRole) {
            reject();
        } else {
            accept();
        }
    });

    auto *layout = contentLayout();
    layout->setSpacing(14);
    layout->addLayout(body);
    layout->addWidget(m_buttons);
    setMinimumWidth(380);
    setMaximumWidth(560);
}

void MessageDialog::setInformativeText(const QString &text)
{
    m_informative->setText(text);
    m_informative->setVisible(!text.isEmpty());
}

void MessageDialog::styleButton(QPushButton *button, QDialogButtonBox::ButtonRole role)
{
    // Primary (the stylesheet's plain QPushButton) for going ahead, red for
    // destructive, the secondary look for everything else.
    if (role == QDialogButtonBox::DestructiveRole) {
        button->setObjectName(QStringLiteral("dangerButton"));
    } else if (role != QDialogButtonBox::AcceptRole && role != QDialogButtonBox::YesRole) {
        button->setObjectName(QStringLiteral("secondaryButton"));
    }
    if (!m_hasDefault) {
        setDefaultButton(button);
    }
}

QPushButton *MessageDialog::addButton(const QString &text, QDialogButtonBox::ButtonRole role)
{
    QPushButton *button = m_buttons->addButton(text, role);
    styleButton(button, role);
    return button;
}

QPushButton *MessageDialog::addButton(QDialogButtonBox::StandardButton standard)
{
    QPushButton *button = m_buttons->addButton(standard);
    styleButton(button, m_buttons->buttonRole(button));
    return button;
}

void MessageDialog::setDefaultButton(QPushButton *button)
{
    m_hasDefault = true;
    button->setDefault(true);
    button->setFocus();
}

QMessageBox::StandardButton MessageDialog::show(Kind kind, QWidget *parent, const QString &title,
                                                const QString &text, QMessageBox::StandardButtons buttons)
{
    MessageDialog dialog(kind, title, text, parent);
    // QMessageBox and QDialogButtonBox share the same StandardButton values.
    QPushButton *escapeButton = nullptr;
    for (QMessageBox::StandardButton standard :
         {QMessageBox::Yes, QMessageBox::Ok, QMessageBox::No, QMessageBox::Cancel, QMessageBox::Close}) {
        if (buttons & standard) {
            QPushButton *button = dialog.addButton(static_cast<QDialogButtonBox::StandardButton>(standard));
            if (standard == QMessageBox::No || standard == QMessageBox::Cancel || standard == QMessageBox::Close) {
                escapeButton = button;
            }
        }
    }
    dialog.exec();
    if (!dialog.clickedButton()) {
        // Escape (or closed): whatever means "no", else the only button.
        if (escapeButton) {
            return static_cast<QMessageBox::StandardButton>(dialog.m_buttons->standardButton(escapeButton));
        }
        return buttons & QMessageBox::Ok ? QMessageBox::Ok : QMessageBox::NoButton;
    }
    return static_cast<QMessageBox::StandardButton>(dialog.m_buttons->standardButton(dialog.clickedButton()));
}

QMessageBox::StandardButton MessageDialog::question(QWidget *parent, const QString &title, const QString &text,
                                                    QMessageBox::StandardButtons buttons)
{
    return show(Kind::Question, parent, title, text, buttons);
}

QMessageBox::StandardButton MessageDialog::information(QWidget *parent, const QString &title, const QString &text)
{
    return show(Kind::Information, parent, title, text, QMessageBox::Ok);
}

QMessageBox::StandardButton MessageDialog::warning(QWidget *parent, const QString &title, const QString &text)
{
    return show(Kind::Warning, parent, title, text, QMessageBox::Ok);
}

QMessageBox::StandardButton MessageDialog::critical(QWidget *parent, const QString &title, const QString &text)
{
    return show(Kind::Critical, parent, title, text, QMessageBox::Ok);
}
