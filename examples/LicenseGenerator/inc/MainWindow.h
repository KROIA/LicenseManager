#pragma once

#include <QMainWindow>
#include "ui_MainWindow.h"
#include "LicenseManager.h"
#include "RibbonImpl.h"

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = nullptr);
	~MainWindow();

private:
	Ui::MainWindow ui;
	RibbonImpl* m_ribbon;
};
