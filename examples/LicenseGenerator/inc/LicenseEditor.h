#pragma once

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include "ui_LicenseEditor.h"
#include "LicenseManager.h"
#include "Logger.h"
#include "Project.h"

class LicenseEditor : public QWidget
{
	Q_OBJECT

public:
	LicenseEditor(Log::LoggerID parentLoggerID, QWidget *parent = nullptr);
	~LicenseEditor();

	void setLicense(std::shared_ptr<Project> project, std::shared_ptr<LicenseManager::License> license);
	std::shared_ptr<LicenseManager::License> getLicense();

signals:
	void licenseNameChanged();

private slots:
	void on_name_lineEdit_textChanged(const QString &text);
private:
	Ui::LicenseEditor ui;

	std::shared_ptr<LicenseManager::License> m_license;

	struct Entry
	{
		QLabel* paramName = nullptr;
		QLineEdit* paramValue = nullptr;
	};
	std::vector<Entry> m_entries;
	QSpacerItem* m_verticalSpacer = nullptr;

	Log::LogObject m_log;
};
