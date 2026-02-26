import SwiftUI

struct ResultsView: View {
    @ObservedObject var viewModel: MarecViewModel
    @State private var searchText = ""
    @State private var errorsExpanded = true
    @State private var successExpanded = false

    private var errorResults: [RenameResponse.RenameResult] {
        viewModel.state.renameResults.filter { !$0.success }
    }

    private var successResults: [RenameResponse.RenameResult] {
        viewModel.state.renameResults.filter { $0.success }
    }

    private var filteredErrors: [RenameResponse.RenameResult] {
        guard !searchText.isEmpty else { return errorResults }
        let query = searchText.lowercased()
        return errorResults.filter {
            $0.oldName.lowercased().contains(query) ||
            $0.newName.lowercased().contains(query)
        }
    }

    private var filteredSuccess: [RenameResponse.RenameResult] {
        guard !searchText.isEmpty else { return successResults }
        let query = searchText.lowercased()
        return successResults.filter {
            $0.oldName.lowercased().contains(query) ||
            $0.newName.lowercased().contains(query)
        }
    }

    var body: some View {
        VStack(spacing: 16) {
            // Summary header
            if let summary = viewModel.state.renameSummary {
                resultsSummary(summary)
            }

            // Results list
            VStack(alignment: .leading, spacing: 8) {
                HStack {
                    Text("Details")
                        .font(Theme.bodySemiBold(14))
                        .foregroundColor(Theme.textPrimary)
                    Spacer()
                }

                if viewModel.state.renameResults.count > 10 {
                    SearchBar(text: $searchText, placeholder: "Rechercher un clip...")
                }

                ScrollView {
                    LazyVStack(spacing: 6) {
                        // Errors section — always first, always expanded if any
                        if !filteredErrors.isEmpty {
                            CollapsibleSection(
                                title: "Erreurs",
                                count: filteredErrors.count,
                                accentColor: Theme.error,
                                isExpanded: $errorsExpanded
                            ) {
                                ForEach(filteredErrors) { result in
                                    ResultRow(result: result)
                                }
                            }
                        }

                        // Success section — collapsed by default when > 20
                        if !filteredSuccess.isEmpty {
                            CollapsibleSection(
                                title: "Renommes avec succes",
                                count: filteredSuccess.count,
                                accentColor: Theme.success,
                                isExpanded: $successExpanded
                            ) {
                                ForEach(filteredSuccess) { result in
                                    ResultRow(result: result)
                                }
                            }
                        }
                    }
                }
            }
            .padding(12)
            .premiumPanel()

            // Export results
            if !viewModel.state.exportResults.isEmpty {
                exportResultsSection
            }

            // Error
            if let error = viewModel.state.errorMessage {
                ErrorBanner(message: error)
            }

            // Actions
            HStack {
                Button(action: { viewModel.newSession() }) {
                    HStack(spacing: 6) {
                        Image(systemName: "arrow.counterclockwise")
                        Text("Nouvelle session")
                    }
                }
                .buttonStyle(PremiumButtonStyle(isPrimary: false))
                .accessibilityIdentifier(AID.newSessionButton)

                Spacer()

                if viewModel.state.exportSettings.enabled && viewModel.state.exportResults.isEmpty {
                    Button(action: {
                        Task { await viewModel.executeExport() }
                    }) {
                        HStack(spacing: 6) {
                            Image(systemName: "square.and.arrow.up")
                            Text("Exporter les clips")
                        }
                    }
                    .buttonStyle(PremiumButtonStyle())
                }
            }
        }
        .onAppear {
            // Auto-expand success if no errors
            if errorResults.isEmpty {
                successExpanded = true
            }
            // Collapse success if there are many items and errors exist
            if !errorResults.isEmpty && successResults.count > 20 {
                successExpanded = false
            }
        }
    }

    // MARK: - Subviews

    private func resultsSummary(_ summary: RenameResponse.RenameSummary) -> some View {
        HStack(spacing: 0) {
            // Success count
            VStack(spacing: 4) {
                Text("\(summary.successCount)")
                    .font(Theme.monoMedium(28))
                    .foregroundColor(Theme.success)
                Text("Renommes")
                    .font(Theme.body(11))
                    .foregroundColor(Theme.textTertiary)
            }
            .frame(maxWidth: .infinity)
            .accessibilityIdentifier(AID.resultsSuccessCount)

            Divider().frame(height: 40).background(Theme.borderSubtle)

            // Error count
            VStack(spacing: 4) {
                Text("\(summary.errorCount)")
                    .font(Theme.monoMedium(28))
                    .foregroundColor(summary.errorCount > 0 ? Theme.error : Theme.textTertiary)
                Text("Erreurs")
                    .font(Theme.body(11))
                    .foregroundColor(Theme.textTertiary)
            }
            .frame(maxWidth: .infinity)
            .accessibilityIdentifier(AID.resultsErrorCount)

            Divider().frame(height: 40).background(Theme.borderSubtle)

            // Total
            VStack(spacing: 4) {
                Text("\(summary.totalCount)")
                    .font(Theme.monoMedium(28))
                    .foregroundColor(Theme.textPrimary)
                Text("Total")
                    .font(Theme.body(11))
                    .foregroundColor(Theme.textTertiary)
            }
            .frame(maxWidth: .infinity)
            .accessibilityIdentifier(AID.resultsTotalCount)

            Divider().frame(height: 40).background(Theme.borderSubtle)

            // Status
            VStack(spacing: 4) {
                Image(systemName: summary.errorCount == 0 ? "checkmark.circle.fill" : "exclamationmark.triangle.fill")
                    .font(.system(size: 24))
                    .foregroundColor(summary.errorCount == 0 ? Theme.success : Theme.warning)
                Text(summary.errorCount == 0 ? "Succes" : "Partiel")
                    .font(Theme.body(11))
                    .foregroundColor(Theme.textTertiary)
            }
            .frame(maxWidth: .infinity)
        }
        .padding(16)
        .premiumPanel()
    }

    private var exportResultsSection: some View {
        VStack(alignment: .leading, spacing: 6) {
            Text("Export")
                .font(Theme.bodySemiBold(12))
                .foregroundColor(Theme.textSecondary)

            ForEach(viewModel.state.exportResults) { result in
                HStack(spacing: 8) {
                    Image(systemName: result.success ? "checkmark.circle.fill" : "xmark.circle.fill")
                        .foregroundColor(result.success ? Theme.success : Theme.error)
                        .font(.system(size: 12))
                    Text(result.trackName)
                        .font(Theme.mono(11))
                        .foregroundColor(Theme.textPrimary)
                    Spacer()
                    Text(result.success ? "OK" : "Erreur")
                        .font(Theme.body(10))
                        .foregroundColor(result.success ? Theme.success : Theme.error)
                }
            }
        }
        .padding(10)
        .premiumCard()
    }
}

// MARK: - Result Row

struct ResultRow: View {
    let result: RenameResponse.RenameResult

    var body: some View {
        HStack(spacing: 10) {
            Image(systemName: result.success ? "checkmark.circle.fill" : "xmark.circle.fill")
                .foregroundColor(result.success ? Theme.success : Theme.error)
                .font(.system(size: 14))

            Text(result.oldName)
                .font(Theme.mono(11))
                .foregroundColor(Theme.textTertiary)
                .strikethrough(result.success, color: Theme.textTertiary)
                .lineLimit(1)

            Image(systemName: "arrow.right")
                .font(.system(size: 9))
                .foregroundColor(Theme.textTertiary)

            Text(result.newName)
                .font(Theme.monoMedium(11))
                .foregroundColor(result.success ? Theme.success : Theme.error)
                .lineLimit(1)

            Spacer()

            if let error = result.error {
                Text(error)
                    .font(Theme.body(10))
                    .foregroundColor(Theme.error)
            }
        }
        .padding(.horizontal, 12)
        .padding(.vertical, 6)
        .background(
            result.success
                ? Theme.success.opacity(0.05)
                : Theme.error.opacity(0.05)
        )
        .cornerRadius(4)
    }
}
