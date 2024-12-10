registry="ketidevit2"
image_name="merging-module"
version="v3.0"

# 1.컨테이너 실행 중지 및 컨테이너, 이미지 삭제
docker stop $image_name
docker rm $image_name 
docker rmi $registry/$image_name:$version 

# 2.컨테이너 이미지 획득 
docker pull $registry/$image_name:$version 

# 3.컨테이너 실행 
docker run -d -it --privileged \
 -p 40202:40202 -p 40204:40204 -p 40209:40209 \
 -e LOG_LEVEL="TRACE" \
 --network host \
 --name $image_name $registry/$image_name:$version 
