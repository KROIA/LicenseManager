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
		// Compile-time XOR encryption/decryption
		constexpr char xorChar(char c, char key) {
			return c ^ key;
		}

		// Get a random character from the __TIME__ macro
		constexpr char randTimeChar()
		{
			const char time[] = __TIME__;
			//return time[sizeof(time)-2];
			size_t size = sizeof(time)-1;
			char encryptionKey = 'A';
			for (size_t i = 0; i < size; ++i)
			{
				encryptionKey = xorChar(encryptionKey, time[i]);
			}
			return encryptionKey;
		}


		/// <summary>
		/// Encrypts a string at compile-time using XOR encryption
		/// </summary>
		/// <description>
		/// Usage:
		/// constexpr auto encrypted = LicenseManager::EncryptedConstant::encrypt_string("Hello, Compile-Time Encryption!",'A');
		/// 
		/// The second parameter is the encryption key which is used to encrypt the string.
		/// Note that the encryption key is stored at the end of the encrypted string.
		/// </description>
		/// <typeparam name="N"></typeparam>
		/// <param name="plainText"></param>
		/// <param name="encryptionKey"></param>
		/// <returns></returns>
		template<std::size_t N>
		constexpr std::array<char, N+1> encrypt_string(const char(&plainText)[N], const char encryptionKey)
		{
			std::array<char, N+1> encrypted{};
			for (std::size_t i = 0; i < N; ++i) {
				encrypted[i] = xorChar(plainText[i], encryptionKey);
			}
			encrypted[N] = encryptionKey;  // Store the encryption key at the end
			return encrypted;
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
		/// <typeparam name="N"></typeparam>
		/// <param name="plainText"></param>
		/// <returns></returns>
		template<std::size_t N>
		constexpr std::array<char, N + 1> encrypt_string(const char(&plainText)[N])
		{
			return encrypt_string(plainText, randTimeChar());
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
			decrypted.resize(N - 1);
			const char encryptionKey = encrypted[N - 1];  // Get the encryption key
			for (size_t i = 0; i < encrypted.size() - 1; ++i) {
				decrypted[i] = xorChar(encrypted[i], encryptionKey);  // Decrypt at runtime
			}
			return decrypted;
		}
	}
}