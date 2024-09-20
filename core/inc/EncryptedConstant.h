#pragma once
#include "LicenseManager_base.h"
#include <string>
#include <array>

/*
	This file contains compile-time XOR encryption/decryption functions.

	This can be usefull to hide constant strings in the binary.
	For example, you can store the public key in the binary in an encrypted form.
	When you need to use the public key, you can decrypt it at runtime.
*/


namespace LicenseManager
{  
	namespace EncryptedConstant
	{
		constexpr std::size_t randKeyLen = 8;

		constexpr std::uint32_t rotate_right(std::uint32_t value, std::uint32_t count) {
			return (value >> count) | (value << (32 - count));
		}

		
		constexpr std::uint32_t simple_hash(const char* input, std::size_t length, std::uint32_t seed = 0x6a09e667) {
			std::uint32_t hash = seed;

			for (std::size_t i = 0; i < length; ++i) {
				hash ^= static_cast<std::uint32_t>(input[i]);
				hash = rotate_right(hash, 5) + 0x9e3779b9;  // Adding a constant for diffusion.
			}

			return hash;
		}

		constexpr std::uint32_t operator "" _hash(const char* str, std::size_t len) {
			return simple_hash(str, len);
		}


		// Compile-time XOR encryption/decryption
		constexpr char xorChar(char c, char key) {
			return c ^ key;
		}

		

		// Get a random character from the __TIME__ macro
		constexpr std::array<char, randKeyLen> randTimeKey()
		{
			const char time[] = __TIME__;
			size_t size = sizeof(time) - 1;			
			
			std::uint32_t hashedTime = simple_hash(time, size+1);
			char encryptionKey = hashedTime;
			for (std::size_t i = 1; i < sizeof(std::uint32_t); ++i)
				encryptionKey ^= (char)((hashedTime & (std::uint32_t(0xFF) << (i * 8)))>>(i*8));

			std::array<char, randKeyLen> key{};

			for (size_t i = 0; i < randKeyLen; ++i)
			{
				int shiftAmount = ((i % sizeof(std::uint32_t)) * 8);
				key[i] = xorChar(((hashedTime & (std::uint32_t(0xFF) << shiftAmount)) >> shiftAmount), time[i%size]);
			}

			for (size_t i = 0; i < randKeyLen; ++i)
			{
				key[i] = simple_hash(key.data(), randKeyLen);
			}
			return key;
		}


		/// <summary>
		/// Encrypts a string at compile-time using XOR encryption
		/// </summary>
		/// <description>
		/// Usage:
		/// constexpr auto encrypted = LicenseManager::EncryptedConstant::encrypt_string("Hello, Compile-Time Encryption!");
		/// 
		/// The encryption key is generated from the __TIME__ macro.
		/// Note that the encryption key is stored at the end of the encrypted string.
		/// Each encrypted char is used to encrypt the next char
		/// </description>
		/// <typeparam name="N"></typeparam>
		/// <param name="plainText"></param>
		/// <returns></returns>
		template<std::size_t N>
		constexpr std::array<char, N + randKeyLen> encrypt_string(const char(&plainText)[N])
		{
			std::array<char, randKeyLen> randKey = randTimeKey();
			std::array<char, N + randKeyLen> encrypted{};
			for (std::size_t i = 0; i < N; ++i) {
				encrypted[i] = plainText[i];
				for (std::size_t j = 0; j < randKeyLen; ++j)
				{
					encrypted[i] = xorChar(encrypted[i], randKey[j]);
				}
				if (i > 0)
				{
					encrypted[i] = xorChar(encrypted[i], encrypted[i - 1]);
				}
			}
			for (std::size_t i = 0; i < randKeyLen; ++i)
				encrypted[N + i] = randKey[i];  // Store the encryption key at the end
			return encrypted;
		}


		/// <summary>
		/// Decrypts a string at runtime using XOR encryption
		/// </summary>
		/// <description>
		/// Usage:
		/// std::string decryptedString = LicenseManager::EncryptedConstant::decrypt_string(encrypted);
		/// 
		/// Note that the encryption key is stored at the end of the encrypted string.
		/// Because of that, the decrypted string has 1 element less than the encrypted string.
		/// </description>
		/// <typeparam name="N"></typeparam>
		/// <param name="encrypted"></param>
		/// <returns></returns>
		template<std::size_t N>
		constexpr std::string decrypt_string(const std::array<char, N>& encrypted)
		{
			std::string decrypted;
			decrypted.resize(N - randKeyLen -1);
			std::array<char, randKeyLen> encryptionKey{};
			// Get the encryption key
			for (std::size_t i = 0; i < randKeyLen; ++i)
			{
				encryptionKey[i] = encrypted[N - randKeyLen + i];
			}
			for (size_t i = encrypted.size() - randKeyLen-1; i>0; --i)
			{
				decrypted[i - 1] = encrypted[i - 1];
				if (i > 1)
				{
					decrypted[i - 1] = xorChar(decrypted[i - 1], encrypted[i - 2]);
				}
				for (std::size_t j = randKeyLen; j > 0; --j)
				{
					decrypted[i - 1] = xorChar(decrypted[i - 1], encryptionKey[j - 1]);  // Decrypt at runtime
				}
			}
			return decrypted;
		}
	}
}