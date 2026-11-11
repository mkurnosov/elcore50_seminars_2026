// Elcore-50 SDK Example

#include <fstream>
#include <vector>

#include <err.h>
#include <error.h>
#include <stdio.h>
#include <unistd.h>

#include <elcorecl/elcorecl.h>

#define DSP_ELF_PATH "dsp-kernel.elf"

#define LOOPS 20

size_t BUF_SIZE = 2 * 1024 * 1024;  // 2M

uint32_t calculate_crc(uint8_t *buf, size_t size)
{
    uint32_t crc, mask;
    crc = 0xFFFFFFFF;
    for (int i = 0; i < size; ++i) {
        crc = crc ^ buf[i];
        for (int j = 7; j >= 0; --j) {
            mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }
    }
    return ~crc;
}

int main(int argc, char **argv)
{
    // Этот код выполняется на ядре ARM64
    ecl_int ret;

    /* Получаем id платформы */
    ecl_platform_id platform_id;
    ret = eclGetPlatformIDs(1, &platform_id, nullptr);
    if (ret != ECL_SUCCESS) errx(1, "Failed to get platform id. Error code: %d", ret);

    /* Количество доступных устройств DSP (тип ECL_DEVICE_TYPE_CUSTOM) */
    ecl_uint ndevs = 0;
    ret = eclGetDeviceIDs(platform_id, ECL_DEVICE_TYPE_CUSTOM, 0, nullptr, &ndevs);
    if (ret != ECL_SUCCESS) errx(1, "Failed to get device count. Error code: %d", ret);
    if (ndevs == 0) errx(1, "No available ECL_DEVICE_TYPE_CUSTOM devices");
    printf("elcore devices: %d\n", ndevs);

    /* Получаем первое устройство (DSP0) */
    ndevs = 1;
    ecl_device_id dev;
    ret = eclGetDeviceIDs(platform_id, ECL_DEVICE_TYPE_CUSTOM, ndevs, &dev, nullptr);
    if (ret != ECL_SUCCESS) errx(1, "Failed to get device id. Error code: %d", ret);

    /* Создаем контекст устройства */
    ecl_context context;
    context = eclCreateContext(nullptr, ndevs, &dev, nullptr, nullptr, &ret);
    if (context == nullptr || ret != ECL_SUCCESS)
        errx(1, "Failed to create context. Error code: %d", ret);

    /* Считываем ELF-файл для elcore50 в буфер */
    std::vector<char> elf_buffer;
    size_t elf_size;
    {
        std::ifstream file(DSP_ELF_PATH, std::ios::binary | std::ios::ate);
        if (!file) errx(1, "Failed to open %s. Error code: %d", DSP_ELF_PATH, errno);
        elf_size = file.tellg();
        file.seekg(0, std::ios::beg);
        elf_buffer.resize(elf_size);
        file.read(elf_buffer.data(), elf_size);
    }
    const unsigned char *elf = reinterpret_cast<unsigned char *>(elf_buffer.data());

    /* Создаем программу -- связываем программу с контексом устройства */
    ecl_program program;
    program = eclCreateProgramWithBinary(context, ndevs, &dev, &elf_size, &elf, nullptr, &ret);
    if (program == nullptr || ret != ECL_SUCCESS)
        errx(1, "Failed to create program. Error code: %d", ret);

    /* Буфер и его размер должны быть выровнены минимум на 64 байт */
    uint8_t *buf = NULL;
    size_t page_size = getpagesize();
    posix_memalign((void **)&buf, page_size, BUF_SIZE);
    uint32_t *dsp_crc = NULL;
    posix_memalign((void **)&dsp_crc, page_size, page_size);

    /* Создаем объект памяти ecl_mem */
    /* В момент вызова eclCreateBuffer() происходит флаш CPU-кэшей.
       Чтобы модифицировать данные на CPU после eclCreateBuffer() необходимо
       выполнить eclEnqueueMapBuffer(), затем модифицировать данные и выполнить
       eclEnqueueUnmapMemObject().
    */
    ecl_mem buf_mem;
    buf_mem = eclCreateBuffer(context, ECL_MEM_USE_HOST_PTR, BUF_SIZE, &buf[0], &ret);
    if (ret != ECL_SUCCESS || buf_mem == nullptr)
        errx(1, "Failed to create memory object 1. Error code: %d", ret);

    ecl_mem crc_mem;
    crc_mem = eclCreateBuffer(context, ECL_MEM_USE_HOST_PTR, page_size, &dsp_crc[0], &ret);
    if (ret != ECL_SUCCESS || crc_mem == nullptr)
        errx(1, "Failed to create memory object 2. Error code: %d", ret);

    /* Создаём вычислительное ядро на ElcoreCL */
    ecl_kernel kernel;
    kernel = eclCreateKernel(program, "_calculate_crc", &ret);
    if (kernel == nullptr || ret != ECL_SUCCESS)
        errx(1, "Failed to create kernel. Error code: %d", ret);

    /* Устанавливаем аргумент kernel'а */
    /* Прототип ядра: void calculate_crc(uint8_t *buf, size_t size, uint32_t *ret_crc) */
    ret = eclSetKernelArgELcoreMem(kernel, 0, buf_mem);
    if (ret != ECL_SUCCESS) errx(1, "Failed to set arg0. Error code: %d", ret);
    ret = eclSetKernelArg(kernel, 1, sizeof(size_t), &BUF_SIZE);
    if (ret != ECL_SUCCESS) errx(1, "Failed to set arg1. Error code: %d", ret);
    ret = eclSetKernelArgELcoreMem(kernel, 2, crc_mem);
    if (ret != ECL_SUCCESS) errx(1, "Failed to set arg0. Error code: %d", ret);

    /* Создаем очередь команд */
    ecl_command_queue command_queue;
    command_queue = eclCreateCommandQueueWithProperties(context, dev, NULL, &ret);
    if (command_queue == nullptr || ret != ECL_SUCCESS)
        errx(1, "Failed to create queue. Error code: %d", ret);

    FILE *file = fopen("/dev/urandom", "r");
    if (file == nullptr) errx(1, "Failed to open /dev/urandom");

    for (int loop = 0; loop < LOOPS; ++loop) {
        /* Подготавливаем данные */
        /* Инвалидируем CPU-кэши */
        eclEnqueueMapBuffer(command_queue, buf_mem, ECL_TRUE, ECL_MAP_READ | ECL_MAP_WRITE, 0,
                            BUF_SIZE, 0, NULL, NULL, &ret);
        if (ret != ECL_SUCCESS) errx(1, "Failed to map buf_mem. Error code: %d", ret);

        ret = fread(buf, 1, BUF_SIZE, file);
        if (ret != BUF_SIZE) errx(1, "Failed to read %u bytes from /dev/urandom", BUF_SIZE);

        /* Флашим CPU-кэши */
        ecl_event event;
        ret = eclEnqueueUnmapMemObject(command_queue, buf_mem, buf, 0, NULL, &event);
        if (ret != ECL_SUCCESS) errx(1, "Failed to unmap buf_mem. Error code: %d", ret);
        eclWaitForEvents(1, &event);

        /* Запускаем ядро на асинхронное выполнение */
        /* Здесь формируются окончательные данные для драйвера на запуск задания:
         все параметры ядра ElcoreCL вносятся в структуру для ioctl и передаются в
         драйвер вместе с адресом ELF-файла и entry point для ядра ElcoreCL.
         Драйвер формирует образ стека и будущие значения регистров в соответствии
         с соглашением о вызовах 
        */
        const size_t global_work_size[1] = {1};
        ret = eclEnqueueNDRangeKernel(command_queue, kernel, 1, nullptr, global_work_size,
                                      nullptr, 0, nullptr, &event);
        if (ret != ECL_SUCCESS) errx(1, "Failed to enqueue kernel. Error code: %d", ret);

        /* Пока DSP вычисляет рассчитаем сумму на CPU ARM64 */
        uint32_t cpu_crc = calculate_crc(buf, BUF_SIZE);

        /* Ожидаем завершения задания на DSP */
        eclWaitForEvents(1, &event);

        /* Инвалидируем CPU-кэши. eclEnqueueMapBuffer() возвращает тот же указатель,
        что передавался в eclCreateBuffer() */
        eclEnqueueMapBuffer(command_queue, crc_mem, ECL_TRUE, ECL_MAP_READ, 0,
                            sizeof(uint32_t), 0, NULL, NULL, &ret);
        if (ret != ECL_SUCCESS) errx(1, "Failed to map crc_mem. Error code: %d", ret);

        printf("loop%d\t", loop);
        if (*dsp_crc == cpu_crc)
            printf("SUCCESS\n");
        else
            printf("FAIL (got: 0x%u expected: 0x%u)\n", *dsp_crc, cpu_crc);
    }

    fclose(file);
    free(buf);
    free(dsp_crc);
    eclReleaseMemObject(buf_mem);
    eclReleaseMemObject(crc_mem);
    eclReleaseCommandQueue(command_queue);
    eclReleaseKernel(kernel);
    eclReleaseProgram(program);
    eclReleaseContext(context);
    eclReleaseDevice(dev);
    return 0;
}
