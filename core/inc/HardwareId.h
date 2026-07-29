#pragma once
#include "LicenseManager_base.h"
#include <string>
#include <vector>
#include <cstdint>

/*
    HardwareId
    ==========
    Builds a stable machine fingerprint from a set of hardware/OS identifiers
    that the *user of the library* selects.

    A license can be bound to the fingerprint (node-locking) so that a valid
    license file only works on the machine it was issued for. Copying the file
    to another machine then fails validation.

    Usage (choose the components you trust to be stable for your users):

        using HW = LicenseManager::HardwareId;
        std::uint32_t components = HW::VolumeSerial | HW::MachineGuid | HW::CpuInfo;
        std::string id = HW::generate(components, "MyApp-2026");   // salt is app specific

    Notes / trade-offs per component:
      - VolumeSerial : changes if the system drive is reformatted.
      - MachineGuid  : stable across reinstalls of *apps*, changes on OS reinstall.
      - ComputerName : user can rename the machine.
      - MacAddress   : changes with the network adapter / docking stations / VPNs.
      - CpuInfo      : model+feature signature, NOT a unique per-chip serial, but
                       stable and shared across many machines of the same model,
                       so use it only as one factor among several.
      - BiosInfo     : baseboard/BIOS identifiers from the registry.
      - UserName     : current OS user.

    Combine several components so a single hardware change does not lock a user
    out. For tolerance against minor hardware changes, see generatePerComponent()
    and countMatching() which let you accept a license when at least K of N
    components still match.
*/

namespace LicenseManager
{
    class LICENSE_MANAGER_API HardwareId
    {
    public:
        enum Component : std::uint32_t
        {
            None         = 0,
            VolumeSerial = 1u << 0,   // System drive volume serial number
            MachineGuid  = 1u << 1,   // Windows installation GUID (registry)
            ComputerName = 1u << 2,   // NetBIOS computer name
            MacAddress   = 1u << 3,   // First physical network adapter MAC
            CpuInfo      = 1u << 4,   // CPUID vendor + feature signature
            BiosInfo     = 1u << 5,   // Baseboard / BIOS identifiers (registry)
            UserName     = 1u << 6,   // Current OS user name

            // Convenience presets
            Default      = VolumeSerial | MachineGuid | CpuInfo,
            All          = VolumeSerial | MachineGuid | ComputerName | MacAddress
                         | CpuInfo | BiosInfo | UserName
        };

        // Raw value of a single component on this machine. Empty if unavailable.
        static std::string getComponent(Component component);

        // Ordered list of the components encoded in a mask (stable order).
        static std::vector<Component> componentList(std::uint32_t mask);

        // Combined fingerprint (hex SHA-256) built from the selected components.
        // The component mask and salt are mixed in, so the same machine yields
        // different fingerprints for different selections / applications.
        // Components that are unavailable on this machine are skipped; if none of
        // the selected components are available an empty string is returned.
        static std::string generate(std::uint32_t components = Default,
                                    const std::string& salt = std::string());

        // One fingerprint hash per selected component (same order as
        // componentList). Use together with countMatching() for fuzzy matching.
        static std::vector<std::string> generatePerComponent(std::uint32_t components = Default,
                                                             const std::string& salt = std::string());

        // Exact match of the combined fingerprint against a stored value.
        static bool matches(const std::string& expected,
                            std::uint32_t components = Default,
                            const std::string& salt = std::string());

        // Fuzzy match helper: number of per-component hashes that are equal.
        // Both vectors must come from generatePerComponent with the same mask.
        static unsigned int countMatching(const std::vector<std::string>& a,
                                          const std::vector<std::string>& b);

        // How many of the selected components are currently readable.
        static unsigned int availableComponentCount(std::uint32_t components);

    private:
        static std::string sha256Hex(const std::string& data);
        static std::string componentTag(Component component);
    };
}
