import XCTest

/// Scenario 3: Clips before first marker — 2 skipped, 1 renamed
final class BeforeFirstMarkerUITest: MARECUITestBase {

    override func setUp() {
        do {
            let result = try TestHarnessHelper.setup(scenario: "MAREC_Test_BeforeFirst")
            XCTAssertEqual(result.status, "ok")
        } catch {
            XCTFail("Setup failed: \(error.localizedDescription)")
        }
        super.setUp()
    }

    func testClipsBeforeFirstMarkerSkipped() throws {
        connectToProTools()
        navigateToPreview()

        // Should see skipped count
        let skippedCount = app.otherElements[AID.previewSummarySkippedCount]
        XCTAssertTrue(skippedCount.waitForExistence(timeout: 5),
                      "Skipped count should be visible when clips are before first marker")

        let renameCount = app.otherElements[AID.previewSummaryRenameCount]
        XCTAssertTrue(renameCount.exists)

        executeRenameAndWaitResults()

        let successElement = app.otherElements[AID.resultsSuccessCount]
        XCTAssertTrue(successElement.waitForExistence(timeout: 5))
    }
}
