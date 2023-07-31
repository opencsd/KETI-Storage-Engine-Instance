#!/bin/bash

# 1분마다 CPU 사용량을 출력하는 함수
function monitor_cpu_usage() {
    while true; do
        # /proc/stat 파일을 읽어서 cpu 정보를 추출
        cpu_stat=$(cat /proc/stat | grep '^cpu ')
        cpu_values=($cpu_stat)  # 공백을 기준으로 값을 배열로 저장

        # cpu 사용량 계산
        total_cpu_time=0
        for value in "${cpu_values[@]:1}"; do
            total_cpu_time=$((total_cpu_time + value))
            echo "value : $value"
        done
        idle_time=${cpu_values[4]}  # idle 상태의 CPU 시간
        active_time=$((total_cpu_time - idle_time))
        cpu_usage=$((100 * active_time / total_cpu_time))

        # 결과 출력
        echo "CPU Usage: $cpu_usage%"

        # 1분 대기
        sleep 3
    done
}

monitor_cpu_usage  # 함수 호출
