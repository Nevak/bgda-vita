cd /vita-dev/bgda-vita/lib/vitaGL

if git apply --reverse --check /vita-dev/bgda-vita/vitaGL.patch; then
  echo "[vitaGL] Patch already applied"
else
  echo "[vitaGL] Applying vitaGL patch"
  git apply /vita-dev/bgda-vita/vitaGL.patch
fi

make clean
make -j64 HAVE_UNFLIPPED_FBOS=0 UNPURE_TEXTURES=1 HAVE_WRAPPED_ALLOCATORS=0 HAVE_PROFILING=0 SOFTFP_ABI=1 LOG_ERRORS=0 HAVE_GLSL_SUPPORT=1 USE_SCRATCH_MEMORY=0 CIRCULAR_VERTEX_POOL=2 HAVE_SHARK_LOG=0 NO_DEBUG=0 HAVE_DEBUGGER=0 SAFE_UNIFORMS=1 STORE_DEPTH_STENCIL=0 HAVE_TEXTURE_CACHE=0 DRAW_SPEEDHACK=2 INDICES_SPEEDHACK=1 INDICES_DRAW_SPEEDHACK=1 HAVE_CPU_TRACER=0 BUFFERS_SPEEDHACK=0 DEBUG_GLSL_TRANSLATOR=0 HAVE_DEVKIT=0 DEPTH_STENCIL_HACK=0 HAVE_RAZOR=0 install 
mkdir -p /vita-dev/bgda-vita/build
cd /vita-dev/bgda-vita/build
cmake ..
make -j64 clean
# Only run 'make send' if nc is available
if command -v nc >/dev/null 2>&1; then
    echo "nc found — running 'make send'"
    make -j64 send
else
    echo "nc not found — skipping 'make send'"
    make -j64
fi
