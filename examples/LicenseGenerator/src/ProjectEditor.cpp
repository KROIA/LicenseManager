#include "ProjectEditor.h"
#include <QPushButton>
#include <QFrame>

ProjectEditor::ProjectEditor(Log::LoggerID parentLoggerID, QWidget *parent)
	: QWidget(parent)
{
	m_project = nullptr;
	ui.setupUi(this);
	m_log.setParentID(parentLoggerID);
	m_log.setName("ProjectEditor");

	m_addNewEntryWidget = ui.addEntry_widget;

	

	// Connect the button to a slot
	connect(ui.addEntry_pushButton, &QPushButton::clicked, this, &ProjectEditor::addNewEntry);
	ui.addEntry_pushButton->setIcon(QIcon(":/icons/add.png"));
	ui.addEntry_pushButton->setText("");
	// make the add button square
	ui.addEntry_pushButton->setFixedSize(30, 30);
	// resize the button icon size to fit the button
	ui.addEntry_pushButton->setIconSize(QSize(30, 30));
	// Make the button invisible
	ui.addEntry_pushButton->setStyleSheet("background-color: rgba(0,0,0,0);");
	ui.addEntry_pushButton->setToolTip("Add a new Key-value pair");



	// Add a grid layout to the scroll area
	//QGridLayout* gridLayout = new QGridLayout(ui.scrollAreaWidgetContents);
	QGridLayout* gridLayout = qobject_cast<QGridLayout*>(ui.scrollAreaWidgetContents->layout());
	gridLayout->setObjectName(QStringLiteral("gridLayout"));
	gridLayout->setContentsMargins(4, 4, 4, 4);
	gridLayout->setSpacing(4);

	// Create 3 columns for the layout
	gridLayout->setColumnStretch(0, 1);
	gridLayout->setColumnStretch(1, 1);
	gridLayout->setColumnStretch(2, 1);

	// Add the widget to the layout. It will be placed in the first row and span all 3 columns
	gridLayout->addWidget(m_addNewEntryWidget, 0, 0, 1, 3);

	// Set the layout to the scroll area
	ui.scrollAreaWidgetContents->setLayout(gridLayout);

	connect(ui.regeneratePrivateKey_pushButton, &QPushButton::clicked, this, &ProjectEditor::regeneratePrivateKey);
	ui.privateKey_plainTextEdit->setReadOnly(true);

	ui.save_pushButton->setIcon(QIcon(":/icons/accept.png"));
	ui.save_pushButton->setText("");
	// make the add button square
	ui.save_pushButton->setFixedSize(30, 30);
	// resize the button icon size to fit the button
	ui.save_pushButton->setIconSize(QSize(30, 30));
	// Make the button invisible
	ui.save_pushButton->setStyleSheet("background-color: rgba(0,0,0,0);");

	connect(ui.save_pushButton, &QPushButton::clicked, [this] {
		emit saveProject();
		hide();
	});
	ui.save_pushButton->setToolTip("Save the project");

	ui.cancel_pushButton->setIcon(QIcon(":/icons/cancel.png"));
	ui.cancel_pushButton->setText("");
	// make the add button square
	ui.cancel_pushButton->setFixedSize(30, 30);
	// resize the button icon size to fit the button
	ui.cancel_pushButton->setIconSize(QSize(30, 30));
	// Make the button invisible
	ui.cancel_pushButton->setStyleSheet("background-color: rgba(0,0,0,0);");

	connect(ui.cancel_pushButton, &QPushButton::clicked, [this] {
		clear();
		hide();
	});
	ui.cancel_pushButton->setToolTip("Cancel and close the project editor");
}

ProjectEditor::~ProjectEditor()
{

}


void ProjectEditor::clear()
{
	// Clear all entries
	for (size_t i = 0; i < m_entries.size(); ++i)
	{
		// Remove the widgets from the layout
		QGridLayout* gridLayout = qobject_cast<QGridLayout*>(ui.scrollAreaWidgetContents->layout());
		gridLayout->removeWidget(m_entries[i].paramName);
		gridLayout->removeWidget(m_entries[i].paramValue);
		gridLayout->removeWidget(m_entries[i].removeButton);

		// Delete the widgets
		delete m_entries[i].paramName;
		delete m_entries[i].paramValue;
		delete m_entries[i].removeButton;
	}

	// Clear the vector
	m_entries.clear();

	ui.projectName_lineEdit->clear();
	ui.privateKey_plainTextEdit->clear();
}

void ProjectEditor::setProject(std::shared_ptr<Project> project)
{
	clear();
	m_project = project;
	if(!project)
		return;
	
	ui.projectName_lineEdit->setText(QString::fromStdString(m_project->getName()));
	addEntry(m_project->getEntries());
	const std::string privateKey = m_project->getPrivateKey();
	if (privateKey.empty())
		regeneratePrivateKey();
	else
	{
		ui.privateKey_plainTextEdit->setPlainText(QString::fromStdString(privateKey));
		ui.publicKey_plainTextEdit->setPlainText(QString::fromStdString(m_project->getPublicKey()));
	}
}
std::shared_ptr<Project> ProjectEditor::project()
{
	if(!m_project)
		m_project = std::make_shared<Project>();
	m_project->clearEntries();
	m_project->setName(ui.projectName_lineEdit->text().toStdString());
	for (size_t i = 0; i < m_entries.size(); ++i)
	{
		m_project->addEntry(m_entries[i].paramName->text().toStdString(), m_entries[i].paramValue->text().toStdString());
	}
	m_project->setPrivateKey(ui.privateKey_plainTextEdit->toPlainText().toStdString());
	return m_project;
}


void ProjectEditor::addNewEntry()
{
	addEntry(Project::Entry());
}
void ProjectEditor::removeEntry()
{
	QPushButton* senderButton = qobject_cast<QPushButton*>(sender());
	if (senderButton)
	{
		// Find the entry that corresponds to the sender button
		for (size_t i = 0; i < m_entries.size(); ++i)
		{
			if (m_entries[i].removeButton == senderButton)
			{
				// Remove the widgets from the layout
				QGridLayout* gridLayout = qobject_cast<QGridLayout*>(ui.scrollAreaWidgetContents->layout());
				gridLayout->removeWidget(m_entries[i].paramName);
				gridLayout->removeWidget(m_entries[i].paramValue);
				gridLayout->removeWidget(m_entries[i].removeButton);

				// Delete the widgets
				delete m_entries[i].paramName;
				delete m_entries[i].paramValue;
				delete m_entries[i].removeButton;

				// Remove the entry from the vector
				m_entries.erase(m_entries.begin() + i);
				break;
			}
		}
	}

}

void ProjectEditor::regeneratePrivateKey()
{
	Project project;
	project.createPrivateKey();
	ui.privateKey_plainTextEdit->setPlainText(QString::fromStdString(project.getPrivateKey()));
	ui.publicKey_plainTextEdit->setPlainText(QString::fromStdString(project.getPublicKey()));
}


void ProjectEditor::addEntry(const Project::Entry& entry)
{
	Entry widgetEntry;
	widgetEntry.paramName = new QLineEdit(ui.scrollAreaWidgetContents);
	widgetEntry.paramName->setObjectName(QStringLiteral("paramName"));
	widgetEntry.paramName->setPlaceholderText("Parameter Name");
	widgetEntry.paramName->setText(QString::fromStdString(entry.paramName));

	widgetEntry.paramValue = new QLineEdit(ui.scrollAreaWidgetContents);
	widgetEntry.paramValue->setObjectName(QStringLiteral("paramValue"));
	widgetEntry.paramValue->setPlaceholderText("Parameter Value");
	widgetEntry.paramValue->setText(QString::fromStdString(entry.paramValue));

	widgetEntry.removeButton = new QPushButton(ui.scrollAreaWidgetContents);
	widgetEntry.removeButton->setObjectName(QStringLiteral("removeButton"));
	widgetEntry.removeButton->setText("");
	widgetEntry.removeButton->setIcon(QIcon(":/icons/delete.png"));
	widgetEntry.removeButton->setFixedSize(30, 30);
	// resize the button icon size to fit the button
	widgetEntry.removeButton->setIconSize(QSize(30, 30));
	// Make the button invisible
	widgetEntry.removeButton->setStyleSheet("background-color: rgba(0,0,0,0);");
	widgetEntry.removeButton->setToolTip("Remove this Key-value pair");



	// Add the widgets to the layout
	QLayout* layout = ui.scrollAreaWidgetContents->layout();
	QGridLayout* gridLayout = qobject_cast<QGridLayout*>(layout);

	// Remove the addNewEntryWidget and add it again at the end
	gridLayout->removeWidget(m_addNewEntryWidget);

	for (int i = 0; i < m_entries.size(); ++i)
	{
		// Remove all entries from the layout
		gridLayout->removeWidget(m_entries[i].paramName);
		gridLayout->removeWidget(m_entries[i].paramValue);
		gridLayout->removeWidget(m_entries[i].removeButton);
	}

	// Add the entry to the vector
	m_entries.push_back(widgetEntry);

	// Add all entries back to the layout
	for (int i = 0; i < m_entries.size(); ++i)
	{
		gridLayout->addWidget(m_entries[i].paramName, i, 0);
		gridLayout->addWidget(m_entries[i].paramValue, i, 1);
		gridLayout->addWidget(m_entries[i].removeButton, i, 2);
	}
	gridLayout->addWidget(m_addNewEntryWidget, m_entries.size(), 0, 1, 3);

	// Connect the remove button to a slot
	connect(widgetEntry.removeButton, &QPushButton::clicked, this, &ProjectEditor::removeEntry);
}

void ProjectEditor::addEntry(const std::vector<Project::Entry>& entries)
{
	// Add the widgets to the layout
	QLayout* layout = ui.scrollAreaWidgetContents->layout();
	QGridLayout* gridLayout = qobject_cast<QGridLayout*>(layout);

	// Remove the addNewEntryWidget and add it again at the end
	gridLayout->removeWidget(m_addNewEntryWidget);

	for (int i = 0; i < m_entries.size(); ++i)
	{
		// Remove all entries from the layout
		gridLayout->removeWidget(m_entries[i].paramName);
		gridLayout->removeWidget(m_entries[i].paramValue);
		gridLayout->removeWidget(m_entries[i].removeButton);
	}

	// Add all entries to the layout
	for(const Project::Entry& entry : entries)
	{
		Entry widgetEntry;
		widgetEntry.paramName = new QLineEdit(ui.scrollAreaWidgetContents);
		widgetEntry.paramName->setObjectName(QStringLiteral("paramName"));
		widgetEntry.paramName->setPlaceholderText("Parameter Name");
		widgetEntry.paramName->setText(QString::fromStdString(entry.paramName));

		widgetEntry.paramValue = new QLineEdit(ui.scrollAreaWidgetContents);
		widgetEntry.paramValue->setObjectName(QStringLiteral("paramValue"));
		widgetEntry.paramValue->setPlaceholderText("Parameter Value");
		widgetEntry.paramValue->setText(QString::fromStdString(entry.paramValue));

		widgetEntry.removeButton = new QPushButton(ui.scrollAreaWidgetContents);
		widgetEntry.removeButton->setObjectName(QStringLiteral("removeButton"));
		widgetEntry.removeButton->setText("");
		widgetEntry.removeButton->setIcon(QIcon(":/icons/delete.png"));
		widgetEntry.removeButton->setFixedSize(30, 30);
		// resize the button icon size to fit the button
		widgetEntry.removeButton->setIconSize(QSize(30, 30));
		// Make the button invisible
		widgetEntry.removeButton->setStyleSheet("background-color: rgba(0,0,0,0);");
		widgetEntry.removeButton->setToolTip("Remove this Key-value pair");

		// Add the entry to the vector
		m_entries.push_back(widgetEntry);

		// Connect the remove button to a slot
		connect(widgetEntry.removeButton, &QPushButton::clicked, this, &ProjectEditor::removeEntry);
	}
	

	// Add all entries back to the layout
	for (int i = 0; i < m_entries.size(); ++i)
	{
		gridLayout->addWidget(m_entries[i].paramName, i, 0);
		gridLayout->addWidget(m_entries[i].paramValue, i, 1);
		gridLayout->addWidget(m_entries[i].removeButton, i, 2);
	}
	gridLayout->addWidget(m_addNewEntryWidget, m_entries.size(), 0, 1, 3);

}

