// Viper unity build example, unity build means
// combine all library sources into one source file, use
// this if you want to build faster and avoid library linking

// Viper sources
#define VP_UNITY_BUILD
#include <viper/app.cpp>
#include <viper/gfx.cpp>
#include <viper/time.cpp>

// Venom ImGui sources
#include <venom/imgui.cpp>
