#ifdef _WIN32
#include <cmath>
#include <exception>
#include <string>
#include <string_view>

#include "engine/engine_facade.h"
#include "engine/foundation/log.h"
#include "engine/renderer/render_graph/render_graph.h"
#include "engine/renderer/rhi/dx12_backend.h"
#include "engine/scene/scene.h"

namespace {

constexpr UINT kFrameCount = 2;

struct DxContext {
    HWND hwnd{};
    UINT width{1280};
    UINT height{720};

    std::shared_ptr<luma::rhi::Backend> backend;
    std::unique_ptr<luma::render_graph::RenderGraph> renderGraph;
};

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
}

void InitWindow(DxContext& ctx) {
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"LumaDX12WindowClass";
    RegisterClassExW(&wc);

    RECT rect = {0, 0, static_cast<LONG>(ctx.width), static_cast<LONG>(ctx.height)};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    ctx.hwnd = CreateWindowExW(
        0,
        wc.lpszClassName,
        L"Luma DX12 Clear",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        nullptr, nullptr, wc.hInstance, nullptr);
    ShowWindow(ctx.hwnd, SW_SHOW);
}

void InitDx(DxContext& ctx) {
    luma::rhi::NativeWindow wnd{ctx.hwnd, ctx.width, ctx.height};
    ctx.backend = luma::rhi::create_dx12_backend(wnd);
    ctx.renderGraph = std::make_unique<luma::render_graph::RenderGraph>(ctx.backend);
}

void PopulateCommandList(DxContext& ctx, float r, float g, float b) {
    ThrowIfFailed(ctx.commandAllocators[ctx.frameIndex]->Reset(), "Allocator Reset");
    ThrowIfFailed(ctx.commandList->Reset(ctx.commandAllocators[ctx.frameIndex].Get(), nullptr), "CommandList Reset");

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = ctx.renderTargets[ctx.frameIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    ctx.commandList->ResourceBarrier(1, &barrier);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = ctx.rtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += static_cast<SIZE_T>(ctx.frameIndex) * ctx.rtvDescriptorSize;
    FLOAT clearColor[] = {r, g, b, 1.0f};
    ctx.commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    ctx.commandList->ResourceBarrier(1, &barrier);

    ThrowIfFailed(ctx.commandList->Close(), "CommandList Close");
}

void WaitForGpu(DxContext& ctx) {
    const UINT64 fenceToWaitFor = ctx.fenceValue;
    ThrowIfFailed(ctx.commandQueue->Signal(ctx.fence.Get(), fenceToWaitFor), "Fence Signal");
    ctx.fenceValue++;
    if (ctx.fence->GetCompletedValue() < fenceToWaitFor) {
        ThrowIfFailed(ctx.fence->SetEventOnCompletion(fenceToWaitFor, ctx.fenceEvent), "SetEventOnCompletion");
        WaitForSingleObject(ctx.fenceEvent, INFINITE);
    }
    ctx.frameIndex = ctx.swapChain->GetCurrentBackBufferIndex();
}

void RenderFrame(DxContext& ctx, float t) {
    const float r = 0.1f + 0.4f * std::sin(t);
    const float g = 0.2f + 0.3f * std::cos(t * 0.5f);
    const float b = 0.4f + 0.2f * std::sin(t * 0.7f);

    ctx.renderGraph->clear(r, g, b);
    ctx.renderGraph->execute();
    ctx.renderGraph->present();
}

}  // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    luma::log_info("Starting DX12 clear app");
    // Minimal scene + engine setup to keep parity with Action-only mutation rule.
    luma::Scene scene;
    scene.add_node(luma::Node{
        .name = "MeshNode",
        .renderable = "asset_mesh_hero",
        .camera = std::nullopt,
        .transform = {}
    });
    scene.add_node(luma::Node{
        .name = "CameraNode",
        .renderable = {},
        .camera = luma::AssetID{"asset_camera_main"},
        .transform = {}
    });

    luma::EngineFacade engine;
    engine.load_scene(scene);
    engine.dispatch_action({luma::ActionType::ApplyLook, "look_dx12_clear", {}, std::nullopt, 1});
    engine.dispatch_action({luma::ActionType::SwitchCamera, "asset_camera_main", {}, std::optional<int>{1}, 2});

    DxContext ctx;
    InitWindow(ctx);
    InitDx(ctx);

    MSG msg = {};
    float time = 0.0f;
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            engine.advance_time(0.016f);
            RenderFrame(ctx, engine.timeline().time());
        }
    }
    WaitForGpu(ctx);
    CloseHandle(ctx.fenceEvent);
    return 0;
}

#else
int main() {
    return 0;
}
#endif

