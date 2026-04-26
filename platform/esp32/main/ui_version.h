/**
 * UI Version Header
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-19
 * Date Updated: 2025-12-23
 * Version: 1.5.4
 *
 * IMPORTANT: UI_VERSION must match the version in docs/UI_Screens.md
 *
 * Changelog:
 * - 1.5.4 (2025-12-23): Disabled auto-hide until touch input implemented, removed test timer
 * - 1.5.3 (2025-12-23): CRITICAL FIX - LVGL task was sleeping 49 days when no timers exist (capped max delay to 100ms)
 * - 1.5.2 (2025-12-23): Fixed timer callback - use repeating timer that pauses after hiding
 * - 1.5.1 (2025-12-23): Fixed footer auto-hide timer (one-shot instead of repeating)
 * - 1.5.0 (2025-12-23): Added SF Pro Rounded font (Apple's rounded system font)
 * - 1.4.0 (2025-12-23): Added custom fonts (GolosText, Poppins, Orbitron, Symbols), footer uses GolosText
 * - 1.3.8 (2025-12-23): Fixed font (montserrat_16), added event debug logging
 * - 1.3.7 (2025-12-23): Set log level to DEBUG, larger logo (273px)
 * - 1.3.6 (2025-12-23): Fixed test pattern script - detect white areas, circular logo mask
 * - 1.3.5 (2025-12-22): Auto-hide footer, event logging, improved buttons, custom test pattern
 * - 1.3.4 (2025-12-22): Splash logo working (compiled C array, RGB565)
 * - 1.3.3 (2025-12-22): Simplified PNG loading + placeholder fallback
 * - 1.3.2 (2025-12-22): Added splash logo (debugging)
 * - 1.3.1 (2025-12-22): LVGL graphical splash screen fully working
 * - 1.3.0 (2025-12-20): Initial LVGL integration
 */

#ifndef UI_VERSION_H
#define UI_VERSION_H

// UI Version - MUST match docs/UI_Screens.md version
#define UI_VERSION_MAJOR    1
#define UI_VERSION_MINOR    5
#define UI_VERSION_PATCH    4
#define UI_VERSION_STRING   "1.5.4"

// Build information
#define UI_BUILD_DATE       __DATE__
#define UI_BUILD_TIME       __TIME__

#endif // UI_VERSION_H
