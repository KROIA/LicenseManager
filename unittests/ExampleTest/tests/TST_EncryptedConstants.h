#pragma once

#include "UnitTest.h"
#include "LicenseManager.h"
#include <map>
#include <QFile>
#include <QCoreApplication>
//#include <QObject>
//#include <QCoreapplication>



constexpr auto encryptedTestString = LicenseManager::EncryptedConstant::encrypt_string("HIDE TEST STRING");

class TST_EncryptedConstants : public UnitTest::Test
{
	TEST_CLASS(TST_EncryptedConstants)
public:
	TST_EncryptedConstants()
		: Test("TST_EncryptedConstants")
	{
		ADD_TEST(TST_EncryptedConstants::encrypt);
		ADD_TEST(TST_EncryptedConstants::decrypt);
		ADD_TEST(TST_EncryptedConstants::randTimeKey);
		ADD_TEST(TST_EncryptedConstants::binaryHideCheck);

	}

private:
	static constexpr char m_orgdecrypted[23] = {"This is a test message"};
	static constexpr std::array<char, 23+ LicenseManager::EncryptedConstant::randKeyLen> encrypted = LicenseManager::EncryptedConstant::encrypt_string<23>(m_orgdecrypted);
	std::string m_encrypted;

	// Tests
	TEST_FUNCTION(encrypt)
	{
		TEST_START;
		m_encrypted = std::string(encrypted.begin(), encrypted.end());
		
		TEST_ASSERT(encrypted.size() == sizeof(m_orgdecrypted) + LicenseManager::EncryptedConstant::randKeyLen);
		bool equalCheck = true;
		for (size_t i = 0; i < encrypted.size(); ++i)
		{
			equalCheck &= encrypted[i] == m_orgdecrypted[i];
		}

		TEST_MESSAGE("Encrypted text: \"" + m_encrypted + "\"");
		TEST_ASSERT_M(!equalCheck, "Encrypted text is equal to the decrypted text");
	}




	TEST_FUNCTION(decrypt)
	{
		TEST_START;

		std::string decrypted(LicenseManager::EncryptedConstant::decrypt_string(encrypted));

		TEST_MESSAGE("Decrypted original: \"" + std::string(m_orgdecrypted) + "\"");
		TEST_MESSAGE("Decrypted text:     \"" + decrypted + "\"");
		std::string org(m_orgdecrypted);
		TEST_ASSERT(decrypted == org)
	}

	
	// Imitates the randTimeKey function from EncryptedConstant.h to test the randomness of the generated keys
	std::string randTimeKeyDummy(const std::string& timeStr)
	{
		std::string key(LicenseManager::EncryptedConstant::randKeyLen, ' ');
		LicenseManager::EncryptedConstant::PCG pcg;
		pcg.rng.inc = LicenseManager::EncryptedConstant::hash_string(timeStr.c_str());
		for (size_t i = 0; i < LicenseManager::EncryptedConstant::randKeyLen; ++i)
		{
			key[i] = pcg();
		}
		return key;
	}
	std::string getTimeStr(int h, int m, int s)
	{
		std::string time;
		if (h < 10)
			time += "0";
		time += std::to_string(h);
		time += ":";
		if (m < 10)
			time += "0";
		time += std::to_string(m);
		time += ":";
		if (s < 10)
			time += "0";
		time += std::to_string(s);
		return time;
	}


	// Test every possible time key
	TEST_FUNCTION(randTimeKey)
	{
		TEST_START;

		// Save the amount of times the same key was generated
		// Best case: all keys are unique
		std::map<std::string, unsigned int> pdf;
		int count = 0;
		for (int h = 0; h < 24; ++h)
		{
			for (int m = 0; m < 60; ++m)
			{
				for (int s = 0; s < 60; ++s)
				{
					std::string time = getTimeStr(h, m, s);
					std::string randKey = randTimeKeyDummy(time);
					count++;
					if (count < 20)
					{
						TEST_MESSAGE("Time: " + time + " Key: \"" + randKey+"\"");
					}
					if (pdf.find(randKey) == pdf.end())
						pdf[randKey] = 1;
					else
						pdf[randKey]++;
				}
			}
		}

		unsigned int min = 99999;
		unsigned int max = 0;
		
		std::string fileBuff;
		for (const auto& it : pdf)
		{
			min = std::min(min, it.second);
			max = std::max(max, it.second);
			fileBuff += std::to_string(it.second) + ";\n";
		}
		TEST_MESSAGE("Min: " + std::to_string(min));
		TEST_MESSAGE("Max: " + std::to_string(max));

		TEST_ASSERT(max < (24*60*60) / LicenseManager::EncryptedConstant::randKeyLen / 2)

		// Save the pdf to a file
		// create a plot to check for the distribution
		QFile file("randTimeKeyDummy_density.csv");
		if (file.open(QIODevice::WriteOnly))
		{
			QByteArray arr(fileBuff.c_str(), fileBuff.length());
			file.write(arr);
			file.close();
		}

	}


	
	TEST_FUNCTION(binaryHideCheck)
	{
		TEST_START;
		
		// Get this exe name
		std::string exeName = QCoreApplication::applicationFilePath().toStdString();

		// Read the exe file
		QFile file(exeName.c_str());
		if (!file.open(QIODevice::ReadOnly))
		{
			TEST_FAIL("Could not open file: \"" + exeName+"\"");
		}
		QByteArray data = file.readAll();
		file.close();

		// Check if the test string is in the file
		std::string testStr(encryptedTestString.begin(), encryptedTestString.end());
		std::string dataStr(data.begin(), data.end());
		TEST_MESSAGE("Encrypted test string: \"" + testStr + "\"");
		TEST_ASSERT(dataStr.find(testStr) != std::string::npos);

		TEST_MESSAGE("Decrypted test string: \"" + LicenseManager::EncryptedConstant::decrypt_string(encryptedTestString) + "\"");
		TEST_ASSERT(dataStr.find(LicenseManager::EncryptedConstant::decrypt_string(encryptedTestString)) == std::string::npos);
	}

};


TEST_INSTANTIATE(TST_EncryptedConstants);