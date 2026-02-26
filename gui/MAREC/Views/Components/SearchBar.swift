import SwiftUI

struct SearchBar: View {
    @Binding var text: String
    var placeholder: String = "Rechercher..."

    var body: some View {
        HStack(spacing: 8) {
            Image(systemName: "magnifyingglass")
                .foregroundColor(Theme.textTertiary)
                .font(.system(size: 12))

            TextField(placeholder, text: $text)
                .font(Theme.body(12))
                .foregroundColor(Theme.textPrimary)
                .textFieldStyle(.plain)

            if !text.isEmpty {
                Button(action: { text = "" }) {
                    Image(systemName: "xmark.circle.fill")
                        .foregroundColor(Theme.textTertiary)
                        .font(.system(size: 12))
                }
                .buttonStyle(.plain)
            }
        }
        .padding(.horizontal, 10)
        .padding(.vertical, 6)
        .background(Theme.backgroundTertiary.opacity(0.5))
        .cornerRadius(Theme.radiusSmall)
        .overlay(
            RoundedRectangle(cornerRadius: Theme.radiusSmall)
                .stroke(Theme.borderSubtle, lineWidth: 1)
        )
    }
}
