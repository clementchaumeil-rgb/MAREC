import XCTest

/// Scenario 1: Basic renaming — 3 markers, 5 clips
final class BasicRenameUITest: MARECUITestBase {

    override func setUp() {
        // Create PT session before launching app
        do {
            let result = try TestHarnessHelper.setup(scenario: "MAREC_Test_Basic")
            XCTAssertEqual(result.status, "ok")
        } catch {
            XCTFail("Setup failed: \(error.localizedDescription)")
        }
        super.setUp()
    }

    func testBasicRenameFlow() throws {
        // Step 1: Connect
        connectToProTools()

        // Step 2: Track selection — all tracks pre-selected
        // Verify markers and tracks are loaded
        let markerLabel = app.staticTexts.matching(identifier: AID.markerCount).firstMatch
        XCTAssertTrue(markerLabel.waitForExistence(timeout: 5))

        // Navigate to preview
        navigateToPreview()

        // Step 3: Preview — verify summary counts
        let renameCount = app.otherElements[AID.previewSummaryRenameCount]
        XCTAssertTrue(renameCount.waitForExistence(timeout: 5))

        let totalCount = app.otherElements[AID.previewSummaryTotalCount]
        XCTAssertTrue(totalCount.exists)

        // Execute rename
        executeRenameAndWaitResults()

        // Step 5: Results — verify success
        let successElement = app.otherElements[AID.resultsSuccessCount]
        XCTAssertTrue(successElement.waitForExistence(timeout: 5))
    }
}
