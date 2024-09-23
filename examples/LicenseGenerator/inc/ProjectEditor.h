#pragma once

#include <QWidget>
#include <vector>

#include "ui_ProjectEditor.h"
#include "LicenseManager.h"
#include "Project.h"
#include "Logger.h"

class ProjectEditor : public QWidget
{
	Q_OBJECT

public:
	ProjectEditor(Log::LoggerID parentLoggerID, QWidget *parent = nullptr);
	~ProjectEditor();

	void clear();

	void setProject(std::shared_ptr<Project> project);
	std::shared_ptr<Project> project();

signals:
	void saveProject();

private slots:
	void addNewEntry();
	void removeEntry();
	void regeneratePrivateKey();
private:
	void addEntry(const Project::Entry &entry);
	void addEntry(const std::vector<Project::Entry> &entries);

	Ui::ProjectEditor ui;

	QWidget* m_addNewEntryWidget;

	struct Entry
	{
		QLineEdit* paramName;
		QLineEdit* paramValue;
		QPushButton* removeButton;
	};
	std::vector<Entry> m_entries;

	std::shared_ptr<Project> m_project;

	Log::LogObject m_log;
};
