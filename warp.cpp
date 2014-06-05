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

void identity(cv::Mat& dst, cv::Mat& src) {
    int height = src.rows;
    int width = src.cols;

    for (int j = 0; j < height; j++){
        int j_new = j;
        for (int i = 0; i < width; i++){
            int i_new = i;
            dst.at<float>(j_new, i_new) = (float)src.at<float>(j, i);
        }
    }
}

void project_flat(cv::Mat& dst, cv::Mat& src) {
    int height = src.rows;
    int width = src.cols;

    float jf = (float)height * 0.75f; // distance from camera to ceiling
    float zf = (float)height * 2.0f;  // distance to camera to projection plane
    float zb = (float)height * 0.5f;  // distance between projection and back planes

    for (int j = 0; j < height; j++){
        int j_new = (int)(jf + (zf * ((float)j - jf) / (zb + zf)));
        for (int i = 0; i < width; i++){
            int i_new = i;
            dst.at<float>(j_new, i_new) = (float)src.at<float>(j, i);
        }
    }
}

cv::Vec3b getSubpix(const cv::Mat& img, cv::Point2f pt)
{
    int x = (int)pt.x;
    int y = (int)pt.y;

    int x0 = cv::borderInterpolate(x,   img.cols, cv::BORDER_REFLECT_101);
    int x1 = cv::borderInterpolate(x+1, img.cols, cv::BORDER_REFLECT_101);
    int y0 = cv::borderInterpolate(y,   img.rows, cv::BORDER_REFLECT_101);
    int y1 = cv::borderInterpolate(y+1, img.rows, cv::BORDER_REFLECT_101);

    float a = pt.x - (float)x;
    float c = pt.y - (float)y;

    uchar b = (uchar)cvRound((img.at<cv::Vec3b>(y0, x0)[0] * (1.f - a) + img.at<cv::Vec3b>(y0, x1)[0] * a) * (1.f - c)
                           + (img.at<cv::Vec3b>(y1, x0)[0] * (1.f - a) + img.at<cv::Vec3b>(y1, x1)[0] * a) * c);
    uchar g = (uchar)cvRound((img.at<cv::Vec3b>(y0, x0)[1] * (1.f - a) + img.at<cv::Vec3b>(y0, x1)[1] * a) * (1.f - c)
                           + (img.at<cv::Vec3b>(y1, x0)[1] * (1.f - a) + img.at<cv::Vec3b>(y1, x1)[1] * a) * c);
    uchar r = (uchar)cvRound((img.at<cv::Vec3b>(y0, x0)[2] * (1.f - a) + img.at<cv::Vec3b>(y0, x1)[2] * a) * (1.f - c)
                           + (img.at<cv::Vec3b>(y1, x0)[2] * (1.f - a) + img.at<cv::Vec3b>(y1, x1)[2] * a) * c);

    return cv::Vec3b(b, g, r);
}

void wrap_cylinder(cv::Mat& dst, cv::Mat& src) {
    int height = src.rows;
    int width = src.cols;

    float zf = (float)height * 2.0f;  // distance to camera to projection plane

    float half_width = (float)width * 0.50f;
    float xf = half_width;
    float r = (float)width * 0.50f;
    float yf = (float)height * 0.75f; // distance from camera to ceiling

    int channels = dst.channels();

    for (int i = 0; i < width; i++)
    {
        // X-axis (columns)
        float zb = r * (cos((half_width - (float)i) / r) - cos(half_width / r));
        float z_ratio = (zb + zf) / zf;
        float i_new = xf + z_ratio * (r * sin(((float)i - half_width) / r));
        for (int j = 0; j < height; j++)
        {
            // Y-axis (rows)
            float j_new = yf + z_ratio * ((float)j - yf);
            if (i_new > 0.0f && i_new < (float)width && j_new > 0.0f && j_new < (float)height) {
                cv::Vec3b new_val = getSubpix(src, cv::Point2f(i_new, j_new));
                dst.at<cv::Vec3b>(j, i) = new_val;
            }
        }
    }
}

int main() {
    cv::Mat src = cv::imread("Lenna_grid.jpg", CV_LOAD_IMAGE_COLOR);
    src.convertTo(src, CV_8UC3);
    cv::Mat dst = cv::Mat::zeros(src.rows, src.cols, src.type());

    wrap_cylinder(dst, src);

    cv::imshow("output", dst);
    cv::waitKey(0);
}
