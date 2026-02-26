import Foundation

/// Centralized accessibility identifiers for XCUITest.
/// Compiled in both the app target and the UI test target.
enum AID {
    // MARK: - Step 1: Connection
    static let connectButton = "connection.connectButton"
    static let connectionError = "connection.errorMessage"

    // MARK: - Step 2: Track Selection
    static let selectAllButton = "trackSelection.selectAllButton"
    static let previewButton = "trackSelection.previewButton"
    static let backButtonStep2 = "trackSelection.backButton"
    static let markerCount = "trackSelection.markerCount"
    static let trackCount = "trackSelection.trackCount"
    static func trackRow(_ name: String) -> String { "trackSelection.track.\(name)" }

    // MARK: - Step 3: Preview
    static let previewSummaryRenameCount = "preview.summary.toRename"
    static let previewSummaryCorrectCount = "preview.summary.alreadyCorrect"
    static let previewSummaryTotalCount = "preview.summary.totalClips"
    static let previewSummarySkippedCount = "preview.summary.skipped"
    static let renameButton = "preview.renameButton"
    static let previewEmptyState = "preview.emptyState"
    static let backButtonStep3 = "preview.backButton"

    // MARK: - Step 5: Results
    static let resultsSuccessCount = "results.successCount"
    static let resultsErrorCount = "results.errorCount"
    static let resultsTotalCount = "results.totalCount"
    static let newSessionButton = "results.newSessionButton"

    // MARK: - Navigation
    static let currentStepLabel = "app.currentStepLabel"
}
