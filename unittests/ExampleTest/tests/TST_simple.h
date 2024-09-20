#pragma once

#include "UnitTest.h"
#include "LicenseManager.h"
#include <QFile>
//#include <QObject>
//#include <QCoreapplication>





class TST_simple : public UnitTest::Test
{
	TEST_CLASS(TST_simple)
public:
	TST_simple()
		: Test("TST_simple")
	{
		ADD_TEST(TST_simple::simpleLicenseCheck);
		ADD_TEST(TST_simple::manipulationTest);

	}

private:
	std::string m_privateKey;
	std::string m_licenseFile = "UnitTestLicense.lic";
	std::string m_licDate = "2020-01-01";

	// Tests
	TEST_FUNCTION(simpleLicenseCheck)
	{
		TEST_START;

		m_privateKey = LicenseManager::License::generatePrivateKey();
		std::map<std::string, std::string> licenseData;

		// Populate license data
		licenseData["name"] = "John Doe";
		licenseData["email"] = "mail";
		licenseData["company"] = "company";
		licenseData["date"] = m_licDate.c_str();

		// Create license
		LicenseManager::License license;
		license.setLicenseData(licenseData);
		TEST_ASSERT(license.signLicense(m_privateKey));
		TEST_ASSERT(license.saveToFile(m_licenseFile));

		LicenseManager::License license2;
		TEST_ASSERT(license2.loadFromFile(m_licenseFile));

		std::string publicKey = LicenseManager::License::getPublicKeyFromPrivateKey(m_privateKey);
		
		TEST_ASSERT(license2.isVerified(publicKey));
		TEST_ASSERT(license == license2);
	}




	TEST_FUNCTION(manipulationTest)
	{
		TEST_START;

		// Edit the license file
		QFile file(m_licenseFile.c_str());
		TEST_ASSERT(file.open(QIODevice::ReadWrite));
		QByteArray data = file.readAll();
		// Find the date

		int pos = data.indexOf(m_licDate.c_str());
		std::string newDate = "2021-01-01";
		TEST_ASSERT(pos != -1);
		data.replace(pos, m_licDate.size(), newDate.c_str());
		file.seek(0);
		file.write(data);
		file.close();

		// Load the license
		LicenseManager::License license2;
		TEST_ASSERT(license2.loadFromFile(m_licenseFile));

		std::string publicKey = LicenseManager::License::getPublicKeyFromPrivateKey(m_privateKey);
		TEST_ASSERT(!license2.isVerified(publicKey));
	}

};

TEST_INSTANTIATE(TST_simple);