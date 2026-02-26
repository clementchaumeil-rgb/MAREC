import XCTest

/// Scenario 4: Clip already named correctly — 0 renames, 1 already correct
final class AlreadyCorrectUITest: MARECUITestBase {

    override func setUp() {
        do {
            let result = try TestHarnessHelper.setup(scenario: "MAREC_Test_AlreadyCorrect")
            XCTAssertEqual(result.status, "ok")
        } catch {
            XCTFail("Setup failed: \(error.localizedDescription)")
        }
        super.setUp()
    }

    func testAlreadyCorrectShowsEmptyState() throws {
        connectToProTools()
        navigateToPreview()

        // Empty state should be displayed (no clips to rename)
        let emptyState = app.staticTexts[AID.previewEmptyState]
        XCTAssertTrue(emptyState.waitForExistence(timeout: 5),
                      "Empty state should appear when all clips are already correctly named")

        // Rename button should NOT exist
        let renameBtn = app.buttons[AID.renameButton]
        XCTAssertFalse(renameBtn.exists, "Rename button should not appear when nothing to rename")

        // Correct count should be visible
        let correctCount = app.otherElements[AID.previewSummaryCorrectCount]
        XCTAssertTrue(correctCount.exists)
    }
}
