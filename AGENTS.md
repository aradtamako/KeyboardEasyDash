# AGENTS.md

KeyboardEasyDash はアラド戦記クライアントに注入される DLL プラグイン。キーボードだけで EasyDash を使うためのもの。
詳細は README.md を参照。

## 構成

- KeyboardEasyDash/Source.cpp: DllMain、DirectInput8 の GetDeviceState フック。DIJOYSTATE2 の lX に左右矢印キーを反映
- KeyboardEasyDash/ImGuiOverlay.cpp, ImGuiOverlay.hpp: DX11 SwapChain Present / ResizeBuffers フックと WndProc フックによる ImGui 設定画面
- KeyboardEasyDash/GuiState.hpp: g_easyDashEnabled、g_overlayVisible、g_targetAdded の共有状態
- KeyboardEasyDash/Util.hpp: ViGEm 初期化と有効/無効切替
- KeyboardEasyDash/VirtualGamePadState.cpp, VirtualGamePadState.hpp: ViGEm クライアントと DS4 ターゲットの保持
- KeyboardEasyDash/DetoursHelper.hpp: Detours の Attach / Detach 補助
- ViGEmClient.vcpkg: vigemclient の overlay-port。vcpkg-configuration.json から参照

## 動作の要点

- オーバーレイ表示切替は Shift + Insert。Insert 単体はゲームに渡す
- Present ポーリングと WndProc の二重トグル防止に g_insertPrevDown と g_insertComboSwallowed を使用。変更時は両経路を保つこと
- 無効時は仮想パッドをバスから外し、DirectInput フックは素通しにする
- 日本語フォントは meiryo.ttc、msgothic.ttc の順で探索。ImGuiOverlay.cpp のみ /utf-8 指定

## ビルド

- 前提: Visual Studio 2022、Windows 10 SDK、vcpkg マニフェストモード
- 構成は Release x64 を使用。Win32 構成は使用しない
- PlatformToolset は v145 指定。手元の VS に v145 が無い場合はコマンドラインでのみ -p:PlatformToolset=v143 を上書きし、vcxproj 自体は書き換えないこと
- Release x64 の vcpkg triplet は x64-windows-static-md
- ViGEmBus ドライバは実行時前提。ビルド成果物 DLL は Arad クライアントの plugins 配下に配置する

## 注意

- ゲームプロセス内で動くため、MessageBox やブロッキング処理の追加は避ける
- WndProc フックで横取りするキーは最小限にし、関係ない入力は必ず元の WndProc に渡す
- ビルド時の ViGEm Client.h の C4828 警告と LNK4099 警告は既知。pwsh 不在の後処理エラーも既知で無視してよい
