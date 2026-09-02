#include "AvatarLoader.h"

#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>
#include <QPointer>
#include <QUrl>
#include <QUrlQuery>

namespace AvatarLoader
{
    void loadInto(QNetworkAccessManager &manager, const QString &seed, QLabel *label, int sizePx)
    {
        if (!label) {
            return;
        }
        label->setFixedSize(sizePx, sizePx);
        label->setStyleSheet(QStringLiteral("background: #e5e7eb; border-radius: 4px;"));

        QUrl url(QStringLiteral("https://api.dicebear.com/9.x/avataaars/png"));
        QUrlQuery query;
        query.addQueryItem(QStringLiteral("seed"), seed);
        query.addQueryItem(QStringLiteral("size"), QString::number(sizePx * 2));
        url.setQuery(query);

        QNetworkReply *reply = manager.get(QNetworkRequest(url));
        QPointer<QLabel> target(label);
        QObject::connect(reply, &QNetworkReply::finished, reply, [reply, target, sizePx]() {
            reply->deleteLater();
            if (!target) {
                return;
            }
            QPixmap pixmap;
            if (reply->error() == QNetworkReply::NoError && pixmap.loadFromData(reply->readAll())) {
                target->setPixmap(pixmap.scaled(
                    sizePx, sizePx, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }
        });
    }
}
