if [ ! -e ./bin/win64 ]; then
    mkdir -p ./bin/win64
fi
NATIVE_CORE_SRC="./src/nativecore/net/net.c ./src/nativecore/dbg/dbg_utils.c ./src/nativecore/net/socks5.c ./src/nativecore/mem/mem_utils.c ./src/nativecore/syscall/syscall_utils.c"
x86_64-w64-mingw32-g++ -fpermissive -w $NATIVE_CORE_SRC ./src/afd_proxy.cpp ./src/connection_db.cpp ./src/hooks.cpp  ./src/library.cpp -shared -s -o ./bin/win64/afdproxy.dll -lws2_32 -static-libgcc -static-libstdc++ 