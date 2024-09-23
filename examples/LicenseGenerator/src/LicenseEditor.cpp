#include "LicenseEditor.h"
#include <QGridLayout>

LicenseEditor::LicenseEditor(Log::LoggerID parentLoggerID, QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	m_log.setParentID(parentLoggerID);
	m_log.setName("LicenseEditor");
}

LicenseEditor::~LicenseEditor()
{
}


void LicenseEditor::setLicense(std::shared_ptr<Project> project,  std::shared_ptr<LicenseManager::License> license)
{
	m_license = license;
	for (auto it = m_entries.begin(); it != m_entries.end(); ++it)
	{
		delete it->paramName;
		delete it->paramValue;
	}
	m_entries.clear();

	if(!m_license || !project)
	{
		ui.name_lineEdit->setText("");
		return;
	}

	std::vector<Project::Entry> entries = project->getEntries();
	std::map<std::string, std::string> licenseData = license->getLicenseData();

	ui.name_lineEdit->setText(QString::fromStdString(license->getName()));

	// Create entries.
	// Each consists of a label and a line edit.
	// Add them to the QGridLayout.
	QGridLayout* layout = dynamic_cast<QGridLayout*>(ui.scrollAreaWidgetContents->layout());
	layout->removeItem(m_verticalSpacer);

	size_t i = 0;
	for (auto it = entries.begin(); it != entries.end(); ++it)
	{
		Entry entry;
		entry.paramName = new QLabel(QString::fromStdString(it->paramName));
		std::string value = "";
		if(licenseData.find(it->paramName) != licenseData.end())
		{
			value = licenseData[it->paramName];
		}
		entry.paramValue = new QLineEdit(QString::fromStdString(value));
		entry.paramValue->setPlaceholderText(QString::fromStdString(it->paramValue));

		layout->addWidget(entry.paramName, i, 0);
		layout->addWidget(entry.paramValue, i, 1);

		m_entries.push_back(entry);
		++i;
	}
	// Add a spacer to the bottom of the layout.
	if(!m_verticalSpacer)
		m_verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
	layout->addItem(m_verticalSpacer, i, 0, 1, 2);

}

std::shared_ptr<LicenseManager::License> LicenseEditor::getLicense()
{
	if (!m_license)
	{
		return m_license;
	}
	std::map<std::string, std::string> licenseData;
	for (auto it = m_entries.begin(); it != m_entries.end(); ++it)
	{
		licenseData[it->paramName->text().toStdString()] = it->paramValue->text().toStdString();
	}

	m_license->setName(ui.name_lineEdit->text().toStdString());
	m_license->setLicenseData(licenseData);

	return m_license;
}

void LicenseEditor::on_name_lineEdit_textChanged(const QString& text)
{
	if (!m_license)
		return;
	m_license->setName(text.toStdString());
	emit licenseNameChanged();
}