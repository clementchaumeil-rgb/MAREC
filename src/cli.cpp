#include "cli.h"

#include <iostream>
#include <sstream>

namespace marec {

static std::vector<std::string> splitComma(const std::string& s)
{
    std::vector<std::string> result;
    std::istringstream stream(s);
    std::string token;
    while (std::getline(stream, token, ',')) {
        // Trim whitespace
        size_t start = token.find_first_not_of(" \t");
        size_t end = token.find_last_not_of(" \t");
        if (start != std::string::npos) {
            result.push_back(token.substr(start, end - start + 1));
        }
    }
    return result;
}

static Step parseStep(const std::string& s)
{
    if (s == "connect") return Step::Connect;
    if (s == "tracks")  return Step::Tracks;
    if (s == "markers") return Step::Markers;
    if (s == "preview") return Step::Preview;
    if (s == "rename")  return Step::Rename;
    if (s == "export")  return Step::Export;
    if (s == "import")  return Step::ImportMarkers;
    return Step::None;
}

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
        } else if (arg == "--json") {
            opts.jsonMode = true;
        } else if (arg == "--step" && i + 1 < argc) {
            opts.step = parseStep(argv[++i]);
        } else if (arg == "--tracks" && i + 1 < argc) {
            opts.trackNames = splitComma(argv[++i]);
        } else if (arg == "--export") {
            opts.exportConfig.enabled = true;
        } else if (arg == "--output-dir" && i + 1 < argc) {
            opts.exportConfig.outputDir = argv[++i];
        } else if (arg == "--format" && i + 1 < argc) {
            opts.exportConfig.fileType = argv[++i];
        } else if (arg == "--bit-depth" && i + 1 < argc) {
            opts.exportConfig.bitDepth = std::stoi(argv[++i]);
        } else if (arg == "--import-markers" && i + 1 < argc) {
            opts.importConfig.filePath = argv[++i];
            opts.importConfig.enabled = true;
        } else if (arg == "--clear-markers") {
            opts.importConfig.clearMarkers = true;
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

Interactive mode (default):
  --dry-run           Preview renaming without executing
  --all-tracks        Skip interactive selection, process all audio tracks
  --rename-file       Also rename source audio files (default: false)
  --export            Export clips as files after renaming
  --output-dir PATH   Export destination (default: session folder)
  --format FORMAT     Export format: wav|aiff (default: wav)
  --bit-depth DEPTH   Export bit depth: 16|24|32 (default: 24)
  --help, -h          Show this help message

Import markers from text export:
  --import-markers FILE   Import markers from Pro Tools text export file
  --clear-markers         Clear all existing markers before importing
  --dry-run               Preview import without creating markers

JSON mode (for GUI integration):
  --json --step connect              Connect and return session info
  --json --step tracks               List audio tracks
  --json --step markers              List markers
  --json --step preview --tracks T   Preview rename actions for tracks T
  --json --step rename --tracks T    Execute renaming for tracks T
  --json --step export --tracks T    Export clips for tracks T
  --json --step import --import-markers FILE   Import markers from text export

  --tracks "Track 1,Track 2"   Comma-separated track names
)";
}

} // namespace marec
