import SwiftUI

struct ContentView: View {
    @ObservedObject var viewModel: MarecViewModel

    var body: some View {
        ZStack {
            Theme.backgroundGradient
                .ignoresSafeArea()

            VStack(spacing: 0) {
                // Header
                HeaderView(step: viewModel.state.currentStep)

                // Step indicator
                StepIndicator(currentStep: viewModel.state.currentStep)
                    .padding(.horizontal, 24)
                    .padding(.top, 12)

                // Main content
                Group {
                    switch viewModel.state.currentStep {
                    case .connection:
                        ConnectionView(viewModel: viewModel)
                    case .trackSelection:
                        TrackSelectionView(viewModel: viewModel)
                    case .preview:
                        PreviewView(viewModel: viewModel)
                    case .executing:
                        ExecutionView(viewModel: viewModel)
                    case .results:
                        ResultsView(viewModel: viewModel)
                    }
                }
                .frame(maxWidth: .infinity, maxHeight: .infinity)
                .padding(24)

                // Footer
                FooterView()
            }
        }
        .frame(minWidth: 800, minHeight: 580)
    }
}

// MARK: - Header

struct HeaderView: View {
    let step: AppStep

    var body: some View {
        HStack {
            // Unified Saya MAREC branding
            HStack(spacing: 0) {
                // Saya icon — rounded rect with centered dash
                ZStack {
                    RoundedRectangle(cornerRadius: 4.5)
                        .stroke(Theme.textPrimary.opacity(0.9), lineWidth: 1.5)
                        .frame(width: 22, height: 15)

                    RoundedRectangle(cornerRadius: 0.8)
                        .fill(Theme.textPrimary.opacity(0.9))
                        .frame(width: 9, height: 1.5)
                }
                .padding(.trailing, 8)

                Text("SAYA")
                    .font(.system(size: 15, weight: .semibold))
                    .tracking(3)
                    .foregroundColor(Theme.textPrimary.opacity(0.9))

                // Thin vertical separator
                Rectangle()
                    .fill(Theme.textPrimary.opacity(0.25))
                    .frame(width: 1, height: 14)
                    .padding(.horizontal, 10)

                Text("MAREC")
                    .font(.system(size: 15, weight: .semibold))
                    .foregroundColor(Theme.textPrimary.opacity(0.9))
            }

            Spacer()

            Text(step.title)
                .font(Theme.bodyMedium(13))
                .foregroundColor(Theme.textTertiary)
                .accessibilityIdentifier(AID.currentStepLabel)
        }
        .padding(.horizontal, 24)
        .padding(.vertical, 12)
        .background(Theme.backgroundPrimary.opacity(0.8))
        .overlay(
            Rectangle()
                .fill(Theme.borderSubtle)
                .frame(height: 1),
            alignment: .bottom
        )
    }
}

// MARK: - Step Indicator

struct StepIndicator: View {
    let currentStep: AppStep

    var body: some View {
        HStack(spacing: 0) {
            ForEach(AppStep.allCases, id: \.rawValue) { step in
                HStack(spacing: 6) {
                    Circle()
                        .fill(fillColor(for: step))
                        .frame(width: 8, height: 8)

                    Text("\(step.stepNumber). \(step.title)")
                        .font(Theme.body(11))
                        .foregroundColor(textColor(for: step))
                }

                if step != AppStep.allCases.last {
                    Rectangle()
                        .fill(lineColor(for: step))
                        .frame(height: 1)
                        .frame(maxWidth: .infinity)
                        .padding(.horizontal, 8)
                }
            }
        }
        .padding(.vertical, 8)
        .padding(.horizontal, 12)
        .premiumCard(cornerRadius: Theme.radiusSmall)
    }

    private func fillColor(for step: AppStep) -> Color {
        if step.rawValue < currentStep.rawValue {
            return Theme.success
        } else if step == currentStep {
            return Theme.accent
        }
        return Theme.textTertiary.opacity(0.5)
    }

    private func textColor(for step: AppStep) -> Color {
        if step == currentStep {
            return Theme.textPrimary
        } else if step.rawValue < currentStep.rawValue {
            return Theme.textSecondary
        }
        return Theme.textTertiary
    }

    private func lineColor(for step: AppStep) -> Color {
        step.rawValue < currentStep.rawValue ? Theme.success.opacity(0.5) : Theme.borderSubtle
    }
}

// MARK: - Footer

struct FooterView: View {
    private var appVersion: String {
        Bundle.main.object(forInfoDictionaryKey: "CFBundleShortVersionString") as? String ?? "1.0"
    }

    var body: some View {
        HStack {
            Text("Version \(appVersion)  Copyright © 2026 Saya. All rights reserved.")
                .font(.system(size: 9.5))
                .foregroundColor(Theme.textTertiary)

            Spacer()
        }
        .padding(.horizontal, 24)
        .padding(.vertical, 10)
        .background(Theme.backgroundPrimary.opacity(0.8))
        .overlay(
            Rectangle()
                .fill(Theme.borderSubtle)
                .frame(height: 1),
            alignment: .top
        )
    }
}
