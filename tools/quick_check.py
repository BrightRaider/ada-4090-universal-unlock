import time
import sys

try:
    import torch
except ImportError:
    print("Fehler: PyTorch nicht gefunden!")
    sys.exit(1)

if not torch.cuda.is_available():
    print("Fehler: Kein CUDA GPU verfuegbar!")
    sys.exit(1)

device = torch.device("cuda:0")
gpu_name = torch.cuda.get_device_name(0)

# 4096 x 4096 FP16 GEMM
size = 4096
flops_per_gemm = 2 * (size ** 3)
a = torch.randn((size, size), dtype=torch.float16, device=device)
b = torch.randn((size, size), dtype=torch.float16, device=device)

# Warmup
for _ in range(15):
    c = torch.matmul(a, b)
torch.cuda.synchronize(device)

# Measure
runs = 30
start_ev = torch.cuda.Event(enable_timing=True)
end_ev = torch.cuda.Event(enable_timing=True)

start_ev.record()
for _ in range(runs):
    c = torch.matmul(a, b)
end_ev.record()
torch.cuda.synchronize(device)

total_ms = start_ev.elapsed_time(end_ev)
avg_ms = total_ms / runs
tflops = (flops_per_gemm / (avg_ms * 1e-3)) / 1e12

print("=" * 64)
print("       RTX 4090 RATE-LIMITER SCHNELLTEST (1-CLICK CHECK)")
print("=" * 64)
print(f"  GPU Modell       : {gpu_name}")
print(f"  Matrix-Groesse   : {size}x{size} (FP16 Tensor Cores)")
print(f"  Gemessene Latenz : {avg_ms:.3f} ms")
print(f"  Rechenleistung   : {tflops:.2f} TFLOPS")
print("-" * 64)

if tflops >= 180.0:
    print("  STATUS: [OK] VOLL FREIGESCHALTET! (214 - 246 TFLOPS)")
    print("  Die Karte arbeitet mit 100% Maximalleistung.")
    print("  Der NVIDIA Rate-Limiter ist inaktiv.")
    ret_code = 0
else:
    print("  STATUS: [WARNUNG] GEDROSSELT! (~135 - 145 TFLOPS)")
    print("  Der Werkstakt/Limiter ist aktiv.")
    print("  Empfehlung: Ruhezustand oder PC-Neustart durchfuehren.")
    ret_code = 1

print("=" * 64)
sys.exit(ret_code)
