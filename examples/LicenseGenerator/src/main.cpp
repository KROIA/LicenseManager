#include <QApplication>
#include "LicenseManager.h"
#include "MainWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
	
#ifndef NDEBUG
	Log::UI::createConsoleView(Log::UI::ConsoleViewType::nativeConsoleView);
#endif
	MainWindow mainWindow;
	mainWindow.show();

	int ret = 0;
	ret = app.exec();
	return ret;
}