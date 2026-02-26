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
        HStack(spacing: 12) {
            // App icon placeholder
            RoundedRectangle(cornerRadius: 8)
                .fill(Theme.accentGradient)
                .frame(width: 32, height: 32)
                .overlay(
                    Text("M")
                        .font(.system(size: 18, weight: .bold, design: .rounded))
                        .foregroundColor(Theme.backgroundPrimary)
                )

            Text("SAYA")
                .font(Theme.bodySemiBold(14))
                .foregroundColor(Theme.textSecondary)

            Rectangle()
                .fill(Theme.borderSubtle)
                .frame(width: 1, height: 18)

            Text("MAREC")
                .font(Theme.bodySemiBold(16))
                .foregroundColor(Theme.textPrimary)

            Spacer()

            Text(step.title)
                .font(Theme.bodyMedium(13))
                .foregroundColor(Theme.textTertiary)
                .accessibilityIdentifier(AID.currentStepLabel)
        }
        .padding(.horizontal, 24)
        .padding(.vertical, 14)
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
    var body: some View {
        HStack {
            Text("MAREC v1.0")
                .font(Theme.mono(10))
                .foregroundColor(Theme.textTertiary)

            Spacer()

            Text("Audi'Art — Groupe Auditorium Artistique")
                .font(Theme.body(10))
                .foregroundColor(Theme.textTertiary)
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
