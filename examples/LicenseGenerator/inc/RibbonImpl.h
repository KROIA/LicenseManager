#pragma once

#include "RibbonWidget.h"

class RibbonImpl : public RibbonWidget::Ribbon
{
	Q_OBJECT
	public:
		RibbonImpl(QWidget* parent = nullptr);


	
		struct ProjectButtons
		{
			RibbonWidget::RibbonButton* createProject;
			RibbonWidget::RibbonButton* editProject;
			RibbonWidget::RibbonButton* loadProject;
		};

		struct LicenseButtons
		{
			RibbonWidget::RibbonButton* createNewLicense;
			RibbonWidget::RibbonButton* openLicense;
			RibbonWidget::RibbonButton* saveLicense;
		};


		ProjectButtons& projectButtons();
		LicenseButtons& licenseButtons();

	private:
		ProjectButtons m_projectButtons;
		LicenseButtons m_licenseButtons;
};