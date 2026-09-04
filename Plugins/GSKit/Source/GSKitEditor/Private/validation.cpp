#include "validation.hpp"
#include "ply_reader.hpp"
#include "ply_writer.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace gskit
{
    constexpr float SH_C0 = 0.28209479177387814f; // 1 / (2 * sqrt(pi))

    ValidationResult ValidateAsset(const std::filesystem::path &path, const ValidationOptions &options)
    {
        PLYReader reader;
        PLYHeader header;
        ValidationResult result{};

        if (!reader.open(path) || !reader.readHeader(header))
        {
            result.summary.errorCount++;
            result.issues.push_back(ValidationIssue{
                Severity::ERROR,
                IssueCode::FILE_CANNOT_OPEN,
                "Cannot open or parse PLY header: " + path.string(),
                std::nullopt});
            return result;
        }

        std::vector<std::string> requiredProperties = {
            "x", "y", "z",
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
                result.issues.push_back(ValidationIssue{
                    Severity::ERROR,
                    IssueCode::MISSING_PROPERTY,
                    "Required property missing: " + prop,
                    std::nullopt});
            }
        }

        int shcount = 0;
        for (const auto &prop : header.properties)
        {
            if (prop.name.rfind("f_rest_", 0) == 0 || prop.name.rfind("f_dc_", 0) == 0)
            {
                shcount++;
            }
        }

        if (shcount % 3 != 0 || shcount <= 0)
        {
            result.summary.errorCount++;
            result.issues.push_back(ValidationIssue{
                Severity::ERROR,
                IssueCode::UNEXPECTED_SH_DEGREE,
                "Unexpected number of SH coefficients: " + std::to_string(shcount),
                std::nullopt});
            return result;
        }

        int coeffs_per_color = shcount / 3;
        int degree = static_cast<int>(std::round(std::sqrt(coeffs_per_color) - 1));
        if (degree < 0 || degree > 3)
        {
            result.summary.errorCount++;
            result.issues.push_back(ValidationIssue{
                Severity::ERROR,
                IssueCode::UNEXPECTED_SH_DEGREE,
                "Unexpected SH degree: " + std::to_string(degree),
                std::nullopt});
            return result;
        }
        result.summary.shDegree = degree;

        Vec3 minBound{std::numeric_limits<float>::infinity(),
                      std::numeric_limits<float>::infinity(),
                      std::numeric_limits<float>::infinity()};
        Vec3 maxBound{-std::numeric_limits<float>::infinity(),
                      -std::numeric_limits<float>::infinity(),
                      -std::numeric_limits<float>::infinity()};

        size_t index = 0;
        GaussianData data;

        while (reader.readGaussianData(data))
        {
            if (options.checkFinite)
            {
                if (!std::isfinite(data.position.x) || !std::isfinite(data.position.y) || !std::isfinite(data.position.z))
                {
                    result.summary.errorCount++;
                    result.issues.push_back(ValidationIssue{
                        Severity::ERROR,
                        IssueCode::NON_FINITE_POSITION,
                        "Non-finite position values",
                        index});
                }
                else
                {
                    minBound.x = std::min(minBound.x, data.position.x);
                    minBound.y = std::min(minBound.y, data.position.y);
                    minBound.z = std::min(minBound.z, data.position.z);
                    maxBound.x = std::max(maxBound.x, data.position.x);
                    maxBound.y = std::max(maxBound.y, data.position.y);
                    maxBound.z = std::max(maxBound.z, data.position.z);
                }

                if (!std::isfinite(data.scale.x) || !std::isfinite(data.scale.y) || !std::isfinite(data.scale.z))
                {
                    result.summary.warningCount++;
                    result.issues.push_back(ValidationIssue{
                        Severity::WARNING,
                        IssueCode::NON_FINITE_SCALE,
                        "Non-finite scale values",
                        index});
                }

                if (!std::isfinite(data.rotation.x) || !std::isfinite(data.rotation.y) ||
                    !std::isfinite(data.rotation.z) || !std::isfinite(data.rotation.w))
                {
                    result.summary.warningCount++;
                    result.issues.push_back(ValidationIssue{
                        Severity::WARNING,
                        IssueCode::NON_FINITE_ROTATION,
                        "Non-finite rotation values",
                        index});
                }

                if (!std::isfinite(data.opacity))
                {
                    result.summary.warningCount++;
                    result.issues.push_back(ValidationIssue{
                        Severity::WARNING,
                        IssueCode::NON_FINITE_OPACITY,
                        "Non-finite opacity value",
                        index});
                }
            }

            if (options.checkRotation)
            {
                float sqNorm = data.rotation.x * data.rotation.x +
                               data.rotation.y * data.rotation.y +
                               data.rotation.z * data.rotation.z +
                               data.rotation.w * data.rotation.w;

                if (sqNorm < 1e-12f)
                {
                    result.summary.errorCount++;
                    result.issues.push_back(ValidationIssue{
                        Severity::ERROR,
                        IssueCode::ZERO_QUATERNION,
                        "Quaternion is zero, cannot represent a valid rotation",
                        index});
                }
                else
                {
                    float magnitude = std::sqrt(sqNorm);
                    float deviation = std::abs(magnitude - 1.0f);
                    if (deviation > 0.5f)
                    {
                        result.summary.errorCount++;
                        result.issues.push_back(ValidationIssue{
                            Severity::ERROR,
                            IssueCode::NON_NORMALIZED_QUATERNION,
                            "Quaternion magnitude far from 1.0 (deviation: " + std::to_string(deviation) + ")",
                            index});
                    }
                    else if (deviation > 0.05f)
                    {
                        result.summary.warningCount++;
                        result.issues.push_back(ValidationIssue{
                            Severity::WARNING,
                            IssueCode::NON_NORMALIZED_QUATERNION,
                            "Quaternion magnitude slightly off from 1.0 (deviation: " + std::to_string(deviation) + ")",
                            index});
                    }
                }
            }

            if (options.checkScale)
            {
                float activated_x = std::exp(data.scale.x);
                float activated_y = std::exp(data.scale.y);
                float activated_z = std::exp(data.scale.z);

                if (activated_x > 1e6f || activated_y > 1e6f || activated_z > 1e6f ||
                    activated_x < 1e-8f || activated_y < 1e-8f || activated_z < 1e-8f)
                {
                    result.summary.warningCount++;
                    result.issues.push_back(ValidationIssue{
                        Severity::WARNING,
                        IssueCode::INVALID_SCALE,
                        "Activated scale value out of reasonable range (activated: " +
                            std::to_string(activated_x) + ", " + std::to_string(activated_y) + ", " + std::to_string(activated_z) + ")",
                        index});
                }

                float min_s = std::min({activated_x, activated_y, activated_z});
                float max_s = std::max({activated_x, activated_y, activated_z});
                if (min_s > 0.0f && (max_s / min_s) > 1000.0f)
                {
                    result.summary.needleCount++;
                    result.summary.warningCount++;
                    result.issues.push_back(ValidationIssue{
                        Severity::WARNING,
                        IssueCode::ANISOTROPIC_SCALE,
                        "Activated scale value has extreme aspect ratio: " + std::to_string(max_s / min_s),
                        index});
                }
            }

            if (options.checkOpacity)
            {
                float activated_opacity = 1.0f / (1.0f + std::exp(-data.opacity));
                if (std::isfinite(activated_opacity))
                {
                    if (activated_opacity < 1e-4f)
                    {
                        result.summary.ghostCount++;
                        result.summary.warningCount++;
                        result.issues.push_back(ValidationIssue{
                            Severity::WARNING,
                            IssueCode::DEAD_GAUSSIAN,
                            "Activated opacity value is very low, gaussian might be dead (activated: " + std::to_string(activated_opacity) + ")",
                            index});
                    }
                    else if (activated_opacity > 1.0f || activated_opacity < 0.0f)
                    {
                        result.summary.warningCount++;
                        result.issues.push_back(ValidationIssue{
                            Severity::WARNING,
                            IssueCode::INVALID_OPACITY,
                            "Activated opacity value out of reasonable range (activated: " + std::to_string(activated_opacity) + ")",
                            index});
                    }
                }
                else
                {
                    result.summary.warningCount++;
                    result.issues.push_back(ValidationIssue{
                        Severity::WARNING,
                        IssueCode::INVALID_OPACITY,
                        "Activated opacity value is not finite",
                        index});
                }
            }

            if (options.checkSH)
            {
                if (data.shCoeffs.size() != static_cast<size_t>(shcount))
                {
                    result.summary.errorCount++;
                    result.issues.push_back(ValidationIssue{
                        Severity::ERROR,
                        IssueCode::UNEXPECTED_SH_DEGREE,
                        "Unexpected number of SH coefficients: " + std::to_string(data.shCoeffs.size()),
                        index});
                }
                else
                {
                    for (const auto &coeff : data.shCoeffs)
                    {
                        if (!std::isfinite(coeff))
                        {
                            result.summary.errorCount++;
                            result.issues.push_back(ValidationIssue{
                                Severity::ERROR,
                                IssueCode::NON_FINITE_SH,
                                "Non-finite SH coefficient",
                                index});
                            break;
                        }
                    }

                    if (data.shCoeffs.size() >= 3)
                    {
                        float r = data.shCoeffs[0] * SH_C0 + 0.5f;
                        float g = data.shCoeffs[1] * SH_C0 + 0.5f;
                        float b = data.shCoeffs[2] * SH_C0 + 0.5f;

                        if (r < -0.5f || g < -0.5f || b < -0.5f || r > 5.0f || g > 5.0f || b > 5.0f)
                        {
                            result.summary.warningCount++;
                            result.issues.push_back(ValidationIssue{
                                Severity::WARNING,
                                IssueCode::INVALID_COLOR,
                                "Base RGB value out of normal range (" +
                                    std::to_string(r) + ", " + std::to_string(g) + ", " + std::to_string(b) + ")",
                                index});
                        }
                    }
                }
            }

            index++;

            if (options.strict && result.summary.errorCount > 0)
            {
                break;
            }
        }

        if (index > 0)
        {
            result.summary.bboxMin = minBound;
            result.summary.bboxMax = maxBound;
        }

        if (options.checkVertexCount && index != header.vertexCount)
        {
            result.summary.errorCount++;
            result.issues.push_back(ValidationIssue{
                Severity::ERROR,
                IssueCode::VERTEX_COUNT_MISMATCH,
                "Vertex count mismatch: header says " + std::to_string(header.vertexCount) +
                    " but read " + std::to_string(index),
                std::nullopt});
        }

        return result;
    }

    bool isValidGaussian(const GaussianData &data, const ValidationOptions &options)
    {
        if (options.checkFinite)
        {
            if (!std::isfinite(data.position.x) || !std::isfinite(data.position.y) || !std::isfinite(data.position.z))
                return false;
            if (!std::isfinite(data.scale.x) || !std::isfinite(data.scale.y) || !std::isfinite(data.scale.z))
                return false;
            if (!std::isfinite(data.rotation.x) || !std::isfinite(data.rotation.y) ||
                !std::isfinite(data.rotation.z) || !std::isfinite(data.rotation.w))
                return false;
            if (!std::isfinite(data.opacity))
                return false;
        }

        if (options.checkRotation)
        {
            float sqNorm = data.rotation.x * data.rotation.x +
                           data.rotation.y * data.rotation.y +
                           data.rotation.z * data.rotation.z +
                           data.rotation.w * data.rotation.w;

            if (sqNorm < 1e-12f)
                return false;

            float magnitude = std::sqrt(sqNorm);
            if (std::abs(magnitude - 1.0f) > 0.5f)
                return false;
        }

        if (options.checkSH)
        {
            for (const auto &coeff : data.shCoeffs)
            {
                if (!std::isfinite(coeff))
                    return false;
            }
        }

        if(options.dropDeadGaussians)
        {
            float activated_opacity = 1.0f / (1.0f + std::exp(-data.opacity));
            if (activated_opacity < 1e-4f)
                return false;
        }

        return true;
    }
}
