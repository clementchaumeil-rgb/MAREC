import Foundation

// MARK: - CLI Output

struct CLIOutput {
    let stdout: String
    let stderr: String
    let exitCode: Int32
}

// MARK: - CLI Errors

enum CLIError: LocalizedError {
    case binaryNotFound
    case executionFailed(String)
    case decodingFailed(String)

    var errorDescription: String? {
        switch self {
        case .binaryNotFound:
            return "Le binaire marec est introuvable dans le bundle."
        case .executionFailed(let msg):
            return "Erreur CLI: \(msg)"
        case .decodingFailed(let msg):
            return "Erreur de decodage JSON: \(msg)"
        }
    }
}

// MARK: - CLI Service
// Launches the `marec` binary as a subprocess with --json --step flags

final class MarecCLIService {

    private let logger = MarecLogger.shared

    // MARK: - Binary Resolution

    /// Finds the `marec` binary inside the app bundle (Contents/Resources/bin/marec)
    /// Falls back to adjacent build directory for development.
    /// Override via MAREC_CLI_PATH environment variable (used by UI tests).
    private func binaryPath() throws -> String {
        // 0. Environment override (for UI tests)
        if let envPath = ProcessInfo.processInfo.environment["MAREC_CLI_PATH"],
           FileManager.default.isExecutableFile(atPath: envPath) {
            return envPath
        }

        // 1. Bundle path (production)
        if let bundled = Bundle.main.path(forResource: "marec", ofType: nil, inDirectory: "bin") {
            return bundled
        }

        // 2. Development fallback: look relative to the app bundle
        let bundlePath = Bundle.main.bundlePath
        let devPaths = [
            (bundlePath as NSString).deletingLastPathComponent + "/build/marec",
            (bundlePath as NSString).deletingLastPathComponent + "/marec",
            "/Users/avid/Downloads/MAREC/build/marec"
        ]

        for path in devPaths {
            if FileManager.default.isExecutableFile(atPath: path) {
                return path
            }
        }

        throw CLIError.binaryNotFound
    }

    // MARK: - Generic Runner

    /// Runs `marec --json --step <step>` with optional extra arguments.
    /// Returns CLIOutput with stdout, stderr, and exit code.
    ///
    /// - Parameter onProgress: Optional callback invoked with progress messages
    ///   parsed from stderr lines matching `PROGRESS: ...`. Called on the main actor.
    func run(step: String, extraArgs: [String] = [], onProgress: ((String) -> Void)? = nil) async throws -> CLIOutput {
        let binary = try binaryPath()
        logger.logCLIInvocation(step: step, args: extraArgs)

        return try await withCheckedThrowingContinuation { continuation in
            DispatchQueue.global(qos: .userInitiated).async { [logger] in
                let process = Process()
                process.executableURL = URL(fileURLWithPath: binary)

                var args = ["--json", "--step", step]
                args.append(contentsOf: extraArgs)
                process.arguments = args

                let stdoutPipe = Pipe()
                let stderrPipe = Pipe()
                process.standardOutput = stdoutPipe
                process.standardError = stderrPipe

                do {
                    // If progress callback provided, stream stderr lines in real-time
                    var stderrChunks = Data()
                    if let onProgress = onProgress {
                        var lineBuffer = Data()
                        stderrPipe.fileHandleForReading.readabilityHandler = { handle in
                            let chunk = handle.availableData
                            guard !chunk.isEmpty else { return }
                            stderrChunks.append(chunk)
                            lineBuffer.append(chunk)

                            // Parse complete lines from the buffer
                            while let range = lineBuffer.range(of: Data([0x0A])) { // newline
                                let lineData = lineBuffer.subdata(in: lineBuffer.startIndex..<range.lowerBound)
                                lineBuffer.removeSubrange(lineBuffer.startIndex...range.lowerBound)

                                if let line = String(data: lineData, encoding: .utf8),
                                   line.hasPrefix("PROGRESS: ") {
                                    let message = String(line.dropFirst("PROGRESS: ".count))
                                    DispatchQueue.main.async {
                                        onProgress(message)
                                    }
                                }
                            }
                        }
                    }

                    try process.run()

                    // Drain stdout in a background thread to avoid pipe deadlock.
                    // When CLI output exceeds the ~64KB pipe buffer, the CLI blocks
                    // on write. If we call waitUntilExit() first, both processes deadlock.
                    var outData = Data()
                    let readGroup = DispatchGroup()
                    readGroup.enter()
                    DispatchQueue.global().async {
                        outData = stdoutPipe.fileHandleForReading.readDataToEndOfFile()
                        readGroup.leave()
                    }

                    // If not using readabilityHandler, drain stderr in background too
                    if onProgress == nil {
                        readGroup.enter()
                        DispatchQueue.global().async {
                            stderrChunks = stderrPipe.fileHandleForReading.readDataToEndOfFile()
                            readGroup.leave()
                        }
                    }

                    process.waitUntilExit()

                    // Clean up readability handler and read any remaining data
                    if onProgress != nil {
                        stderrPipe.fileHandleForReading.readabilityHandler = nil
                        let remaining = stderrPipe.fileHandleForReading.readDataToEndOfFile()
                        stderrChunks.append(remaining)
                    }

                    readGroup.wait()

                    let outString = String(data: outData, encoding: .utf8) ?? ""
                    let errString = String(data: stderrChunks, encoding: .utf8) ?? ""

                    logger.logCLIOutput(
                        step: step,
                        stdout: outString,
                        stderr: errString,
                        exitCode: process.terminationStatus
                    )

                    if process.terminationStatus != 0 && outString.isEmpty {
                        continuation.resume(throwing: CLIError.executionFailed(
                            errString.isEmpty ? "Exit code \(process.terminationStatus)" : errString
                        ))
                    } else {
                        // CLI outputs JSON to stdout even on error (status: "error")
                        continuation.resume(returning: CLIOutput(
                            stdout: outString,
                            stderr: errString,
                            exitCode: process.terminationStatus
                        ))
                    }
                } catch {
                    logger.error("CLI process launch failed: \(error.localizedDescription)")
                    continuation.resume(throwing: CLIError.executionFailed(error.localizedDescription))
                }
            }
        }
    }

    // MARK: - Typed Step Methods

    func connect() async throws -> SessionResponse {
        let output = try await run(step: "connect")
        return try decode(output.stdout)
    }

    func tracks() async throws -> TracksResponse {
        let output = try await run(step: "tracks")
        return try decode(output.stdout)
    }

    func markers() async throws -> MarkersResponse {
        let output = try await run(step: "markers")
        return try decode(output.stdout)
    }

    func preview(trackNames: [String], onProgress: ((String) -> Void)? = nil) async throws -> PreviewResponse {
        var args: [String] = []
        if !trackNames.isEmpty {
            args += ["--tracks", trackNames.joined(separator: ",")]
        }
        let output = try await run(step: "preview", extraArgs: args, onProgress: onProgress)
        return try decode(output.stdout)
    }

    func rename(trackNames: [String], renameFile: Bool = false, onProgress: ((String) -> Void)? = nil) async throws -> RenameResponse {
        var args: [String] = []
        if !trackNames.isEmpty {
            args += ["--tracks", trackNames.joined(separator: ",")]
        }
        if renameFile {
            args.append("--rename-file")
        }
        let output = try await run(step: "rename", extraArgs: args, onProgress: onProgress)
        return try decode(output.stdout)
    }

    func export(
        trackNames: [String],
        outputDir: String? = nil,
        fileType: String = "wav",
        bitDepth: Int = 24
    ) async throws -> ExportResponse {
        var args = ["--export"]
        if !trackNames.isEmpty {
            args += ["--tracks", trackNames.joined(separator: ",")]
        }
        if let dir = outputDir, !dir.isEmpty {
            args += ["--output-dir", dir]
        }
        args += ["--format", fileType]
        args += ["--bit-depth", String(bitDepth)]
        let output = try await run(step: "export", extraArgs: args)
        return try decode(output.stdout)
    }

    func importMarkers(
        filePath: String,
        clearExisting: Bool = false,
        dryRun: Bool = false,
        onProgress: ((String) -> Void)? = nil
    ) async throws -> ImportMarkersResponse {
        var args = ["--import-markers", filePath]
        if clearExisting {
            args.append("--clear-markers")
        }
        if dryRun {
            args.append("--dry-run")
        }
        let output = try await run(step: "import", extraArgs: args, onProgress: onProgress)
        return try decode(output.stdout)
    }

    // MARK: - Decoding

    private func decode<T: Decodable>(_ jsonString: String) throws -> T {
        guard let data = jsonString.data(using: .utf8) else {
            throw CLIError.decodingFailed("Invalid UTF-8 output")
        }
        do {
            return try JSONDecoder().decode(T.self, from: data)
        } catch {
            throw CLIError.decodingFailed("\(error.localizedDescription)\nRaw: \(jsonString.prefix(500))")
        }
    }
}
