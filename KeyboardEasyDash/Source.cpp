#include <Windows.h>
#include <iostream>

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

#include "DetoursHelper.hpp"

#define FUNCTION_INDEX_GET_DEVICE_STATE 9

LPDIRECTINPUT8 pDirectInput = nullptr;
LPDIRECTINPUTDEVICE8 pDirectInputDevice = nullptr;
HRESULT(*funcGetDeviceState)(IDirectInputDevice8* pThis, DWORD cbData, LPVOID lpvData);

uintptr_t GetVirtualFunctionAddress(uintptr_t base, int functionIndex) {
	auto pFunction = reinterpret_cast<uintptr_t*>(base + (functionIndex * sizeof(uintptr_t)));
	return *reinterpret_cast<uintptr_t*>(pFunction);
}

void DetourGetDeviceState() {
	static auto OriginalFunction = funcGetDeviceState;

	decltype(funcGetDeviceState) HookFunction = [](IDirectInputDevice8* pThis, DWORD cbData, LPVOID lpvData) -> HRESULT {
		auto result = OriginalFunction(pThis, cbData, lpvData);

		switch (cbData) {
			case sizeof(DIJOYSTATE) : {
				break;
			}
			case sizeof(DIJOYSTATE2) : {
				auto data = reinterpret_cast<DIJOYSTATE2*>(lpvData);

				// EasyDash for keyboard
				if (GetAsyncKeyState(VK_LEFT) & 0x8000) {
					data->lX = -1000;
				}
				else if (GetAsyncKeyState(VK_RIGHT) & 0x8000) {
					data->lX = 1000;
				}
				break;
			}
		}

		return result;
		};

	DetourFunction(true, reinterpret_cast<PVOID*>(&OriginalFunction), HookFunction);
}

BOOL CALLBACK EnumDevicesCallback(const LPCDIDEVICEINSTANCE pDirectInputDeviceInstance, VOID* pContext) {
	if (pDirectInput->CreateDevice(pDirectInputDeviceInstance->guidInstance, &pDirectInputDevice, NULL) != DI_OK) {
		pDirectInput->Release();
		return DIENUM_CONTINUE;
	}
	else {
		funcGetDeviceState = reinterpret_cast<decltype(funcGetDeviceState)>(GetVirtualFunctionAddress(*reinterpret_cast<uintptr_t*>(pDirectInputDevice), FUNCTION_INDEX_GET_DEVICE_STATE));
		DetourGetDeviceState();
		return DIENUM_STOP;
	}
}

void WaitForDirectInputLoad() {
	while (GetModuleHandle(L"dinput8.dll") == NULL) {
		Sleep(1000);
	}
}

void MainThread() {
	WaitForDirectInputLoad();

	auto hInstance = GetModuleHandle(NULL);

	while (true) {
		if (DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, reinterpret_cast<LPVOID*>(&pDirectInput), NULL) != DI_OK) {
			// std::wcout << "DirectInput8Create failed!" << std::endl;
			continue;
		}

		if (pDirectInput->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumDevicesCallback, NULL, DIEDFL_ATTACHEDONLY) != DI_OK) {
			// std::wcout << "Failed to enumerate devices" << std::endl;
			continue;
		}

		Sleep(10 * 1000);
	}
}

#pragma unmanaged
BOOL WINAPI DllMain(HINSTANCE hinstModule, DWORD dwReason, LPVOID lpvReserved) {
	if (dwReason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(hinstModule);

		CreateThread(NULL, NULL, reinterpret_cast<LPTHREAD_START_ROUTINE>(&MainThread), NULL, NULL, NULL);
	}

	return TRUE;
}