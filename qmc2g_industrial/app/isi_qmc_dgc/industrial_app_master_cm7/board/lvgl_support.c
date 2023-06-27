/*
 * Copyright 2019-2023 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "lvgl_support.h"
#include "lvgl.h"
#if defined(SDK_OS_FREE_RTOS)
#include "FreeRTOS.h"
#include "semphr.h"
#endif
#include "board.h"

#include "fsl_gpio.h"
#include "fsl_cache.h"
#include "fsl_debug_console.h"

#if LV_USE_GPU_NXP_PXP
#include "src/gpu/lv_gpu_nxp_pxp.h"
#include "src/gpu/lv_gpu_nxp_pxp_osa.h"
#endif

#if LV_USE_GPU_NXP_VG_LITE
#include "vg_lite.h"
#include "vg_lite_platform.h"
#endif

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Cache line size. */
#ifndef FSL_FEATURE_L2CACHE_LINESIZE_BYTE
#define FSL_FEATURE_L2CACHE_LINESIZE_BYTE 0
#endif
#ifndef FSL_FEATURE_L1DCACHE_LINESIZE_BYTE
#define FSL_FEATURE_L1DCACHE_LINESIZE_BYTE 0
#endif

#if (FSL_FEATURE_L2CACHE_LINESIZE_BYTE > FSL_FEATURE_L1DCACHE_LINESIZE_BYTE)
#define DEMO_CACHE_LINE_SIZE FSL_FEATURE_L2CACHE_LINESIZE_BYTE
#else
#define DEMO_CACHE_LINE_SIZE FSL_FEATURE_L1DCACHE_LINESIZE_BYTE
#endif

#if (DEMO_CACHE_LINE_SIZE > FRAME_BUFFER_ALIGN)
#define DEMO_FB_ALIGN DEMO_CACHE_LINE_SIZE
#else
#define DEMO_FB_ALIGN FRAME_BUFFER_ALIGN
#endif

#if (LV_ATTRIBUTE_MEM_ALIGN_SIZE > DEMO_FB_ALIGN)
#undef DEMO_FB_ALIGN
#define DEMO_FB_ALIGN LV_ATTRIBUTE_MEM_ALIGN_SIZE
#endif

#define DEMO_FB_SIZE \
    (((DEMO_BUFFER_WIDTH * DEMO_BUFFER_HEIGHT * LCD_FB_BYTE_PER_PIXEL) + DEMO_FB_ALIGN - 1) & ~(DEMO_FB_ALIGN - 1))

#if LV_USE_GPU_NXP_VG_LITE
#define VG_LITE_MAX_CONTIGUOUS_SIZE 0x200000
#define VG_LITE_COMMAND_BUFFER_SIZE (256 << 10)
#endif

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
static void DEMO_FlushDisplay(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p);
static void DEMO_WaitFlush(lv_disp_drv_t *disp_drv);

#if (LV_USE_GPU_NXP_VG_LITE || LV_USE_GPU_NXP_PXP)
static void DEMO_CleanInvalidateCache(lv_disp_drv_t *disp_drv);
#endif

static void DEMO_BufferSwitchOffCallback(void *param, void *switchOffBuffer);

#if LV_USE_GPU_NXP_VG_LITE
static status_t BOARD_PrepareVGLiteController(void);

static status_t BOARD_InitVGliteClock(void);
#endif /* LV_USE_GPU_NXP_VG_LITE */

/*******************************************************************************
 * Variables
 ******************************************************************************/
AT_NONCACHEABLE_OCTALRAM_SECTION_ALIGN(static uint8_t s_frameBuffer[2][DEMO_FB_SIZE], DEMO_FB_ALIGN);

#if defined(SDK_OS_FREE_RTOS)
static SemaphoreHandle_t s_transferDone;
#else
static volatile bool s_transferDone;
#endif

#if LV_USE_GPU_NXP_VG_LITE
static uint32_t registerMemBase = 0x41800000;
static uint32_t gpu_mem_base    = 0x0;

/*
 * In case custom VGLite memory parameters are used, the application needs to
 * allocate and publish the VGLite heap base, its size and the size of the
 * command buffer(s) using the following global variables:
 */
extern void *vglite_heap_base;
extern uint32_t vglite_heap_size;
extern uint32_t vglite_cmd_buff_size;

#if (CUSTOM_VGLITE_MEMORY_CONFIG == 0)
/* VGLite driver heap */
AT_NONCACHEABLE_OCTALRAM_SECTION_ALIGN(static uint8_t vglite_contiguous_mem[VG_LITE_MAX_CONTIGUOUS_SIZE], 64);

void *vglite_heap_base        = &vglite_contiguous_mem;
uint32_t vglite_heap_size     = VG_LITE_MAX_CONTIGUOUS_SIZE;
uint32_t vglite_cmd_buff_size = VG_LITE_COMMAND_BUFFER_SIZE;
#endif /* CUSTOM_VGLITE_MEMORY_CONFIG */

#endif /* LV_USE_GPU_NXP_VG_LITE */

/*******************************************************************************
 * Code
 ******************************************************************************/

void lv_port_pre_init(void)
{
}

void lv_port_disp_init(void)
{
    static lv_disp_draw_buf_t disp_buf;
    static lv_disp_drv_t disp_drv; /*Descriptor of a display driver*/

    memset(s_frameBuffer, 0, sizeof(s_frameBuffer));
    lv_disp_draw_buf_init(&disp_buf, s_frameBuffer[0], s_frameBuffer[1], LCD_WIDTH * LCD_HEIGHT);

    status_t status;
    dc_fb_info_t fbInfo;

#if LV_USE_GPU_NXP_VG_LITE
    /* Initialize GPU. */
    BOARD_PrepareVGLiteController();
#endif

    /*-------------------------
     * Initialize your display
     * -----------------------*/
    BOARD_PrepareDisplayController();

    status = g_dc.ops->init(&g_dc);
    if (kStatus_Success != status)
    {
        assert(0);
    }

    g_dc.ops->getLayerDefaultConfig(&g_dc, 0, &fbInfo);
    fbInfo.pixelFormat = DEMO_BUFFER_PIXEL_FORMAT;
    fbInfo.width       = DEMO_BUFFER_WIDTH;
    fbInfo.height      = DEMO_BUFFER_HEIGHT;
    fbInfo.startX      = DEMO_BUFFER_START_X;
    fbInfo.startY      = DEMO_BUFFER_START_Y;
    fbInfo.strideBytes = DEMO_BUFFER_STRIDE_BYTE;
    g_dc.ops->setLayerConfig(&g_dc, 0, &fbInfo);

    g_dc.ops->setCallback(&g_dc, 0, DEMO_BufferSwitchOffCallback, &disp_drv);

#if defined(SDK_OS_FREE_RTOS)
    s_transferDone = xSemaphoreCreateBinary();
    if (NULL == s_transferDone)
    {
        PRINTF("Frame semaphore create failed\r\n");
        assert(0);
    }
#else
    s_transferDone = false;
#endif

    /* lvgl starts render in frame buffer 0, so show frame buffer 1 first. */
    g_dc.ops->setFrameBuffer(&g_dc, 0, (void *)s_frameBuffer[1]);

    /* Wait for frame buffer sent to display controller video memory. */
    if ((g_dc.ops->getProperty(&g_dc) & kDC_FB_ReserveFrameBuffer) == 0)
    {
#if defined(SDK_OS_FREE_RTOS)
        if (xSemaphoreTake(s_transferDone, portMAX_DELAY) != pdTRUE)
        {
            PRINTF("Wait semaphore error: s_transferDone\r\n");
            assert(0);
        }
#else
        while (false == s_transferDone)
        {
        }
#endif
    }

    g_dc.ops->enableLayer(&g_dc, 0);

    /*-----------------------------------
     * Register the display in LittlevGL
     *----------------------------------*/

    lv_disp_drv_init(&disp_drv); /*Basic initialization*/

    /*Set up the functions to access to your display*/

    /*Set the resolution of the display*/
    disp_drv.hor_res = LCD_WIDTH;
    disp_drv.ver_res = LCD_HEIGHT;

    /*Used to copy the buffer's content to the display*/
    disp_drv.flush_cb = DEMO_FlushDisplay;

    disp_drv.wait_cb = DEMO_WaitFlush;

#if (LV_USE_GPU_NXP_VG_LITE || LV_USE_GPU_NXP_PXP)
    disp_drv.clean_dcache_cb = DEMO_CleanInvalidateCache;
#endif

    /*Set a display buffer*/
    disp_drv.draw_buf = &disp_buf;

    /* Partial refresh */
    disp_drv.full_refresh = 1;

    /*Finally register the driver*/
    lv_disp_drv_register(&disp_drv);

#if LV_USE_GPU_NXP_VG_LITE
    if (vg_lite_init(64, 64) != VG_LITE_SUCCESS)
    {
        PRINTF("VGLite init error. STOP.");
        vg_lite_close();
        assert(0);
    }
#endif
}

static void DEMO_BufferSwitchOffCallback(void *param, void *switchOffBuffer)
{
    lv_disp_drv_t *disp_drv = (lv_disp_drv_t *)param;

    /* IMPORTANT!!!
     * Inform the graphics library that you are ready with the flushing*/
    lv_disp_flush_ready(disp_drv);

#if defined(SDK_OS_FREE_RTOS)
    BaseType_t taskAwake = pdFALSE;

    xSemaphoreGiveFromISR(s_transferDone, &taskAwake);
    portYIELD_FROM_ISR(taskAwake);
#else
    s_transferDone = true;
#endif
}

#if (LV_USE_GPU_NXP_VG_LITE || LV_USE_GPU_NXP_PXP)
static void DEMO_CleanInvalidateCache(lv_disp_drv_t *disp_drv)
{
#if __CORTEX_M == 4
    L1CACHE_CleanInvalidateSystemCache();
#else
    SCB_CleanInvalidateDCache();
#endif
}
#endif

static void DEMO_WaitFlush(lv_disp_drv_t *disp_drv)
{
#if defined(SDK_OS_FREE_RTOS)
    if (xSemaphoreTake(s_transferDone, portMAX_DELAY) != pdTRUE)
    {
        PRINTF("Display flush failed\r\n");
        assert(0);
    }
#else
    while (false == s_transferDone)
    {
    }
#endif
}

static void DEMO_FlushDisplay(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    /*
     * Before new frame flushing, clear previous frame flush done status.
     */
#if !defined(SDK_OS_FREE_RTOS)
    s_transferDone = false;
#else
    (void)xSemaphoreTake(s_transferDone, 0);
#endif

    DCACHE_CleanInvalidateByRange((uint32_t)color_p, DEMO_FB_SIZE);
    g_dc.ops->setFrameBuffer(&g_dc, 0, (void *)color_p);
}

void lv_port_indev_init(void)
{
    /*------------------
     * Touchpad
     * -----------------*/
}

#if LV_USE_GPU_NXP_VG_LITE
void GPU2D_IRQHandler(void)
{
    vg_lite_IRQHandler();
}

static status_t BOARD_InitVGliteClock(void)
{
    const clock_root_config_t gc355ClockConfig = {
        .clockOff = false,
        .mux      = kCLOCK_GC355_ClockRoot_MuxVideoPllOut,
        .div      = 2,
    };

    CLOCK_SetRootClock(kCLOCK_Root_Gc355, &gc355ClockConfig);

    CLOCK_GetRootClockFreq(kCLOCK_Root_Gc355);

    CLOCK_EnableClock(kCLOCK_Gpu2d);

    NVIC_SetPriority(GPU2D_IRQn, 3);

    EnableIRQ(GPU2D_IRQn);

    return kStatus_Success;
}

static status_t BOARD_PrepareVGLiteController(void)
{
    status_t status;

    status = BOARD_InitVGliteClock();

    if (kStatus_Success != status)
    {
        return status;
    }

    vg_lite_init_mem(registerMemBase, gpu_mem_base, vglite_heap_base, vglite_heap_size);

    vg_lite_set_command_buffer_size(vglite_cmd_buff_size);

    return kStatus_Success;
}
#endif /* LV_USE_GPU_NXP_VG_LITE */
