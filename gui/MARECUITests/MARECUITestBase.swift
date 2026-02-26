import XCTest

/// Base class for MAREC UI tests.
/// Provides shared setup/teardown and navigation helpers.
class MARECUITestBase: XCTestCase {
    var app: XCUIApplication!

    override func setUp() {
        super.setUp()
        continueAfterFailure = false
        app = XCUIApplication()
        // Point CLI service at the development binary
        app.launchEnvironment["MAREC_CLI_PATH"] = "/Users/avid/Downloads/MAREC/build/marec"
        app.launch()
    }

    override func tearDown() {
        TestHarnessHelper.teardown()
        app.terminate()
        super.tearDown()
    }

    // MARK: - Navigation Helpers

    /// Click "Connecter" and wait for track selection to appear.
    func connectToProTools() {
        let connectBtn = app.buttons[AID.connectButton]
        XCTAssertTrue(connectBtn.waitForExistence(timeout: 5), "Connect button should exist")
        connectBtn.click()
        XCTAssertTrue(
            app.buttons[AID.previewButton].waitForExistence(timeout: 15),
            "Should navigate to track selection after connect"
        )
    }

    /// Click "Apercu" and wait for preview screen.
    func navigateToPreview() {
        let previewBtn = app.buttons[AID.previewButton]
        XCTAssertTrue(previewBtn.isEnabled, "Preview button should be enabled")
        previewBtn.click()

        // Wait for either rename button OR empty state
        let rename = app.buttons[AID.renameButton]
        let empty = app.staticTexts[AID.previewEmptyState]
        let appeared = rename.waitForExistence(timeout: 15) || empty.waitForExistence(timeout: 1)
        XCTAssertTrue(appeared, "Should see either rename button or empty state")
    }

    /// Click "Renommer" and wait for results.
    func executeRenameAndWaitResults() {
        let renameBtn = app.buttons[AID.renameButton]
        XCTAssertTrue(renameBtn.exists, "Rename button should exist")
        renameBtn.click()
        XCTAssertTrue(
            app.buttons[AID.newSessionButton].waitForExistence(timeout: 30),
            "Should navigate to results after rename"
        )
    }

    // MARK: - Assertion Helpers

    /// Assert the current step label shows the expected text.
    func assertCurrentStep(_ expected: String) {
        let label = app.staticTexts[AID.currentStepLabel]
        XCTAssertTrue(label.waitForExistence(timeout: 5))
        XCTAssertEqual(label.label, expected)
    }

    /// Get the text value of a static text element by accessibility ID.
    func staticTextValue(_ aid: String, timeout: TimeInterval = 5) -> String? {
        let element = app.staticTexts[aid]
        guard element.waitForExistence(timeout: timeout) else { return nil }
        return element.label
    }
}
