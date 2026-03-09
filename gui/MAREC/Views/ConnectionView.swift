import SwiftUI
import UniformTypeIdentifiers

struct ConnectionView: View {
    @ObservedObject var viewModel: MarecViewModel
    @State private var showFileImporter = false

    var body: some View {
        VStack(spacing: 24) {
            Spacer()

            // Icon
            ZStack {
                Circle()
                    .fill(Theme.accent.opacity(0.1))
                    .frame(width: 80, height: 80)

                Image(systemName: "cable.connector")
                    .font(.system(size: 32))
                    .foregroundColor(Theme.accent)
            }

            VStack(spacing: 8) {
                Text("Connexion a Pro Tools")
                    .font(Theme.bodySemiBold(18))
                    .foregroundColor(Theme.textPrimary)

                Text("Assurez-vous que Pro Tools est ouvert avec une session active.")
                    .font(Theme.body(13))
                    .foregroundColor(Theme.textSecondary)
                    .multilineTextAlignment(.center)
                    .frame(maxWidth: 400)
            }

            // Connect button
            Button(action: {
                Task { await viewModel.connect() }
            }) {
                HStack(spacing: 8) {
                    if viewModel.state.isLoading {
                        ProgressView()
                            .progressViewStyle(.circular)
                            .scaleEffect(0.7)
                            .frame(width: 16, height: 16)
                    } else {
                        Image(systemName: "bolt.fill")
                    }
                    Text(viewModel.state.isLoading ? "Connexion..." : "Connecter")
                }
                .frame(minWidth: 160)
            }
            .buttonStyle(PremiumButtonStyle())
            .disabled(viewModel.state.isLoading)
            .accessibilityIdentifier(AID.connectButton)

            // Import markers button (available anytime — connects internally)
            Button(action: {
                showFileImporter = true
            }) {
                HStack(spacing: 8) {
                    Image(systemName: "arrow.down.doc")
                    Text("Importer Marqueurs")
                }
                .frame(minWidth: 160)
            }
            .buttonStyle(PremiumButtonStyle(isPrimary: false))
            .disabled(viewModel.state.isLoading)
            .accessibilityIdentifier(AID.importMarkersButton)

            // Error message
            if let error = viewModel.state.errorMessage {
                HStack(spacing: 8) {
                    Image(systemName: "exclamationmark.triangle.fill")
                        .foregroundColor(Theme.error)
                    Text(error)
                        .font(Theme.body(12))
                        .foregroundColor(Theme.error)
                }
                .padding(12)
                .premiumCard()
                .accessibilityIdentifier(AID.connectionError)
            }

            Spacer()

            // Info box
            HStack(spacing: 12) {
                Image(systemName: "info.circle")
                    .foregroundColor(Theme.accent)
                VStack(alignment: .leading, spacing: 4) {
                    Text("MAREC se connecte via PTSL (localhost:31416)")
                        .font(Theme.body(11))
                        .foregroundColor(Theme.textSecondary)
                    Text("Pro Tools doit etre lance avant de connecter.")
                        .font(Theme.body(11))
                        .foregroundColor(Theme.textTertiary)
                }
                Spacer()
            }
            .padding(12)
            .premiumCard()
        }
        .fileImporter(
            isPresented: $showFileImporter,
            allowedContentTypes: [UTType.plainText],
            allowsMultipleSelection: false
        ) { result in
            switch result {
            case .success(let urls):
                if let url = urls.first {
                    viewModel.state.importFilePath = url
                    Task { await viewModel.previewImport() }
                }
            case .failure(let error):
                viewModel.state.errorMessage = error.localizedDescription
            }
        }
        .sheet(isPresented: Binding(
            get: { viewModel.state.showImportSheet },
            set: { viewModel.state.showImportSheet = $0 }
        )) {
            ImportMarkersView(viewModel: viewModel)
        }
    }
}
