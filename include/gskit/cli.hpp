#pragma once

namespace gskit
{
    /// Entry point for the gskit command-line interface.
    /// Parses arguments and dispatches to the appropriate subcommand.
    /// Returns the process exit code.
    int runCli(int argc, char const *argv[]);
} // namespace gskit
