#include "cli.h"

#include <iostream>

namespace marec {

CliOptions Cli::parse(int argc, char* argv[])
{
    CliOptions opts;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            opts.help = true;
        } else if (arg == "--dry-run") {
            opts.dryRun = true;
        } else if (arg == "--all-tracks") {
            opts.allTracks = true;
        } else if (arg == "--rename-file") {
            opts.renameFile = true;
        } else if (arg == "--export") {
            opts.exportConfig.enabled = true;
        } else if (arg == "--output-dir" && i + 1 < argc) {
            opts.exportConfig.outputDir = argv[++i];
        } else if (arg == "--format" && i + 1 < argc) {
            opts.exportConfig.fileType = argv[++i];
        } else if (arg == "--bit-depth" && i + 1 < argc) {
            opts.exportConfig.bitDepth = std::stoi(argv[++i]);
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            opts.help = true;
        }
    }

    return opts;
}

void Cli::printHelp()
{
    std::cout <<
R"(marec - Pro Tools clip renaming tool (Audi'Art)

Usage: marec [options]

Options:
  --dry-run           Preview renaming without executing
  --export            Export clips as files after renaming
  --output-dir PATH   Export destination (default: session folder)
  --format FORMAT     Export format: wav|aiff (default: wav)
  --bit-depth DEPTH   Export bit depth: 16|24|32 (default: 24)
  --rename-file       Also rename source audio files (default: false)
  --all-tracks        Skip interactive selection, process all audio tracks
  --help, -h          Show this help message
)";
}

} // namespace marec
