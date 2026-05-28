#ifndef ISHA_AI_GGUF_LOADER_HPP
#define ISHA_AI_GGUF_LOADER_HPP

#include <string>
#include <map>
#include <vector>

namespace isha {

struct GGUFHeader {
    char magic[4];
    uint32_t version;
    uint64_t tensor_count;
    uint64_t metadata_kv_count;
};

struct GGUFTensorInfo {
    std::string name;
    uint32_t dimensions_count;
    std::vector<uint64_t> dimensions;
    uint32_t type;
    uint64_t offset;
};

class GGUFLoader {
public:
    GGUFLoader();
    ~GGUFLoader() = default;

    bool parseFile(const std::string& path);
    
    const GGUFHeader& getHeader() const { return header_; }
    const std::map<std::string, std::string>& getMetadata() const { return metadata_; }
    const std::vector<GGUFTensorInfo>& getTensors() const { return tensors_; }

private:
    GGUFHeader header_;
    std::map<std::string, std::string> metadata_;
    std::vector<GGUFTensorInfo> tensors_;
};

} // namespace isha

#endif // ISHA_AI_GGUF_LOADER_HPP
