#pragma once

#include <Windows.h>
#include <ViGEm/Client.h>
#pragma comment(lib, "setupapi.lib")

#include "MainForm.h"

namespace KeyboardEasyDash {

	using namespace System::Windows::Forms;

	PVIGEM_TARGET InitializeVirtualGamePad() {
		VirtualGamePadState::Client = vigem_alloc();
		auto err = vigem_connect(VirtualGamePadState::Client);

		if (!VIGEM_SUCCESS(err)) {
			vigem_free(VirtualGamePadState::Client);
			VirtualGamePadState::Client = nullptr;
			::MessageBox(nullptr, L"Failed initialize ViGEm", L"Error", MB_OK);

			return nullptr;
		}

		VirtualGamePadState::Target = vigem_target_ds4_alloc();
		vigem_target_add(VirtualGamePadState::Client, VirtualGamePadState::Target);

		return VirtualGamePadState::Target;
	}

	void InitializeMainForm() {
		Application::EnableVisualStyles();
		Application::SetCompatibleTextRenderingDefault(false);
		KeyboardEasyDash::MainForm form;
		Application::Run(% form);
	}
};
