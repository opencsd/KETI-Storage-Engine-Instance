registry="ketidevit2"
image_name="offloading-module"
version="v3.0"

# 1.컨테이너 실행 중지 및 컨테이너, 이미지 삭제
docker stop $image_name
docker rm $image_name 
docker rmi $registry/$image_name:$version 

# 2.컨테이너 이미지 획득 
docker pull $registry/$image_name:$version 

# 3.컨테이너 실행 
docker run -d -it --privileged \
 -e STORAGE_NODE_IP="10.0.4.83" -e LOG_LEVEL="TRACE" \
 -p 40201:40201 \
 --network host \
 -v /mnt/gluster:/mnt/gluster \
 --name $image_name $registry/$image_name:$version 
