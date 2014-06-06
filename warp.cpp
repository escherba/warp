/*
 * =====================================================================================
 *
 *       Filename:  warp.cpp
 *
 *    Description:  Demonstrate various projection of an image onto a flat screen
 *
 *        Version:  0.0.1
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

using namespace cv;

cv::Vec4b getSubpix(const cv::Mat& src, cv::Point2f pt)
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

    const uchar b = (uchar)cvRound(
        (src.at<cv::Vec4b>(y0, x0)[0] * (1.f - a) + src.at<cv::Vec4b>(y0, x1)[0] * a) * (1.f - c) +
        (src.at<cv::Vec4b>(y1, x0)[0] * (1.f - a) + src.at<cv::Vec4b>(y1, x1)[0] * a) * c
    );
    const uchar g = (uchar)cvRound(
        (src.at<cv::Vec4b>(y0, x0)[1] * (1.f - a) + src.at<cv::Vec4b>(y0, x1)[1] * a) * (1.f - c) +
        (src.at<cv::Vec4b>(y1, x0)[1] * (1.f - a) + src.at<cv::Vec4b>(y1, x1)[1] * a) * c
    );
    const uchar r = (uchar)cvRound(
        (src.at<cv::Vec4b>(y0, x0)[2] * (1.f - a) + src.at<cv::Vec4b>(y0, x1)[2] * a) * (1.f - c) +
        (src.at<cv::Vec4b>(y1, x0)[2] * (1.f - a) + src.at<cv::Vec4b>(y1, x1)[2] * a) * c
    );
    const uchar t = (uchar)cvRound(
        (src.at<cv::Vec4b>(y0, x0)[3] * (1.f - a) + src.at<cv::Vec4b>(y0, x1)[3] * a) * (1.f - c) +
        (src.at<cv::Vec4b>(y1, x0)[3] * (1.f - a) + src.at<cv::Vec4b>(y1, x1)[3] * a) * c
    );
    return cv::Vec4b(b, g, r, t);
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
            dst.at<cv::Vec4b>(j, i) = src.at<cv::Vec4b>(j_src, i_src);
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
                dst.at<cv::Vec4b>(j, i) = getSubpix(src, cv::Point2f(i_src, j_src));
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
    const float x_scale = r * sin(half_width / r) / half_width;

    for (int i = 0; i < width; i++)
    {
        // X-axis (columns)
        //
        const float zb = r * (cos((half_width - (float)i) / r) - zfr);
        const float z_ratio = (zb + zf) / zf;

        const float i_src = r * asin(x_scale * ((float)i - xf) * z_ratio / r) + half_width;
        //const float i_src = xf + z_ratio * r * sin(((float)i - half_width) / r);
        for (int j = 0; j < height; j++)
        {
            // Y-axis (rows)
            const float j_src = yf + z_ratio * x_scale * ((float)j - yf);
            if (i_src >= 0.0f && i_src <= (float)width &&
                j_src >= 0.0f && j_src <= (float)height)
            {
                dst.at<cv::Vec4b>(j, i) = getSubpix(src, cv::Point2f(i_src, j_src));
            }
        }
    }
}

cv::Mat* project_cylinder2(cv::Mat& src)
{
    // Project from a cylindrical background onto a flat screen
    //
    const int height = src.rows;
    const int width  = src.cols;

    const float r  = (float)width * 0.5f;    // cylinder radius ((width / 2) <= r < inf)
    const float zf = hypot((float)height, (float)width);
                                             // distance from camera to screen (default: hypothenuse)
    const float xf = (float)width * 0.5f;     // distance from camera to left edge
    const float yf = (float)height * 0.75f;  // distance from camera to ceiling

    // precompute some constants
    const float half_width = (float)width * 0.5f;
    const float zfr = cos(half_width / r); // distance from cylinder center to projection plane
    const float xfr = sin(half_width / r);
    const float x_scale = r * xfr / half_width;

    const int adj_height = (int)ceil((float)height / x_scale);

    std::cout << "before height: " << height << ", after height: " << adj_height << std::endl;
    cv::Mat *dst = new cv::Mat(cv::Mat::zeros(adj_height, width, src.type()));

    for (int i = 0; i < width; i++)
    {
        // X-axis (columns)
        //
        const float zb = r * (cos((half_width - (float)i) / r) - zfr);
        const float z_ratio = (zb + zf) / zf;

        const float i_src = r * asin(x_scale * ((float)i - xf) * z_ratio / r) + half_width;
        //const float i_src = xf + z_ratio * r * sin(((float)i - half_width) / r);
        for (int j = 0; j < adj_height; j++)
        {
            // Y-axis (rows)
            const float j_src = yf + z_ratio * (x_scale * (float)j - yf);
            if (i_src >= 0.0f && i_src <= (float)width &&
                j_src >= 0.0f && j_src <= (float)height)
            {
                dst->at<cv::Vec4b>(j, i) = getSubpix(src, cv::Point2f(i_src, j_src));
            }
        }
    }
    return dst;
}

int main(int argc, char *argv[]) {
    cv::Mat src = cv::imread(argv[1], CV_LOAD_IMAGE_UNCHANGED);
    cv::cvtColor(src, src, CV_RGB2RGBA);
    src.convertTo(src, CV_8UC4); // convert to 8-bit BGRA

    // fix jaggies on top and bottom by adding a 2-px border
    cv::copyMakeBorder(src, src, 2, 2, 0, 0, cv::BORDER_CONSTANT, cv::Scalar::all(0));

    cv::Mat *dst = project_cylinder2(src);
    cv::imwrite(argv[2], *dst);
}

