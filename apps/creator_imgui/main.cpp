#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <chrono>
#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>

#include "engine/engine_facade.h"
#include "engine/scene/scene.h"
#include "engine/actions/action.h"

#include "imgui.h"
#include "backends/imgui_impl_dx12.h"
#include "backends/imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

using Microsoft::WRL::ComPtr;

inline void CheckHR(HRESULT hr, const char* where) {
    if (FAILED(hr)) {
        std::cerr << "[luma_imgui] DX12 failure at " << where << " hr=0x" << std::hex << hr << std::dec << std::endl;
        std::terminate();
    }
}

static void EnableDebugLayer() {
    ComPtr<ID3D12Debug> debug;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) {
        debug->EnableDebugLayer();
        std::cerr << "[luma_imgui] DX12 debug layer enabled" << std::endl;
    } else {
        std::cerr << "[luma_imgui] DX12 debug layer not available" << std::endl;
    }
}

static void EnableInfoQueue(ComPtr<ID3D12Device> device) {
    ComPtr<ID3D12InfoQueue> infoQueue;
    if (SUCCEEDED(device.As(&infoQueue))) {
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, FALSE);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, FALSE);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, FALSE);
        std::cerr << "[luma_imgui] InfoQueue enabled (no break)" << std::endl;
    }
}

static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
    switch (msg) {
        case WM_SIZE:
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

struct DxContext {
    HWND hwnd{};
    UINT width{1280}, height{720};
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12DescriptorHeap> rtvHeap;
    ComPtr<ID3D12DescriptorHeap> srvHeap;  // ImGui SRV heap
    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<IDXGISwapChain3> swapchain;
    std::vector<ComPtr<ID3D12CommandAllocator>> allocators;
    ComPtr<ID3D12GraphicsCommandList> cmdList;
    ComPtr<ID3D12Fence> fence;
    UINT64 fenceValue{0};
    HANDLE fenceEvent{nullptr};
    UINT frameIndex{0};
    UINT rtvDescriptorSize{0};
    std::vector<ComPtr<ID3D12Resource>> renderTargets;
};

static void CreateDevice(DxContext& ctx) {
    ComPtr<IDXGIFactory6> factory;
    EnableDebugLayer();
    CheckHR(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)), "CreateDXGIFactory2");

    ComPtr<IDXGIAdapter1> adapter;
    // Prefer high-performance GPU if available.
    for (UINT i = 0;
         factory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND;
         ++i) {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
        std::wcerr << L"[luma_imgui] Using adapter: " << desc.Description << std::endl;
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&ctx.device)))) {
            break;
        }
    }
    if (!ctx.device) {
        // Fallback: first hardware adapter
        for (UINT i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
            std::wcerr << L"[luma_imgui] Fallback adapter: " << desc.Description << std::endl;
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&ctx.device)))) {
                break;
            }
        }
    }
    CheckHR(ctx.device ? S_OK : E_FAIL, "D3D12CreateDevice");

    D3D12_COMMAND_QUEUE_DESC qdesc{};
    qdesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    CheckHR(ctx.device->CreateCommandQueue(&qdesc, IID_PPV_ARGS(&ctx.queue)), "CreateCommandQueue");
    std::cerr << "[luma_imgui] Device/Queue ready" << std::endl;

    EnableInfoQueue(ctx.device);
}

static void CreateSwapchain(DxContext& ctx) {
    ComPtr<IDXGIFactory6> factory;
    CheckHR(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)), "CreateDXGIFactory2 (swapchain)");
    DXGI_SWAP_CHAIN_DESC1 desc{};
    desc.BufferCount = 2;
    desc.Width = ctx.width;
    desc.Height = ctx.height;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapchain1;
    CheckHR(factory->CreateSwapChainForHwnd(ctx.queue.Get(), ctx.hwnd, &desc, nullptr, nullptr, &swapchain1),
            "CreateSwapChainForHwnd");
    CheckHR(swapchain1.As(&ctx.swapchain), "Swapchain As");
    ctx.frameIndex = ctx.swapchain->GetCurrentBackBufferIndex();
    std::cerr << "[luma_imgui] Swapchain ready" << std::endl;
}

static void CreateRTV(DxContext& ctx) {
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
    heapDesc.NumDescriptors = 2;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    ctx.device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&ctx.rtvHeap));
    ctx.rtvDescriptorSize = ctx.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    ctx.renderTargets.resize(2);
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = ctx.rtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (UINT i = 0; i < 2; i++) {
        CheckHR(ctx.swapchain->GetBuffer(i, IID_PPV_ARGS(&ctx.renderTargets[i])), "GetBuffer RTV");
        ctx.device->CreateRenderTargetView(ctx.renderTargets[i].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += ctx.rtvDescriptorSize;
    }
    std::cerr << "[luma_imgui] RTV heap ready" << std::endl;
}

static void CreateCommands(DxContext& ctx) {
    ctx.allocators.resize(2);
    for (auto& alloc : ctx.allocators) {
        CheckHR(ctx.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&alloc)), "CreateCommandAllocator");
    }
    CheckHR(ctx.device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, ctx.allocators[0].Get(), nullptr, IID_PPV_ARGS(&ctx.cmdList)),
            "CreateCommandList");
    ctx.cmdList->Close();
    CheckHR(ctx.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&ctx.fence)), "CreateFence");
    ctx.fenceValue = 1;
    ctx.fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    std::cerr << "[luma_imgui] Command objects ready" << std::endl;
}

static void WaitForGPU(DxContext& ctx) {
    const UINT64 fenceToWait = ctx.fenceValue;
    ctx.queue->Signal(ctx.fence.Get(), fenceToWait);
    ctx.fenceValue++;
    if (ctx.fence->GetCompletedValue() < fenceToWait) {
        ctx.fence->SetEventOnCompletion(fenceToWait, ctx.fenceEvent);
        WaitForSingleObject(ctx.fenceEvent, INFINITE);
    }
    ctx.frameIndex = ctx.swapchain->GetCurrentBackBufferIndex();
}

static void Render(DxContext& ctx, ImVec4 clear_color) {
    if (!ctx.renderTargets[ctx.frameIndex]) {
        std::cerr << "[luma_imgui] Null render target for frame " << ctx.frameIndex << std::endl;
        return;
    }
    auto& allocator = ctx.allocators[ctx.frameIndex];
    CheckHR(allocator->Reset(), "Allocator Reset");
    CheckHR(ctx.cmdList->Reset(allocator.Get(), nullptr), "CommandList Reset");
    std::cerr << "[luma_imgui] Render frame " << ctx.frameIndex << " begin" << std::endl;

    // Set descriptor heaps AFTER Reset and BEFORE any draw calls
    ID3D12DescriptorHeap* heaps[] = { ctx.srvHeap.Get() };
    ctx.cmdList->SetDescriptorHeaps(1, heaps);

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = ctx.renderTargets[ctx.frameIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    ctx.cmdList->ResourceBarrier(1, &barrier);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = ctx.rtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtv.ptr += static_cast<SIZE_T>(ctx.frameIndex) * ctx.rtvDescriptorSize;
    ctx.cmdList->ClearRenderTargetView(rtv, reinterpret_cast<float*>(&clear_color), 0, nullptr);
    ctx.cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
    std::cerr << "[luma_imgui] RT cleared" << std::endl;

    if (ImDrawData* dd = ImGui::GetDrawData()) {
        std::cerr << "[luma_imgui] ImGui draw lists: " << dd->CmdListsCount
                  << " TotalVtx: " << dd->TotalVtxCount
                  << " TotalIdx: " << dd->TotalIdxCount << std::endl;
        if (dd->CmdListsCount > 0) {
            ImGui_ImplDX12_RenderDrawData(dd, ctx.cmdList.Get());
            std::cerr << "[luma_imgui] ImGui draw submitted" << std::endl;
        }
    } else {
        std::cerr << "[luma_imgui] No draw data" << std::endl;
    }

    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    ctx.cmdList->ResourceBarrier(1, &barrier);
    CheckHR(ctx.cmdList->Close(), "CommandList Close");

    ID3D12CommandList* lists[] = {ctx.cmdList.Get()};
    ctx.queue->ExecuteCommandLists(1, lists);
    CheckHR(ctx.swapchain->Present(1, 0), "Present");
    if (HRESULT reason = ctx.device->GetDeviceRemovedReason(); reason != S_OK) {
        std::cerr << "[luma_imgui] Device removed. HR=0x" << std::hex << reason << std::dec << std::endl;
        throw std::runtime_error("Device removed");
    }
    WaitForGPU(ctx);
    std::cerr << "[luma_imgui] Render frame end" << std::endl;
}

static int RunApp() {
    HINSTANCE hInstance = GetModuleHandleW(nullptr);
    // Engine state
    luma::Scene scene;
    scene.add_node(luma::Node{"MeshNode", "asset_mesh_hero", std::nullopt, {}});
    scene.add_node(luma::Node{"CameraNode", {}, luma::AssetID{"asset_camera_main"}, {}});
    luma::EngineFacade engine;
    engine.load_scene(scene);

    // Win32 window
    WNDCLASSEXW wc{sizeof(WNDCLASSEXW), CS_CLASSDC, WndProc, 0L, 0L, hInstance, nullptr, nullptr, nullptr, nullptr, L"LumaImGui", nullptr};
    RegisterClassExW(&wc);
    HWND hwnd = CreateWindowW(wc.lpszClassName, L"Luma ImGui Creator", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 720, nullptr, nullptr, wc.hInstance, nullptr);

    DxContext ctx;
    ctx.hwnd = hwnd;
    RECT rc;
    GetClientRect(hwnd, &rc);
    ctx.width = rc.right - rc.left;
    ctx.height = rc.bottom - rc.top;
    CreateDevice(ctx);
    CreateSwapchain(ctx);
    CreateRTV(ctx);
    CreateCommands(ctx);

    // ImGui init
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hwnd);
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.NumDescriptors = 64;  // ample descriptors for imgui SRVs
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    CheckHR(ctx.device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&ctx.srvHeap)), "CreateDescriptorHeap srvHeap");
    if (!ImGui_ImplDX12_Init(ctx.device.Get(), 2, DXGI_FORMAT_R8G8B8A8_UNORM, ctx.srvHeap.Get(),
                             ctx.srvHeap->GetCPUDescriptorHandleForHeapStart(),
                             ctx.srvHeap->GetGPUDescriptorHandleForHeapStart())) {
        std::cerr << "[luma_imgui] ImGui_ImplDX12_Init failed" << std::endl;
        return 1;
    }
    if (!ImGui_ImplDX12_CreateDeviceObjects()) {
        std::cerr << "[luma_imgui] ImGui_ImplDX12_CreateDeviceObjects failed" << std::endl;
        return 1;
    }
    std::cerr << "[luma_imgui] ImGui initialized" << std::endl;

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    // UI state
    int variant = 0;
    std::unordered_map<std::string, std::string> params;
    params["param0"] = "0.5";
    bool show_demo = false;
    uint64_t frameCounter = 0;

    MSG msg{};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }
        std::cerr << "[luma_imgui] Frame #" << frameCounter << " start" << std::endl;

        try {
            ImGui_ImplDX12_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();
        } catch (...) {
            std::cerr << "[luma_imgui] NewFrame exception" << std::endl;
            break;
        }

        if (ImGui::Begin("Assets")) {
            if (ImGui::Selectable("look_qt_viewport")) {
                engine.dispatch_action({luma::ActionType::ApplyLook, "look_qt_viewport", {}, std::nullopt, 1});
            }
            if (ImGui::Selectable("asset_material_default")) {
                // select material
            }
        }
        ImGui::End();

        if (ImGui::Begin("Inspector")) {
            ImGui::Text("Active Camera: %s", engine.scene().active_camera() ? engine.scene().active_camera()->c_str() : "<none>");
            ImGui::Text("Look: %s", engine.look().id.c_str());
            if (ImGui::SliderInt("Variant", &variant, 0, 5)) {
                engine.dispatch_action({luma::ActionType::SetMaterialVariant, "asset_material_default", "", variant, 2});
            }
            if (ImGui::CollapsingHeader("Params", ImGuiTreeNodeFlags_DefaultOpen)) {
                int idx = 0;
                for (auto& kv : params) {
                    char buf[64];
                    strncpy(buf, kv.second.c_str(), sizeof(buf));
                    buf[sizeof(buf) - 1] = '\0';
                    if (ImGui::InputText(("##v" + std::to_string(idx)).c_str(), buf, sizeof(buf))) {
                        kv.second = buf;
                        engine.dispatch_action({luma::ActionType::SetParameter,
                                                "asset_material_default/" + kv.first,
                                                kv.second,
                                                std::nullopt,
                                                3});
                    }
                    ImGui::SameLine();
                    ImGui::Text("%s", kv.first.c_str());
                    idx++;
                }
            }
            if (ImGui::Button("Add Param")) {
                const std::string name = "param" + std::to_string(params.size());
                params[name] = "0";
                engine.dispatch_action({luma::ActionType::SetParameter,
                                        "asset_material_default/" + name,
                                        "0",
                                        std::nullopt,
                                        4});
            }
            ImGui::Checkbox("Show ImGui Demo", &show_demo);
        }
        ImGui::End();

        if (ImGui::Begin("Timeline")) {
            static float t = 0.0f;
            if (ImGui::SliderFloat("Time", &t, 0.0f, 1.0f)) {
                engine.set_time(t);
                engine.dispatch_action({luma::ActionType::SetParameter, "TimelineCurve", std::to_string(t), std::nullopt, 5});
            }
            if (ImGui::Button("Play")) {
                engine.dispatch_action({luma::ActionType::PlayAnimation, "MeshNode", "clip_timeline", std::nullopt, 6});
            }
        }
        ImGui::End();

        if (show_demo) ImGui::ShowDemoWindow(&show_demo);

        ImGui::Render();
        ImVec4 clear_color = ImVec4(0.1f, 0.1f, 0.15f, 1.0f);
        Render(ctx, clear_color);
        std::cerr << "[luma_imgui] Frame #" << frameCounter << " done" << std::endl;
        frameCounter++;
    }

    WaitForGPU(ctx);
    CloseHandle(ctx.fenceEvent);
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    return 0;
}

static LONG WINAPI CrashFilter(_In_ struct _EXCEPTION_POINTERS* info) {
    DWORD code = info ? info->ExceptionRecord->ExceptionCode : 0;
    std::cerr << "[luma_imgui] SEH exception caught. Code=0x" << std::hex << code << std::dec << std::endl;
    MessageBoxA(nullptr, ("SEH exception code=0x" + std::to_string(code)).c_str(), "luma_creator_imgui", MB_OK | MB_ICONERROR);
    return EXCEPTION_EXECUTE_HANDLER;
}

int main() {
    SetUnhandledExceptionFilter(CrashFilter);
    try {
        return RunApp();
    } catch (const std::exception& e) {
        std::cerr << "[luma_imgui] std::exception: " << e.what() << std::endl;
        MessageBoxA(nullptr, e.what(), "luma_creator_imgui", MB_OK | MB_ICONERROR);
        return -1;
    } catch (...) {
        std::cerr << "[luma_imgui] Unknown exception" << std::endl;
        MessageBoxA(nullptr, "Unknown exception", "luma_creator_imgui", MB_OK | MB_ICONERROR);
        return -1;
    }
}
#else
int main() { return 0; }
#endif

