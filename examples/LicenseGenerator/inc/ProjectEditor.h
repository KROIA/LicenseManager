#pragma once

#include <QWidget>
#include <vector>

#include "ui_ProjectEditor.h"
#include "LicenseManager.h"
#include "Project.h"

class ProjectEditor : public QWidget
{
	Q_OBJECT

public:
	ProjectEditor(QWidget *parent = nullptr);
	~ProjectEditor();

	void clear();

	void setProject(const Project &project);
	Project project() const;

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
};
