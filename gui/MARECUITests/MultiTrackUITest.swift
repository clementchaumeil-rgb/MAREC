import XCTest

/// Scenario 5: Multi-track — 3 tracks, 9 clips
final class MultiTrackUITest: MARECUITestBase {

    override func setUp() {
        do {
            let result = try TestHarnessHelper.setup(scenario: "MAREC_Test_MultiTrack")
            XCTAssertEqual(result.status, "ok")
            XCTAssertEqual(result.tracks.count, 3, "Should have 3 tracks")
        } catch {
            XCTFail("Setup failed: \(error.localizedDescription)")
        }
        super.setUp()
    }

    func testMultiTrackRename() throws {
        connectToProTools()

        // Step 2: Verify track count in session info
        let trackCountLabel = app.staticTexts.matching(identifier: AID.trackCount).firstMatch
        XCTAssertTrue(trackCountLabel.waitForExistence(timeout: 5))

        navigateToPreview()

        // Preview should show rename actions
        let renameCount = app.otherElements[AID.previewSummaryRenameCount]
        XCTAssertTrue(renameCount.waitForExistence(timeout: 5))

        executeRenameAndWaitResults()

        // Results: expect some successes and some errors (known multi-track limitation)
        let successElement = app.otherElements[AID.resultsSuccessCount]
        XCTAssertTrue(successElement.waitForExistence(timeout: 5))

        let errorElement = app.otherElements[AID.resultsErrorCount]
        XCTAssertTrue(errorElement.exists)
    }
}
