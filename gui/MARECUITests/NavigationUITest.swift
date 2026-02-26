import XCTest

/// Test navigation: back buttons and new session reset.
final class NavigationUITest: MARECUITestBase {

    override func setUp() {
        do {
            let result = try TestHarnessHelper.setup(scenario: "MAREC_Test_Basic")
            XCTAssertEqual(result.status, "ok")
        } catch {
            XCTFail("Setup failed: \(error.localizedDescription)")
        }
        super.setUp()
    }

    func testBackNavigation() throws {
        // Connect → Track Selection
        connectToProTools()
        assertCurrentStep("Pistes")

        // Track Selection → Preview
        navigateToPreview()
        assertCurrentStep("Apercu")

        // Preview → back to Track Selection
        let backBtn3 = app.buttons[AID.backButtonStep3]
        XCTAssertTrue(backBtn3.exists, "Back button should exist on preview step")
        backBtn3.click()

        // Verify we're back at track selection
        XCTAssertTrue(app.buttons[AID.previewButton].waitForExistence(timeout: 5),
                      "Should be back at track selection")
        assertCurrentStep("Pistes")

        // Track Selection → back to Connection
        let backBtn2 = app.buttons[AID.backButtonStep2]
        XCTAssertTrue(backBtn2.exists, "Back button should exist on track selection step")
        backBtn2.click()

        // Verify we're back at connection
        XCTAssertTrue(app.buttons[AID.connectButton].waitForExistence(timeout: 5),
                      "Should be back at connection")
        assertCurrentStep("Connexion")
    }

    func testNewSessionResetsToStep1() throws {
        // Go all the way through to results
        connectToProTools()
        navigateToPreview()
        executeRenameAndWaitResults()
        assertCurrentStep("Resultats")

        // Click "Nouvelle session"
        let newSessionBtn = app.buttons[AID.newSessionButton]
        XCTAssertTrue(newSessionBtn.exists)
        newSessionBtn.click()

        // Verify reset to connection step
        XCTAssertTrue(app.buttons[AID.connectButton].waitForExistence(timeout: 5),
                      "Should reset to connection step")
        assertCurrentStep("Connexion")
    }
}
