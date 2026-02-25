#pragma once

#include "session_data.h"
#include <string>
#include <vector>

namespace marec {

class Cli {
public:
    static CliOptions parse(int argc, char* argv[]);
    static void printHelp();
};

} // namespace marec
