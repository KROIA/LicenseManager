#include "RibbonImpl.h"


RibbonImpl::RibbonImpl(QWidget* parent)
	: RibbonWidget::Ribbon(parent)
{
	Q_INIT_RESOURCE(RibbonIcons); // Init the resource file
	// Create tabs
	RibbonWidget::RibbonTab* mainTab	= new RibbonWidget::RibbonTab("LicenseGenerator", ":/icons/developerTool.png", this);

	// Create groups
	RibbonWidget::RibbonButtonGroup* projectGroup	= new RibbonWidget::RibbonButtonGroup("Project", mainTab);
	RibbonWidget::RibbonButtonGroup* licenseGroup		= new RibbonWidget::RibbonButtonGroup("License", mainTab);

	// Create buttons
	m_projectButtons.createProject	= new RibbonWidget::RibbonButton("Create Project", "Create Project", ":/icons/plus.png", true, projectGroup);
	m_projectButtons.editProject	= new RibbonWidget::RibbonButton("Edit Project", "Edit Project", ":/icons/pencil.png", true, projectGroup);
	m_projectButtons.loadProject	= new RibbonWidget::RibbonButton("Load Project", "Load Project", ":/icons/folder.png", true, projectGroup);

	m_licenseButtons.createNewLicense	= new RibbonWidget::RibbonButton("Create New License", "Create New License", ":/icons/plus.png", true, licenseGroup);
	m_licenseButtons.openLicense		= new RibbonWidget::RibbonButton("Open License", "Open License", ":/icons/folder.png", true, licenseGroup);
	m_licenseButtons.saveLicense		= new RibbonWidget::RibbonButton("Save License", "Save License", ":/icons/floppy_disk.png", true, licenseGroup);

	//m_settingsButtons.settings1->setIconSize(QSize(40, 40));


	// Add tabs
	addTab(mainTab);
}

RibbonImpl::ProjectButtons& RibbonImpl::projectButtons()
{
	return m_projectButtons;
}
RibbonImpl::LicenseButtons& RibbonImpl::licenseButtons()
{
	return m_licenseButtons;
}
