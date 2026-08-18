#include <gskit/ply_writer.hpp>
#include <fstream>
#include <string>
#include <gskit/ply_reader.hpp>

namespace
{
    std::string plyTypeToString(gskit::PLYType type)
    {
        switch (type)
        {
        case gskit::PLYType::Char:
            return "char";
        case gskit::PLYType::UChar:
            return "uchar";
        case gskit::PLYType::Short:
            return "short";
        case gskit::PLYType::UShort:
            return "ushort";
        case gskit::PLYType::Int:
            return "int";
        case gskit::PLYType::UInt:
            return "uint";
        case gskit::PLYType::Float32:
            return "float32";
        case gskit::PLYType::Float:
            return "float";
        case gskit::PLYType::Double:
            return "double";
        default:
            return "";
        }
    }
}
bool gskit::PLYWriter::open(const std::filesystem::path &path)
{
    file_.open(path, std::ios::binary);
    return file_.is_open();
}

bool gskit::PLYWriter::writeHeader(const gskit::PLYHeader &header, size_t vertexCount)
{
    std::string headerStr = "ply\nformat binary_little_endian 1.0\n";
    headerStr += "element vertex " + std::to_string(vertexCount) + "\n";
    for (const auto &property : header.properties)
    {
        headerStr += "property " + plyTypeToString(property.type) + " " + property.name + "\n";
    }
    headerStr += "end_header\n";

    file_ << headerStr;
    return true;
}

bool gskit::PLYWriter::writeVertex(const char *data, size_t size)
{
    if (file_.is_open())
    {
        file_.write(data, size);
        return true;
    }
    return false;
}

bool gskit::PLYWriter::close()
{
    file_.close();
    return !file_.is_open();
}
