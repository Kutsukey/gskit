#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <unordered_map>

namespace gskit
{
    enum class PLYType
    {
        Float,
        Float32,
        Double,
        Int,
        UInt,
        Short,
        UShort,
        Char,
        UChar
    };

    struct PLYProperty
    {
        std::string name;
        PLYType type;
    };

    enum class PLYFormat
    {
        ASCII,
        BINARY_LITTLE_ENDIAN,
        BINARY_BIG_ENDIAN
    };

    struct PLYHeader
    {
        PLYFormat format;
        size_t vertexCount;
        std::vector<PLYProperty> properties;
    };

    struct Vec3
    {
        float x,y,z;
    };

    struct Quat
    {
        float x,y,z,w;
    };
    

    struct GaussianData
    {
        Vec3 position;
        Vec3 scale;
        Quat rotation;
        float opacity;
    };

    class PLYReader
    {
    private:
        std::ifstream file;
        PLYHeader header_;
        std::unordered_map<std::string, std::pair<size_t, size_t>> offsetMap_;
        size_t stride_;
        std::vector<char> buffer_;
        bool mapReady_ = false;
    public:
        bool open(const std::filesystem::path& path);
        bool readHeader(PLYHeader& header);
        bool readGaussianData(GaussianData& data);
        void buildOffsetMap();
        std::vector<char> getBuffer() const { return buffer_; }
        size_t getStride() const { return stride_; }
    };
}