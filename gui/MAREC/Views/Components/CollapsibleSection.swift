import SwiftUI

struct CollapsibleSection<Content: View>: View {
    let title: String
    let count: Int
    let subtitle: String?
    let accentColor: Color
    @Binding var isExpanded: Bool
    @ViewBuilder let content: () -> Content

    init(
        title: String,
        count: Int,
        subtitle: String? = nil,
        accentColor: Color = Theme.textPrimary,
        isExpanded: Binding<Bool>,
        @ViewBuilder content: @escaping () -> Content
    ) {
        self.title = title
        self.count = count
        self.subtitle = subtitle
        self.accentColor = accentColor
        self._isExpanded = isExpanded
        self.content = content
    }

    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            // Header
            Button(action: {
                withAnimation(.easeInOut(duration: 0.2)) {
                    isExpanded.toggle()
                }
            }) {
                HStack(spacing: 8) {
                    Image(systemName: "chevron.right")
                        .font(.system(size: 10, weight: .semibold))
                        .foregroundColor(Theme.textTertiary)
                        .rotationEffect(.degrees(isExpanded ? 90 : 0))

                    Text(title)
                        .font(Theme.bodySemiBold(12))
                        .foregroundColor(Theme.textPrimary)

                    Text("\(count)")
                        .font(Theme.monoMedium(11))
                        .foregroundColor(accentColor)
                        .padding(.horizontal, 6)
                        .padding(.vertical, 2)
                        .background(accentColor.opacity(0.15))
                        .cornerRadius(4)

                    if let subtitle {
                        Text(subtitle)
                            .font(Theme.body(11))
                            .foregroundColor(Theme.textTertiary)
                    }

                    Spacer()
                }
                .padding(.horizontal, 12)
                .padding(.vertical, 8)
                .background(Theme.backgroundTertiary.opacity(0.3))
                .cornerRadius(Theme.radiusSmall)
            }
            .buttonStyle(.plain)

            // Content
            if isExpanded {
                content()
                    .transition(.opacity.combined(with: .move(edge: .top)))
            }
        }
    }
}
