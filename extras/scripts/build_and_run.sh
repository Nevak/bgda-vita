cd /vita-dev/vitaGL 
make clean
make SOFTFP_ABI=1 LOG_ERRORS=1 HAVE_GLSL_SUPPORT=0 HAVE_SHARK_LOG=1 install 
cd /vita-dev/soulcalibur_vita/build 
cmake .. -DCMAKE_BUILD_TYPE=Debug 
make clean
make send