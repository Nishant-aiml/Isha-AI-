# ISHA AI — llama.cpp Static Library Build Script
# Compiles llama.cpp b4966 into a static library for CPU-only inference.
# NO GPU (CUDA/Metal/Vulkan/OpenCL) enabled.

$vcvars = "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
$llama_dir = "third_party/llama.cpp"
$obj_dir = "obj/llama"
$lib_out = "lib/llama.lib"

# Ensure output directories exist
if (-not (Test-Path $obj_dir)) { New-Item -ItemType Directory -Path $obj_dir -Force }
if (-not (Test-Path "lib")) { New-Item -ItemType Directory -Path "lib" }

# Clean old objects
Get-ChildItem -Path $obj_dir -Filter "*.obj" -ErrorAction SilentlyContinue | Remove-Item -Force

# Include paths
$includes = @(
    "/I`"$llama_dir/include`"",
    "/I`"$llama_dir/src`"",
    "/I`"$llama_dir/ggml/include`"",
    "/I`"$llama_dir/ggml/src`"",
    "/I`"$llama_dir/ggml/src/ggml-cpu`"",
    "/I`"$llama_dir/common`""
) -join " "

# Defines: CPU-only, no GPU
$defines = "/DGGML_USE_CPU /D_CRT_SECURE_NO_WARNINGS /DNDEBUG /DNOMINMAX"

# Compiler flags
$cflags = "/EHsc /std:c++17 /O2 /MT /c $defines $includes"
$cflags_c = "/std:c17 /O2 /MT /c $defines $includes"

Write-Host "=== Building llama.cpp static library (CPU-only) ===" -ForegroundColor Magenta
Write-Host "Tag: b4966" -ForegroundColor Cyan
Write-Host "Commit: bd40678df768ab189b26b8b03e3de4062e3b71a3" -ForegroundColor Cyan

# C++ source files (llama core)
$cpp_sources = @(
    "$llama_dir/src/llama.cpp",
    "$llama_dir/src/llama-adapter.cpp",
    "$llama_dir/src/llama-arch.cpp",
    "$llama_dir/src/llama-batch.cpp",
    "$llama_dir/src/llama-chat.cpp",
    "$llama_dir/src/llama-context.cpp",
    "$llama_dir/src/llama-cparams.cpp",
    "$llama_dir/src/llama-grammar.cpp",
    "$llama_dir/src/llama-graph.cpp",
    "$llama_dir/src/llama-hparams.cpp",
    "$llama_dir/src/llama-impl.cpp",
    "$llama_dir/src/llama-io.cpp",
    "$llama_dir/src/llama-kv-cache.cpp",
    "$llama_dir/src/llama-memory.cpp",
    "$llama_dir/src/llama-mmap.cpp",
    "$llama_dir/src/llama-model-loader.cpp",
    "$llama_dir/src/llama-model.cpp",
    "$llama_dir/src/llama-quant.cpp",
    "$llama_dir/src/llama-sampling.cpp",
    "$llama_dir/src/llama-vocab.cpp",
    "$llama_dir/src/unicode.cpp",
    "$llama_dir/src/unicode-data.cpp"
)

# C++ source files (ggml)
$cpp_sources += @(
    "$llama_dir/ggml/src/ggml-backend.cpp",
    "$llama_dir/ggml/src/ggml-backend-reg.cpp",
    "$llama_dir/ggml/src/ggml-opt.cpp",
    "$llama_dir/ggml/src/ggml-threading.cpp",
    "$llama_dir/ggml/src/gguf.cpp"
)

# C++ source files (ggml-cpu) — RENAMED to avoid obj collision with ggml-cpu.c
$cpp_sources += @(
    "$llama_dir/ggml/src/ggml-cpu/ggml-cpu-traits.cpp",
    "$llama_dir/ggml/src/ggml-cpu/ggml-cpu-aarch64.cpp",
    "$llama_dir/ggml/src/ggml-cpu/cpu-feats-x86.cpp"
)

# C++ source files (common - minimal subset)
$cpp_sources += @(
    "$llama_dir/common/common.cpp",
    "$llama_dir/common/sampling.cpp",
    "$llama_dir/common/log.cpp"
)

# C source files
$c_sources = @(
    "$llama_dir/ggml/src/ggml.c",
    "$llama_dir/ggml/src/ggml-alloc.c",
    "$llama_dir/ggml/src/ggml-quants.c",
    "$llama_dir/ggml/src/ggml-cpu/ggml-cpu.c",
    "$llama_dir/ggml/src/ggml-cpu/ggml-cpu-quants.c"
)

# Step 1: Compile C++ sources (all except ggml-cpu.cpp which collides with ggml-cpu.c)
Write-Host "`nCompiling C++ sources..." -ForegroundColor Yellow
$cpp_list = $cpp_sources -join " "
cmd.exe /c "call `"$vcvars`" && cl.exe $cflags $cpp_list /Fo$obj_dir/"
if ($LASTEXITCODE -ne 0) {
    Write-Host "C++ compilation FAILED" -ForegroundColor Red
    exit 1
}

# Step 2: Compile ggml-cpu.cpp separately with renamed output to avoid collision
Write-Host "`nCompiling ggml-cpu.cpp (renamed to ggml-cpu-cpp.obj)..." -ForegroundColor Yellow
cmd.exe /c "call `"$vcvars`" && cl.exe $cflags `"$llama_dir/ggml/src/ggml-cpu/ggml-cpu.cpp`" /Fo$obj_dir/ggml-cpu-cpp.obj"
if ($LASTEXITCODE -ne 0) {
    Write-Host "ggml-cpu.cpp compilation FAILED" -ForegroundColor Red
    exit 1
}

# Step 3: Compile C sources
Write-Host "`nCompiling C sources..." -ForegroundColor Yellow
$c_list = $c_sources -join " "
cmd.exe /c "call `"$vcvars`" && cl.exe $cflags_c $c_list /Fo$obj_dir/"
if ($LASTEXITCODE -ne 0) {
    Write-Host "C compilation FAILED" -ForegroundColor Red
    exit 1
}

# Link into static library
Write-Host "`nCreating static library..." -ForegroundColor Yellow
$all_objs = (Get-ChildItem -Path $obj_dir -Filter "*.obj" | ForEach-Object { "$obj_dir\$($_.Name)" }) -join " "
cmd.exe /c "call `"$vcvars`" && lib.exe /OUT:$lib_out $all_objs"
if ($LASTEXITCODE -ne 0) {
    Write-Host "Library creation FAILED" -ForegroundColor Red
    exit 1
}

$lib_size = (Get-Item $lib_out).Length / 1MB
Write-Host "`n=== llama.cpp static library built successfully ===" -ForegroundColor Green
Write-Host "Output: $lib_out ($([math]::Round($lib_size, 1))MB)" -ForegroundColor Cyan
Write-Host "Objects: $(Get-ChildItem -Path $obj_dir -Filter '*.obj' | Measure-Object | Select-Object -ExpandProperty Count)" -ForegroundColor Cyan
