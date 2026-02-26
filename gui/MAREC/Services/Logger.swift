import Foundation
import os

// MARK: - Structured Logger for MAREC
// Logs to both os.Logger (Console.app) and ~/Library/Logs/MAREC/marec.log

final class MarecLogger {
    static let shared = MarecLogger()

    private let osLog = os.Logger(subsystem: "com.audiart.marec", category: "app")
    private let fileHandle: FileHandle?
    private let queue = DispatchQueue(label: "com.audiart.marec.logger", qos: .utility)
    private let dateFormatter: ISO8601DateFormatter = {
        let f = ISO8601DateFormatter()
        f.formatOptions = [.withInternetDateTime, .withFractionalSeconds]
        return f
    }()

    private init() {
        let logDir = FileManager.default.homeDirectoryForCurrentUser
            .appendingPathComponent("Library/Logs/MAREC", isDirectory: true)

        try? FileManager.default.createDirectory(at: logDir, withIntermediateDirectories: true)

        let logFile = logDir.appendingPathComponent("marec.log")

        if !FileManager.default.fileExists(atPath: logFile.path) {
            FileManager.default.createFile(atPath: logFile.path, contents: nil)
        }

        self.fileHandle = try? FileHandle(forWritingTo: logFile)
        fileHandle?.seekToEndOfFile()
    }

    deinit {
        try? fileHandle?.close()
    }

    // MARK: - Log Levels

    func info(_ message: String) {
        log(level: "INFO", message: message)
        osLog.info("\(message, privacy: .public)")
    }

    func debug(_ message: String) {
        log(level: "DEBUG", message: message)
        osLog.debug("\(message, privacy: .public)")
    }

    func warning(_ message: String) {
        log(level: "WARN", message: message)
        osLog.warning("\(message, privacy: .public)")
    }

    func error(_ message: String) {
        log(level: "ERROR", message: message)
        osLog.error("\(message, privacy: .public)")
    }

    // MARK: - Specialized Helpers

    func logCLIInvocation(step: String, args: [String]) {
        info("CLI invoke: --json --step \(step) \(args.joined(separator: " "))")
    }

    func logCLIOutput(step: String, stdout: String, stderr: String, exitCode: Int32) {
        let stdoutPreview = String(stdout.prefix(500))
        let stderrPreview = String(stderr.prefix(500))
        if exitCode == 0 {
            debug("CLI[\(step)] exit=0 stdout=\(stdoutPreview)")
        } else {
            warning("CLI[\(step)] exit=\(exitCode) stdout=\(stdoutPreview) stderr=\(stderrPreview)")
        }
    }

    func logStepTransition(from: String, to: String) {
        info("Step: \(from) → \(to)")
    }

    // MARK: - File Writing

    private func log(level: String, message: String) {
        queue.async { [weak self] in
            guard let self, let handle = self.fileHandle else { return }
            let timestamp = self.dateFormatter.string(from: Date())
            let line = "[\(timestamp)] [\(level)] \(message)\n"
            if let data = line.data(using: .utf8) {
                handle.write(data)
            }
        }
    }
}
