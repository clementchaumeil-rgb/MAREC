import XCTest

/// Scenario 2: Heavy duplicates — 3 clips under one marker, 1 under another
final class DuplicatesUITest: MARECUITestBase {

    override func setUp() {
        do {
            let result = try TestHarnessHelper.setup(scenario: "MAREC_Test_Duplicates")
            XCTAssertEqual(result.status, "ok")
        } catch {
            XCTFail("Setup failed: \(error.localizedDescription)")
        }
        super.setUp()
    }

    func testDuplicateNaming() throws {
        connectToProTools()
        navigateToPreview()

        // All 4 clips should need renaming
        let renameCount = app.otherElements[AID.previewSummaryRenameCount]
        XCTAssertTrue(renameCount.waitForExistence(timeout: 5))

        executeRenameAndWaitResults()

        let successElement = app.otherElements[AID.resultsSuccessCount]
        XCTAssertTrue(successElement.waitForExistence(timeout: 5))
    }
}
