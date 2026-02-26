import SwiftUI

struct ExecutionView: View {
    @ObservedObject var viewModel: MarecViewModel

    @State private var animateGlow = false

    var body: some View {
        VStack(spacing: 32) {
            Spacer()

            // Animated icon
            ZStack {
                Circle()
                    .fill(Theme.accent.opacity(animateGlow ? 0.15 : 0.05))
                    .frame(width: 100, height: 100)
                    .animation(.easeInOut(duration: 1.2).repeatForever(autoreverses: true), value: animateGlow)

                Circle()
                    .fill(Theme.accent.opacity(0.1))
                    .frame(width: 70, height: 70)

                Image(systemName: "pencil.and.outline")
                    .font(.system(size: 28))
                    .foregroundColor(Theme.accent)
            }
            .onAppear { animateGlow = true }

            VStack(spacing: 8) {
                Text("Renommage en cours...")
                    .font(Theme.bodySemiBold(18))
                    .foregroundColor(Theme.textPrimary)

                Text("MAREC communique avec Pro Tools via PTSL.")
                    .font(Theme.body(13))
                    .foregroundColor(Theme.textSecondary)

                Text("Ne fermez pas Pro Tools pendant l'operation.")
                    .font(Theme.body(12))
                    .foregroundColor(Theme.textTertiary)
            }

            ProgressView()
                .progressViewStyle(.linear)
                .tint(Theme.accent)
                .frame(maxWidth: 300)

            // Error
            if let error = viewModel.state.errorMessage {
                ErrorBanner(message: error)
                    .frame(maxWidth: 400)
            }

            Spacer()
        }
    }
}
