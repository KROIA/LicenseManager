#pragma once

#include <vector>
#include <string>
#include "LicenseManager.h"
#include "Logger.h"

class Project
{
	public:

	struct Entry
	{
		std::string paramName;
		std::string paramValue;
	};


	Project();
	~Project();

	void clear();
	void addEntry(const std::string& paramName, const std::string& paramValue);
	void removeEntry(size_t index);
	const std::vector<Entry>& getEntries() const{ return m_entries; }
	void clearEntries() { m_entries.clear(); }

	void setName(const std::string& name){ m_name = name; }
	const std::string& getName() const { return m_name; }

	void setPrivateKey(const std::string& privateKey);
	const std::string& getPrivateKey() const { return m_privateKey; }
	const std::string& getPublicKey() const { return m_publicKey; }
	void createPrivateKey();

	void addLicense(std::shared_ptr<LicenseManager::License> license);
	void setLicenses(const std::vector< std::shared_ptr<LicenseManager::License>>& licenses);
	void clearLicenses() { m_licenses.clear(); }
	void removeLicense(const std::string &signature);
	const std::vector< std::shared_ptr<LicenseManager::License>>& getLicenses() const { return m_licenses; }

	bool save(const std::string& path);
	bool load(const std::string& path);
	std::string getProjectFilePath(const std::string& path) const;
	
	static Log::LogObject& log();
	private:
	std::string m_name;
	std::vector<Entry> m_entries;

	std::string m_privateKey;
	std::string m_publicKey;

	// Licenses generated for users
	std::vector< std::shared_ptr<LicenseManager::License>> m_licenses;


	
};