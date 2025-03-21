 #include <stdio.h>
 #include <string.h>
 // Include the ffmpeg header
 #include <libavutil/log.h>
 #include <libavutil/avutil.h>
 #include <libavformat/avformat.h>


 #include <so_util/so_util.h>
 
 extern so_module so_mod_libxmv;
 
 void patch_ffmpeg(void) {
    hook_addr(so_symbol(&so_mod_libxmv, "av_log"), (uintptr_t)av_log);
    hook_addr(so_symbol(&so_mod_libxmv, "avformat_alloc_context"), (uintptr_t)avformat_alloc_context);
    hook_addr(so_symbol(&so_mod_libxmv, "avformat_open_input"), (uintptr_t)avformat_open_input);
 }
 