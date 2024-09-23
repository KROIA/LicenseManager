#include "RibbonImpl.h"


RibbonImpl::RibbonImpl(QWidget* parent)
	: RibbonWidget::Ribbon(parent)
{
	Q_INIT_RESOURCE(RibbonIcons); // Init the resource file
	// Create tabs
	RibbonWidget::RibbonTab* mainTab	= new RibbonWidget::RibbonTab("LicenseGenerator", ":/icons/license.png", this);

	// Create groups
	RibbonWidget::RibbonButtonGroup* projectGroup	= new RibbonWidget::RibbonButtonGroup("Project", mainTab);
	RibbonWidget::RibbonButtonGroup* licenseGroup		= new RibbonWidget::RibbonButtonGroup("License", mainTab);

	// Create buttons
	m_projectButtons.createProject	= new RibbonWidget::RibbonButton("Create", "Create new Project", ":/icons/add.png", true, projectGroup);
	m_projectButtons.editProject	= new RibbonWidget::RibbonButton("Edit", "Edit Project", ":/icons/edit.png", true, projectGroup);
	m_projectButtons.loadProject	= new RibbonWidget::RibbonButton("Load", "Load existing Project", ":/icons/folder.png", true, projectGroup);

	m_licenseButtons.createNewLicense	= new RibbonWidget::RibbonButton("Create New", "Create new License", ":/icons/add_license.png", true, licenseGroup);
	//m_licenseButtons.openLicense		= new RibbonWidget::RibbonButton("Open", "Open existing License", ":/icons/folder.png", true, licenseGroup);
	m_licenseButtons.saveLicense		= new RibbonWidget::RibbonButton("Save", "Save License", ":/icons/save.png", true, licenseGroup);

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
