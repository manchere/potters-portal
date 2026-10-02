#pragma once

#include <QString>

class QNetworkAccessManager;

// Best-effort "where is this machine" lookup for pre-filling the Add Item
// location field: IP-based geolocation for an approximate lat/lon, then
// reverse-geocoded to a street name. Desktop-only (not linked into
// PottersPortalServer) since it describes where the person adding the
// item physically is, not anything server-side.
namespace LocationLookup
{
    // Returns a street name (e.g. "5th Avenue"), or an empty string if it
    // could not be determined (offline, API down, no street on the reverse
    // geocode result, etc.) -- callers should treat that as "leave the
    // field blank", not an error to surface to the user.
    QString currentStreetName(QNetworkAccessManager &manager);
}
