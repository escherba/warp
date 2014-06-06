/*
 * =====================================================================================
 *
 *       Filename:  warp.cpp
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  06/03/2014 13:38:27
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Eugene Scherba
 *   Organization:  Livefyre
 *
 * =====================================================================================
 */

#include <cmath>
#include <opencv2/opencv.hpp>


cv::Vec3b getSubpix(const cv::Mat& src, cv::Point2f pt)
{
    // Simple bilinear interpolation
    //
    const int x = (int)pt.x;
    const int y = (int)pt.y;

    const int x0 = cv::borderInterpolate(x,     src.cols, cv::BORDER_REFLECT_101);
    const int x1 = cv::borderInterpolate(x + 1, src.cols, cv::BORDER_REFLECT_101);
    const int y0 = cv::borderInterpolate(y,     src.rows, cv::BORDER_REFLECT_101);
    const int y1 = cv::borderInterpolate(y + 1, src.rows, cv::BORDER_REFLECT_101);

    const float a = pt.x - (float)x;
    const float c = pt.y - (float)y;

    const uchar b = (uchar)cvRound((src.at<cv::Vec3b>(y0, x0)[0] * (1.f - a) + src.at<cv::Vec3b>(y0, x1)[0] * a) * (1.f - c)
            + (src.at<cv::Vec3b>(y1, x0)[0] * (1.f - a) + src.at<cv::Vec3b>(y1, x1)[0] * a) * c);
    const uchar g = (uchar)cvRound((src.at<cv::Vec3b>(y0, x0)[1] * (1.f - a) + src.at<cv::Vec3b>(y0, x1)[1] * a) * (1.f - c)
            + (src.at<cv::Vec3b>(y1, x0)[1] * (1.f - a) + src.at<cv::Vec3b>(y1, x1)[1] * a) * c);
    const uchar r = (uchar)cvRound((src.at<cv::Vec3b>(y0, x0)[2] * (1.f - a) + src.at<cv::Vec3b>(y0, x1)[2] * a) * (1.f - c)
            + (src.at<cv::Vec3b>(y1, x0)[2] * (1.f - a) + src.at<cv::Vec3b>(y1, x1)[2] * a) * c);

    return cv::Vec3b(b, g, r);
}

void project_identity(cv::Mat& dst, cv::Mat& src)
{
    // Identity projection (no change)
    //
    const int height = dst.rows;
    const int width  = dst.cols;

    for (int j = 0; j < height; j++) {
        const int j_src = j;
        for (int i = 0; i < width; i++) {
            const int i_src = i;
            dst.at<cv::Vec3b>(j, i) = src.at<cv::Vec3b>(j_src, i_src);
        }
    }
}

void project_flat(cv::Mat& dst, cv::Mat& src)
{
    // Project from a flat background onto a flat screen
    //
    const int height = dst.rows;
    const int width  = dst.cols;

    const float jf = (float)height * 0.75f; // distance from camera to ceiling
    const float zf = (float)height * 2.0f;  // distance from camera to screen
    const float zb = (float)height * 0.5f;  // distance from screen to image

    for (int j = 0; j < height; j++) {
        const float z_ratio = (zb + zf) / zf;
        const float j_src = jf + z_ratio * ((float)j - jf);
        for (int i = 0; i < width; i++){
            const int i_src = i;
            if (i_src >= 0.0f && i_src <= (float)width &&
                j_src >= 0.0f && j_src <= (float)height) {
                dst.at<cv::Vec3b>(j, i) = getSubpix(src, cv::Point2f(i_src, j_src));
            }
        }
    }
}

void project_cylinder(cv::Mat& dst, cv::Mat& src)
{
    // Project from a cylindrical background onto a flat screen
    //
    const int height = dst.rows;
    const int width  = dst.cols;

    const float r  = (float)width * 0.5f;    // cylinder radius ((width / 2) <= r < inf)
    const float yf = (float)height * 0.75f;  // distance from camera to ceiling
    const float zf = hypot((float)height, (float)width);
                                             // distance from camera to screen (default: hypothenuse)
    const float xf = (float)width * 0.50f;   // distance from camera to left edge

    // precompute some constants
    const float half_width = (float)width * 0.50f;
    const float zfr = cos(half_width / r); // distance from cylinder center to projection plane

    for (int i = 0; i < width; i++)
    {
        // X-axis (columns)
        const float zb = r * (cos((half_width - (float)i) / r) - zfr);
        const float z_ratio = (zb + zf) / zf;
        const float i_src = xf + z_ratio * r * sin(((float)i - half_width) / r);
        for (int j = 0; j < height; j++)
        {
            // Y-axis (rows)
            const float j_src = yf + z_ratio * ((float)j - yf);
            if (i_src >= 0.0f && i_src <= (float)width &&
                j_src >= 0.0f && j_src <= (float)height)
            {
                dst.at<cv::Vec3b>(j, i) = getSubpix(src, cv::Point2f(i_src, j_src));
            }
        }
    }
}

int main() {
    cv::Mat src = cv::imread("Lenna_grid.jpg", CV_LOAD_IMAGE_COLOR);
    src.convertTo(src, CV_8UC3);
    cv::Mat dst = cv::Mat::zeros(src.rows, src.cols, src.type());

    project_cylinder(dst, src);

    cv::imshow("output", dst);
    cv::waitKey(0);
}
