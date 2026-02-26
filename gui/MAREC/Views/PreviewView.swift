import SwiftUI

struct PreviewView: View {
    @ObservedObject var viewModel: MarecViewModel
    @State private var searchText = ""
    @State private var showOnlyToRename = false
    @State private var expandedTracks: Set<String> = []

    var body: some View {
        VStack(spacing: 16) {
            // Summary bar
            if let summary = viewModel.state.previewSummary {
                summaryBar(summary)
            }

            // Actions table
            VStack(alignment: .leading, spacing: 8) {
                // Header row with title and controls
                HStack {
                    Text("Actions de renommage")
                        .font(Theme.bodySemiBold(14))
                        .foregroundColor(Theme.textPrimary)

                    Spacer()

                    Text("\(viewModel.state.previewActions.count) action(s)")
                        .font(Theme.mono(11))
                        .foregroundColor(Theme.textTertiary)
                }

                // Search + filter bar (only shown when there's enough data)
                if viewModel.state.previewActions.count > 10 {
                    HStack(spacing: 10) {
                        SearchBar(text: $searchText, placeholder: "Rechercher un clip...")

                        Button(action: { showOnlyToRename.toggle() }) {
                            HStack(spacing: 4) {
                                Image(systemName: showOnlyToRename ? "line.3.horizontal.decrease.circle.fill" : "line.3.horizontal.decrease.circle")
                                    .font(.system(size: 12))
                                Text("A renommer")
                                    .font(Theme.body(11))
                            }
                            .foregroundColor(showOnlyToRename ? Theme.accent : Theme.textTertiary)
                            .padding(.horizontal, 8)
                            .padding(.vertical, 6)
                            .background(showOnlyToRename ? Theme.accent.opacity(0.15) : Theme.backgroundTertiary.opacity(0.5))
                            .cornerRadius(Theme.radiusSmall)
                        }
                        .buttonStyle(.plain)
                    }
                }

                // Content: flat list or grouped by track
                if viewModel.state.previewActions.isEmpty {
                    emptyState
                } else if groupedActions.count > 1 {
                    groupedContent
                } else {
                    flatContent
                }
            }
            .padding(12)
            .premiumPanel()

            // Options
            optionsBar

            // Error
            if let error = viewModel.state.errorMessage {
                ErrorBanner(message: error)
            }

            // Actions
            HStack {
                Button("Retour") {
                    viewModel.goBack()
                }
                .buttonStyle(PremiumButtonStyle(isPrimary: false))
                .accessibilityIdentifier(AID.backButtonStep3)

                Spacer()

                if !viewModel.state.previewActions.isEmpty {
                    Button(action: {
                        Task { await viewModel.executeRename() }
                    }) {
                        HStack(spacing: 6) {
                            Image(systemName: "pencil.line")
                            Text("Renommer \(viewModel.state.previewActions.count) clip(s)")
                        }
                    }
                    .buttonStyle(PremiumButtonStyle())
                    .accessibilityIdentifier(AID.renameButton)
                }
            }
        }
    }

    // MARK: - Computed

    private var groupedActions: [(trackName: String, actions: [PreviewResponse.RenameAction])] {
        let grouped = Dictionary(grouping: filteredActions, by: \.trackName)
        // Preserve original track order
        var seen = Set<String>()
        var order: [String] = []
        for action in filteredActions {
            if seen.insert(action.trackName).inserted {
                order.append(action.trackName)
            }
        }
        return order.map { name in (trackName: name, actions: grouped[name]!) }
    }

    private var filteredActions: [PreviewResponse.RenameAction] {
        var actions = viewModel.state.previewActions

        if showOnlyToRename {
            actions = actions.filter { $0.oldName != $0.newName }
        }

        if !searchText.isEmpty {
            let query = searchText.lowercased()
            actions = actions.filter {
                $0.oldName.lowercased().contains(query) ||
                $0.newName.lowercased().contains(query) ||
                $0.trackName.lowercased().contains(query) ||
                $0.markerName.lowercased().contains(query)
            }
        }

        return actions
    }

    // MARK: - Subviews

    private func summaryBar(_ summary: PreviewResponse.PreviewSummary) -> some View {
        HStack(spacing: 20) {
            SummaryItem(label: "Total clips", value: "\(summary.totalClips)", color: Theme.textPrimary)
                .accessibilityIdentifier(AID.previewSummaryTotalCount)
            SummaryItem(label: "A renommer", value: "\(summary.toRename)", color: Theme.warning)
                .accessibilityIdentifier(AID.previewSummaryRenameCount)
            SummaryItem(label: "Deja corrects", value: "\(summary.alreadyCorrect)", color: Theme.success)
                .accessibilityIdentifier(AID.previewSummaryCorrectCount)
            if summary.skippedBeforeFirstMarker > 0 {
                SummaryItem(label: "Ignores (avant 1er marqueur)", value: "\(summary.skippedBeforeFirstMarker)", color: Theme.textTertiary)
                    .accessibilityIdentifier(AID.previewSummarySkippedCount)
            }
            Spacer()
        }
        .padding(10)
        .premiumCard()
    }

    private var emptyState: some View {
        VStack(spacing: 8) {
            Spacer()
            Image(systemName: "checkmark.seal.fill")
                .font(.system(size: 28))
                .foregroundColor(Theme.success)
            Text("Aucun clip a renommer")
                .font(Theme.bodyMedium(14))
                .foregroundColor(Theme.textSecondary)
                .accessibilityIdentifier(AID.previewEmptyState)
            Text("Tous les clips sont deja correctement nommes.")
                .font(Theme.body(12))
                .foregroundColor(Theme.textTertiary)
            Spacer()
        }
        .frame(maxWidth: .infinity)
    }

    /// Grouped view: sections per track, collapsible
    private var groupedContent: some View {
        ScrollView {
            LazyVStack(spacing: 4) {
                ForEach(groupedActions, id: \.trackName) { group in
                    let toRenameCount = group.actions.filter { $0.oldName != $0.newName }.count
                    let correctCount = group.actions.count - toRenameCount
                    let subtitle = "\(toRenameCount) a renommer" + (correctCount > 0 ? ", \(correctCount) correct(s)" : "")

                    CollapsibleSection(
                        title: group.trackName,
                        count: group.actions.count,
                        subtitle: subtitle,
                        accentColor: Theme.warning,
                        isExpanded: Binding(
                            get: { expandedTracks.contains(group.trackName) },
                            set: { newValue in
                                if newValue {
                                    expandedTracks.insert(group.trackName)
                                } else {
                                    expandedTracks.remove(group.trackName)
                                }
                            }
                        )
                    ) {
                        // Table header
                        actionTableHeader
                            .padding(.top, 4)

                        ForEach(group.actions) { action in
                            ActionRow(action: action)
                        }
                    }
                }
            }
        }
    }

    /// Flat list for single-track sessions
    private var flatContent: some View {
        VStack(alignment: .leading, spacing: 0) {
            actionTableHeader

            Divider().background(Theme.borderSubtle)

            ScrollView {
                LazyVStack(spacing: 2) {
                    ForEach(filteredActions) { action in
                        ActionRow(action: action)
                    }
                }
            }
        }
    }

    private var actionTableHeader: some View {
        HStack(spacing: 0) {
            Text("Piste")
                .frame(width: 120, alignment: .leading)
            Text("Nom actuel")
                .frame(maxWidth: .infinity, alignment: .leading)
            Image(systemName: "arrow.right")
                .frame(width: 30)
            Text("Nouveau nom")
                .frame(maxWidth: .infinity, alignment: .leading)
            Text("Position")
                .frame(width: 100, alignment: .trailing)
        }
        .font(Theme.bodySemiBold(11))
        .foregroundColor(Theme.textTertiary)
        .padding(.horizontal, 12)
        .padding(.vertical, 6)
    }

    private var optionsBar: some View {
        HStack(spacing: 16) {
            Toggle(isOn: Binding(
                get: { viewModel.state.renameFile },
                set: { viewModel.state.renameFile = $0 }
            )) {
                HStack(spacing: 6) {
                    Image(systemName: "doc")
                        .foregroundColor(Theme.textTertiary)
                    Text("Renommer aussi les fichiers audio")
                        .font(Theme.body(12))
                        .foregroundColor(Theme.textSecondary)
                }
            }
            .toggleStyle(.checkbox)

            Spacer()

            Toggle(isOn: Binding(
                get: { viewModel.state.exportSettings.enabled },
                set: { viewModel.state.exportSettings.enabled = $0 }
            )) {
                HStack(spacing: 6) {
                    Image(systemName: "square.and.arrow.up")
                        .foregroundColor(Theme.textTertiary)
                    Text("Exporter apres renommage")
                        .font(Theme.body(12))
                        .foregroundColor(Theme.textSecondary)
                }
            }
            .toggleStyle(.checkbox)
        }
        .padding(10)
        .premiumCard()
    }
}

// MARK: - Summary Item

struct SummaryItem: View {
    let label: String
    let value: String
    let color: Color

    var body: some View {
        VStack(spacing: 2) {
            Text(value)
                .font(Theme.monoMedium(16))
                .foregroundColor(color)
            Text(label)
                .font(Theme.body(10))
                .foregroundColor(Theme.textTertiary)
        }
    }
}

// MARK: - Action Row

struct ActionRow: View {
    let action: PreviewResponse.RenameAction

    var body: some View {
        HStack(spacing: 0) {
            Text(action.trackName)
                .font(Theme.body(11))
                .foregroundColor(Theme.textSecondary)
                .frame(width: 120, alignment: .leading)
                .lineLimit(1)

            Text(action.oldName)
                .font(Theme.mono(11))
                .foregroundColor(Theme.error.opacity(0.9))
                .frame(maxWidth: .infinity, alignment: .leading)
                .lineLimit(1)

            Image(systemName: "arrow.right")
                .font(.system(size: 9))
                .foregroundColor(Theme.textTertiary)
                .frame(width: 30)

            Text(action.newName)
                .font(Theme.monoMedium(11))
                .foregroundColor(Theme.success)
                .frame(maxWidth: .infinity, alignment: .leading)
                .lineLimit(1)

            Text(action.timeFormatted)
                .font(Theme.mono(10))
                .foregroundColor(Theme.textTertiary)
                .frame(width: 100, alignment: .trailing)
        }
        .padding(.horizontal, 12)
        .padding(.vertical, 6)
        .background(Theme.backgroundTertiary.opacity(0.2))
        .cornerRadius(4)
    }
}
