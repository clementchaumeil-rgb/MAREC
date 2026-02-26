import SwiftUI

// MARK: - SAYA MAREC Theme
// Deep navy blues with cyan/white accents — adapted from MediaDiff

enum Theme {
    // MARK: - Primary Background Colors (Navy Blues)

    static let backgroundPrimary = Color(red: 0.039, green: 0.082, blue: 0.145)
    static let backgroundSecondary = Color(red: 0.075, green: 0.145, blue: 0.220)
    static let backgroundTertiary = Color(red: 0.110, green: 0.192, blue: 0.290)
    static let panelBackground = Color(red: 0.130, green: 0.200, blue: 0.300).opacity(0.7)

    // MARK: - Accent Colors (Cyan/Teal)

    static let accent = Color(red: 0.290, green: 0.612, blue: 0.753)
    static let accentLight = Color(red: 0.478, green: 0.753, blue: 0.878)
    static let accentDark = Color(red: 0.200, green: 0.450, blue: 0.580)

    // MARK: - Semantic Colors

    static let error = Color(red: 0.780, green: 0.380, blue: 0.380)
    static let success = Color(red: 0.310, green: 0.680, blue: 0.520)
    static let warning = Color(red: 0.850, green: 0.650, blue: 0.300)

    // MARK: - Text Colors

    static let textPrimary = Color.white
    static let textSecondary = Color.white.opacity(0.75)
    static let textTertiary = Color.white.opacity(0.5)
    static let textAccent = accent

    // MARK: - Border Colors

    static let borderAccent = accent.opacity(0.4)
    static let borderSubtle = Color.white.opacity(0.10)
    static let borderError = error.opacity(0.5)

    // MARK: - Gradients

    static let backgroundGradient = LinearGradient(
        colors: [
            backgroundPrimary,
            backgroundSecondary,
            Color(red: 0.063, green: 0.157, blue: 0.255)
        ],
        startPoint: .topLeading,
        endPoint: .bottomTrailing
    )

    static let panelGradient = LinearGradient(
        colors: [
            Color(red: 0.130, green: 0.200, blue: 0.300).opacity(0.6),
            Color(red: 0.090, green: 0.155, blue: 0.235).opacity(0.8)
        ],
        startPoint: .topLeading,
        endPoint: .bottomTrailing
    )

    static let accentGradient = LinearGradient(
        colors: [accentLight, accent, accentDark],
        startPoint: .topLeading,
        endPoint: .bottomTrailing
    )

    static let successGradient = LinearGradient(
        colors: [
            Color(red: 0.35, green: 0.75, blue: 0.55),
            success,
            Color(red: 0.25, green: 0.55, blue: 0.40)
        ],
        startPoint: .topLeading,
        endPoint: .bottomTrailing
    )

    static let errorGradient = LinearGradient(
        colors: [
            Color(red: 0.9, green: 0.3, blue: 0.3),
            Color(red: 0.78, green: 0.2, blue: 0.2),
            Color(red: 0.6, green: 0.15, blue: 0.15)
        ],
        startPoint: .topLeading,
        endPoint: .bottomTrailing
    )

    // MARK: - Shadows

    static let shadowColor = Color.black.opacity(0.5)
    static let glowAccent = accent.opacity(0.20)

    // MARK: - Corner Radii

    static let radiusSmall: CGFloat = 6
    static let radiusMedium: CGFloat = 10
    static let radiusLarge: CGFloat = 14
    static let radiusXLarge: CGFloat = 20

    // MARK: - Fonts (system fallbacks)

    static func body(_ size: CGFloat = 13) -> Font {
        .system(size: size, weight: .regular)
    }

    static func bodyMedium(_ size: CGFloat = 13) -> Font {
        .system(size: size, weight: .medium)
    }

    static func bodySemiBold(_ size: CGFloat = 13) -> Font {
        .system(size: size, weight: .semibold)
    }

    static func bodyBold(_ size: CGFloat = 13) -> Font {
        .system(size: size, weight: .bold)
    }

    static func mono(_ size: CGFloat = 12) -> Font {
        .system(size: size, weight: .regular, design: .monospaced)
    }

    static func monoMedium(_ size: CGFloat = 12) -> Font {
        .system(size: size, weight: .medium, design: .monospaced)
    }
}

// MARK: - View Modifiers

extension View {
    func premiumPanel(cornerRadius: CGFloat = Theme.radiusMedium) -> some View {
        self
            .background(
                RoundedRectangle(cornerRadius: cornerRadius)
                    .fill(Theme.panelGradient)
            )
            .overlay(
                RoundedRectangle(cornerRadius: cornerRadius)
                    .stroke(Theme.borderAccent, lineWidth: 1)
            )
            .shadow(color: Theme.shadowColor, radius: 20, y: 10)
    }

    func premiumCard(cornerRadius: CGFloat = Theme.radiusSmall) -> some View {
        self
            .background(
                RoundedRectangle(cornerRadius: cornerRadius)
                    .fill(Theme.backgroundTertiary.opacity(0.5))
            )
            .overlay(
                RoundedRectangle(cornerRadius: cornerRadius)
                    .stroke(Theme.borderSubtle, lineWidth: 1)
            )
    }
}

// MARK: - Button Styles

struct PremiumButtonStyle: ButtonStyle {
    var isPrimary: Bool = true

    func makeBody(configuration: Configuration) -> some View {
        configuration.label
            .font(Theme.bodyMedium())
            .foregroundColor(isPrimary ? Theme.backgroundPrimary : Theme.textAccent)
            .padding(.horizontal, 16)
            .padding(.vertical, 8)
            .background(
                RoundedRectangle(cornerRadius: Theme.radiusSmall)
                    .fill(isPrimary ? Theme.accentGradient : Theme.panelGradient)
            )
            .overlay(
                RoundedRectangle(cornerRadius: Theme.radiusSmall)
                    .stroke(Theme.borderAccent, lineWidth: isPrimary ? 0 : 1)
            )
            .opacity(configuration.isPressed ? 0.8 : 1.0)
            .scaleEffect(configuration.isPressed ? 0.98 : 1.0)
            .animation(.easeOut(duration: 0.15), value: configuration.isPressed)
    }
}

struct PremiumCancelButtonStyle: ButtonStyle {
    func makeBody(configuration: Configuration) -> some View {
        configuration.label
            .font(Theme.bodyMedium())
            .foregroundColor(Theme.error)
            .padding(.horizontal, 16)
            .padding(.vertical, 8)
            .background(
                RoundedRectangle(cornerRadius: Theme.radiusSmall)
                    .fill(Theme.error.opacity(0.15))
            )
            .overlay(
                RoundedRectangle(cornerRadius: Theme.radiusSmall)
                    .stroke(Theme.borderError, lineWidth: 1)
            )
            .opacity(configuration.isPressed ? 0.8 : 1.0)
            .animation(.easeOut(duration: 0.15), value: configuration.isPressed)
    }
}
