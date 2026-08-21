#include <string>
#include <optional>
#include <vector>
#include <filesystem>
#include <gskit/ply_reader.hpp>

namespace gskit
{
    enum class Severity
    {
        OK,
        WARNING,
        ERROR
    };

    enum class IssueCode
    {
        FILE_CANNOT_OPEN,
        INVALID_FILE_FORMAT,
        MISSING_PROPERTY,
        NON_FINITE_POSITION,
        NON_FINITE_SCALE,
        INVALID_SCALE,
        ANISOTROPIC_SCALE,
        NON_FINITE_ROTATION,
        ZERO_QUATERNION,
        NON_NORMALIZED_QUATERNION,
        NON_FINITE_OPACITY,
        INVALID_OPACITY,
        DEAD_GAUSSIAN,
        NON_FINITE_SH,
        UNEXPECTED_SH_DEGREE,
        INVALID_COLOR,
        INVALID_BOUNDS,
        VERTEX_COUNT_MISMATCH,
    };

    struct ValidationIssue
    {
        Severity sev;
        IssueCode issue;
        std::string message;
        std::optional<size_t> gaussianIndex;
    };

    struct ValidationOptions
    {
        bool strict = false;
        bool checkFinite = true;
        bool checkScale = true;
        bool checkRotation = true;
        bool checkOpacity = true;
        bool checkSH = true;
        bool checkBounds = true;
        bool checkVertexCount = true;
    };

    struct ValidationSummary
    {
        size_t gaussianCount;
        uint16_t shDegree;
        uint16_t warningCount;
        uint16_t errorCount;
        size_t ghostCount;
        size_t needleCount;
        Vec3 bboxMin{0.0f, 0.0f, 0.0f};
        Vec3 bboxMax{0.0f, 0.0f, 0.0f};
    };

    struct ValidationResult
    {
        ValidationSummary summary;
        std::vector<ValidationIssue> issues;
    };

    ValidationResult ValidateAsset(const std::filesystem::path &path, const ValidationOptions &options);
    bool isValidGaussian(const GaussianData &data, const ValidationOptions &options);
} // namespace gskit