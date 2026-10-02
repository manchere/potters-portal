#include "Portal.h"

#include <QApplication>
#include <QMessageBox>

#include "Database/Database.h"
#include "Style.h"
#include "Views/MainWindow.h"

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	applyTheme(savedTheme());

	const QString databaseUrl = qEnvironmentVariable("DATABASE_URL");
	QString connectError;
	if (databaseUrl.isEmpty()) {
		connectError = QStringLiteral(
			"DATABASE_URL is not set. Set it to a Postgres connection string "
			"(e.g. postgresql://user:password@host/dbname?sslmode=require) and restart.");
	} else if (!Database::connect(databaseUrl, &connectError)) {
		// connectError is filled in by Database::connect on failure.
	}

	if (!connectError.isEmpty()) {
		QMessageBox::warning(nullptr, QStringLiteral("Database Connection"), connectError);
	}

	// No login gate at startup -- the app is usable read-only right away.
	// Admin mode is unlocked from the title bar's lock icon (see
	// MainWindow::adminButtonClicked).
	MainWindow window;
	window.show();

	return app.exec();
}
