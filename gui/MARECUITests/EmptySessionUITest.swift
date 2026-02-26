import XCTest

/// Scenario 6: Empty session — 1 track, no markers, no clips
final class EmptySessionUITest: MARECUITestBase {

    override func setUp() {
        do {
            let result = try TestHarnessHelper.setup(scenario: "MAREC_Test_Empty")
            XCTAssertEqual(result.status, "ok")
        } catch {
            XCTFail("Setup failed: \(error.localizedDescription)")
        }
        super.setUp()
    }

    func testEmptySessionShowsEmptyPreview() throws {
        connectToProTools()
        navigateToPreview()

        // Empty state should appear (no clips at all)
        let emptyState = app.staticTexts[AID.previewEmptyState]
        XCTAssertTrue(emptyState.waitForExistence(timeout: 5),
                      "Empty state should appear when session has no clips")

        // Rename button should NOT exist
        let renameBtn = app.buttons[AID.renameButton]
        XCTAssertFalse(renameBtn.exists)
    }
}
