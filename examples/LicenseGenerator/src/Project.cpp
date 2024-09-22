#include "Project.h"
#include "LicenseManager.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QFileInfo>
#include <QDir>

Project::Project()
{
}

Project::~Project()
{
}

void Project::clear()
{
	m_entries.clear();
}

void Project::addEntry(const std::string& paramName, const std::string& paramValue)
{
	Entry entry;
	entry.paramName = paramName;
	entry.paramValue = paramValue;
	m_entries.push_back(entry);
}

void Project::removeEntry(size_t index)
{
	m_entries.erase(m_entries.begin() + index);
}

void Project::setPrivateKey(const std::string& privateKey)
{
	m_privateKey = privateKey;
	m_publicKey = LicenseManager::License::getPublicKeyFromPrivateKey(m_privateKey);
}
void Project::createPrivateKey()
{
	m_privateKey = LicenseManager::License::generatePrivateKey();
	m_publicKey = LicenseManager::License::getPublicKeyFromPrivateKey(m_privateKey);
}

bool Project::save(const std::string& filename)
{
	QJsonObject json;
	json["name"] = QString::fromStdString(m_name);
	json["privateKey"] = QString::fromStdString(m_privateKey);
	json["publicKey"] = QString::fromStdString(m_publicKey);

	QJsonArray entries;
	for (size_t i = 0; i < m_entries.size(); ++i)
	{
		QJsonObject entry;
		entry["paramName"] = QString::fromStdString(m_entries[i].paramName);
		entry["paramValue"] = QString::fromStdString(m_entries[i].paramValue);
		entries.append(entry);
	}
	json["entries"] = entries;


	// Create Path if it does not exist
	QFileInfo fileInfo(QString::fromStdString(filename));
	QDir().mkpath(fileInfo.absolutePath());


	QJsonDocument doc(json);
	QFile file(QString::fromStdString(filename));
	if (file.open(QIODevice::WriteOnly))
	{
		file.write(doc.toJson());
		file.close();
		return true;
	}
	return false;
}
bool Project::load(const std::string& filename)
{
	QFile file(QString::fromStdString(filename));
	if (file.open(QIODevice::ReadOnly))
	{
		QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
		QJsonObject json = doc.object();

		m_name = json["name"].toString().toStdString();
		m_privateKey = json["privateKey"].toString().toStdString();
		//m_publicKey = json["publicKey"].toString().toStdString();
		m_publicKey = LicenseManager::License::getPublicKeyFromPrivateKey(m_privateKey);

		QJsonArray entries = json["entries"].toArray();
		m_entries.clear();
		for (int i = 0; i < entries.size(); ++i)
		{
			QJsonObject entry = entries[i].toObject();
			Entry e;
			e.paramName = entry["paramName"].toString().toStdString();
			e.paramValue = entry["paramValue"].toString().toStdString();
			m_entries.push_back(e);
		}
		file.close();
		return true;
	}
	return false;
}

