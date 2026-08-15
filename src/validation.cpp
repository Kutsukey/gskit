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
    size_t index = 0;

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

        GaussianData data;
        while (reader.readGaussianData(data))
        {
            if (options.checkFinite)
            {
                if (!std::isfinite(data.position.x) || !std::isfinite(data.position.y) || !std::isfinite(data.position.z))
                {
                    result.summary.errorCount++;
                    result.issues.push_back(ValidationIssue{Severity::ERROR,
                                                            IssueCode::NON_FINITE_POSITION,
                                                            "Non-finite position values",
                                                            index});
                }
                if (!std::isfinite(data.scale.x) || !std::isfinite(data.scale.y) || !std::isfinite(data.scale.z))
                {
                    result.summary.warningCount++;
                    result.issues.push_back(ValidationIssue{Severity::WARNING,
                                                            IssueCode::NON_FINITE_SCALE,
                                                            "Non-finite scale values",
                                                            index});
                }
                if (!std::isfinite(data.rotation.x) || !std::isfinite(data.rotation.y) || !std::isfinite(data.rotation.z) || !std::isfinite(data.rotation.w))
                {
                    result.summary.warningCount++;
                    result.issues.push_back(ValidationIssue{Severity::WARNING,
                                                            IssueCode::NON_FINITE_ROTATION,
                                                            "Non-finite rotation values",
                                                            index});
                }
                if (!std::isfinite(data.opacity))
                {
                    result.summary.warningCount++;
                    result.issues.push_back(ValidationIssue{Severity::WARNING,
                                                            IssueCode::NON_FINITE_OPACITY,
                                                            "Non-finite opacity value",
                                                            index});
                }
            }

            if (options.checkScale)
            {
                if (data.scale.x > 10.0f || data.scale.x < -10.0f || data.scale.y > 10.0f || data.scale.y < -10.0f || data.scale.z > 10.0f || data.scale.z < -10.0f)
                {
                    result.summary.warningCount++;
                    result.issues.push_back(ValidationIssue{Severity::WARNING,
                                                            IssueCode::INVALID_SCALE,
                                                            "Scale value out of reasonable range (raw > 10 or < -10)",
                                                            index});
                }
            }

            if (options.checkRotation)
            {
                float magnitude = std::sqrt(data.rotation.x * data.rotation.x +
                                            data.rotation.y * data.rotation.y +
                                            data.rotation.z * data.rotation.z +
                                            data.rotation.w * data.rotation.w);

                float deviation = std::abs(magnitude - 1.0f);
                if (deviation > 0.5f)
                {
                    result.summary.errorCount++;
                    result.issues.push_back(ValidationIssue{Severity::ERROR,
                                                            IssueCode::NON_NORMALIZED_QUATERNION,
                                                            "Quaternion magnitude far from 1.0 (deviation: " + std::to_string(deviation) + ")",
                                                            index});
                }
                else if (deviation > 0.05f)
                {
                    result.summary.warningCount++;
                    result.issues.push_back(ValidationIssue{Severity::WARNING,
                                                            IssueCode::NON_NORMALIZED_QUATERNION,
                                                            "Quaternion magnitude slightly off from 1.0 (deviation: " + std::to_string(deviation) + ")",
                                                            index});
                }
            }

            index++;

            if (options.strict && result.summary.errorCount > 0)
            {
                break;
            }
        }
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

    if (options.checkVertexCount)
    {
        if (index != header.vertexCount)
        {
            result.summary.errorCount++;
            result.issues.push_back(ValidationIssue{Severity::ERROR,
                                                    IssueCode::VERTEX_COUNT_MISMATCH,
                                                    "Vertex count mismatch: header says " + std::to_string(header.vertexCount) +
                                                        " but read " + std::to_string(index),
                                                    std::nullopt});
            return result;
        }
    }
    return result;
}