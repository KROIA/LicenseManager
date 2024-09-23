#include "MainWindow.h"
#include <QCloseEvent>

#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include "AppSettingsImpl.h"

Q_DECLARE_SMART_POINTER_METATYPE(std::shared_ptr);
Q_DECLARE_METATYPE(std::shared_ptr<LicenseManager::License>);

const LicenseManager::LibraryInfo::Version MainWindow::m_appVersion{ 1, 0, 0 };

QColor MainWindow::Colors::normal = QColor(255, 255, 255);
//QColor MainWindow::Colors::selected = QColor(200, 200, 200);
QColor MainWindow::Colors::error = QColor(255, 0, 0);



MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
{
	m_currentProject = nullptr;
	ui.setupUi(this);
	m_ribbon = new RibbonImpl(ui.ribbon_widget);
	setWindowTitle("License Generator");
	m_log.setName(windowTitle().toStdString());

	m_projectEditor = new ProjectEditor(m_log.getID());
	m_projectEditor->hide();

	m_licenseEditor = new LicenseEditor(m_log.getID());
	Project::log().setParentID(m_log.getID());
	AppSettings::Logger::setParentID(m_log.getID());
	ui.licenseEditor_frame->layout()->addWidget(m_licenseEditor);
	connect(m_licenseEditor, &LicenseEditor::licenseNameChanged, this, &MainWindow::onLicenseEditorLicenseNameChanged);

	RibbonImpl::ProjectButtons& projectButtons = m_ribbon->projectButtons();
	RibbonImpl::LicenseButtons& licenseButtons = m_ribbon->licenseButtons();
	connect(projectButtons.createProject, &RibbonWidget::RibbonButton::clicked, [this] {
		// Create new project
		m_projectEditor->clear();
		m_projectEditor->show();
	});
	connect(projectButtons.editProject, &RibbonWidget::RibbonButton::clicked, [this] {
		// Edit project
		m_projectEditor->setProject(m_currentProject);
		m_projectEditor->show();
	});
	connect(projectButtons.loadProject, &RibbonWidget::RibbonButton::clicked, [this] {
		// Load project
		QString filePath = QFileDialog::getOpenFileName(this, "Open Project", 
			AppSettingsImpl::getProjectGroup().projectsPath.getValue().toString(), "Project files (*.json)");

		// Rempve the json filename from the path
		filePath = filePath.left(filePath.lastIndexOf("/"));

		if(filePath.size() > 0)
			loadProject(filePath.toStdString());
	});

	connect(licenseButtons.createNewLicense, &RibbonWidget::RibbonButton::clicked, this, &MainWindow::onNewLicense);
	connect(licenseButtons.saveLicense, &RibbonWidget::RibbonButton::clicked, this, &MainWindow::onSaveLicense);

	connect(m_projectEditor, &ProjectEditor::saveProject, this, &MainWindow::onProjectEditorSaveProject);


	connect(ui.actionVersion, &QAction::triggered, this, &MainWindow::onVersion);
	connect(ui.actionAbout, &QAction::triggered, this, &MainWindow::onAbout);


	AppSettingsImpl::load();
	if (AppSettingsImpl::getGeneralGroup().showConsole.getValue().toBool())
	{
		// Use the QTreeConsoleView
		//Log::UI::createConsoleView(Log::UI::ConsoleViewType::qTreeConsoleView);
		//Log::UI::getConsoleView<Log::UI::QTreeConsoleView>()->show();

		// Use the QConsoleView
		Log::UI::createConsoleView(Log::UI::ConsoleViewType::qConsoleView);
		Log::UI::getConsoleView<Log::UI::QConsoleView>()->show();
	}
	QDir().mkpath(AppSettingsImpl::getProjectGroup().projectsPath.getValue().toString());



	loadLastProject();
}

MainWindow::~MainWindow()
{
	AppSettingsImpl::save();
	m_projectEditor->hide();
	m_projectEditor->deleteLater();	
	delete m_ribbon;

}

void MainWindow::closeEvent(QCloseEvent* event)
{
	m_projectEditor->hide();
	event->accept();
	Log::UI::destroyAllConsoleViews();
}


void MainWindow::loadLastProject()
{
	std::string filePath = AppSettingsImpl::getProjectGroup().lastLoadedProject.getValue().toString().toStdString();
	if(filePath.size() > 0)
		loadProject(filePath);
}

void MainWindow::loadProject(const std::string& path)
{
	// Load project from file
	std::shared_ptr<Project> project = std::make_shared<Project>();
	if (project->load(path))
	{
		m_log.logInfo("Project loaded from file: "+path);
		if(addProject(project))
			setProject(project);
		else
		{
			for (size_t i = 0; i < m_projects.size(); ++i)
			{
				if (m_projects[i]->getName() == project->getName())
				{
					setProject(m_projects[i]);
					break;
				}
			}
		}
	}
	else
	{
		m_log.logError("Can't load project from file: "+path);
	}
}
void MainWindow::setProject(std::shared_ptr<Project> project)
{
	if (m_currentProject)
	{
		// Save the current project
		saveProject(m_currentProject);
		
	}
	m_currentProject = project;
	populateLicenseListView();

	if (m_currentProject)
	{
		std::string dir = AppSettingsImpl::getProjectGroup().projectsPath.getValue().toString().toStdString();
		AppSettingsImpl::getProjectGroup().lastLoadedProject.setValue(QString::fromStdString(m_currentProject->getProjectFilePath(dir)));
		AppSettingsImpl::save();
		ui.currentProject_label->setText(QString::fromStdString(m_currentProject->getName()));
	}
	else
		ui.currentProject_label->setText("none");

	
	//m_projectEditor->setProject(project);

}
bool MainWindow::addProject(std::shared_ptr<Project> project)
{
	// Add project to the list of projects
	for(size_t i=0; i<m_projects.size(); ++i)
	{
		if(m_projects[i] == project)
			return false;
		if(m_projects[i]->getName() == project->getName())
		{
			m_projects[i] = project;
			return false;
		}
	}
	m_projects.push_back(project);
	return true;
}

void MainWindow::onVersion()
{
	// Show the version of the application
	QString version = windowTitle() + " v"+ m_appVersion.toString().c_str() + "\n";
	version += LicenseManager::LibraryInfo::getInfoStr().c_str();
	m_log.logInfo(version.toStdString());

	QMessageBox::information(this, "Version", version);
}
void MainWindow::onAbout()
{
	// Write a about message
	QString about = "License Generator\n";
	about += "This application is used to generate licenses for your software.\n";
	about += "You can create projects with different parameters and generate licenses for them.\n";
	about += "The licenses can be saved and loaded from files.\n";
	about += "The application uses the LicenseManager library to generate and verify licenses.\n";
	about += "The library is open source and can be found on GitHub.\n";
	about += "For more information visit: https://github.com/KROIA/LicenseManager";
	m_log.logInfo(about.toStdString());

	QMessageBox::information(this, "About", about);
}

void MainWindow::saveProject(std::shared_ptr<Project> project)
{
	std::string dir = AppSettingsImpl::getProjectGroup().projectsPath.getValue().toString().toStdString();
	if (m_currentProject->save(dir))
	{
		m_log.logInfo("Project saved to: " + dir);
		manageLicenseColor();
	}
	else
	{
		m_log.logError("Can't save project to: " + dir);
	}
}

void MainWindow::onProjectEditorSaveProject()
{
	std::shared_ptr<Project> project = m_projectEditor->project();
	if (project != m_currentProject)
	{
		if (addProject(project))
		{
			setProject(project);
		}
		else
		{
			m_log.logError("Project with the same name already exists");
			return;
		}
	}

	// Get the current selected License
	std::shared_ptr<LicenseManager::License> license = m_licenseEditor->getLicense();

	// reset the license to the editor to update for new license entries
	m_licenseEditor->setLicense(m_currentProject, license);




	saveProject(project);
}

void MainWindow::onLicenseEditorLicenseNameChanged()
{
	// Update the license name in the list view
	QStandardItemModel* model = dynamic_cast<QStandardItemModel*>(ui.licenses_listView->model());
	if (!model)
		return;

	QModelIndexList selected = ui.licenses_listView->selectionModel()->selectedIndexes();
	if (selected.size() == 0)
		return;

	QStandardItem* item = model->itemFromIndex(selected[0]);

	if (!item)
		return;

	std::shared_ptr<LicenseManager::License> license = item->data().value<std::shared_ptr<LicenseManager::License>>();
	if (!license)
		return;

	item->setText(license->getName().c_str());
}
void MainWindow::onNewLicense()
{
	// Create new license
	std::shared_ptr<LicenseManager::License> license = std::make_shared<LicenseManager::License>();
	m_currentProject->addLicense(license);
	populateLicenseListView(license);
	m_licenseEditor->setLicense(m_currentProject, license);
	
	
}
void MainWindow::onSaveLicense()
{
	// Save the license
	std::shared_ptr<LicenseManager::License> license = m_licenseEditor->getLicense();
	if (!license)
		return;

	m_currentProject->addLicense(license);
	saveProject(m_currentProject);
}

void MainWindow::onLicenseListViewClicked(const QModelIndex& index)
{
	// Select the license which gets clicked
	QStandardItemModel* model = dynamic_cast<QStandardItemModel*>(ui.licenses_listView->model());
	if (!model)
		return;

	QStandardItem* item = model->itemFromIndex(index);
	if (!item)
		return;

	std::shared_ptr<LicenseManager::License> license = item->data().value<std::shared_ptr<LicenseManager::License>>();
	if (!license)
		return;

	m_licenseEditor->setLicense(m_currentProject, license);
	//setLicenseColor(license, Colors::selected);

}

void MainWindow::populateLicenseListView(std::shared_ptr<LicenseManager::License> select)
{
	if (!m_currentProject)
	{
		QStandardItemModel* model = new QStandardItemModel();
		ui.licenses_listView->setModel(model);
		return;
	}
	// Create a model for the list view
	QStandardItemModel* model = new QStandardItemModel();
	model->setColumnCount(2);

	// populate the model with licenses
	const std::vector<std::shared_ptr<LicenseManager::License>>& licenses = m_currentProject->getLicenses();
	for (size_t i = 0; i < licenses.size(); ++i)
	{
		// Create a row for each license and add the shared ptr to the license as data
		QStandardItem* name = new QStandardItem(QString::fromStdString(licenses[i]->getName()));
		name->setData(QVariant::fromValue(licenses[i]));
		model->appendRow(name);
	}

	// Set the model to the list view
	ui.licenses_listView->setModel(model);
	manageLicenseColor();

	// Set slot for on click event
	connect(ui.licenses_listView, &QListView::clicked, this, &MainWindow::onLicenseListViewClicked);

	if(select)
	{
		// Select the license
		for (int i = 0; i < model->rowCount(); ++i)
		{
			QStandardItem* item = model->item(i);
			std::shared_ptr<LicenseManager::License> license = item->data().value<std::shared_ptr<LicenseManager::License>>();
			if (license == select)
			{
				ui.licenses_listView->setCurrentIndex(model->index(i, 0));
				onLicenseListViewClicked(model->index(i, 0));
				return;
			}
		}
	}
	else
	{
		// Select first element if it exists
		if (model->rowCount() > 0)
		{
			ui.licenses_listView->setCurrentIndex(model->index(0, 0));
			onLicenseListViewClicked(model->index(0, 0));
		}
		else
		{
			m_licenseEditor->setLicense(m_currentProject ,nullptr);
		}
	}
}
void MainWindow::manageLicenseColor()
{
	// Set the color of the license in the list view
	QStandardItemModel* model = dynamic_cast<QStandardItemModel*>(ui.licenses_listView->model());
	if (!model)
		return;

	for (int i = 0; i < model->rowCount(); ++i)
	{
		QStandardItem* item = model->item(i);
		std::shared_ptr<LicenseManager::License> license = item->data().value<std::shared_ptr<LicenseManager::License>>();
		if (license->isVerified(m_currentProject->getPublicKey()))
			setLicenseColor(license, Colors::normal);
		else
			setLicenseColor(license, Colors::error);
	}
}
void MainWindow::setLicenseColor(std::shared_ptr<LicenseManager::License> lic, const QColor& color)
{
	QStandardItemModel* model = dynamic_cast<QStandardItemModel*>(ui.licenses_listView->model());
	if (!model)
		return;

	for (int i = 0; i < model->rowCount(); ++i)
	{
		QStandardItem* item = model->item(i);
		std::shared_ptr<LicenseManager::License> license = item->data().value<std::shared_ptr<LicenseManager::License>>();
		if (license == lic)
		{
			QBrush brush(color);
			item->setBackground(brush);
			return;
		}
	}
}