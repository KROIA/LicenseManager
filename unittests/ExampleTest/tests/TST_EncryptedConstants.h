#pragma once

#include "UnitTest.h"
#include "LicenseManager.h"
#include <map>
#include <QFile>
//#include <QObject>
//#include <QCoreapplication>





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

	}

private:
	static constexpr char m_orgdecrypted[] = {"This is a test message"};
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


	std::string randTimeKeyDummy(const std::string timeStr)
	{
		const char *time = timeStr.c_str();
		size_t size = timeStr.size() - 1;

		std::uint32_t hashedTime = LicenseManager::EncryptedConstant::simple_hash(time, size + 1);
		char encryptionKey = hashedTime;
		for (std::size_t i = 1; i < sizeof(std::uint32_t); ++i)
			encryptionKey ^= (char)((hashedTime & (std::uint32_t(0xFF) << (i * 8))) >> (i * 8));

		std::string key(LicenseManager::EncryptedConstant::randKeyLen,' ');

		for (size_t i = 0; i < LicenseManager::EncryptedConstant::randKeyLen; ++i)
		{
			int shiftAmount = ((i % sizeof(std::uint32_t)) * 8);
			key[i] = LicenseManager::EncryptedConstant::xorChar(((hashedTime & (std::uint32_t(0xFF) << shiftAmount)) >> shiftAmount), time[i % size]);
		}

		for (size_t i = 0; i < LicenseManager::EncryptedConstant::randKeyLen; ++i)
		{
			key[i] = LicenseManager::EncryptedConstant::simple_hash(key.data(), LicenseManager::EncryptedConstant::randKeyLen);
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

	TEST_FUNCTION(randTimeKey)
	{
		TEST_START;
		std::map<std::string, unsigned int> pdf;
		for (int h = 0; h < 24; ++h)
		{
			for (int m = 0; m < 60; ++m)
			{
				for (int s = 0; s < 60; ++s)
				{
					std::string time = getTimeStr(h, m, s);
					std::string randKey = randTimeKeyDummy(time);
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


		QFile file("randTimeKeyDummy_density.csv");
		if (file.open(QIODevice::WriteOnly))
		{
			QByteArray arr(fileBuff.c_str(), fileBuff.length());
			file.write(arr);
			file.close();
		}

	}

};

TEST_INSTANTIATE(TST_EncryptedConstants);