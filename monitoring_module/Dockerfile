FROM ubuntu:18.04
ADD cmake/build/monitoring-module /usr/bin/monitoring-module

COPY ./shared_library /usr/lib/x86_64-linux-gnu
COPY ./cmake/table_manager_data /usr/local/table_manager_data

EXPOSE 40203
WORKDIR /usr/bin
ENTRYPOINT ["/usr/bin/monitoring-module"]