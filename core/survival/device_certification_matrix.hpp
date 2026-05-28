#ifndef ISHA_AI_DEVICE_CERTIFICATION_MATRIX_HPP
#define ISHA_AI_DEVICE_CERTIFICATION_MATRIX_HPP

#include <string>

namespace isha {

enum class CertificationTier {
    CERTIFIED,
    SUPPORTED,
    EXPERIMENTAL,
    UNSAFE
};

struct DeviceSpecs {
    std::string chipset_family; // "Snapdragon", "Tensor", "MediaTek", "Exynos", "Generic"
    unsigned int ram_gb;
    bool is_emmc;
    unsigned int android_sdk_version;
    bool high_oem_restrictions;
};

class DeviceCertificationMatrix {
public:
    static std::string tierToString(CertificationTier tier);

    // Classifies hardware profile into a deployment safety category
    static CertificationTier evaluateSpecs(const DeviceSpecs& specs);

    // Enforces deployment block or warning
    static bool isDeploymentAllowed(CertificationTier tier);
};

} // namespace isha

#endif // ISHA_AI_DEVICE_CERTIFICATION_MATRIX_HPP
