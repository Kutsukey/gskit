#include <gskit/cli.hpp>

#include <filesystem>
#include <iostream>
#include <string>

#include <gskit/ply_reader.hpp>
#include <gskit/validation.hpp>

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

        void printIssues(const ValidationResult &result)
        {
            for (const auto &issue : result.issues)
            {
                std::cout << "Severity: " << (issue.sev == Severity::ERROR ? "ERROR" : "WARNING")
                          << ", Issue: " << static_cast<int>(issue.issue)
                          << ", Message: " << issue.message;
                if (issue.gaussian_index)
                {
                    std::cout << ", Gaussian Index: " << *issue.gaussian_index;
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
            return EXIT_OK;
        }

        void printUsage()
        {
            std::cerr << "Usage: gskit validate <file.ply> [--strict]\n";
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

        std::cerr << "Unknown command: " << command << '\n';
        printUsage();
        return EXIT_ERROR;
    }
} // namespace gskit

int main(int argc, char const *argv[])
{
    return gskit::runCli(argc, argv);
}
