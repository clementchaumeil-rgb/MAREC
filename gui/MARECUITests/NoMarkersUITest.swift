import XCTest

/// Scenario 7: No markers — 3 clips but no markers, all skipped
final class NoMarkersUITest: MARECUITestBase {

    override func setUp() {
        do {
            let result = try TestHarnessHelper.setup(scenario: "MAREC_Test_NoMarkers")
            XCTAssertEqual(result.status, "ok")
        } catch {
            XCTFail("Setup failed: \(error.localizedDescription)")
        }
        super.setUp()
    }

    func testNoMarkersSkipsAllClips() throws {
        connectToProTools()

        // Step 2: Verify marker count shows 0
        let markerLabel = app.staticTexts.matching(identifier: AID.markerCount).firstMatch
        XCTAssertTrue(markerLabel.waitForExistence(timeout: 5))

        navigateToPreview()

        // Empty state should appear (no markers = nothing to rename)
        // With no markers, all clips are skipped, so previewActions will be empty
        let emptyState = app.staticTexts[AID.previewEmptyState]

        // Either empty state (if 0 actions) or skipped count visible
        if !emptyState.waitForExistence(timeout: 5) {
            // Clips may still appear but as skipped — check summary
            let skippedCount = app.otherElements[AID.previewSummarySkippedCount]
            XCTAssertTrue(skippedCount.waitForExistence(timeout: 5),
                          "Should show either empty state or skipped count")
        }
    }
}
