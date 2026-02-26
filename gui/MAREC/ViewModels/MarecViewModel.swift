import Foundation
import SwiftUI
import Combine

@MainActor
final class MarecViewModel: ObservableObject {
    let state: AppState
    private let cli = MarecCLIService()
    private let logger = MarecLogger.shared
    private var stateSink: AnyCancellable?

    init() {
        let s = AppState()
        self.state = s
        // Forward AppState changes to trigger SwiftUI updates on this ViewModel
        self.stateSink = s.objectWillChange.sink { [weak self] _ in
            self?.objectWillChange.send()
        }
        logger.info("MarecViewModel initialized")
    }

    // MARK: - Step 1: Connect

    func connect() async {
        logger.logStepTransition(from: "connection", to: "connecting...")
        state.isLoading = true
        state.errorMessage = nil

        do {
            let response = try await cli.connect()
            guard response.status == "ok", let session = response.session else {
                let msg = response.error ?? "Connexion echouee"
                logger.error("Connect failed: \(msg)")
                state.errorMessage = msg
                state.isLoading = false
                return
            }

            logger.info("Connected: \(session.sampleRate)Hz, format=\(session.counterFormat)")
            state.sampleRate = session.sampleRate
            state.counterFormat = session.counterFormat

            // Fetch tracks and markers in parallel
            async let tracksResult = cli.tracks()
            async let markersResult = cli.markers()

            let tracks = try await tracksResult
            let markers = try await markersResult

            if let trackList = tracks.tracks {
                state.availableTracks = trackList
                state.selectedTrackNames = Set(trackList.map(\.name))
                logger.info("Tracks: \(trackList.count) audio track(s)")
            }

            if let markerList = markers.markers {
                state.markers = markerList
                logger.info("Markers: \(markerList.count) marker(s)")
            }

            logger.logStepTransition(from: "connection", to: "trackSelection")
            state.currentStep = .trackSelection
        } catch {
            logger.error("Connect error: \(error.localizedDescription)")
            state.errorMessage = error.localizedDescription
        }

        state.isLoading = false
    }

    // MARK: - Step 2 → 3: Preview

    func preview() async {
        let selectedNames = Array(state.selectedTrackNames)
        logger.logStepTransition(from: "trackSelection", to: "preview")
        logger.info("Preview: \(selectedNames.count) track(s) selected")
        state.isLoading = true
        state.errorMessage = nil

        do {
            let response = try await cli.preview(trackNames: selectedNames)

            guard response.status == "ok" else {
                let msg = response.error ?? "Apercu echoue"
                logger.error("Preview failed: \(msg)")
                state.errorMessage = msg
                state.isLoading = false
                return
            }

            let actions = response.actions ?? []
            state.previewActions = actions
            state.previewSummary = response.summary

            if let summary = response.summary {
                logger.info("Preview: \(summary.toRename) to rename, \(summary.alreadyCorrect) already correct, \(summary.skippedBeforeFirstMarker) skipped")
            }

            state.currentStep = .preview
        } catch {
            logger.error("Preview error: \(error.localizedDescription)")
            state.errorMessage = error.localizedDescription
        }

        state.isLoading = false
    }

    // MARK: - Step 3 → 4: Execute Rename

    func executeRename() async {
        logger.logStepTransition(from: "preview", to: "executing")
        state.currentStep = .executing
        state.isLoading = true
        state.errorMessage = nil

        do {
            let response = try await cli.rename(
                trackNames: Array(state.selectedTrackNames),
                renameFile: state.renameFile
            )

            guard response.status == "ok" else {
                let msg = response.error ?? "Renommage echoue"
                logger.error("Rename failed: \(msg)")
                state.errorMessage = msg
                state.isLoading = false
                return
            }

            let results = response.results ?? []
            state.renameResults = results
            state.renameSummary = response.summary

            if let summary = response.summary {
                logger.info("Rename done: \(summary.successCount)/\(summary.totalCount) succeeded, \(summary.errorCount) errors")
            }

            // Log individual errors with real PTSL messages
            for result in results where !result.success {
                let errMsg = result.error ?? "Unknown error"
                logger.error("Rename failed: '\(result.oldName)' -> '\(result.newName)': \(errMsg)")
            }

            // Auto-export if enabled
            if state.exportSettings.enabled {
                logger.info("Auto-export enabled, starting export...")
                await executeExport()
            }

            logger.logStepTransition(from: "executing", to: "results")
            state.currentStep = .results
        } catch {
            logger.error("Rename error: \(error.localizedDescription)")
            state.errorMessage = error.localizedDescription
        }

        state.isLoading = false
    }

    // MARK: - Export

    func executeExport() async {
        logger.info("Export starting...")
        do {
            let response = try await cli.export(
                trackNames: Array(state.selectedTrackNames),
                outputDir: state.exportSettings.outputDir.isEmpty ? nil : state.exportSettings.outputDir,
                fileType: state.exportSettings.fileType,
                bitDepth: state.exportSettings.bitDepth
            )
            state.exportResults = response.results ?? []

            if let summary = response.summary {
                logger.info("Export done: \(summary.exported)/\(summary.total) tracks exported")
            }
        } catch {
            // Export errors are non-fatal — show in results
            logger.error("Export error: \(error.localizedDescription)")
            state.errorMessage = (state.errorMessage ?? "") + "\nExport: \(error.localizedDescription)"
        }
    }

    // MARK: - Navigation

    func goBack() {
        let from = state.currentStep.title
        switch state.currentStep {
        case .connection:
            break
        case .trackSelection:
            state.currentStep = .connection
        case .preview:
            state.currentStep = .trackSelection
        case .executing:
            break // Can't go back during execution
        case .results:
            state.currentStep = .preview
        }
        logger.logStepTransition(from: from, to: state.currentStep.title)
    }

    func newSession() {
        logger.info("New session: resetting state")
        state.reset()
    }
}
