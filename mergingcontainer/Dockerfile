FROM ubuntu:18.04
ADD /cmake/build/merging-container /usr/bin/merging-container
ADD cmake/table_manager_data /tmp

COPY shared_library/libboost_thread.so.1.65.1 /usr/lib/x86_64-linux-gnu/
COPY shared_library/libpthread.so.0 /lib/x86_64-linux-gnu/
COPY shared_library/libboost_system.so.1.65.1 /usr/lib/x86_64-linux-gnu/libboost_system.so.1.65.1
COPY shared_library/libstdc++.so.6 /usr/lib/x86_64-linux-gnu/
COPY shared_library/libgcc_s.so.1 /lib/x86_64-linux-gnu/
COPY shared_library/libc.so.6 /lib/x86_64-linux-gnu/
COPY shared_library/libm.so.6 /lib/x86_64-linux-gnu/
COPY shared_library/librt.so.1 /lib/x86_64-linux-gnu/

EXPOSE 40202 40204
ENTRYPOINT ["/usr/bin/merging-container"]