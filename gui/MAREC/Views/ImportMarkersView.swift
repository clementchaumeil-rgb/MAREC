import SwiftUI

struct ImportMarkersView: View {
    @ObservedObject var viewModel: MarecViewModel

    var body: some View {
        VStack(spacing: 20) {
            // Header
            HStack {
                Image(systemName: "arrow.down.doc.fill")
                    .foregroundColor(Theme.accent)
                    .font(.system(size: 20))
                Text("Importer des Marqueurs")
                    .font(Theme.bodySemiBold(16))
                    .foregroundColor(Theme.textPrimary)
                Spacer()
            }
            .padding(.bottom, 4)

            // File info
            if let url = viewModel.state.importFilePath {
                HStack(spacing: 8) {
                    Image(systemName: "doc.text")
                        .foregroundColor(Theme.textSecondary)
                    Text(url.lastPathComponent)
                        .font(Theme.mono(12))
                        .foregroundColor(Theme.textPrimary)
                    Spacer()
                }
                .padding(10)
                .premiumCard()
            }

            // Preview summary
            if let preview = viewModel.state.importPreview, let summary = preview.summary {
                VStack(spacing: 12) {
                    // Source session info
                    if let header = preview.header {
                        HStack {
                            Text("Source: \(header.sessionName)")
                                .font(Theme.body(12))
                                .foregroundColor(Theme.textSecondary)
                            Spacer()
                            Text("\(header.sampleRate) Hz")
                                .font(Theme.mono(11))
                                .foregroundColor(Theme.textTertiary)
                        }
                    }

                    Divider().background(Theme.borderSubtle)

                    // Counts
                    HStack(spacing: 16) {
                        summaryItem(
                            label: "Dans le fichier",
                            count: summary.parsed,
                            color: Theme.textPrimary
                        )
                        summaryItem(
                            label: "A creer",
                            count: summary.created,
                            color: Theme.success
                        )
                        .accessibilityIdentifier(AID.importPreviewCount)
                        summaryItem(
                            label: "Existants",
                            count: summary.skipped,
                            color: Theme.warning
                        )
                    }

                    // Warnings
                    if let warnings = preview.warnings, !warnings.isEmpty {
                        VStack(alignment: .leading, spacing: 4) {
                            ForEach(warnings.indices, id: \.self) { i in
                                HStack(spacing: 4) {
                                    Image(systemName: "exclamationmark.triangle")
                                        .font(.system(size: 10))
                                        .foregroundColor(Theme.warning)
                                    Text(warnings[i])
                                        .font(Theme.body(11))
                                        .foregroundColor(Theme.warning)
                                }
                            }
                        }
                    }
                }
                .padding(14)
                .premiumCard()
            }

            // Import results (after execution)
            if let results = viewModel.state.importResults, let summary = results.summary {
                VStack(spacing: 12) {
                    HStack {
                        Image(systemName: "checkmark.circle.fill")
                            .foregroundColor(Theme.success)
                        Text("Import termine")
                            .font(Theme.bodySemiBold(14))
                            .foregroundColor(Theme.textPrimary)
                        Spacer()
                    }

                    HStack(spacing: 16) {
                        summaryItem(label: "Crees", count: summary.created, color: Theme.success)
                        summaryItem(label: "Ignores", count: summary.skipped, color: Theme.warning)
                        if summary.errors > 0 {
                            summaryItem(label: "Erreurs", count: summary.errors, color: Theme.error)
                        }
                        if summary.cleared > 0 {
                            summaryItem(label: "Supprimes", count: summary.cleared, color: Theme.textTertiary)
                        }
                    }
                }
                .padding(14)
                .premiumCard()
            }

            // Clear existing toggle
            if viewModel.state.importResults == nil {
                Toggle(isOn: Binding(
                    get: { viewModel.state.importClearExisting },
                    set: { viewModel.state.importClearExisting = $0 }
                )) {
                    HStack(spacing: 6) {
                        Image(systemName: "trash")
                            .foregroundColor(Theme.error)
                        Text("Supprimer les marqueurs existants")
                            .font(Theme.body(12))
                            .foregroundColor(Theme.textSecondary)
                    }
                }
                .toggleStyle(.switch)
                .accessibilityIdentifier(AID.importClearToggle)
            }

            // Error
            if let error = viewModel.state.errorMessage {
                HStack(spacing: 8) {
                    Image(systemName: "exclamationmark.triangle.fill")
                        .foregroundColor(Theme.error)
                    Text(error)
                        .font(Theme.body(12))
                        .foregroundColor(Theme.error)
                }
                .padding(10)
                .premiumCard()
            }

            Spacer()

            // Action buttons
            HStack(spacing: 12) {
                Button(action: {
                    viewModel.state.showImportSheet = false
                    viewModel.state.importPreview = nil
                    viewModel.state.importResults = nil
                    viewModel.state.errorMessage = nil
                }) {
                    Text(viewModel.state.importResults != nil ? "Fermer" : "Annuler")
                        .frame(minWidth: 80)
                }
                .buttonStyle(PremiumButtonStyle(isPrimary: false))
                .accessibilityIdentifier(AID.importCancelButton)

                if viewModel.state.importResults == nil {
                    Button(action: {
                        Task { await viewModel.executeImport() }
                    }) {
                        HStack(spacing: 6) {
                            if viewModel.state.isLoading {
                                ProgressView()
                                    .progressViewStyle(.circular)
                                    .scaleEffect(0.7)
                                    .frame(width: 14, height: 14)
                            } else {
                                Image(systemName: "arrow.down.circle.fill")
                            }
                            Text("Importer")
                        }
                        .frame(minWidth: 100)
                    }
                    .buttonStyle(PremiumButtonStyle())
                    .disabled(viewModel.state.isLoading || viewModel.state.importPreview == nil)
                    .accessibilityIdentifier(AID.importConfirmButton)
                }
            }

            // Progress
            if let progress = viewModel.state.progressMessage {
                Text(progress)
                    .font(Theme.mono(11))
                    .foregroundColor(Theme.textTertiary)
            }
        }
        .padding(24)
        .frame(minWidth: 420, minHeight: 380)
        .background(Theme.backgroundGradient)
    }

    private func summaryItem(label: String, count: Int, color: Color) -> some View {
        VStack(spacing: 2) {
            Text("\(count)")
                .font(Theme.bodyBold(18))
                .foregroundColor(color)
            Text(label)
                .font(Theme.body(11))
                .foregroundColor(Theme.textTertiary)
        }
        .frame(maxWidth: .infinity)
    }
}
