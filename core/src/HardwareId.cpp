#include "HardwareId.h"

#include <openssl/evp.h>

#include <array>
#include <cstdio>
#include <cstring>

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
    #include <intrin.h>
    #include <iphlpapi.h>
    #pragma comment(lib, "iphlpapi.lib")
    #pragma comment(lib, "advapi32.lib")
#endif

namespace LicenseManager
{
#ifdef _WIN32
    // Read a REG_SZ value from HKEY_LOCAL_MACHINE (forcing the 64-bit view so a
    // 32-bit build reads the same keys as a 64-bit one).
    static std::string readRegistryString(const wchar_t* subKey, const wchar_t* valueName)
    {
        HKEY hKey = nullptr;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, subKey, 0,
                          KEY_READ | KEY_WOW64_64KEY, &hKey) != ERROR_SUCCESS)
            return std::string();

        wchar_t buffer[512];
        DWORD size = sizeof(buffer);
        DWORD type = 0;
        LONG res = RegQueryValueExW(hKey, valueName, nullptr, &type,
                                    reinterpret_cast<LPBYTE>(buffer), &size);
        RegCloseKey(hKey);

        if (res != ERROR_SUCCESS || type != REG_SZ)
            return std::string();

        int chars = static_cast<int>(size / sizeof(wchar_t));
        while (chars > 0 && buffer[chars - 1] == L'\0')
            --chars;

        if (chars <= 0)
            return std::string();

        int needed = WideCharToMultiByte(CP_UTF8, 0, buffer, chars, nullptr, 0, nullptr, nullptr);
        std::string out(static_cast<size_t>(needed), '\0');
        WideCharToMultiByte(CP_UTF8, 0, buffer, chars, out.data(), needed, nullptr, nullptr);
        return out;
    }

    static std::string getVolumeSerial()
    {
        wchar_t systemPath[MAX_PATH] = {0};
        if (GetWindowsDirectoryW(systemPath, MAX_PATH) == 0)
            return std::string();

        // Keep only the drive root, e.g. "C:\".
        wchar_t root[4] = { systemPath[0], L':', L'\\', L'\0' };

        DWORD serial = 0;
        if (!GetVolumeInformationW(root, nullptr, 0, &serial, nullptr, nullptr, nullptr, 0))
            return std::string();

        char buf[16];
        std::snprintf(buf, sizeof(buf), "%08lX", static_cast<unsigned long>(serial));
        return std::string(buf);
    }

    static std::string getComputerNameStr()
    {
        wchar_t name[MAX_COMPUTERNAME_LENGTH + 1] = {0};
        DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
        if (!GetComputerNameW(name, &size))
            return std::string();
        int needed = WideCharToMultiByte(CP_UTF8, 0, name, static_cast<int>(size), nullptr, 0, nullptr, nullptr);
        std::string out(static_cast<size_t>(needed), '\0');
        WideCharToMultiByte(CP_UTF8, 0, name, static_cast<int>(size), out.data(), needed, nullptr, nullptr);
        return out;
    }

    static std::string getUserNameStr()
    {
        wchar_t name[256] = {0};
        DWORD size = 256;
        if (!GetUserNameW(name, &size) || size == 0)
            return std::string();
        int needed = WideCharToMultiByte(CP_UTF8, 0, name, static_cast<int>(size) - 1, nullptr, 0, nullptr, nullptr);
        std::string out(static_cast<size_t>(needed), '\0');
        WideCharToMultiByte(CP_UTF8, 0, name, static_cast<int>(size) - 1, out.data(), needed, nullptr, nullptr);
        return out;
    }

    static std::string getMacAddress()
    {
        ULONG bufLen = 0;
        if (GetAdaptersInfo(nullptr, &bufLen) != ERROR_BUFFER_OVERFLOW || bufLen == 0)
            return std::string();

        std::vector<unsigned char> buffer(bufLen);
        IP_ADAPTER_INFO* adapters = reinterpret_cast<IP_ADAPTER_INFO*>(buffer.data());
        if (GetAdaptersInfo(adapters, &bufLen) != ERROR_SUCCESS)
            return std::string();

        for (IP_ADAPTER_INFO* a = adapters; a != nullptr; a = a->Next)
        {
            // Only consider ethernet/wifi adapters with a 6-byte hardware address
            // and skip obviously virtual/loopback ones (address all zero).
            if (a->AddressLength != 6)
                continue;
            bool allZero = true;
            for (UINT i = 0; i < a->AddressLength; ++i)
                if (a->Address[i] != 0) { allZero = false; break; }
            if (allZero)
                continue;

            char buf[18];
            std::snprintf(buf, sizeof(buf), "%02X%02X%02X%02X%02X%02X",
                          a->Address[0], a->Address[1], a->Address[2],
                          a->Address[3], a->Address[4], a->Address[5]);
            return std::string(buf);
        }
        return std::string();
    }

    static std::string getCpuInfo()
    {
        int regs[4] = {0};
        char vendor[13] = {0};

        __cpuid(regs, 0);
        std::memcpy(vendor + 0, &regs[1], 4);   // EBX
        std::memcpy(vendor + 4, &regs[3], 4);   // EDX
        std::memcpy(vendor + 8, &regs[2], 4);   // ECX

        __cpuid(regs, 1);   // feature / signature info in EAX, feature bits in ECX/EDX

        char buf[64];
        std::snprintf(buf, sizeof(buf), "%s-%08X-%08X-%08X",
                      vendor, static_cast<unsigned>(regs[0]),
                      static_cast<unsigned>(regs[2]), static_cast<unsigned>(regs[3]));
        return std::string(buf);
    }

    static std::string getBiosInfo()
    {
        const wchar_t* biosKey = L"HARDWARE\\DESCRIPTION\\System\\BIOS";
        std::string manufacturer = readRegistryString(biosKey, L"SystemManufacturer");
        std::string product      = readRegistryString(biosKey, L"SystemProductName");
        std::string board        = readRegistryString(biosKey, L"BaseBoardProduct");
        std::string biosVersion  = readRegistryString(biosKey, L"BIOSVersion");
        std::string combined = manufacturer + "|" + product + "|" + board + "|" + biosVersion;
        if (combined == "|||")
            return std::string();
        return combined;
    }
#endif // _WIN32

    std::string HardwareId::sha256Hex(const std::string& data)
    {
        unsigned char digest[EVP_MAX_MD_SIZE];
        unsigned int digestLen = 0;

        EVP_MD_CTX* ctx = EVP_MD_CTX_new();
        if (!ctx)
            return std::string();

        std::string out;
        if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) == 1 &&
            EVP_DigestUpdate(ctx, data.data(), data.size()) == 1 &&
            EVP_DigestFinal_ex(ctx, digest, &digestLen) == 1)
        {
            static const char* hex = "0123456789abcdef";
            out.reserve(digestLen * 2);
            for (unsigned int i = 0; i < digestLen; ++i)
            {
                out.push_back(hex[(digest[i] >> 4) & 0xF]);
                out.push_back(hex[digest[i] & 0xF]);
            }
        }
        EVP_MD_CTX_free(ctx);
        return out;
    }

    std::string HardwareId::componentTag(Component component)
    {
        switch (component)
        {
            case VolumeSerial: return "VOL";
            case MachineGuid:  return "GUID";
            case ComputerName: return "HOST";
            case MacAddress:   return "MAC";
            case CpuInfo:      return "CPU";
            case BiosInfo:     return "BIOS";
            case UserName:     return "USER";
            default:           return "";
        }
    }

    std::string HardwareId::getComponent(Component component)
    {
#ifdef _WIN32
        switch (component)
        {
            case VolumeSerial: return getVolumeSerial();
            case MachineGuid:  return readRegistryString(L"SOFTWARE\\Microsoft\\Cryptography", L"MachineGuid");
            case ComputerName: return getComputerNameStr();
            case MacAddress:   return getMacAddress();
            case CpuInfo:      return getCpuInfo();
            case BiosInfo:     return getBiosInfo();
            case UserName:     return getUserNameStr();
            default:           return std::string();
        }
#else
        (void)component;
        return std::string();
#endif
    }

    std::vector<HardwareId::Component> HardwareId::componentList(std::uint32_t mask)
    {
        static const Component ordered[] = {
            VolumeSerial, MachineGuid, ComputerName, MacAddress, CpuInfo, BiosInfo, UserName
        };
        std::vector<Component> list;
        for (Component c : ordered)
            if (mask & static_cast<std::uint32_t>(c))
                list.push_back(c);
        return list;
    }

    std::string HardwareId::generate(std::uint32_t components, const std::string& salt)
    {
        std::string material = "LMHWID\x1e" + salt + "\x1e";
        material += std::to_string(components);
        material += "\x1e";

        bool any = false;
        for (Component c : componentList(components))
        {
            std::string value = getComponent(c);
            if (value.empty())
                continue;   // skip unavailable components deterministically
            any = true;
            material += componentTag(c);
            material += ':';
            material += value;
            material += '\x1f';
        }

        if (!any)
            return std::string();

        return sha256Hex(material);
    }

    std::vector<std::string> HardwareId::generatePerComponent(std::uint32_t components,
                                                              const std::string& salt)
    {
        std::vector<std::string> hashes;
        for (Component c : componentList(components))
        {
            std::string value = getComponent(c);
            if (value.empty())
            {
                hashes.push_back(std::string());
                continue;
            }
            hashes.push_back(sha256Hex("LMHWID1\x1e" + salt + "\x1e" + componentTag(c) + ":" + value));
        }
        return hashes;
    }

    bool HardwareId::matches(const std::string& expected, std::uint32_t components, const std::string& salt)
    {
        std::string current = generate(components, salt);
        if (current.empty() || expected.empty())
            return false;
        // constant-time-ish comparison to avoid trivial early-out timing
        if (current.size() != expected.size())
            return false;
        unsigned char diff = 0;
        for (size_t i = 0; i < current.size(); ++i)
            diff |= static_cast<unsigned char>(current[i] ^ expected[i]);
        return diff == 0;
    }

    unsigned int HardwareId::countMatching(const std::vector<std::string>& a,
                                           const std::vector<std::string>& b)
    {
        unsigned int count = 0;
        size_t n = (a.size() < b.size()) ? a.size() : b.size();
        for (size_t i = 0; i < n; ++i)
            if (!a[i].empty() && a[i] == b[i])
                ++count;
        return count;
    }

    unsigned int HardwareId::availableComponentCount(std::uint32_t components)
    {
        unsigned int count = 0;
        for (Component c : componentList(components))
            if (!getComponent(c).empty())
                ++count;
        return count;
    }
}
