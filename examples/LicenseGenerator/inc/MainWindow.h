#pragma once

#include <QMainWindow>
#include "ui_MainWindow.h"
#include "LicenseManager.h"
#include "RibbonImpl.h"
#include "ProjectEditor.h"
#include "Project.h"
#include "LicenseEditor.h"
#include <memory>
#include <QStandardItemModel>

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
	void setProject(std::shared_ptr<Project> project);
	bool addProject(std::shared_ptr<Project> project);

private slots:
	void onProjectEditorSaveProject();
	void onLicenseEditorLicenseNameChanged();

	void onNewLicense();
	void onSaveLicense();

	// Slot for on click event of the license list view
	void onLicenseListViewClicked(const QModelIndex &index);

	// Slot for the Version action in the ribbon
	void onVersion();
	// Slot for the About action in the ribbon
	void onAbout();
	

private:
	void saveProject(std::shared_ptr<Project> project);
	void populateLicenseListView(std::shared_ptr<LicenseManager::License> select = nullptr);
	void manageLicenseColor();

	// Function to change the color of a License in the list view
	void setLicenseColor(std::shared_ptr<LicenseManager::License> lic, const QColor& color);

	Ui::MainWindow ui;
	RibbonImpl* m_ribbon;

	ProjectEditor* m_projectEditor;
	LicenseEditor* m_licenseEditor;

	Log::LogObject m_log;
	std::vector<std::shared_ptr<Project>> m_projects;
	std::shared_ptr<Project> m_currentProject;

	struct Colors
	{
		static QColor normal;
		//static QColor selected;
		static QColor error;
	};

	static const LicenseManager::LibraryInfo::Version m_appVersion;
};

