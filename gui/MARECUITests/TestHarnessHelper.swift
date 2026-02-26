import Foundation

/// Result from `--setup-only` call.
struct SetupResult {
    let status: String
    let scenario: String
    let tracks: [String]
    let sessionDir: String
}

/// Calls `marec_test_harness` as a subprocess to create/destroy Pro Tools sessions.
class TestHarnessHelper {
    static let harnessPath = "/Users/avid/Downloads/MAREC/build/marec_test_harness"
    static let baseDir = "/tmp/marec_ui_tests"

    /// Create a Pro Tools session for the given scenario.
    /// Returns the parsed JSON result with track names and session directory.
    static func setup(scenario: String) throws -> SetupResult {
        let output = try run(args: ["--setup-only", scenario, "--base-dir", baseDir])

        guard let data = output.data(using: .utf8),
              let json = try? JSONSerialization.jsonObject(with: data) as? [String: Any],
              let status = json["status"] as? String else {
            throw TestHarnessError.invalidResponse(output)
        }

        if status != "ok" {
            let error = json["error"] as? String ?? "Unknown error"
            throw TestHarnessError.setupFailed(error)
        }

        return SetupResult(
            status: status,
            scenario: json["scenario"] as? String ?? scenario,
            tracks: json["tracks"] as? [String] ?? [],
            sessionDir: json["sessionDir"] as? String ?? ""
        )
    }

    /// Close the Pro Tools session and remove test files.
    static func teardown() {
        _ = try? run(args: ["--teardown", "--base-dir", baseDir])
    }

    /// List available scenario names.
    static func listScenarios() throws -> [String] {
        let output = try run(args: ["--list-scenarios"])
        guard let data = output.data(using: .utf8),
              let scenarios = try? JSONSerialization.jsonObject(with: data) as? [String] else {
            throw TestHarnessError.invalidResponse(output)
        }
        return scenarios
    }

    // MARK: - Private

    private static func run(args: [String]) throws -> String {
        guard FileManager.default.isExecutableFile(atPath: harnessPath) else {
            throw TestHarnessError.binaryNotFound(harnessPath)
        }

        let process = Process()
        process.executableURL = URL(fileURLWithPath: harnessPath)
        process.arguments = args

        let stdoutPipe = Pipe()
        let stderrPipe = Pipe()
        process.standardOutput = stdoutPipe
        process.standardError = stderrPipe

        try process.run()
        process.waitUntilExit()

        let outData = stdoutPipe.fileHandleForReading.readDataToEndOfFile()
        let stdout = String(data: outData, encoding: .utf8) ?? ""

        if process.terminationStatus != 0 && stdout.isEmpty {
            let errData = stderrPipe.fileHandleForReading.readDataToEndOfFile()
            let stderr = String(data: errData, encoding: .utf8) ?? ""
            throw TestHarnessError.processFailed(exitCode: process.terminationStatus, stderr: stderr)
        }

        return stdout
    }

    enum TestHarnessError: LocalizedError {
        case binaryNotFound(String)
        case processFailed(exitCode: Int32, stderr: String)
        case setupFailed(String)
        case invalidResponse(String)

        var errorDescription: String? {
            switch self {
            case .binaryNotFound(let path):
                return "Test harness binary not found at \(path)"
            case .processFailed(let code, let stderr):
                return "Test harness exited with code \(code): \(stderr)"
            case .setupFailed(let msg):
                return "Setup failed: \(msg)"
            case .invalidResponse(let output):
                return "Invalid JSON response: \(output.prefix(200))"
            }
        }
    }
}
