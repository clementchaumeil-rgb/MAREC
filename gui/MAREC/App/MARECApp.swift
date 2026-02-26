import SwiftUI

@main
struct MARECApp: App {
    @StateObject private var viewModel = MarecViewModel()

    var body: some Scene {
        WindowGroup {
            ContentView(viewModel: viewModel)
                .preferredColorScheme(.dark)
        }
        .windowStyle(.hiddenTitleBar)
        .defaultSize(width: 900, height: 650)
    }
}
