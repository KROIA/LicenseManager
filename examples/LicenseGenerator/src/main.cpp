#include <QApplication>
#include "LicenseManager.h"
#include "MainWindow.h"

int main(int argc, char* argv[])
{
	::ShowWindow(::GetConsoleWindow(), SW_HIDE);
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
	// Enable scaling for high resolution displays
	QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
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