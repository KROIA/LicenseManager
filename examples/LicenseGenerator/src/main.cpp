#include <QApplication>
#include "LicenseManager.h"
#include "MainWindow.h"

int main(int argc, char* argv[])
{
	::ShowWindow(::GetConsoleWindow(), SW_HIDE);
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
	// Enable scaling for high resolution displays
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	// Both are deprecated no-ops on Qt 6: high-DPI scaling is always on.
	QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
	QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif
    QApplication app(argc, argv);

	
#ifndef NDEBUG
	Log::UI::createConsoleView(Log::UI::ConsoleViewType::nativeConsoleView);
#else
	
#endif
	MainWindow mainWindow;
	mainWindow.show();

	int ret = 0;
	ret = app.exec();
	return ret;
}