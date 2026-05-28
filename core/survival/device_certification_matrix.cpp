#include "device_certification_matrix.hpp"

namespace isha {

std::string DeviceCertificationMatrix::tierToString(CertificationTier tier) {
    switch (tier) {
        case CertificationTier::CERTIFIED: return "CERTIFIED";
        case CertificationTier::SUPPORTED: return "SUPPORTED";
        case CertificationTier::EXPERIMENTAL: return "EXPERIMENTAL";
        case CertificationTier::UNSAFE: return "UNSAFE";
        default: return "UNKNOWN";
    }
}

CertificationTier DeviceCertificationMatrix::evaluateSpecs(const DeviceSpecs& specs) {
    // 1. Extreme hardware limitations => UNSAFE / BLOCKED
    if (specs.ram_gb < 3) {
        return CertificationTier::UNSAFE; // Block devices below 3GB RAM immediately to avoid boot crashes
    }
    if (specs.android_sdk_version < 26) {
        return CertificationTier::UNSAFE; // Block Android Oreo / SDK < 26
    }

    // 2. High risk configurations => EXPERIMENTAL
    if (specs.ram_gb == 3 || (specs.ram_gb == 4 && specs.is_emmc && specs.high_oem_restrictions)) {
        return CertificationTier::EXPERIMENTAL;
    }

    // 3. Mainstream Qualcomm/Google chips with UFS and decent RAM => CERTIFIED
    bool premium_soc = (specs.chipset_family == "Snapdragon" || specs.chipset_family == "Tensor");
    if (premium_soc && specs.ram_gb >= 6 && !specs.is_emmc) {
        return CertificationTier::CERTIFIED;
    }

    // 4. Everything else decent => SUPPORTED
    return CertificationTier::SUPPORTED;
}

bool DeviceCertificationMatrix::isDeploymentAllowed(CertificationTier tier) {
    return tier != CertificationTier::UNSAFE;
}

} // namespace isha
