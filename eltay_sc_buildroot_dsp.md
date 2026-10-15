# Сборка образа Buildroot для SBC ELTAY SC с поддержкой DSP-ядер elcore50

В конфигурационном файле `eltay_sc_rev101_defconfig` по умолчанию не выбраны пакеты поддержки DSP-ядер для включения в образ целевой системы (https://docs.elron.tech/microcomputers/eltay-sc/os/buildroot/buildroot-sc). 

## Сборка Buildroot в Docker-контейнере

**1. Удаляем несуществующие пути из `docker-build.sh`**

В файле `eltay-sc-sdk/docker-build.sh` в команде `docker run` указаны несуществующие каталоги хоста для монтирования в контейнер (`/usr/corp/Projects`, `/usr/corpneo`, `/usr/git-mirrors`). Для избежания ошибок рекомендуется удалить их из команды запуска контейнера:

```
docker run --rm \
    $USE_TTY \
    -v $HOME:$HOME \
    -v $PWD/.etcpasswd:/etc/passwd:ro \
    --workdir=$THIS_FILE_DIR \
    -u $(id -u):$(id -g) \
    --network=$DOCKER_NETWORK \
    -e BR2_RSYNC_APPLY_PATCHES \
    -e BR2_PRIMARY_SITE \
    -e BUILD_ID \
    -e ENABLE_PRIVATE \
    $DOCKER_IMAGE_TAG \
    /bin/bash -cuxe "$CONTAINER_CMD"
```

**2. Добавляем в конфигурацию Buildroot пакеты для поддержки DSP-ядер**

В файле `eltay-sc-sdk/external-eltaySC/configs/eltay_sc_rev101_defconfig` содержится список пакетов, которые входят в состав формируемого образа целевой системы. Для поддержки DSP-ядер elcore50 следует добавить в файл:
```
BR2_PACKAGE_DSP_COREMARK=y
BR2_PACKAGE_ELVEES_COREMARK_THREADS=4
BR2_PACKAGE_ELVEES_COREMARK=y
BR2_PACKAGE_DSP_COREMARK=y
BR2_PACKAGE_DSP_ELCORE50_LIBRARIES=y
BR2_PACKAGE_DSP_MEMTESTER=y
BR2_PACKAGE_ELCORECL_EXAMPLES=y
BR2_PACKAGE_ELCORECL_EXTRA=y
BR2_PACKAGE_ELCORENN_EXAMPLES=y
BR2_PACKAGE_MGDBSERVER=y
BR2_PACKAGE_PYTHON_ELCORECL=y
BR2_PACKAGE_PYTHON_ELCORENN=y
BR2_BINUTILS_ELCORE50=y
```

Аналогичного результата можно добиться ручным выбором пакетов в menuconfig:
```
./docker-build.sh make eltay_sc_rev101_defconfig
./docker-build.sh make menuconfig
```

**3. Собираем образ**

```
cd eltay-sc-sdk
export DOCKERFILE=Dockerfile.centos8stream
export ENABLE_NETWORK=1
export BUILD_ID=eltay-sc-$(date +%Y%m%d-%H%M)

./docker-build.sh make eltay_sc_rev101_defconfig
./docker-build.sh make -j$(nproc)
```

**4. Проверяем наличие собранного модуля ядра для поддержки DSP-ядер**

```
ls eltay-sc-sdk/buildroot/output/target/usr/lib/modules/6.6.30/updates/elcore50.ko
```

**5. Записываем образ на SD-карту**

```
sudo buildroot/output/images/tar2dev -n eltay-sc buildroot/output/images/rootfs.tar.gz /dev/sdX
sync
```


