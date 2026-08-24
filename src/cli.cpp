#include <gskit/cli.hpp>

#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>

#include <gskit/ply_reader.hpp>
#include <gskit/ply_writer.hpp>
#include <gskit/validation.hpp>

namespace gskit
{
    namespace
    {
        constexpr int EXIT_OK = 0;
        constexpr int EXIT_WARNINGS = 1;
        constexpr int EXIT_ERROR = 2;
        constexpr std::string_view VERSION = "0.1.0";

        void printGaussianPreview(const std::filesystem::path &path)
        {
            PLYReader reader{};
            PLYHeader header{};

            if (!reader.open(path) || !reader.readHeader(header))
            {
                std::cerr << "Failed to open or read header from: " << path.string() << '\n';
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
            case IssueCode::FILE_CANNOT_OPEN: return "FILE_CANNOT_OPEN";
            case IssueCode::INVALID_FILE_FORMAT: return "INVALID_FILE_FORMAT";
            case IssueCode::MISSING_PROPERTY: return "MISSING_PROPERTY";
            case IssueCode::NON_FINITE_POSITION: return "NON_FINITE_POSITION";
            case IssueCode::NON_FINITE_SCALE: return "NON_FINITE_SCALE";
            case IssueCode::INVALID_SCALE: return "INVALID_SCALE";
            case IssueCode::ANISOTROPIC_SCALE: return "ANISOTROPIC_SCALE";
            case IssueCode::NON_FINITE_ROTATION: return "NON_FINITE_ROTATION";
            case IssueCode::ZERO_QUATERNION: return "ZERO_QUATERNION";
            case IssueCode::NON_NORMALIZED_QUATERNION: return "NON_NORMALIZED_QUATERNION";
            case IssueCode::NON_FINITE_OPACITY: return "NON_FINITE_OPACITY";
            case IssueCode::INVALID_OPACITY: return "INVALID_OPACITY";
            case IssueCode::DEAD_GAUSSIAN: return "DEAD_GAUSSIAN";
            case IssueCode::NON_FINITE_SH: return "NON_FINITE_SH";
            case IssueCode::UNEXPECTED_SH_DEGREE: return "UNEXPECTED_SH_DEGREE";
            case IssueCode::INVALID_COLOR: return "INVALID_COLOR";
            case IssueCode::INVALID_BOUNDS: return "INVALID_BOUNDS";
            case IssueCode::VERTEX_COUNT_MISMATCH: return "VERTEX_COUNT_MISMATCH";
            default: return "UNKNOWN_ISSUE_CODE";
            }
        }

        std::string severityToString(Severity sev)
        {
            switch (sev)
            {
            case Severity::WARNING: return "WARNING";
            case Severity::ERROR: return "ERROR";
            default: return "UNKNOWN_SEVERITY";
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

        void printGlobalHelp()
        {
            std::cout << "gskit - 3D Gaussian Splatting validation, sanitization and inspection toolkit\n\n"
                      << "Usage:\n"
                      << "  gskit <command> [arguments] [options]\n\n"
                      << "Commands:\n"
                      << "  validate    Verify PLY integrity, bounds, and GS data invariants\n"
                      << "  sanitize    Filter invalid/corrupt Gaussians and write a clean PLY\n"
                      << "  info        Output structured JSON metadata and validation summary\n"
                      << "  preview     Print the first 5 Gaussians from a PLY file\n\n"
                      << "Global Options:\n"
                      << "  -h, --help     Show help information\n"
                      << "  -v, --version  Show version information\n\n"
                      << "Run 'gskit <command> --help' for command-specific options.\n";
        }

        void printValidateHelp()
        {
            std::cout << "Usage:\n"
                      << "  gskit validate <file.ply> [options]\n\n"
                      << "Options:\n"
                      << "  --strict       Treat warnings as errors\n"
                      << "  -h, --help     Show this help message\n";
        }

        void printSanitizeHelp()
        {
            std::cout << "Usage:\n"
                      << "  gskit sanitize <input.ply> <output.ply> [options]\n"
                      << "  gskit sanitize <input.ply> -o <output.ply> [options]\n\n"
                      << "Options:\n"
                      << "  -o, --output <file>  Target clean PLY output path\n"
                      << "  --drop-ghosts        Also prune nearly invisible Gaussians (alpha < 1e-4)\n"
                      << "  -h, --help           Show this help message\n";
        }

        void printInfoHelp()
        {
            std::cout << "Usage:\n"
                      << "  gskit info <file.ply> [options]\n\n"
                      << "Options:\n"
                      << "  -h, --help     Show this help message\n";
        }

        void printPreviewHelp()
        {
            std::cout << "Usage:\n"
                      << "  gskit preview <file.ply>\n";
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
                return options.strict ? EXIT_ERROR : EXIT_WARNINGS;
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
                std::cerr << "Failed to open file: " << path.string() << '\n';
                return EXIT_ERROR;
            }
            if (!reader.readHeader(header))
            {
                std::cerr << "Failed to read header from file: " << path.string() << '\n';
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
            }

            PLYWriter writer{};
            if (!writer.open(outputPath))
            {
                std::cerr << "Failed to open output file: " << outputPath.string() << '\n';
                return EXIT_ERROR;
            }
            if (!writer.writeHeader(header, validCount))
            {
                std::cerr << "Failed to write header to output file: " << outputPath.string() << '\n';
                return EXIT_ERROR;
            }
            if (!writer.writeVertex(cleanBytes.data(), cleanBytes.size()))
            {
                std::cerr << "Failed to write Gaussian data to output file: " << outputPath.string() << '\n';
                return EXIT_ERROR;
            }
            writer.close();

            std::cout << "Sanitization complete:\n";
            std::cout << "  Input Gaussians:  " << header.vertexCount << "\n";
            std::cout << "  Valid (Kept):     " << validCount << "\n";
            std::cout << "  Removed:          " << (header.vertexCount - validCount) << "\n";
            std::cout << "Output saved to: " << outputPath.string() << "\n";

            return EXIT_OK;
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
                jsonOutput += "      \"issue\": \"" + issueCodeToString(result.issues[i].issue) + "\",\n";
                jsonOutput += "      \"severity\": \"" + severityToString(result.issues[i].sev) + "\",\n";
                jsonOutput += "      \"message\": \"" + result.issues[i].message + "\"";
                if (result.issues[i].gaussianIndex)
                {
                    jsonOutput += ",\n      \"gaussianIndex\": " + std::to_string(*result.issues[i].gaussianIndex);
                }
                jsonOutput += "\n    }";
            }
            jsonOutput += "\n  ]\n";
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
        if (argc < 2)
        {
            printGlobalHelp();
            return EXIT_ERROR;
        }

        std::string_view command{argv[1]};

        if (command == "--help" || command == "-h")
        {
            printGlobalHelp();
            return EXIT_OK;
        }
        
        if (command == "--version" || command == "-v")
        {
            std::cout << "gskit version " << VERSION << "\n";
            return EXIT_OK;
        }

        if (command == "validate")
        {
            std::filesystem::path path{};
            ValidationOptions options{};

            for (int i = 2; i < argc; ++i)
            {
                std::string_view arg{argv[i]};
                if (arg == "-h" || arg == "--help")
                {
                    printValidateHelp();
                    return EXIT_OK;
                }
                else if (arg == "--strict")
                {
                    options.strict = true;
                }
                else if (path.empty() && arg[0] != '-')
                {
                    path = arg;
                }
                else
                {
                    std::cerr << "Unknown option: " << arg << "\n\n";
                    printValidateHelp();
                    return EXIT_ERROR;
                }
            }

            if (path.empty())
            {
                std::cerr << "Error: No input file specified.\n\n";
                printValidateHelp();
                return EXIT_ERROR;
            }

            return runValidate(path, options);
        }

        if (command == "sanitize")
        {
            std::filesystem::path inputPath{};
            std::filesystem::path outputPath{};
            ValidationOptions options{};

            for (int i = 2; i < argc; ++i)
            {
                std::string_view arg{argv[i]};
                if (arg == "-h" || arg == "--help")
                {
                    printSanitizeHelp();
                    return EXIT_OK;
                }
                else if (arg == "-o" || arg == "--output")
                {
                    if (i + 1 < argc)
                    {
                        outputPath = argv[++i];
                    }
                    else
                    {
                        std::cerr << "Error: " << arg << " requires a file path argument.\n\n";
                        printSanitizeHelp();
                        return EXIT_ERROR;
                    }
                }
                else if (arg == "--drop-ghosts" || arg == "--drop-dead")
                {
                    options.dropDeadGaussians = true;
                }
                else if (inputPath.empty() && arg[0] != '-')
                {
                    inputPath = arg;
                }
                else if (outputPath.empty() && arg[0] != '-')
                {
                    outputPath = arg;
                }
                else
                {
                    std::cerr << "Unknown option: " << arg << "\n\n";
                    printSanitizeHelp();
                    return EXIT_ERROR;
                }
            }

            if (inputPath.empty())
            {
                std::cerr << "Error: Input file must be specified.\n\n";
                printSanitizeHelp();
                return EXIT_ERROR;
            }

            if (outputPath.empty())
            {
                std::cerr << "Error: Output file must be specified (via positional argument or -o / --output).\n\n";
                printSanitizeHelp();
                return EXIT_ERROR;
            }

            return runSanitize(inputPath, outputPath, options);
        }

        if (command == "preview")
        {
            std::filesystem::path path{};

            for (int i = 2; i < argc; ++i)
            {
                std::string_view arg{argv[i]};
                if (arg == "-h" || arg == "--help")
                {
                    printPreviewHelp();
                    return EXIT_OK;
                }
                else if (path.empty() && arg[0] != '-')
                {
                    path = arg;
                }
                else
                {
                    std::cerr << "Unknown option: " << arg << "\n\n";
                    printPreviewHelp();
                    return EXIT_ERROR;
                }
            }

            if (path.empty())
            {
                std::cerr << "Error: No input file specified.\n\n";
                printPreviewHelp();
                return EXIT_ERROR;
            }

            printGaussianPreview(path);
            return EXIT_OK;
        }

        if (command == "info")
        {
            std::filesystem::path path{};
            ValidationOptions options{};

            for (int i = 2; i < argc; ++i)
            {
                std::string_view arg{argv[i]};
                if (arg == "-h" || arg == "--help")
                {
                    printInfoHelp();
                    return EXIT_OK;
                }
                else if (path.empty() && arg[0] != '-')
                {
                    path = arg;
                }
                else
                {
                    std::cerr << "Unknown option: " << arg << "\n\n";
                    printInfoHelp();
                    return EXIT_ERROR;
                }
            }

            if (path.empty())
            {
                std::cerr << "Error: No input file specified.\n\n";
                printInfoHelp();
                return EXIT_ERROR;
            }

            return runInfo(path, options);
        }

        std::cerr << "Unknown command: " << command << "\n\n";
        printGlobalHelp();
        return EXIT_ERROR;
    }
} // namespace gskit

int main(int argc, char const *argv[])
{
    return gskit::runCli(argc, argv);
}