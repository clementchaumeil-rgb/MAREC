import SwiftUI

struct TrackSelectionView: View {
    @ObservedObject var viewModel: MarecViewModel
    @State private var markersExpanded = false
    @State private var trackSearchText = ""

    var body: some View {
        VStack(spacing: 16) {
            // Session info bar
            sessionInfoBar

            // Track list
            VStack(alignment: .leading, spacing: 8) {
                HStack {
                    Text("Pistes audio")
                        .font(Theme.bodySemiBold(14))
                        .foregroundColor(Theme.textPrimary)

                    Spacer()

                    Button(action: toggleAll) {
                        Text(allSelected ? "Tout deselectionner" : "Tout selectionner")
                            .font(Theme.body(11))
                            .foregroundColor(Theme.textAccent)
                    }
                    .buttonStyle(.plain)
                    .accessibilityIdentifier(AID.selectAllButton)
                }

                if viewModel.state.availableTracks.count > 10 {
                    SearchBar(text: $trackSearchText, placeholder: "Filtrer les pistes...")
                }

                ScrollView {
                    LazyVStack(spacing: 4) {
                        ForEach(filteredTracks) { track in
                            TrackRow(
                                track: track,
                                isSelected: viewModel.state.selectedTrackNames.contains(track.name),
                                onToggle: { toggleTrack(track.name) }
                            )
                        }
                    }
                }
                .frame(maxHeight: .infinity)
                .premiumCard()
            }
            .premiumPanel()
            .padding(1)

            // Markers summary
            markersSummary

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
                .accessibilityIdentifier(AID.backButtonStep2)

                Spacer()

                Text("\(viewModel.state.selectedTrackNames.count) piste(s)")
                    .font(Theme.body(12))
                    .foregroundColor(Theme.textTertiary)

                if viewModel.state.isLoading, let progress = viewModel.state.progressMessage {
                    Text(progress)
                        .font(Theme.mono(11))
                        .foregroundColor(Theme.textSecondary)
                        .transition(.opacity)
                }

                Button(action: {
                    Task { await viewModel.preview() }
                }) {
                    HStack(spacing: 6) {
                        if viewModel.state.isLoading {
                            ProgressView()
                                .progressViewStyle(.circular)
                                .scaleEffect(0.7)
                                .frame(width: 14, height: 14)
                        }
                        Text("Apercu")
                        Image(systemName: "arrow.right")
                    }
                }
                .buttonStyle(PremiumButtonStyle())
                .disabled(viewModel.state.selectedTrackNames.isEmpty || viewModel.state.isLoading)
                .accessibilityIdentifier(AID.previewButton)
            }
        }
    }

    // MARK: - Subviews

    private var sessionInfoBar: some View {
        HStack(spacing: 16) {
            Label {
                Text("\(viewModel.state.sampleRate) Hz")
                    .font(Theme.monoMedium(12))
                    .foregroundColor(Theme.textPrimary)
            } icon: {
                Image(systemName: "waveform")
                    .foregroundColor(Theme.accent)
            }

            Divider().frame(height: 16)

            Label {
                Text("\(viewModel.state.markers.count) marqueur(s)")
                    .font(Theme.monoMedium(12))
                    .foregroundColor(Theme.textPrimary)
            } icon: {
                Image(systemName: "mappin")
                    .foregroundColor(Theme.accent)
            }
            .accessibilityIdentifier(AID.markerCount)

            Divider().frame(height: 16)

            Label {
                Text("\(viewModel.state.availableTracks.count) piste(s)")
                    .font(Theme.monoMedium(12))
                    .foregroundColor(Theme.textPrimary)
            } icon: {
                Image(systemName: "slider.horizontal.3")
                    .foregroundColor(Theme.accent)
            }
            .accessibilityIdentifier(AID.trackCount)

            Spacer()
        }
        .padding(10)
        .premiumCard()
    }

    private var markersSummary: some View {
        VStack(alignment: .leading, spacing: 6) {
            // Collapsed header with summary
            Button(action: {
                withAnimation(.easeInOut(duration: 0.2)) {
                    markersExpanded.toggle()
                }
            }) {
                HStack(spacing: 8) {
                    Image(systemName: "chevron.right")
                        .font(.system(size: 10, weight: .semibold))
                        .foregroundColor(Theme.textTertiary)
                        .rotationEffect(.degrees(markersExpanded ? 90 : 0))

                    Text("Marqueurs detectes")
                        .font(Theme.bodySemiBold(12))
                        .foregroundColor(Theme.textSecondary)

                    Text("\(viewModel.state.markers.count)")
                        .font(Theme.monoMedium(11))
                        .foregroundColor(Theme.accent)
                        .padding(.horizontal, 6)
                        .padding(.vertical, 2)
                        .background(Theme.accent.opacity(0.15))
                        .cornerRadius(4)

                    // Preview of first markers when collapsed
                    if !markersExpanded {
                        markerPreviewChips
                    }

                    Spacer()
                }
            }
            .buttonStyle(.plain)

            // Expanded: vertical scrollable list
            if markersExpanded {
                ScrollView {
                    LazyVStack(spacing: 2) {
                        ForEach(viewModel.state.markers) { marker in
                            HStack(spacing: 8) {
                                Text("#\(marker.number)")
                                    .font(Theme.mono(10))
                                    .foregroundColor(Theme.textTertiary)
                                    .frame(width: 30, alignment: .trailing)

                                Text(marker.name)
                                    .font(Theme.monoMedium(11))
                                    .foregroundColor(Theme.textPrimary)
                                    .lineLimit(1)

                                Spacer()

                                Text(marker.startTime)
                                    .font(Theme.mono(10))
                                    .foregroundColor(Theme.textTertiary)
                            }
                            .padding(.horizontal, 10)
                            .padding(.vertical, 4)
                            .background(Theme.backgroundTertiary.opacity(0.2))
                            .cornerRadius(4)
                        }
                    }
                }
                .frame(maxHeight: 200)
                .transition(.opacity.combined(with: .move(edge: .top)))
            }
        }
        .padding(10)
        .premiumCard()
    }

    /// Shows first few markers as chips when the section is collapsed
    @ViewBuilder
    private var markerPreviewChips: some View {
        let previewCount = min(viewModel.state.markers.count, 5)
        let markers = Array(viewModel.state.markers.prefix(previewCount))

        HStack(spacing: 4) {
            ForEach(markers) { marker in
                Text(marker.name)
                    .font(Theme.mono(10))
                    .foregroundColor(Theme.textTertiary)
                    .padding(.horizontal, 6)
                    .padding(.vertical, 2)
                    .background(Theme.backgroundTertiary.opacity(0.5))
                    .cornerRadius(4)
                    .lineLimit(1)
            }

            if viewModel.state.markers.count > previewCount {
                Text("+\(viewModel.state.markers.count - previewCount)")
                    .font(Theme.mono(10))
                    .foregroundColor(Theme.textTertiary)
            }
        }
    }

    // MARK: - Computed

    private var filteredTracks: [TracksResponse.TrackItem] {
        if trackSearchText.isEmpty {
            return viewModel.state.availableTracks
        }
        let query = trackSearchText.lowercased()
        return viewModel.state.availableTracks.filter {
            $0.name.lowercased().contains(query)
        }
    }

    // MARK: - Helpers

    private var allSelected: Bool {
        viewModel.state.selectedTrackNames.count == viewModel.state.availableTracks.count
    }

    private func toggleAll() {
        if allSelected {
            viewModel.state.selectedTrackNames.removeAll()
        } else {
            viewModel.state.selectedTrackNames = Set(viewModel.state.availableTracks.map(\.name))
        }
    }

    private func toggleTrack(_ name: String) {
        if viewModel.state.selectedTrackNames.contains(name) {
            viewModel.state.selectedTrackNames.remove(name)
        } else {
            viewModel.state.selectedTrackNames.insert(name)
        }
    }
}

// MARK: - Track Row

struct TrackRow: View {
    let track: TracksResponse.TrackItem
    let isSelected: Bool
    let onToggle: () -> Void

    var body: some View {
        Button(action: onToggle) {
            HStack(spacing: 10) {
                Image(systemName: isSelected ? "checkmark.circle.fill" : "circle")
                    .foregroundColor(isSelected ? Theme.accent : Theme.textTertiary)
                    .font(.system(size: 16))

                Text(track.name)
                    .font(Theme.bodyMedium(13))
                    .foregroundColor(Theme.textPrimary)

                Spacer()

                Text("idx \(track.index)")
                    .font(Theme.mono(10))
                    .foregroundColor(Theme.textTertiary)
            }
            .padding(.horizontal, 12)
            .padding(.vertical, 8)
            .background(isSelected ? Theme.accent.opacity(0.08) : Color.clear)
            .cornerRadius(Theme.radiusSmall)
        }
        .buttonStyle(.plain)
        .accessibilityIdentifier(AID.trackRow(track.name))
    }
}

// MARK: - Error Banner (reusable)

struct ErrorBanner: View {
    let message: String

    var body: some View {
        HStack(spacing: 8) {
            Image(systemName: "exclamationmark.triangle.fill")
                .foregroundColor(Theme.error)
            Text(message)
                .font(Theme.body(12))
                .foregroundColor(Theme.error)
            Spacer()
        }
        .padding(12)
        .background(Theme.error.opacity(0.1))
        .cornerRadius(Theme.radiusSmall)
        .overlay(
            RoundedRectangle(cornerRadius: Theme.radiusSmall)
                .stroke(Theme.borderError, lineWidth: 1)
        )
    }
}
