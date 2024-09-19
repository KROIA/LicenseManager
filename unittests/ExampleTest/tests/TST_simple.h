#pragma once

#include "UnitTest.h"
#include "LicenseManager.h"
//#include <QObject>
//#include <QCoreapplication>





class TST_simple : public UnitTest::Test
{
	TEST_CLASS(TST_simple)
public:
	TST_simple()
		: Test("TST_simple")
	{
		ADD_TEST(TST_simple::test1);
		ADD_TEST(TST_simple::test2);

	}

private:

	// Tests
	TEST_FUNCTION(test1)
	{
		TEST_START;

		int a = 0;
		TEST_MESSAGE("is a == 0?");
		TEST_ASSERT(a == 0);

		std::string privateKey = LicenseManager::License::generatePrivateKey();
		std::map<std::string, std::string> licenseData;
		licenseData["name"] = "John Doe";
		licenseData["email"] = "mail";
		licenseData["company"] = "company";
		licenseData["date"] = "2020-01-01";

		LicenseManager::License license;
		license.setLicenseData(licenseData);
		license.signLicense(privateKey);
		license.saveToFile("license.lic");

		LicenseManager::License license2;
		license2.loadFromFile("license.lic");

		std::string publicKey = LicenseManager::License::getPublicKeyFromPrivateKey(privateKey);
		
		TEST_ASSERT(license2.isVerified(publicKey));
		TEST_ASSERT(license == license2);
	}




	TEST_FUNCTION(test2)
	{
		TEST_START;

		int a = 0;
		TEST_ASSERT_M(a == 0, "is a == 0?");

		int b = 0;
		if (b != 0)
		{
			TEST_FAIL("b is not 0");
		}

		// fails if a != b
		TEST_COMPARE(a, b);
	}

};

TEST_INSTANTIATE(TST_simple);