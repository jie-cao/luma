"""
3DDFA_V2 PyTorch -> ONNX Conversion Script
Reconstructs the exact 3DDFA_V2 MobileNetV1 architecture to match checkpoint keys,
then exports to ONNX format.

Usage:
    cd models
    pip install torch onnx onnxscript
    python convert_3ddfa.py
"""

import os
import sys
import urllib.request

WEIGHTS_URL = "https://raw.githubusercontent.com/cleardusk/3DDFA_V2/master/weights/mb1_120x120.pth"
PTH_FILE = "mb1_120x120.pth"
ONNX_FILE = "mb1_120x120.onnx"


def download_weights():
    if os.path.exists(PTH_FILE):
        print(f"[OK] {PTH_FILE} already exists, skipping download.")
        return True

    print(f"Downloading {PTH_FILE} from 3DDFA_V2 repository...")
    try:
        urllib.request.urlretrieve(WEIGHTS_URL, PTH_FILE)
        size_mb = os.path.getsize(PTH_FILE) / (1024 * 1024)
        print(f"[OK] Downloaded {PTH_FILE} ({size_mb:.1f} MB)")
        return True
    except Exception as e:
        print(f"[ERROR] Download failed: {e}")
        print(f"Please manually download from:")
        print(f"  https://github.com/cleardusk/3DDFA_V2/tree/master/weights")
        print(f"  and place {PTH_FILE} in this directory.")
        return False


def build_3ddfa_mobilenet():
    """
    Reconstruct the exact MobileNetV1 architecture used by 3DDFA_V2.
    Must match state_dict keys: module.conv1, module.dw2_1, ..., module.fc_param, module.fc_lm
    """
    import torch
    import torch.nn as nn

    class DepthwiseSeparable(nn.Module):
        """Depthwise separable conv block matching 3DDFA_V2 naming."""
        def __init__(self, inp, oup, stride):
            super().__init__()
            self.conv_dw = nn.Conv2d(inp, inp, 3, stride, 1, groups=inp, bias=False)
            self.bn_dw = nn.BatchNorm2d(inp)
            self.conv_sep = nn.Conv2d(inp, oup, 1, 1, 0, bias=False)
            self.bn_sep = nn.BatchNorm2d(oup)
            self.relu = nn.ReLU(inplace=True)

        def forward(self, x):
            x = self.relu(self.bn_dw(self.conv_dw(x)))
            x = self.relu(self.bn_sep(self.conv_sep(x)))
            return x

    class MobileNet_3DDFA(nn.Module):
        """
        3DDFA_V2's MobileNetV1 with exact attribute names to match checkpoint.
        Standard MobileNetV1 config: (in_ch, out_ch, stride)
        """
        def __init__(self, num_params=62):
            super().__init__()
            # Initial conv: 3 -> 32, stride 2
            self.conv1 = nn.Conv2d(3, 32, 3, 2, 1, bias=False)
            self.bn1 = nn.BatchNorm2d(32)
            self.relu = nn.ReLU(inplace=True)

            # Depthwise separable blocks (named to match 3DDFA_V2 checkpoint)
            # Stage 2: 2 blocks
            self.dw2_1 = DepthwiseSeparable(32, 64, 1)
            self.dw2_2 = DepthwiseSeparable(64, 128, 2)
            # Stage 3: 2 blocks
            self.dw3_1 = DepthwiseSeparable(128, 128, 1)
            self.dw3_2 = DepthwiseSeparable(128, 256, 2)
            # Stage 4: 2 blocks
            self.dw4_1 = DepthwiseSeparable(256, 256, 1)
            self.dw4_2 = DepthwiseSeparable(256, 512, 2)
            # Stage 5: 5 blocks stride 1, then 1 block stride 2 expanding to 1024
            self.dw5_1 = DepthwiseSeparable(512, 512, 1)
            self.dw5_2 = DepthwiseSeparable(512, 512, 1)
            self.dw5_3 = DepthwiseSeparable(512, 512, 1)
            self.dw5_4 = DepthwiseSeparable(512, 512, 1)
            self.dw5_5 = DepthwiseSeparable(512, 512, 1)
            self.dw5_6 = DepthwiseSeparable(512, 1024, 2)
            # Stage 6: 1 block (1024 -> 1024)
            self.dw6 = DepthwiseSeparable(1024, 1024, 1)

            # Average pooling
            self.avgpool = nn.AdaptiveAvgPool2d(1)

            # 3DMM parameter head (62 = 40 shape + 10 expression + 12 pose)
            self.fc_param = nn.Linear(1024, num_params)

        def forward(self, x):
            x = self.relu(self.bn1(self.conv1(x)))
            x = self.dw2_1(x)
            x = self.dw2_2(x)
            x = self.dw3_1(x)
            x = self.dw3_2(x)
            x = self.dw4_1(x)
            x = self.dw4_2(x)
            x = self.dw5_1(x)
            x = self.dw5_2(x)
            x = self.dw5_3(x)
            x = self.dw5_4(x)
            x = self.dw5_5(x)
            x = self.dw5_6(x)
            x = self.dw6(x)
            x = self.avgpool(x)
            x = x.view(x.size(0), -1)
            x = self.fc_param(x)
            return x

    return MobileNet_3DDFA()


def convert_to_onnx():
    if os.path.exists(ONNX_FILE):
        print(f"[OK] {ONNX_FILE} already exists. Delete it first to re-convert.")
        return True

    try:
        import torch
    except ImportError:
        print("[ERROR] PyTorch not installed. Run: pip install torch")
        return False

    print("Building 3DDFA_V2 MobileNetV1 architecture...")
    model = build_3ddfa_mobilenet()

    print(f"Loading weights from {PTH_FILE}...")
    checkpoint = torch.load(PTH_FILE, map_location="cpu", weights_only=False)

    # 3DDFA_V2 checkpoint format: {"epoch": N, "state_dict": {...}}
    # The state_dict keys have "module." prefix from DataParallel training
    if isinstance(checkpoint, dict) and "state_dict" in checkpoint:
        print(f"  Checkpoint has keys: {list(checkpoint.keys())}")
        state = checkpoint["state_dict"]
    elif isinstance(checkpoint, dict) and "model_state_dict" in checkpoint:
        state = checkpoint["model_state_dict"]
    else:
        state = checkpoint

    # Strip "module." prefix from DataParallel
    stripped = {}
    for k, v in state.items():
        new_key = k.replace("module.", "", 1) if k.startswith("module.") else k
        stripped[new_key] = v

    print(f"  State dict has {len(stripped)} keys")
    sample_keys = list(stripped.keys())[:5]
    print(f"  Sample keys (after stripping): {sample_keys}")

    # Remove fc_lm (landmark head) if present - we only need fc_param
    stripped = {k: v for k, v in stripped.items() if not k.startswith("fc_lm")}

    try:
        model.load_state_dict(stripped, strict=True)
        print("[OK] All weights loaded successfully (strict match)")
    except RuntimeError as e:
        print(f"[WARNING] Strict load failed: {e}")
        print("Attempting partial load...")
        model_dict = model.state_dict()
        matched = {k: v for k, v in stripped.items()
                   if k in model_dict and v.shape == model_dict[k].shape}
        model_dict.update(matched)
        model.load_state_dict(model_dict)
        print(f"  Loaded {len(matched)}/{len(model_dict)} parameters")
        if len(matched) < len(model_dict) * 0.8:
            print("[ERROR] Too many parameters missing. ONNX model may not work correctly.")

    model.eval()

    print(f"Exporting to {ONNX_FILE}...")
    dummy_input = torch.randn(1, 3, 120, 120)

    try:
        # PyTorch 2.x: try legacy exporter first (no onnxscript dependency)
        torch.onnx.export(
            model,
            dummy_input,
            ONNX_FILE,
            input_names=["input"],
            output_names=["output"],
            dynamic_axes={"input": {0: "batch"}, "output": {0: "batch"}},
            opset_version=11,
            dynamo=False,
        )
    except TypeError:
        # Older PyTorch versions don't have dynamo parameter
        torch.onnx.export(
            model,
            dummy_input,
            ONNX_FILE,
            input_names=["input"],
            output_names=["output"],
            dynamic_axes={"input": {0: "batch"}, "output": {0: "batch"}},
            opset_version=11,
        )

    size_mb = os.path.getsize(ONNX_FILE) / (1024 * 1024)
    print(f"[OK] Exported {ONNX_FILE} ({size_mb:.1f} MB)")

    # Verify
    try:
        import onnx
        onnx_model = onnx.load(ONNX_FILE)
        onnx.checker.check_model(onnx_model)
        print("[OK] ONNX model verification passed")
    except ImportError:
        print("[INFO] Install 'onnx' package to verify: pip install onnx")
    except Exception as e:
        print(f"[WARNING] ONNX verification: {e}")

    return True


def main():
    print("=" * 60)
    print("3DDFA_V2 Model Conversion: PyTorch -> ONNX")
    print("=" * 60)
    print()

    if not download_weights():
        sys.exit(1)

    if not convert_to_onnx():
        sys.exit(1)

    print()
    print("Done! mb1_120x120.onnx is ready in the models/ directory.")

    # Cleanup
    if os.path.exists(PTH_FILE) and os.path.exists(ONNX_FILE):
        answer = input(f"\nDelete {PTH_FILE} to save space? [y/N]: ").strip().lower()
        if answer == "y":
            os.remove(PTH_FILE)
            print(f"Deleted {PTH_FILE}")


if __name__ == "__main__":
    main()
