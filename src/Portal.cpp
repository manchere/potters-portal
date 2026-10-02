#include "Portal.h"

#include <QApplication>
#include <QCoreApplication>
#include <QMessageBox>

#include "Database/Database.h"
#include "Language.h"
#include "Style.h"
#include "Views/MainWindow.h"

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	installLanguage(savedLanguage());
	applyTheme(savedTheme());

	const QString databaseUrl = qEnvironmentVariable("DATABASE_URL");
	QString connectError;
	if (databaseUrl.isEmpty()) {
		connectError = QCoreApplication::translate("Portal",
			"DATABASE_URL is not set. Set it to a Postgres connection string "
			"(e.g. postgresql://user:password@host/dbname?sslmode=require) and restart.");
	} else if (!Database::connect(databaseUrl, &connectError)) {
		// connectError is filled in by Database::connect on failure.
	}

	if (!connectError.isEmpty()) {
		QMessageBox::warning(nullptr, QCoreApplication::translate("Portal", "Database Connection"), connectError);
	}

	// No login gate at startup -- the app is usable read-only right away.
	// Admin mode is unlocked from the title bar's lock icon (see
	// MainWindow::adminButtonClicked).
	MainWindow window;
	window.show();

	return app.exec();
}
