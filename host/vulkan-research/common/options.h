#pragma once
// Minimal renderer settings adapter; preserves fog/bloom and disables added post effects.
namespace game {
struct RenderOptions { bool fog=true,bloom=true,hdr=false;unsigned screen_filter=0,anti_aliasing=0; };
inline const RenderOptions& Opt(){static const RenderOptions options;return options;}
}
