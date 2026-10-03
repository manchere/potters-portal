#include "Portal.h"

#include <QApplication>
#include <QCoreApplication>
#include <QSettings>

#include "Database/Database.h"
#include "Language.h"
#include "Style.h"
#include "Views/DatabaseSetupDialog.h"
#include "Views/MainWindow.h"

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	installLanguage(savedLanguage());
	applyTheme(savedTheme());

	// Where the database is: DATABASE_URL if set (developers, servers),
	// otherwise the connection saved on this computer. If neither works --
	// a fresh install, or the saved one stopped working -- ask for it, and
	// save one that connects (installed copies never carry it).
	QSettings settings(QStringLiteral("PottersPortal"), QStringLiteral("PottersPortal"));
	const QString envUrl = qEnvironmentVariable("DATABASE_URL");
	const QString savedUrl = settings.value(QStringLiteral("databaseUrl")).toString();
	QString databaseUrl = envUrl.isEmpty() ? savedUrl : envUrl;
	QString connectError;
	bool connected = !databaseUrl.isEmpty() && Database::connect(databaseUrl, &connectError);
	while (!connected) {
		DatabaseSetupDialog dialog(connectError, databaseUrl);
		if (dialog.exec() != QDialog::Accepted) {
			return 0; // Quit
		}
		databaseUrl = dialog.connectionUrl();
		connected = true; // the dialog only accepts once it has connected
		if (envUrl.isEmpty() || databaseUrl != envUrl) {
			settings.setValue(QStringLiteral("databaseUrl"), databaseUrl);
		}
	}

	// No login gate at startup -- the app is usable read-only right away.
	// Admin mode is unlocked from the title bar's lock icon (see
	// MainWindow::adminButtonClicked).
	MainWindow window;
	window.show();

	return app.exec();
}
