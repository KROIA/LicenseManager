#include "MainWindow.h"

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	m_ribbon = new RibbonImpl(ui.ribbon_widget);
}

MainWindow::~MainWindow()
{
	delete m_ribbon;
}
