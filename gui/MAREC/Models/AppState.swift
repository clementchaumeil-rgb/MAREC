import Foundation
import SwiftUI

// MARK: - App Step (State Machine)

enum AppStep: Int, CaseIterable {
    case connection = 0
    case trackSelection
    case preview
    case executing
    case results

    var title: String {
        switch self {
        case .connection: return "Connexion"
        case .trackSelection: return "Pistes"
        case .preview: return "Apercu"
        case .executing: return "Renommage"
        case .results: return "Resultats"
        }
    }

    var stepNumber: Int { rawValue + 1 }
    var totalSteps: Int { AppStep.allCases.count }
}

// MARK: - Export Settings

struct ExportSettings {
    var enabled: Bool = false
    var outputDir: String = ""
    var fileType: String = "wav"
    var bitDepth: Int = 24
}

// MARK: - App State

@MainActor
final class AppState: ObservableObject {
    @Published var currentStep: AppStep = .connection
    @Published var isLoading: Bool = false
    @Published var errorMessage: String?

    // Session data
    @Published var sampleRate: Int = 0
    @Published var counterFormat: String = ""

    // Tracks
    @Published var availableTracks: [TracksResponse.TrackItem] = []
    @Published var selectedTrackNames: Set<String> = []

    // Markers
    @Published var markers: [MarkersResponse.MarkerItem] = []

    // Preview
    @Published var previewActions: [PreviewResponse.RenameAction] = []
    @Published var previewSummary: PreviewResponse.PreviewSummary?

    // Results
    @Published var renameResults: [RenameResponse.RenameResult] = []
    @Published var renameSummary: RenameResponse.RenameSummary?

    // Export
    @Published var exportSettings = ExportSettings()
    @Published var exportResults: [ExportResponse.ExportResult] = []

    // Rename file option
    @Published var renameFile: Bool = false

    func reset() {
        currentStep = .connection
        isLoading = false
        errorMessage = nil
        sampleRate = 0
        counterFormat = ""
        availableTracks = []
        selectedTrackNames = []
        markers = []
        previewActions = []
        previewSummary = nil
        renameResults = []
        renameSummary = nil
        exportResults = []
    }
}
