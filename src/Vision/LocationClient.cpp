#include "LocationClient.h"

#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

#include <chrono>

#include <winrt/Windows.Devices.Geolocation.h>
#include <winrt/Windows.Foundation.h>

namespace
{
    // Same blocking-on-a-local-event-loop trick as GroqVisionClient: this is
    // called from a synchronous "open the Add Item form" code path, so a
    // simple blocking GET is simpler than reworking the caller around
    // QFuture for one best-effort lookup.
    QByteArray getJsonSync(QNetworkAccessManager &manager, const QUrl &url, int timeoutMs)
    {
        QNetworkRequest request(url);
        request.setRawHeader("Accept", "application/json");
        // Nominatim's usage policy requires a descriptive User-Agent
        // identifying the application (anonymous/default Qt UA gets 403'd).
        request.setRawHeader("User-Agent", "PottersPortal/1.0 (church portal desktop app)");

        QNetworkReply *reply = manager.get(request);
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QTimer::singleShot(timeoutMs, &loop, &QEventLoop::quit);
        loop.exec();

        QByteArray data;
        if (reply->isFinished() && reply->error() == QNetworkReply::NoError) {
            data = reply->readAll();
        }
        reply->deleteLater();
        return data;
    }

    QString reverseGeocodeStreet(QNetworkAccessManager &manager, double latitude, double longitude)
    {
        QUrl reverseUrl(QStringLiteral("https://nominatim.openstreetmap.org/reverse"));
        QUrlQuery query;
        query.addQueryItem(QStringLiteral("format"), QStringLiteral("jsonv2"));
        query.addQueryItem(QStringLiteral("lat"), QString::number(latitude, 'f', 6));
        query.addQueryItem(QStringLiteral("lon"), QString::number(longitude, 'f', 6));
        reverseUrl.setQuery(query);

        const QByteArray reverseResponse = getJsonSync(manager, reverseUrl, 4000);
        if (reverseResponse.isEmpty()) {
            return QString();
        }
        const QJsonObject address = QJsonDocument::fromJson(reverseResponse).object()
            .value(QStringLiteral("address")).toObject();
        return address.value(QStringLiteral("road")).toString();
    }

    // The actual device location, via Windows Location Services (GPS / Wi-Fi
    // / cell positioning, whatever the OS has available) -- this is "where
    // this machine physically is right now", as opposed to an IP-address
    // guess or anything read out of a photo. Requires Location turned on in
    // Windows Settings > Privacy; if it's off, denied, or unavailable this
    // just returns false so the caller can fall back to IP geolocation.
    bool deviceCoordinates(double *outLatitude, double *outLongitude)
    {
        using namespace winrt::Windows::Devices::Geolocation;
        using namespace winrt::Windows::Foundation;

        try {
            winrt::init_apartment(winrt::apartment_type::multi_threaded);
        } catch (winrt::hresult_error const &) {
            // COM already initialized on this thread with a different
            // concurrency model (e.g. by a native Qt file dialog) -- the
            // existing apartment still works fine for WinRT calls.
        }

        try {
            IAsyncOperation<GeolocationAccessStatus> accessOp = Geolocator::RequestAccessAsync();
            if (accessOp.wait_for(std::chrono::milliseconds(5000)) != AsyncStatus::Completed) {
                return false;
            }
            if (accessOp.GetResults() != GeolocationAccessStatus::Allowed) {
                return false;
            }

            Geolocator locator;
            locator.DesiredAccuracy(PositionAccuracy::Default);
            IAsyncOperation<Geoposition> positionOp = locator.GetGeopositionAsync();
            if (positionOp.wait_for(std::chrono::milliseconds(8000)) != AsyncStatus::Completed) {
                positionOp.Cancel();
                return false;
            }

            const Geoposition position = positionOp.GetResults();
            const BasicGeoposition coordinates = position.Coordinate().Point().Position();
            *outLatitude = coordinates.Latitude;
            *outLongitude = coordinates.Longitude;
            return true;
        } catch (winrt::hresult_error const &) {
            return false;
        }
    }
}

namespace LocationLookup
{
    QString currentStreetName(QNetworkAccessManager &manager)
    {
        double latitude = 0.0;
        double longitude = 0.0;
        if (deviceCoordinates(&latitude, &longitude)) {
            const QString street = reverseGeocodeStreet(manager, latitude, longitude);
            if (!street.isEmpty()) {
                return street;
            }
        }

        // Windows Location Services unavailable/denied -- fall back to an
        // IP-based approximation of the device's location (still never
        // anything derived from a photo).
        const QByteArray geoResponse =
            getJsonSync(manager, QUrl(QStringLiteral("https://ipapi.co/json/")), 4000);
        if (geoResponse.isEmpty()) {
            return QString();
        }
        const QJsonObject geo = QJsonDocument::fromJson(geoResponse).object();
        if (!geo.contains(QStringLiteral("latitude")) || !geo.contains(QStringLiteral("longitude"))) {
            return QString();
        }
        return reverseGeocodeStreet(manager, geo.value(QStringLiteral("latitude")).toDouble(),
                                     geo.value(QStringLiteral("longitude")).toDouble());
    }
}
