#pragma once

#include <atomic>

namespace KeyboardEasyDash {

// EasyDash機能の有効/無効。ImGuiのラジオボタンから切り替える。
inline std::atomic<bool> g_easyDashEnabled{ true };
// オーバーレイWindowの表示/非表示。Insertキーで切り替える。
inline std::atomic<bool> g_overlayVisible{ true };
// ViGEmターゲットの追加状態。
inline std::atomic<bool> g_targetAdded{ false };

} // namespace KeyboardEasyDash
