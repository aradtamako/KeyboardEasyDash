// ImGui overlay for DirectX 11 (Arad / 2D game).
// Hooks IDXGISwapChain::Present via Detours and renders a small
// settings window with two radio buttons.

#include "ImGuiOverlay.hpp"

#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#include "DetoursHelper.hpp"
#include "GuiState.hpp"
#include "Util.hpp"
#include "VirtualGamePadState.hpp"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace KeyboardEasyDash {
namespace {

// Present / ResizeBuffers are __stdcall member functions of IDXGISwapChain.
using PresentFn = HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain* pSwapChain, UINT syncInterval, UINT flags);
using ResizeBuffersFn = HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain* pSwapChain, UINT bufferCount, UINT width, UINT height,
                                                    DXGI_FORMAT newFormat, UINT swapChainFlags);

constexpr int kPresentVtableIndex = 8;
constexpr int kResizeBuffersVtableIndex = 13;

LRESULT CALLBACK HookWndProc_Impl(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void PollInsertToggleKey();

PresentFn g_originalPresent = nullptr;
ResizeBuffersFn g_originalResizeBuffers = nullptr;

WNDPROC g_originalWndProc = nullptr;
HWND g_hWnd = nullptr;

ID3D11Device* g_device = nullptr;
ID3D11DeviceContext* g_context = nullptr;
ID3D11RenderTargetView* g_renderTargetView = nullptr;

bool g_imguiInitialized = false;
bool g_hookInstalled = false;
bool g_insertPrevDown = false;

void* GetVtableFunction(void* instance, int index) {
  auto vtable = *reinterpret_cast<void***>(instance);
  return vtable[index];
}

void PollInsertToggle() {
  const bool down = (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;
  if (down && !g_insertPrevDown) {
    g_overlayVisible.store(!g_overlayVisible.load());
  }
  g_insertPrevDown = down;
}

void ReleaseRenderTarget() {
  if (g_renderTargetView != nullptr) {
    g_renderTargetView->Release();
    g_renderTargetView = nullptr;
  }
}

bool EnsureRenderTarget(IDXGISwapChain* pSwapChain) {
  if (g_renderTargetView != nullptr) {
    return true;
  }
  ID3D11Texture2D* pBackBuffer = nullptr;
  if (FAILED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer)))) {
    return false;
  }
  const HRESULT hr = g_device->CreateRenderTargetView(pBackBuffer, nullptr, &g_renderTargetView);
  pBackBuffer->Release();
  return SUCCEEDED(hr);
}

void SetupJapaneseFont() {
  ImGuiIO& io = ImGui::GetIO();
  // アラド戦記のUI文言を表示するため、日本語グリフを追加する。
  constexpr const char* kFontCandidates[] = {
      "C:\\Windows\\Fonts\\meiryo.ttc",
      "C:\\Windows\\Fonts\\msgothic.ttc",
  };
  for (const char* path : kFontCandidates) {
    if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES) {
      continue;
    }
    if (io.Fonts->AddFontFromFileTTF(path, 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese()) != nullptr) {
      return;
    }
  }
  io.Fonts->AddFontDefault();
}

bool InitializeImGui(IDXGISwapChain* pSwapChain) {
  if (FAILED(pSwapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&g_device)))) {
    return false;
  }
  g_device->GetImmediateContext(&g_context);

  DXGI_SWAP_CHAIN_DESC desc{};
  pSwapChain->GetDesc(&desc);
  g_hWnd = desc.OutputWindow;

  // ゲームWindowのWndProcをフックし、表示中はImGuiにマウス入力を渡す。
  g_originalWndProc =
      reinterpret_cast<WNDPROC>(SetWindowLongPtr(g_hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&HookWndProc_Impl)));
  if (g_originalWndProc == nullptr) {
    return false;
  }

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.IniFilename = nullptr;
  io.LogFilename = nullptr;

  SetupJapaneseFont();
  ImGui::StyleColorsDark();

  if (!ImGui_ImplWin32_Init(g_hWnd)) {
    return false;
  }
  if (!ImGui_ImplDX11_Init(g_device, g_context)) {
    ImGui_ImplWin32_Shutdown();
    return false;
  }
  g_imguiInitialized = true;
  return true;
}

void RenderSettingsWindow() {
  ImGui::Begin("KeyboardEasyDash", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
  ImGui::TextUnformatted("EasyDash:");
  const bool enabled = g_easyDashEnabled.load();
  if (ImGui::RadioButton("有効", enabled)) {
    SetEasyDashEnabled(true);
  }
  if (ImGui::RadioButton("無効", !enabled)) {
    SetEasyDashEnabled(false);
  }
  ImGui::Spacing();
  ImGui::TextDisabled("Insert: 表示/非表示");
  ImGui::End();
}

HRESULT STDMETHODCALLTYPE HookPresent(IDXGISwapChain* pSwapChain, UINT syncInterval, UINT flags) {
  PollInsertToggle();

  if (!g_imguiInitialized) {
    if (!InitializeImGui(pSwapChain)) {
      return g_originalPresent(pSwapChain, syncInterval, flags);
    }
  }

  if (g_overlayVisible.load() && EnsureRenderTarget(pSwapChain)) {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    RenderSettingsWindow();

    ImGui::Render();
    g_context->OMSetRenderTargets(1, &g_renderTargetView, nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
  }

  return g_originalPresent(pSwapChain, syncInterval, flags);
}

HRESULT STDMETHODCALLTYPE HookResizeBuffers(IDXGISwapChain* pSwapChain, UINT bufferCount, UINT width, UINT height,
                                            DXGI_FORMAT newFormat, UINT swapChainFlags) {
  ReleaseRenderTarget();
  return g_originalResizeBuffers(pSwapChain, bufferCount, width, height, newFormat, swapChainFlags);
}

LRESULT CALLBACK HookWndProc_Impl(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  // Insertは表示/非表示の切り替えに使う。ゲーム側へは渡さない。
  if (msg == WM_KEYDOWN && wParam == VK_INSERT) {
    PollInsertToggleKey();
    return 0;
  }
  if (msg == WM_KEYUP && wParam == VK_INSERT) {
    return 0;
  }

  if (!g_overlayVisible.load()) {
    return CallWindowProc(g_originalWndProc, hWnd, msg, wParam, lParam);
  }

  // 表示中はImGuiに先に処理させ、キャプチャされた入力はゲームに渡さない。
  if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam) != 0) {
    return 0;
  }
  const ImGuiIO& io = ImGui::GetIO();
  const bool isMouse = (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST) || msg == WM_MOUSEWHEEL || msg == WM_MOUSEHWHEEL;
  const bool isKeyboard = (msg >= WM_KEYFIRST && msg <= WM_KEYLAST) || msg == WM_CHAR;
  if ((isMouse && io.WantCaptureMouse) || (isKeyboard && io.WantCaptureKeyboard)) {
    return 0;
  }
  return CallWindowProc(g_originalWndProc, hWnd, msg, wParam, lParam);
}

// WM_KEYDOWNはGetAsyncKeyStateのポーリングと二重に反応するため、
// WndProc側はキー状態のエッジを見てInsertを処理する。
void PollInsertToggleKey() {
  g_overlayVisible.store(!g_overlayVisible.load());
  g_insertPrevDown = true;
}

// ゲーム本体とは無関係なダミーSwapChainからPresent/ResizeBuffersのアドレスを取得する。
bool ResolveSwapChainFunctions(PresentFn* outPresent, ResizeBuffersFn* outResizeBuffers) {
  WNDCLASSA wc{};
  wc.style = CS_OWNDC;
  wc.lpfnWndProc = DefWindowProcA;
  wc.hInstance = GetModuleHandle(nullptr);
  wc.lpszClassName = "KeyboardEasyDashDummy";
  if (RegisterClassA(&wc) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
    return false;
  }
  HWND hDummyWnd = CreateWindowA(wc.lpszClassName, "", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, nullptr, nullptr,
                                 wc.hInstance, nullptr);
  if (hDummyWnd == nullptr) {
    return false;
  }

  DXGI_SWAP_CHAIN_DESC desc{};
  desc.BufferCount = 1;
  desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  desc.OutputWindow = hDummyWnd;
  desc.SampleDesc.Count = 1;
  desc.Windowed = TRUE;
  desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

  constexpr D3D_FEATURE_LEVEL kLevels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0};
  ID3D11Device* pDevice = nullptr;
  IDXGISwapChain* pSwapChain = nullptr;
  const HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, kLevels,
                                                  ARRAYSIZE(kLevels), D3D11_SDK_VERSION, &desc, &pSwapChain, &pDevice,
                                                  nullptr, nullptr);
  DestroyWindow(hDummyWnd);

  if (FAILED(hr) || pSwapChain == nullptr) {
    if (pDevice != nullptr) {
      pDevice->Release();
    }
    return false;
  }
  *outPresent = reinterpret_cast<PresentFn>(GetVtableFunction(pSwapChain, kPresentVtableIndex));
  *outResizeBuffers = reinterpret_cast<ResizeBuffersFn>(GetVtableFunction(pSwapChain, kResizeBuffersVtableIndex));
  pSwapChain->Release();
  pDevice->Release();
  return *outPresent != nullptr && *outResizeBuffers != nullptr;
}

} // namespace

bool InitializeOverlay() {
  if (g_hookInstalled) {
    return true;
  }
  PresentFn pPresent = nullptr;
  ResizeBuffersFn pResizeBuffers = nullptr;
  if (!ResolveSwapChainFunctions(&pPresent, &pResizeBuffers)) {
    return false;
  }
  g_originalPresent = pPresent;
  g_originalResizeBuffers = pResizeBuffers;

  bool ok = DetourFunction(true, reinterpret_cast<PVOID*>(&g_originalPresent), reinterpret_cast<PVOID>(&HookPresent));
  if (!ok) {
    return false;
  }
  ok = DetourFunction(true, reinterpret_cast<PVOID*>(&g_originalResizeBuffers),
                      reinterpret_cast<PVOID>(&HookResizeBuffers));
  if (!ok) {
    DetourFunction(false, reinterpret_cast<PVOID*>(&g_originalPresent), reinterpret_cast<PVOID>(&HookPresent));
    return false;
  }
  g_hookInstalled = true;
  return true;
}

void ShutdownOverlay() {
  if (!g_hookInstalled) {
    return;
  }
  DetourFunction(false, reinterpret_cast<PVOID*>(&g_originalPresent), reinterpret_cast<PVOID>(&HookPresent));
  DetourFunction(false, reinterpret_cast<PVOID*>(&g_originalResizeBuffers), reinterpret_cast<PVOID>(&HookResizeBuffers));
  g_hookInstalled = false;

  if (g_imguiInitialized) {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    g_imguiInitialized = false;
  }
  ReleaseRenderTarget();
  if (g_context != nullptr) {
    g_context->Release();
    g_context = nullptr;
  }
  if (g_device != nullptr) {
    g_device->Release();
    g_device = nullptr;
  }
}

} // namespace KeyboardEasyDash
