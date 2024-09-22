#pragma once
#include "LicenseManager_base.h"
#include <string>
#include <array>

/*
	This file contains compile-time XOR encryption/decryption functions.

	This can be usefull to hide constant strings in the binary.
	For example, you can store the public key in the binary in an encrypted form.
	When you need to use the public key, you can decrypt it at runtime.

	Usage:
	constexpr auto encryptedPublicKey = LicenseManager::EncryptedConstant::encrypt_string(
		"-----BEGIN PUBLIC KEY-----\n"
		"XXX"
		"-----END PUBLIC KEY-----\n");
	
	std::string decryptedPublicKey = LicenseManager::EncryptedConstant::decrypt_string(encryptedPublicKey);

	Detailed description:
	1. The encryption key is generated from the __TIME__ macro.
	   If the LicenseManager::EncryptedConstant::randKeyLen is set to a large enough value,
	   the encryption key will be unique for each timestamp.
	2. The encryption key has a length of <LicenseManager::EncryptedConstant::randKeyLen> size
	3. The encryption key is stored at the end of the encrypted string.
	4. Each char gets encrypted one by one. 
	5. Each char gets XORed with each char of the encryption key.
	   After that the char gets XORed with the previous, already encrypted char.

	6. The decryption is done in reverse order.

	Note that the encryption key is stored at the end of the encrypted string.
	It is technically possible to decrypt the binary file using every possible key but since the 
	text gets XORed with the previous char, it is highly unlikely that the decryption will be successful, 
	unless the exact location in the binary file is known.


*/


namespace LicenseManager
{  
	namespace EncryptedConstant
	{
		// The length of the encryption key
		// 6 bytes is enough to generate an unique key for each __TIME__ macro value
		constexpr std::size_t randKeyLen = 5;

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

		constexpr uint64_t hash_string(const char* str, uint64_t hash = 0xcbf29ce484222325) {
			return (*str == 0) ? hash : hash_string(str + 1, (hash ^ static_cast<uint64_t>(*str)) * 0x100000001b3);
		}

		constexpr auto seed()
		{
			return hash_string(__TIME__);
		}
		struct PCG
		{
			struct pcg32_random_t { std::uint64_t state = 0;  std::uint64_t inc = seed(); };
			pcg32_random_t rng;
			typedef std::uint32_t result_type;

			constexpr PCG()
			{
				pcg32_random_r();
				pcg32_random_r();
			}

			constexpr result_type operator()()
			{
				return pcg32_random_r();
			}
		private:
			constexpr std::uint32_t pcg32_random_r()
			{
				std::uint64_t oldstate = rng.state;
				// Advance internal state
				rng.state = oldstate * 6364136223846793005ULL + (rng.inc | 1);
				// Calculate output function (XSH RR), uses old state for max ILP
				std::uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
				std::uint32_t rot = oldstate >> 59u;
				std::uint32_t result = (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
				return result;
			}
		};

		// Get a random character from the __TIME__ macro
		constexpr std::array<char, randKeyLen> randTimeKey()
		{
			std::array<char, randKeyLen> key{};
			PCG pcg;
			pcg.rng.inc = seed();
			for (size_t i = 0; i < randKeyLen; ++i)
			{
				key[i] = pcg();
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
		constexpr std::array<char, N + randKeyLen-1> encrypt_string(const char(&plainText)[N])
		{
			std::array<char, randKeyLen> randKey = randTimeKey();
			std::array<char, N + randKeyLen-1> encrypted{};

			// N-1 to skip the null terminator
			for (std::size_t i = 0; i < N-1; ++i) {
				encrypted[i] = plainText[i];

				//encrypted[i] = xorChar(encrypted[i], 'H');
				
				// Use the char at index i+1 to encrypt the char at index i
				if (i < N - 2)
				{
					encrypted[i] = xorChar(encrypted[i], plainText[i + 1]);
				}

				// Use the char at index i+2 to encrypt the char at index i
				if (i < N - 3)
				{
					encrypted[i] = xorChar(encrypted[i], plainText[i + 2]);
				}

				// Use the char at index i-1 to encrypt the char at index i
				if (i > 0)
				{
					encrypted[i] = xorChar(encrypted[i], encrypted[i - 1]);
				}

				// Use the char at index i-2 to encrypt the char at index i
				if (i > 1)
				{
					encrypted[i] = xorChar(encrypted[i], encrypted[i - 2]);
				}

				// Use the encryption key to encrypt the char at index i
				for (std::size_t j = 0; j < randKeyLen; ++j)
				{
					encrypted[i] = xorChar(encrypted[i], randKey[j]);
				}
				
			}
			for (std::size_t i = 0; i < randKeyLen; ++i)
				encrypted[N + i-1] = randKey[i];  // Store the encryption key at the end
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
			decrypted.resize(N - randKeyLen);
			std::array<char, randKeyLen> encryptionKey{};
			// Get the encryption key
			for (std::size_t i = 0; i < randKeyLen; ++i)
			{
				encryptionKey[i] = encrypted[N - randKeyLen + i];
			}
			size_t messageSize = N - randKeyLen;
			for (size_t l = messageSize; l>0; --l)
			{
				size_t i = l-1;
				decrypted[i] = encrypted[i];
				
				// Use the encryption key to decrypt the char at index i
				for (std::size_t j = randKeyLen; j > 0; --j)
				{
					decrypted[i] = xorChar(decrypted[i], encryptionKey[j - 1]);  // Decrypt at runtime
				}

				// Use the char at index i-1 to decrypt the char at index i
				if (i > 1)
				{
					decrypted[i] = xorChar(decrypted[i], encrypted[i - 2]);
				}

				// Use the char at index i-1 to decrypt the char at index i
				if (i > 0)
				{
					decrypted[i] = xorChar(decrypted[i], encrypted[i - 1]);
				}

				// Use the char at index i+2 to decrypt the char at index i
				if (i < messageSize - 2)
				{
					decrypted[i] = xorChar(decrypted[i], decrypted[i+2]);
				}

				// Use the char at index i+1 to decrypt the char at index i
				if (i < messageSize - 1)
				{
					decrypted[i] = xorChar(decrypted[i], decrypted[i+1]);
				}
			}
			decrypted[messageSize] = '\0';  // Null-terminate the string
			return decrypted;
		}
	}
}