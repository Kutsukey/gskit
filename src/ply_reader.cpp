#include <gskit/ply_reader.hpp>
#include <optional>
#include <sstream>
#include <cstdint>
#include <cstring>

namespace
{
    std::optional<gskit::PLYType> parsePLYType(const std::string &type)
    {
        if (type == "float")
            return gskit::PLYType::Float;
        if (type == "float32")
            return gskit::PLYType::Float32;
        if (type == "double")
            return gskit::PLYType::Double;
        if (type == "int")
            return gskit::PLYType::Int;
        if (type == "uint")
            return gskit::PLYType::UInt;
        if (type == "short")
            return gskit::PLYType::Short;
        if (type == "ushort")
            return gskit::PLYType::UShort;
        if (type == "char")
            return gskit::PLYType::Char;
        if (type == "uchar")
            return gskit::PLYType::UChar;
        return std::nullopt;
    }

    std::optional<gskit::PLYFormat> parsePLYFormat(const std::string &format)
    {
        if (format == "ascii")
            return gskit::PLYFormat::ASCII;
        if (format == "binary_little_endian")
            return gskit::PLYFormat::BINARY_LITTLE_ENDIAN;
        if (format == "binary_big_endian")
            return gskit::PLYFormat::BINARY_BIG_ENDIAN;
        return std::nullopt;
    }

    size_t typeSize(gskit::PLYType type)
    {
        switch (type)
        {
        case gskit::PLYType::Float:
            return sizeof(float);
        case gskit::PLYType::Float32:
            return sizeof(float);
        case gskit::PLYType::Double:
            return sizeof(double);
        case gskit::PLYType::Int:
            return sizeof(int32_t);
        case gskit::PLYType::UInt:
            return sizeof(uint32_t);
        case gskit::PLYType::Short:
            return sizeof(int16_t);
        case gskit::PLYType::UShort:
            return sizeof(uint16_t);
        case gskit::PLYType::Char:
            return sizeof(int8_t);
        case gskit::PLYType::UChar:
            return sizeof(uint8_t);
        }
        return 0; // Should never reach here
    }
}

bool gskit::PLYReader::open(const std::filesystem::path &path)
{
    file.open(path, std::ios::binary);
    return file.is_open();
}

bool gskit::PLYReader::readHeader(PLYHeader &header)
{
    if (!file.is_open())
        return false;

    // Reset internal state so repeated calls don't accumulate data.
    header_ = PLYHeader{};
    offsetMap_.clear();
    stride_ = 0;
    buffer_.clear();
    mapReady_ = false;

    std::string line;
    if (!std::getline(file, line) || line != "ply")
        return false;

    std::string keyword;
    std::string elementName;
    bool foundVertex = false;
    bool foundEndHeader = false;

    while (std::getline(file, line))
    {
        std::istringstream stream(line);
        stream >> keyword;

        if (keyword == "format")
        {
            std::string formatName;
            stream >> formatName;
            auto format = parsePLYFormat(formatName);
            if (!format)
                return false;
            header.format = *format;
            header_.format = *format;
        }
        else if (keyword == "element")
        {
            std::string name;
            size_t count = 0;
            if (!(stream >> name >> count))
                return false;
            elementName = name;
            if (name == "vertex")
            {
                header.vertexCount = count;
                header_.vertexCount = count;
                foundVertex = true;
            }
        }
        else if (keyword == "property")
        {
            // Only vertex (Gaussian) properties are collected;
            // face/edge/etc. properties are skipped.
            if (elementName == "vertex")
            {
                std::string type;
                std::string propertyName;
                if (!(stream >> type >> propertyName))
                    return false;
                auto propertyType = parsePLYType(type);
                if (!propertyType)
                    return false;
                header.properties.push_back(PLYProperty{propertyName, *propertyType});
                header_.properties.push_back(PLYProperty{propertyName, *propertyType});
            }
        }
        else if (keyword == "end_header")
        {
            foundEndHeader = true;
            break; // Stop reading header
        }
        // comment, obj_info, etc. are ignored
    }
    this->header_ = header;
    // A Gaussian PLY must contain a vertex element and a properly
    // terminated header; otherwise the parse is considered a failure.
    return foundEndHeader && foundVertex;
}
bool gskit::PLYReader::readGaussianData(GaussianData &data)
{
    if (!file.is_open())
        return false;

    // 1. Is it first time reading data? If so, build the offset map.
    if (!mapReady_)
        buildOffsetMap();

    // 2. Read the next vertex to the buffer.
    if (!file.read(buffer_.data(), stride_))
        return false;

    // 3. Extract the properties from the buffer using the offset map.
    auto readFloat = [&](const std::string &name, float &target)
    {
        auto it = offsetMap_.find(name);
        if (it != offsetMap_.end())
        {
            std::memcpy(&target, buffer_.data() + it->second.first, sizeof(float));
        }
    };

    readFloat("x", data.position.x);
    readFloat("y", data.position.y);
    readFloat("z", data.position.z);
    readFloat("scale_0", data.scale.x);
    readFloat("scale_1", data.scale.y);
    readFloat("scale_2", data.scale.z);
    readFloat("rot_0", data.rotation.w);
    readFloat("rot_1", data.rotation.x);
    readFloat("rot_2", data.rotation.y);
    readFloat("rot_3", data.rotation.z);
    readFloat("opacity", data.opacity);

    // 4. position, scale, rotation, opacity

    return true;
}

void gskit::PLYReader::buildOffsetMap()
{
    offsetMap_.clear();
    stride_ = 0;

    for (const auto &property : header_.properties)
    {
        size_t size = typeSize(property.type);
        offsetMap_[property.name] = {stride_, size};
        stride_ += size;
    }

    buffer_.resize(stride_);
    mapReady_ = true;
}