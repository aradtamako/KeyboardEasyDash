#pragma once

namespace KeyboardEasyDash {

// DirectX 11 PresentフックによるImGuiオーバーレイを開始する。
// 成功時はtrueを返す。ゲーム側の初期化が未完了の場合はfalseを返すため、
// 呼び出し側でリトライすること。
bool InitializeOverlay();

// フックを解除する。
void ShutdownOverlay();

} // namespace KeyboardEasyDash
