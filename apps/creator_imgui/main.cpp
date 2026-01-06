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

#include "engine/engine_facade.h"
#include "engine/scene/scene.h"
#include "engine/actions/action.h"

#include "imgui.h"
#include "backends/imgui_impl_dx12.h"
#include "backends/imgui_impl_win32.h"

using Microsoft::WRL::ComPtr;

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
    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<IDXGISwapChain3> swapchain;
    ComPtr<ID3D12CommandAllocator> allocator;
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
    CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));
    ComPtr<IDXGIAdapter1> adapter;
    for (UINT i = 0; DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(i, &adapter); i++) {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
        if (D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr) == S_OK)
            break;
    }
    D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&ctx.device));

    D3D12_COMMAND_QUEUE_DESC qdesc{};
    qdesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    ctx.device->CreateCommandQueue(&qdesc, IID_PPV_ARGS(&ctx.queue));
}

static void CreateSwapchain(DxContext& ctx) {
    ComPtr<IDXGIFactory6> factory;
    CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));
    DXGI_SWAP_CHAIN_DESC1 desc{};
    desc.BufferCount = 2;
    desc.Width = ctx.width;
    desc.Height = ctx.height;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapchain1;
    factory->CreateSwapChainForHwnd(ctx.queue.Get(), ctx.hwnd, &desc, nullptr, nullptr, &swapchain1);
    swapchain1.As(&ctx.swapchain);
    ctx.frameIndex = ctx.swapchain->GetCurrentBackBufferIndex();
}

static void CreateRTV(DxContext& ctx) {
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
    heapDesc.NumDescriptors = 2;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    ctx.device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&ctx.rtvHeap));
    ctx.rtvDescriptorSize = ctx.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    ctx.renderTargets.resize(2);
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(ctx.rtvHeap->GetCPUDescriptorHandleForHeapStart());
    for (UINT i = 0; i < 2; i++) {
        ctx.swapchain->GetBuffer(i, IID_PPV_ARGS(&ctx.renderTargets[i]));
        ctx.device->CreateRenderTargetView(ctx.renderTargets[i].Get(), nullptr, rtvHandle);
        rtvHandle.Offset(1, ctx.rtvDescriptorSize);
    }
}

static void CreateCommands(DxContext& ctx) {
    ctx.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&ctx.allocator));
    ctx.device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, ctx.allocator.Get(), nullptr, IID_PPV_ARGS(&ctx.cmdList));
    ctx.cmdList->Close();
    ctx.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&ctx.fence));
    ctx.fenceValue = 1;
    ctx.fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
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
    ctx.allocator->Reset();
    ctx.cmdList->Reset(ctx.allocator.Get(), nullptr);

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        ctx.renderTargets[ctx.frameIndex].Get(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET);
    ctx.cmdList->ResourceBarrier(1, &barrier);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(ctx.rtvHeap->GetCPUDescriptorHandleForHeapStart(), ctx.frameIndex, ctx.rtvDescriptorSize);
    ctx.cmdList->ClearRenderTargetView(rtv, reinterpret_cast<float*>(&clear_color), 0, nullptr);
    ctx.cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), ctx.cmdList.Get());

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        ctx.renderTargets[ctx.frameIndex].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT);
    ctx.cmdList->ResourceBarrier(1, &barrier);
    ctx.cmdList->Close();

    ID3D12CommandList* lists[] = {ctx.cmdList.Get()};
    ctx.queue->ExecuteCommandLists(1, lists);
    ctx.swapchain->Present(1, 0);
    WaitForGPU(ctx);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int) {
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
    heapDesc.NumDescriptors = 1;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ComPtr<ID3D12DescriptorHeap> imguiHeap;
    ctx.device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&imguiHeap));
    ImGui_ImplDX12_Init(ctx.device.Get(), 2, DXGI_FORMAT_R8G8B8A8_UNORM, imguiHeap.Get(),
                        imguiHeap->GetCPUDescriptorHandleForHeapStart(),
                        imguiHeap->GetGPUDescriptorHandleForHeapStart());

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    // UI state
    int variant = 0;
    std::unordered_map<std::string, std::string> params;
    params["param0"] = "0.5";
    bool show_demo = false;

    MSG msg{};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }

        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

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
        ID3D12DescriptorHeap* heaps[] = {imguiHeap.Get()};
        ctx.cmdList->SetDescriptorHeaps(1, heaps);
        Render(ctx, clear_color);
    }

    WaitForGPU(ctx);
    CloseHandle(ctx.fenceEvent);
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    return 0;
}
#else
int main() { return 0; }
#endif

