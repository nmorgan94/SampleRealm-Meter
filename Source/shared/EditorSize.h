#pragma once

//==============================================================================
/** The editor's size in pixels, which the user can drag between these limits. As webui/src/Editor.tsx. */
namespace EditorSize
{
    inline constexpr int defaultWidth = 960, defaultHeight = 600;
    // At its shortest the dashboard is one row, just tall enough for the loudness readings
    inline constexpr int minWidth = 800, minHeight = 220;
    inline constexpr int maxWidth = 1920, maxHeight = 1200;
}
