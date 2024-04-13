这两个脚本开始是为了解决宿主机和 docker 容器无法及时同步文件的问题.

但这样写感觉不太优雅,废弃

如果要再使用,注意的是要放在正确的目录

host.mkiso.sh 放到 kernel/mkiso.sh
container.mkiso.sh 放到 kernel/scripts/mkiso.sh

host.mkiso.sh要在宿主机调用
container.mkiso.sh会提交到 docker 容器执行
