#pragma once
#include <filesystem>
#include "ply_reader.hpp"

namespace gskit
{
    class PLYWriter
    {
    private:
        std::ofstream file_;
    public:
        bool open(const std::filesystem::path &path);
        bool writeHeader(const PLYHeader &header, size_t vertexCount);
        bool writeVertex(const char *data, size_t size);
        bool close();
    };
} // namespace gskit

