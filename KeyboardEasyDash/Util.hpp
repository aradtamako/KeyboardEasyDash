#pragma once

#include <Windows.h>
#include <ViGEm/Client.h>
#pragma comment(lib, "setupapi.lib")

#include "VirtualGamePadState.hpp"
#include "GuiState.hpp"

namespace KeyboardEasyDash {

	inline PVIGEM_TARGET InitializeVirtualGamePad() {
		VirtualGamePadState::Client = vigem_alloc();
		auto err = vigem_connect(VirtualGamePadState::Client);

		if (!VIGEM_SUCCESS(err)) {
			vigem_free(VirtualGamePadState::Client);
			VirtualGamePadState::Client = nullptr;
			::MessageBox(nullptr, L"Failed initialize ViGEm", L"Error", MB_OK);

			return nullptr;
		}

		VirtualGamePadState::Target = vigem_target_ds4_alloc();
		if (VIGEM_SUCCESS(vigem_target_add(VirtualGamePadState::Client, VirtualGamePadState::Target))) {
			g_targetAdded.store(true);
		}

		return VirtualGamePadState::Target;
	}

	// ImGuiのラジオボタンから有効/無効を切り替える。
	// 無効時は仮想パッドをバスから取り外し、DirectInputフックも素通しにする。
	inline void SetEasyDashEnabled(bool enabled) {
		g_easyDashEnabled.store(enabled);

		if (VirtualGamePadState::Client == nullptr || VirtualGamePadState::Target == nullptr) {
			return;
		}
		if (enabled && !g_targetAdded.load()) {
			if (VIGEM_SUCCESS(vigem_target_add(VirtualGamePadState::Client, VirtualGamePadState::Target))) {
				g_targetAdded.store(true);
			}
		} else if (!enabled && g_targetAdded.load()) {
			if (VIGEM_SUCCESS(vigem_target_remove(VirtualGamePadState::Client, VirtualGamePadState::Target))) {
				g_targetAdded.store(false);
			}
		}
	}
};
