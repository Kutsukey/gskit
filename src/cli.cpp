#include <gskit/cli.hpp>

#include <filesystem>
#include <iostream>
#include <string>

#include <gskit/ply_reader.hpp>
#include <gskit/validation.hpp>
#include <gskit/ply_writer.hpp>

namespace gskit
{
    namespace
    {
        constexpr int EXIT_OK = 0;
        constexpr int EXIT_WARNINGS = 1;
        constexpr int EXIT_ERROR = 2;

        void printGaussianPreview(const std::filesystem::path &path)
        {
            PLYReader reader{};
            PLYHeader header{};

            if (!reader.open(path) || !reader.readHeader(header))
            {
                std::cerr << "Failed to open or read header from: " << path << '\n';
                return;
            }

            constexpr size_t previewCount = 5;
            for (size_t i = 0; i < previewCount && i < header.vertexCount; ++i)
            {
                GaussianData data{};
                if (reader.readGaussianData(data))
                {
                    std::cout << "Gaussian " << i << ": Position(" << data.position.x << ", "
                              << data.position.y << ", " << data.position.z << "), "
                              << "Scale(" << data.scale.x << ", " << data.scale.y << ", "
                              << data.scale.z << "), "
                              << "Rotation(" << data.rotation.w << ", " << data.rotation.x << ", "
                              << data.rotation.y << ", " << data.rotation.z << "), "
                              << "Opacity(" << data.opacity << ")\n";
                }
                else
                {
                    std::cerr << "Failed to read Gaussian data for index " << i << ".\n";
                    return;
                }
            }
        }

        std::string issueCodeToString(IssueCode code)
        {
            switch (code)
            {
            case IssueCode::FILE_CANNOT_OPEN:
                return "FILE_CANNOT_OPEN";
            case IssueCode::INVALID_FILE_FORMAT:
                return "INVALID_FILE_FORMAT";
            case IssueCode::MISSING_PROPERTY:
                return "MISSING_PROPERTY";
            case IssueCode::NON_FINITE_POSITION:
                return "NON_FINITE_POSITION";
            case IssueCode::NON_FINITE_SCALE:
                return "NON_FINITE_SCALE";
            case IssueCode::INVALID_SCALE:
                return "INVALID_SCALE";
            case IssueCode::NON_FINITE_ROTATION:
                return "NON_FINITE_ROTATION";
            case IssueCode::NON_NORMALIZED_QUATERNION:
                return "NON_NORMALIZED_QUATERNION";
            case IssueCode::NON_FINITE_OPACITY:
                return "NON_FINITE_OPACITY";
            case IssueCode::INVALID_OPACITY:
                return "INVALID_OPACITY";
            case IssueCode::NON_FINITE_SH:
                return "NON_FINITE_SH";
            case IssueCode::UNEXPECTED_SH_DEGREE:
                return "UNEXPECTED_SH_DEGREE";
            case IssueCode::INVALID_BOUNDS:
                return "INVALID_BOUNDS";
            case IssueCode::VERTEX_COUNT_MISMATCH:
                return "VERTEX_COUNT_MISMATCH";
            case IssueCode::DEAD_GAUSSIAN:
                return "DEAD_GAUSSIAN";
            case IssueCode::INVALID_COLOR:
                return "INVALID_COLOR";
            case IssueCode::ANISOTROPIC_SCALE:
                return "ANISOTROPIC_SCALE";
            case IssueCode::ZERO_QUATERNION:
                return "ZERO_QUATERNION";
            default:
                return "UNKNOWN_ISSUE_CODE";
            }
        }

        std::string severityToString(Severity sev)
        {
            switch (sev)
            {
            case Severity::WARNING:
                return "WARNING";
            case Severity::ERROR:
                return "ERROR";
            default:
                return "UNKNOWN_SEVERITY";
            }
        }
        void printIssues(const ValidationResult &result)
        {
            for (const auto &issue : result.issues)
            {
                std::cout << "Severity: " << severityToString(issue.sev)
                          << ", Issue: " << issueCodeToString(issue.issue)
                          << ", Message: " << issue.message;
                if (issue.gaussianIndex)
                {
                    std::cout << ", Gaussian Index: " << *issue.gaussianIndex;
                }
                std::cout << '\n';
            }
        }

        int runValidate(const std::filesystem::path &path, const ValidationOptions &options)
        {
            ValidationResult result{ValidateAsset(path, options)};

            if (result.summary.errorCount > 0)
            {
                std::cout << "Validation failed with " << result.summary.errorCount << " errors and "
                          << result.summary.warningCount << " warnings.\n";
                printIssues(result);
                return EXIT_ERROR;
            }

            if (result.summary.warningCount > 0)
            {
                std::cout << "Validation completed with " << result.summary.warningCount << " warnings.\n";
                printIssues(result);
                return EXIT_WARNINGS;
            }

            std::cout << result.summary.gaussianCount << " Gaussians validated successfully with SH degree "
                      << result.summary.shDegree << ".\n";
            std::cout << "Bounds: [" << result.summary.bboxMin.x << ", " << result.summary.bboxMin.y << ", " << result.summary.bboxMin.z
                      << "] to ["
                      << result.summary.bboxMax.x << ", " << result.summary.bboxMax.y << ", " << result.summary.bboxMax.z
                      << "]\n";
            return EXIT_OK;
        }

        int runSanitize(const std::filesystem::path &path, const std::filesystem::path &outputPath, const ValidationOptions &options)
        {
            PLYReader reader{};
            PLYHeader header{};
            if (!reader.open(path))
            {
                std::cerr << "Failed to open file: " << path << '\n';
                return EXIT_ERROR;
            }
            if (!reader.readHeader(header))
            {
                std::cerr << "Failed to read header from file: " << path << '\n';
                return EXIT_ERROR;
            }

            std::vector<char> cleanBytes;
            size_t validCount = 0;
            GaussianData data{};
            while (reader.readGaussianData(data))
            {
                if (isValidGaussian(data, options))
                {
                    const auto &rawBuffer = reader.getBuffer();
                    cleanBytes.insert(cleanBytes.end(), rawBuffer.begin(), rawBuffer.end());
                    ++validCount;
                }
                else
                {
                    std::cerr << "Invalid Gaussian data encountered. Skipping.\n";
                }
            }
            PLYWriter writer{};
            if (!writer.open(outputPath))
            {
                std::cerr << "Failed to open output file: " << outputPath << '\n';
                return EXIT_ERROR;
            }
            if (!writer.writeHeader(header, validCount))
            {
                std::cerr << "Failed to write header to output file: " << outputPath << '\n';
                return EXIT_ERROR;
            }
            if (!writer.writeVertex(cleanBytes.data(), cleanBytes.size()))
            {
                std::cerr << "Failed to write Gaussian data to output file: " << outputPath << '\n';
                return EXIT_ERROR;
            }
            writer.close();

            std::cout << "Sanitization complete:\n";
            std::cout << "  Input Gaussians:  " << header.vertexCount << "\n";
            std::cout << "  Valid (Kept):     " << validCount << "\n";
            std::cout << "  Removed:          " << (header.vertexCount - validCount) << "\n";
            std::cout << "Output saved to: " << outputPath << "\n";

            return EXIT_OK;
        }

        void printUsage()
        {
            std::cerr << "Usage: gskit <command> <file.ply> [--strict]\n";
            std::cerr << "Commands:\n";
            std::cerr << "  validate: Validates the specified PLY file.\n";
            std::cerr << "  preview: Previews the specified PLY file.\n";
            std::cerr << "  info: Displays information about the specified PLY file.\n";
            std::cerr << "  sanitize: Sanitizes the specified PLY file and saves to a new file.\n";
        }

        int runInfo(const std::filesystem::path &path, const ValidationOptions &options)
        {
            ValidationResult result{ValidateAsset(path, options)};

            std::string jsonOutput = "{\n";
            jsonOutput += "  \"file\": \"" + path.generic_string() + "\",\n";
            jsonOutput += "  \"gaussianCount\": " + std::to_string(result.summary.gaussianCount) + ",\n";
            jsonOutput += "  \"shDegree\": " + std::to_string(result.summary.shDegree) + ",\n";
            jsonOutput += "  \"errorCount\": " + std::to_string(result.summary.errorCount) + ",\n";
            jsonOutput += "  \"warningCount\": " + std::to_string(result.summary.warningCount) + ",\n";
            jsonOutput += "  \"ghostCount\": " + std::to_string(result.summary.ghostCount) + ",\n";
            jsonOutput += "  \"needleCount\": " + std::to_string(result.summary.needleCount) + ",\n";
            jsonOutput += "  \"bounds\": {\n";
            jsonOutput += "    \"min\": [" + std::to_string(result.summary.bboxMin.x) + ", " + std::to_string(result.summary.bboxMin.y) + ", " + std::to_string(result.summary.bboxMin.z) + "],\n";
            jsonOutput += "    \"max\": [" + std::to_string(result.summary.bboxMax.x) + ", " + std::to_string(result.summary.bboxMax.y) + ", " + std::to_string(result.summary.bboxMax.z) + "]\n";
            jsonOutput += "  },\n";
            jsonOutput += "  \"issues\": [\n";
            for (size_t i = 0; i < result.issues.size(); ++i)
            {
                if (i > 0)
                {
                    jsonOutput += ",\n";
                }
                jsonOutput += "    {\n";
                jsonOutput += " \"issue\": \"" + issueCodeToString(result.issues[i].issue) + "\",\n";
                jsonOutput += " \"severity\": \"" + severityToString(result.issues[i].sev) + "\",\n";
                jsonOutput += " \"message\": \"" + result.issues[i].message + "\"";
                if (result.issues[i].gaussianIndex)
                {
                    jsonOutput += ",\n \"gaussianIndex\": " + std::to_string(*result.issues[i].gaussianIndex);
                }
                jsonOutput += "\n    }";
            }
            jsonOutput += "  ]\n";
            jsonOutput += "}\n";
            std::cout << jsonOutput;
            if (result.summary.errorCount > 0)
            {
                return EXIT_ERROR;
            }
            if (result.summary.warningCount > 0)
            {
                return EXIT_WARNINGS;
            }
            return EXIT_OK;
        }

    } // namespace

    int runCli(int argc, char const *argv[])
    {
        if (argc < 3)
        {
            std::cerr << "At least 3 arguments needed. Given: " << argc << '\n';
            printUsage();
            return EXIT_ERROR;
        }

        std::string command{argv[1]};
        std::filesystem::path path{argv[2]};

        if (command == "validate")
        {
            ValidationOptions options{};

            if (argc == 4)
            {
                std::string flag{argv[3]};
                if (flag == "--strict")
                {
                    options.strict = true;
                }
                else
                {
                    std::cerr << "Unknown option: " << flag << '\n';
                    printUsage();
                    return EXIT_ERROR;
                }
            }

            return runValidate(path, options);
        }

        if (command == "preview")
        {
            printGaussianPreview(path);
            return EXIT_OK;
        }

        if (command == "info")
        {
            ValidationOptions options{};
            if (argc == 4)
            {
                std::string flag{argv[3]};
                if (flag == "--strict")
                {
                    options.strict = true;
                    return runInfo(path, options);
                }
                else
                {
                    std::cerr << "Unknown option: " << flag << '\n';
                    printUsage();
                    return EXIT_ERROR;
                }
            }

            return runInfo(path, options);
        }

        if (command == "sanitize")
        {
            if (argc < 4)
            {
                std::cerr << "Sanitize command requires an output file path.\n";
                printUsage();
                return EXIT_ERROR;
            }

            std::filesystem::path outputPath{argv[3]};
            ValidationOptions options{};

            if (argc == 5)
            {
                std::string flag{argv[4]};
                if (flag == "--strict")
                {
                    options.strict = true;
                }
                else
                {
                    std::cerr << "Unknown option: " << flag << '\n';
                    printUsage();
                    return EXIT_ERROR;
                }
            }

            return runSanitize(path, outputPath, options);
        }

        std::cerr << "Unknown command: " << command << '\n';
        printUsage();
        return EXIT_ERROR;
    }
} // namespace gskit

int main(int argc, char const *argv[])
{
    return gskit::runCli(argc, argv);
}
