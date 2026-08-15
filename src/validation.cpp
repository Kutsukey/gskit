#include <gskit/validation.hpp>
#include <gskit/ply_reader.hpp>
#include <algorithm>
#include <cmath>

gskit::ValidationResult gskit::ValidateAsset(const std::filesystem::path &path, const ValidationOptions &options)
{
    PLYReader reader;
    PLYHeader header;
    ValidationResult result{};

    if (!reader.open(path) || !reader.readHeader(header))
    {
        result.summary.errorCount++;
        result.issues.push_back(ValidationIssue{Severity::ERROR,
                                                IssueCode::FILE_CANNOT_OPEN,
                                                "Cannot open or parse PLY header: " + path.string(),
                                                std::nullopt});
        return result;
    }

    std::vector<std::string> requiredProperties = {"x", "y", "z",
                                                   "scale_0", "scale_1", "scale_2",
                                                   "rot_0", "rot_1", "rot_2", "rot_3",
                                                   "opacity",
                                                   "f_dc_0", "f_dc_1", "f_dc_2"};

    result.summary.gaussianCount = header.vertexCount;
    for (const auto &prop : requiredProperties)
    {
        auto it = std::find_if(header.properties.begin(), header.properties.end(),
                               [&prop](const PLYProperty &p)
                               { return p.name == prop; });
        if (it == header.properties.end())
        {
            result.summary.errorCount++;
            result.issues.push_back(ValidationIssue{Severity::ERROR,
                                                    IssueCode::MISSING_PROPERTY,
                                                    "Required property missing: " + prop,
                                                    std::nullopt});
        }
    }
    int shcount{0};
    for (const auto &prop : header.properties)
    {
        if (prop.name.rfind("f_rest_", 0) == 0 || prop.name.rfind("f_dc_", 0) == 0)
        {
            shcount++;
        }
    }
    int coeffs_per_color = 0;
    int degree = 0;
    if (shcount % 3 == 0 && shcount > 0)
    {
        coeffs_per_color = shcount / 3;
        degree = static_cast<int>(std::round(std::sqrt(coeffs_per_color) - 1));
        if (degree < 0)
        {
            result.summary.errorCount++;
            result.issues.push_back(ValidationIssue{Severity::ERROR,
                                                    IssueCode::UNEXPECTED_SH_DEGREE,
                                                    "Unexpected SH degree: " + std::to_string(degree),
                                                    std::nullopt});
            return result;
        }
        if (degree > 3)
        {
            result.summary.errorCount++;
            result.issues.push_back(ValidationIssue{Severity::ERROR,
                                                    IssueCode::UNEXPECTED_SH_DEGREE,
                                                    "Unexpected SH degree: " + std::to_string(degree),
                                                    std::nullopt});
            return result;
        }
        result.summary.shDegree = degree;
    }
    else
    {
        result.summary.errorCount++;
        result.issues.push_back(ValidationIssue{Severity::ERROR,
                                                IssueCode::UNEXPECTED_SH_DEGREE,
                                                "Unexpected number of SH coefficients: " + std::to_string(shcount),
                                                std::nullopt});
        return result;
    }
    return result;
}