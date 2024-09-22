#pragma once

#include <QMainWindow>
#include "ui_MainWindow.h"
#include "LicenseManager.h"
#include "RibbonImpl.h"
#include "ProjectEditor.h"
#include "Project.h"

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = nullptr);
	~MainWindow();

	// override close event
	void closeEvent(QCloseEvent* event) override;

	void loadLastProject();
	void loadProject(const std::string& path);
	void setProject(const Project& project);

private slots:
	void onProjectEditorSaveProject();
private:
	Ui::MainWindow ui;
	RibbonImpl* m_ribbon;

	ProjectEditor* m_projectEditor;

	Log::LogObject m_log;
	Project m_project;
};
