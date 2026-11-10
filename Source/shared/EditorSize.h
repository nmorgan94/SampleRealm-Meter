#pragma once

//==============================================================================
/** The editor's size in pixels, which the user can drag between these limits. As webui/src/Editor.tsx. */
namespace EditorSize
{
    inline constexpr int defaultWidth = 960, defaultHeight = 600;
    inline constexpr int minWidth = 800, minHeight = 500;
    inline constexpr int maxWidth = 1920, maxHeight = 1200;
}
