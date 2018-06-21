#include "mbed.h"
#include "opencv.hpp"
#include "EasyAttach_CameraAndLCD.h"
#include "USBMouse.h"

#define PLOT_INTERVAL          (30)
#define DIST_SCALE_FACTOR_X    (6.0)
#define DIST_SCALE_FACTOR_Y    (6.0)

/*! Frame buffer stride: Frame buffer stride should be set to a multiple of 32 or 128
    in accordance with the frame buffer burst transfer mode. */
#define VIDEO_PIXEL_HW         (160u)  /* HQVGA */
#define VIDEO_PIXEL_VW         (120u)  /* HQVGA */

#define FRAME_BUFFER_STRIDE    (((VIDEO_PIXEL_HW * 2) + 31u) & ~31u)
#define FRAME_BUFFER_HEIGHT    (VIDEO_PIXEL_VW)

#if defined(__ICCARM__)
#pragma data_alignment=32
static uint8_t user_frame_buffer0[FRAME_BUFFER_STRIDE * FRAME_BUFFER_HEIGHT]@ ".mirrorram";
#else
static uint8_t user_frame_buffer0[FRAME_BUFFER_STRIDE * FRAME_BUFFER_HEIGHT]__attribute((section("NC_BSS"),aligned(32)));
#endif
static volatile int Vfield_Int_Cnt = 0;

DisplayBase Display;
DigitalOut  led1(LED1);
USBMouse    mouse;
static Thread mainTask(osPriorityNormal, 1024 * 16);

static void IntCallbackFunc_Vfield(DisplayBase::int_type_t int_type) {
    if (Vfield_Int_Cnt > 0) {
        Vfield_Int_Cnt--;
    }
}

static void wait_new_image(void) {
    Vfield_Int_Cnt = 1;
    while (Vfield_Int_Cnt > 0) {
        Thread::wait(1);
    }
}

static void Start_Video_Camera(void) {
    // Field end signal for recording function in scaler 0
    Display.Graphics_Irq_Handler_Set(DisplayBase::INT_TYPE_S0_VFIELD, 0, IntCallbackFunc_Vfield);

    // Video capture setting (progressive form fixed)
    Display.Video_Write_Setting(
        DisplayBase::VIDEO_INPUT_CHANNEL_0,
        DisplayBase::COL_SYS_NTSC_358,
        (void *)user_frame_buffer0,
        FRAME_BUFFER_STRIDE,
        DisplayBase::VIDEO_FORMAT_YCBCR422,
        DisplayBase::WR_RD_WRSWA_32_16BIT,
        VIDEO_PIXEL_VW,
        VIDEO_PIXEL_HW
    );
    EasyAttach_CameraStart(Display, DisplayBase::VIDEO_INPUT_CHANNEL_0);
}

#if MBED_CONF_APP_LCD
static void Start_LCD_Display(void) {
    DisplayBase::rect_t rect;

    rect.vs = (LCD_PIXEL_HEIGHT - VIDEO_PIXEL_VW) / 2;
    rect.vw = VIDEO_PIXEL_VW;
    rect.hs = (LCD_PIXEL_WIDTH - VIDEO_PIXEL_HW);
    rect.hw = VIDEO_PIXEL_HW;
    Display.Graphics_Read_Setting(
        DisplayBase::GRAPHICS_LAYER_0,
        (void *)user_frame_buffer0,
        FRAME_BUFFER_STRIDE,
        DisplayBase::GRAPHICS_FORMAT_YCBCR422,
        DisplayBase::WR_RD_WRSWA_32_16BIT,
        &rect
    );
    Display.Graphics_Start(DisplayBase::GRAPHICS_LAYER_0);

    Thread::wait(50);
    EasyAttach_LcdBacklight(true);
}
#endif

static void main_task(void) {
    cv::Mat prev_image;
    cv::Mat curr_image;
    std::vector<cv::Point2f> prev_pts;
    std::vector<cv::Point2f> curr_pts;
    cv::Point2f point;
    int16_t  x = 0;
    int16_t  y = 0;

    EasyAttach_Init(Display);
    Start_Video_Camera();
#if MBED_CONF_APP_LCD
    Start_LCD_Display();
#endif

    // Initialization of optical flow
    point.y = (VIDEO_PIXEL_VW / 2) + (PLOT_INTERVAL * 1);
    for (int32_t i = 0; i < 3; i++) {
        point.x = (VIDEO_PIXEL_HW / 2) - (PLOT_INTERVAL * 1);
        for (int32_t j = 0; j < 3; j++) {
            prev_pts.push_back(point);
            point.x += PLOT_INTERVAL;
        }
        point.y -= PLOT_INTERVAL;
    }

    while (1) {
        // Wait for image input
        wait_new_image();

        // Convert from YUV422 to grayscale
        cv::Mat img_yuv(VIDEO_PIXEL_VW, VIDEO_PIXEL_HW, CV_8UC2, user_frame_buffer0);
        cv::cvtColor(img_yuv, curr_image, cv::COLOR_YUV2GRAY_YUY2);

        point = cv::Point2f(0, 0);
        if ((!curr_image.empty()) && (!prev_image.empty())) {
            // Optical flow
            std::vector<uchar> status;
            std::vector<float> err;
            cv::calcOpticalFlowPyrLK(prev_image, curr_image, prev_pts, curr_pts, status, err, cv::Size(21, 21), 0);

            // Setting movement distance of feature point
            std::vector<cv::Scalar> samples;
            for (size_t i = 0; i < (size_t)status.size(); i++) {
                if (status[i]) {
                    cv::Point2f vec = curr_pts[i] - prev_pts[i];
                    cv::Scalar sample = cv::Scalar(vec.x, vec.y);
                    samples.push_back(sample);
                }
            }

            // Mean and standard deviation
            if (samples.size() >= 6) {
                cv::Scalar mean;
                cv::Scalar stddev;
                cv::meanStdDev((cv::InputArray)samples, mean, stddev);
                //printf("%d,  stddev=%lf, %lf\r\n", samples.size(), stddev[0], stddev[1]);  // for debug
                if ((stddev[0] < 10.0) && (stddev[1] < 10.0)) {
                    point.x = mean[0];
                    point.y = mean[1];
                }
            }
        }
        cv::swap(prev_image, curr_image);

        x = (int16_t)(point.x * DIST_SCALE_FACTOR_X) * -1;
        y = (int16_t)(point.y * DIST_SCALE_FACTOR_Y);

        if ((x != 0) || (y != 0)) {
            led1 = 1;
            //printf("x=%d, y=%d\r\n", x, y);  // for debug
            mouse.move(x, y);
        } else {
            led1 = 0;
        }
    }
}

int main(void) {
    mainTask.start(callback(main_task));
    mainTask.join();
}
