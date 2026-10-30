import time
import torch
import sys

def run_benchmark():
    if not torch.cuda.is_available():
        print("ERROR: CUDA is not available!")
        sys.exit(1)

    device = torch.device("cuda:0")
    props = torch.cuda.get_device_properties(0)
    print("=" * 70)
    print("         RTX 4090 HARDWARE & COMPUTE PERFORMANCE BENCHMARK       ")
    print("=" * 70)
    print(f"Device Name        : {props.name}")
    print(f"Compute Capability : {props.major}.{props.minor}")
    print(f"Total VRAM         : {props.total_memory / (1024**3):.2f} GB")
    print(f"Multi-Processors   : {props.multi_processor_count} SMs")
    print(f"PyTorch Version    : {torch.__version__}")
    print(f"CUDA Version       : {torch.version.cuda}")
    print("=" * 70)

    # Matrix sizes for GEMM (General Matrix Multiply)
    sizes = [4096, 8192, 16384]
    
    dtypes = [
        ("FP16 Tensor Cores", torch.float16),
        ("BF16 Tensor Cores", torch.bfloat16),
        ("FP32 CUDA / TF32", torch.float32),
    ]

    print("\n--- 1. TENSOR CORE & CUDA COMPUTE THROUGHPUT (GEMM) ---")
    print(f"{'Precision':<20} | {'Matrix Size':<12} | {'Avg Latency (ms)':<16} | {'Throughput (TFLOPS)':<20}")
    print("-" * 75)

    for dtype_name, dtype in dtypes:
        for N in sizes:
            # FLOPs for C = A @ B (where A is N x N, B is N x N) is 2 * N^3
            flops = 2.0 * (N ** 3)
            
            try:
                a = torch.randn(N, N, device=device, dtype=dtype)
                b = torch.randn(N, N, device=device, dtype=dtype)
                
                # Warmup iterations
                for _ in range(5):
                    c = torch.matmul(a, b)
                torch.cuda.synchronize()

                # Timed iterations
                num_iters = 10 if N <= 8192 else 5
                start_event = torch.cuda.Event(enable_timing=True)
                end_event = torch.cuda.Event(enable_timing=True)

                start_event.record()
                for _ in range(num_iters):
                    c = torch.matmul(a, b)
                end_event.record()
                torch.cuda.synchronize()

                elapsed_ms = start_event.elapsed_time(end_event) / num_iters
                tflops = (flops / (elapsed_ms / 1000.0)) / 1e12

                print(f"{dtype_name:<20} | {f'{N}x{N}':<12} | {elapsed_ms:>13.3f} ms | {tflops:>17.2f} TFLOPS")

                del a, b, c
                torch.cuda.empty_cache()
            except Exception as e:
                print(f"{dtype_name:<20} | {f'{N}x{N}':<12} | ERROR: {e}")

    print("\n--- 2. VRAM MEMORY BANDWIDTH TEST ---")
    # Allocate 4 GB tensors and measure bandwidth
    num_elements = 512 * 1024 * 1024  # 512M elements of FP32 = 2 GB each
    try:
        x = torch.empty(num_elements, device=device, dtype=torch.float32)
        y = torch.empty(num_elements, device=device, dtype=torch.float32)
        
        # Warmup
        y.copy_(x)
        torch.cuda.synchronize()

        start_event = torch.cuda.Event(enable_timing=True)
        end_event = torch.cuda.Event(enable_timing=True)

        num_bw_iters = 10
        start_event.record()
        for _ in range(num_bw_iters):
            y.copy_(x)
        end_event.record()
        torch.cuda.synchronize()

        bw_ms = start_event.elapsed_time(end_event) / num_bw_iters
        # 2 GB read + 2 GB write = 4 GB transferred per iteration
        bytes_transferred = 2 * (num_elements * 4) 
        gbps = (bytes_transferred / (bw_ms / 1000.0)) / (1024**3)

        print(f"Copy Bandwidth (2 GB -> 2 GB): {bw_ms:.3f} ms -> {gbps:.2f} GB/s (Raw theoretical max: ~1008 GB/s)")
        del x, y
        torch.cuda.empty_cache()
    except Exception as e:
        print(f"VRAM Bandwidth ERROR: {e}")

    print("=" * 70)
    print("BENCHMARK COMPLETED SUCCESSFULLY.")
    print("=" * 70)

if __name__ == "__main__":
    run_benchmark()
