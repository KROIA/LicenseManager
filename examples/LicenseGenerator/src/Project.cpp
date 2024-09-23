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
	m_licenses.clear();
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
void Project::addLicense(std::shared_ptr<LicenseManager::License> license)
{
	for(size_t i=0; i<m_licenses.size(); ++i)
	{
		if (m_licenses[i]->getName() == license->getName())
		{
			m_licenses[i] = license;
			return;
		}
	}
	license->signLicense(m_privateKey);
	m_licenses.push_back(license);
}
void Project::setLicenses(const std::vector<std::shared_ptr<LicenseManager::License>>& licenses)
{
	m_licenses = licenses;
}

void Project::removeLicense(const std::string& signature)
{
	for (size_t i = 0; i < m_licenses.size(); ++i)
	{
		if (m_licenses[i]->getSignature() == signature)
		{
			m_licenses.erase(m_licenses.begin() + i);
			break;
		}
	}
}

bool Project::save(const std::string& path)
{
	bool success = true;
	std::string projectPath = path + "/" + m_name;
	std::string projectFile = projectPath + "/" + m_name + ".json";
	std::string licensesPath = projectPath + "/licenses";
	QDir().mkpath(projectPath.c_str());
	QDir().mkpath(licensesPath.c_str());

	// Save Project settings to file
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
	
	QFileInfo fileInfo(QString::fromStdString(projectFile));
	QDir().mkpath(fileInfo.absolutePath());


	QJsonDocument doc(json);
	QFile file(QString::fromStdString(projectFile));
	bool fop = file.open(QIODevice::WriteOnly);
	if (fop)
	{
		file.write(doc.toJson());
		file.close();
		log().logInfo("Project file saved to: " + projectFile);
	}
	else
	{
		success = false;
		log().logError("Can't save project file: " + projectFile);
	}

	// Save Licenses to file
	for(size_t i = 0; i < m_licenses.size(); ++i)
	{
		m_licenses[i]->signLicense(m_privateKey);
		std::string fileName = m_licenses[i]->getName() + ".lic";
		std::string licenseFile = licensesPath + "/"+ fileName;
		if (!m_licenses[i]->saveToFile(licenseFile))
		{
			success = false;
			log().logError("Can't save license file: " + licenseFile);
		}
		else
		{
			log().logInfo("License file saved to: " + licenseFile);
		}
	}
	return success;
}
bool Project::load(const std::string& path)
{
	bool success = true;
	std::string projectPath = path;

	// Check if the path is a directory or a file
	QFileInfo fileInfo(QString::fromStdString(path));
	if (fileInfo.isDir())
	{
		// extract project name from path
		size_t pos = path.find_last_of("/\\");
		if (pos == std::string::npos)
		{
			// Search for any json file in the directory
			QDir dir(QString::fromStdString(path));
			QStringList filters;
			filters << "*.json";
			QStringList files = dir.entryList(filters, QDir::Files);
			if (files.size() == 0)
			{
				log().logError("No project file found in directory: " + path);
				return false;
			}
			m_name = files[0].toStdString();
		}
		else
		{
			m_name = path.substr(pos + 1);
		}
	}
	else
	{
		// Remove the file name from the path
		size_t pos = projectPath.find_last_of("/\\");
		if (pos != std::string::npos)
		{
			projectPath = projectPath.substr(0, pos);
		}
		// extract project name from file name
		pos = path.find_last_of("/\\");
		if (pos == std::string::npos)
		{
			m_name = path;
		}
		else
		{
			m_name = path.substr(pos + 1);
		}
	}


	// Remove the extention from the name
	size_t dotPos = m_name.find_last_of(".");
	if (dotPos != std::string::npos)
	{
		m_name = m_name.substr(0, dotPos);
	}

	
	
	std::string projectFile = projectPath + "/" + m_name + ".json";
	std::string licensesPath = projectPath + "/licenses";


	QFile file(QString::fromStdString(projectFile));
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
		log().logInfo("Project file loaded: " + projectFile);
	}
	else
	{
		log().logError("Can't load project file: " + projectFile);
		success = false;
	}

	// Load Licenses from file
	QDir dir(QString::fromStdString(licensesPath));
	QStringList filters;
	filters << "*.lic";
	QStringList files = dir.entryList(filters, QDir::Files);
	m_licenses.clear();
	for (int i = 0; i < files.size(); ++i)
	{
		std::shared_ptr<LicenseManager::License> license = std::make_shared<LicenseManager::License>();
		if (license->loadFromFile(licensesPath + "/" + files[i].toStdString()))
		{
			m_licenses.push_back(license);
			log().logInfo("License file loaded: " + files[i].toStdString());
		}
		else
		{
			log().logError("Can't load license file: " + files[i].toStdString());
			success = false;
		}
	}
	
	return success;
}


std::string Project::getProjectFilePath(const std::string& path) const
{
	return path + "/" + m_name + "/" + m_name + ".json";
}

Log::LogObject& Project::log()
{
	static Log::LogObject log("Project");
	return log;
}
