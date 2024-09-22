#include "MainWindow.h"
#include <QCloseEvent>
#include <QFileDialog>
#include <QDir>
#include "AppSettingsImpl.h"

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	m_ribbon = new RibbonImpl(ui.ribbon_widget);
	setWindowTitle("License Generator");
	m_log.setName(windowTitle().toStdString());

	m_projectEditor = new ProjectEditor();
	m_projectEditor->hide();

	RibbonImpl::ProjectButtons& projectButtons = m_ribbon->projectButtons();
	connect(projectButtons.createProject, &RibbonWidget::RibbonButton::clicked, [this] {
		// Create new project
		m_projectEditor->clear();
		m_projectEditor->show();
	});
	connect(projectButtons.editProject, &RibbonWidget::RibbonButton::clicked, [this] {
		// Edit project
		m_projectEditor->setProject(m_project);
		m_projectEditor->show();
	});
	connect(projectButtons.loadProject, &RibbonWidget::RibbonButton::clicked, [this] {
		// Load project
		QString filePath = QFileDialog::getOpenFileName(this, "Open Project", 
			AppSettingsImpl::getProjectGroup().projectsPath.getValue().toString(), "Project files (*.json)");
		if(filePath.size() > 0)
			loadProject(filePath.toStdString());
	});

	connect(m_projectEditor, &ProjectEditor::saveProject, this, &MainWindow::onProjectEditorSaveProject);


	AppSettingsImpl::load();
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
	Project project;
	if (project.load(path))
	{
		m_log.logInfo("Project loaded from file: "+path);
		setProject(project);
	}
	else
	{
		m_log.logError("Can't load project from file: "+path);
	}
}
void MainWindow::setProject(const Project& project)
{
	m_project = project;
	//m_projectEditor->setProject(project);

}

void MainWindow::onProjectEditorSaveProject()
{
	m_project = m_projectEditor->project();
	//std::string filePath = AppSettingsImpl::getProjectGroup().lastLoadedProject.getValue().toString().toStdString();
	//if(filePath.size() == 0)
	std::string filePath = AppSettingsImpl::getProjectGroup().projectsPath.getValue().toString().toStdString() + "/"+ m_project.getName()+"/"+ m_project.getName()+".json";
	
	if(m_project.save(filePath))
	{
		m_log.logInfo("Project saved to file: "+filePath);
		AppSettingsImpl::getProjectGroup().lastLoadedProject.setValue(QString::fromStdString(filePath));
		AppSettingsImpl::save();
		setProject(m_project);
	}
	else
	{
		m_log.logError("Can't save project to file: "+filePath);
	}
}
