#pragma once

#include <vector>
#include <string>

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
	const std::vector<Entry>& entries() const{ return m_entries; }

	void setName(const std::string& name){ m_name = name; }
	const std::string& getName() const { return m_name; }

	void setPrivateKey(const std::string& privateKey);
	const std::string& getPrivateKey() const { return m_privateKey; }
	const std::string& getPublicKey() const { return m_publicKey; }
	void createPrivateKey();

	bool save(const std::string& filename);
	bool load(const std::string& filename);

	private:
	std::string m_name;
	std::vector<Entry> m_entries;

	std::string m_privateKey;
	std::string m_publicKey;

};