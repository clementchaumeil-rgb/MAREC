import Foundation

// MARK: - CLI JSON Response Models
// Mirror the JSON output from `marec --json --step <step>`

struct SessionResponse: Codable {
    let status: String
    let session: SessionInfo?
    let error: String?
    let code: String?

    struct SessionInfo: Codable {
        let sampleRate: Int
        let counterFormat: String
    }
}

struct TracksResponse: Codable {
    let status: String
    let tracks: [TrackItem]?
    let error: String?
    let code: String?

    struct TrackItem: Codable, Identifiable, Hashable {
        let name: String
        let id: String
        let index: Int
    }
}

struct MarkersResponse: Codable {
    let status: String
    let markers: [MarkerItem]?
    let error: String?
    let code: String?

    struct MarkerItem: Codable, Identifiable {
        let number: Int
        let name: String
        let startTime: String
        let startSamples: Int64

        var id: Int { number }
    }
}

struct PreviewResponse: Codable {
    let status: String
    let actions: [RenameAction]?
    let summary: PreviewSummary?
    let error: String?
    let code: String?

    struct RenameAction: Codable, Identifiable {
        let trackName: String
        let oldName: String
        let newName: String
        let startSamples: Int64
        let timeFormatted: String
        let markerName: String

        var id: String { "\(trackName):\(oldName)" }
    }

    struct PreviewSummary: Codable {
        let totalClips: Int
        let toRename: Int
        let skippedBeforeFirstMarker: Int
        let alreadyCorrect: Int
    }
}

struct RenameResponse: Codable {
    let status: String
    let results: [RenameResult]?
    let summary: RenameSummary?
    let error: String?
    let code: String?

    struct RenameResult: Codable, Identifiable {
        let oldName: String
        let newName: String
        let success: Bool
        let error: String?

        var id: String { oldName }
    }

    struct RenameSummary: Codable {
        let successCount: Int
        let errorCount: Int
        let totalCount: Int
    }
}

struct ExportResponse: Codable {
    let status: String
    let results: [ExportResult]?
    let summary: ExportSummary?
    let error: String?
    let code: String?

    struct ExportResult: Codable, Identifiable {
        let trackName: String
        let success: Bool

        var id: String { trackName }
    }

    struct ExportSummary: Codable {
        let exported: Int
        let total: Int
    }
}
