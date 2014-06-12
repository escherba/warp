/*
 * =====================================================================================
 *
 *       Filename:  warp.cpp
 *
 *    Description:  Demonstrate various projection of an image onto a flat screen
 *
 *        Version:  0.0.2
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

template<typename T, typename U>
U get_subpixel4(const cv::Mat& src, cv::Point2f pt)
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

    const float one_minus_a = 1.f - a;
    const float one_minus_c = 1.f - c;

    const U y0_x0 = src.at<U>(y0, x0);
    const U y1_x0 = src.at<U>(y1, x0);
    const U y0_x1 = src.at<U>(y0, x1);
    const U y1_x1 = src.at<U>(y1, x1);

    const T b = (T)cvRound(
        (one_minus_a * (float)(y0_x0[0]) + a * (float)(y0_x1[0])) * one_minus_c +
        (one_minus_a * (float)(y1_x0[0]) + a * (float)(y1_x1[0])) * c
    );
    const T g = (T)cvRound(
        (one_minus_a * (float)(y0_x0[1]) + a * (float)(y0_x1[1])) * one_minus_c +
        (one_minus_a * (float)(y1_x0[1]) + a * (float)(y1_x1[1])) * c
    );
    const T r = (T)cvRound(
        (one_minus_a * (float)(y0_x0[2]) + a * (float)(y0_x1[2])) * one_minus_c +
        (one_minus_a * (float)(y1_x0[2]) + a * (float)(y1_x1[2])) * c
    );
    const T t = (T)cvRound(
        (one_minus_a * (float)(y0_x0[3]) + a * (float)(y0_x1[3])) * one_minus_c +
        (one_minus_a * (float)(y1_x0[3]) + a * (float)(y1_x1[3])) * c
    );
    return U(b, g, r, t);
}

template<typename U>
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
            dst.at<U>(j, i) = src.at<U>(j_src, i_src);
        }
    }
}

template<typename T, typename U>
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
                dst.at<U>(j, i) = get_subpixel4<T, U>(src, cv::Point2f(i_src, j_src));
            }
        }
    }
}

template<typename T, typename U>
void project_cylinder(cv::Mat& dst, cv::Mat& src)
{
    // Project from a cylindrical background onto a flat screen
    // Results in reduction of effective image size
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
        for (int j = 0; j < height; j++)
        {
            // Y-axis (rows)
            const float j_src = yf + z_ratio * x_scale * ((float)j - yf);
            if (i_src >= 0.0f && i_src <= (float)width &&
                j_src >= 0.0f && j_src <= (float)height)
            {
                dst.at<U>(j, i) = get_subpixel4<T, U>(src, cv::Point2f(i_src, j_src));
            }
        }
    }
}

template<typename T, typename U>
cv::Mat* project_cylinder_fixw(cv::Mat& src)
{
    // Project from a cylindrical background onto a flat screen
    // Keeps width of the stretched image same as the input image
    //
    const int height = src.rows;
    const int width  = src.cols;

    const float r  = (float)width * 0.5f;    // cylinder radius ((width / 2) <= r < inf)
    const float zf = hypot((float)height, (float)width);
                                             // distance from camera to screen (default: hypothenuse)
    const float xf = (float)width * 0.5f;    // distance from camera to left edge
    const float yf = (float)height * 0.75f;  // distance from camera to ceiling

    // precompute some constants
    const float half_width = (float)width * 0.5f;
    const float zfr = cos(half_width / r); // distance from cylinder center to projection plane
    const float xfr = sin(half_width / r);
    const float x_scale = r * xfr / half_width;
    const int adj_height = (int)ceil((float)height / x_scale);

    cv::Mat *dst = new cv::Mat(cv::Mat::zeros(adj_height, width, src.type()));

    for (int i = 0; i < width; i++)
    {
        // X-axis (columns)
        //
        const float zb = r * (cos((half_width - (float)i) / r) - zfr);
        const float z_ratio = (zb + zf) / zf;

        const float i_src = r * asin(x_scale * ((float)i - xf) * z_ratio / r) + half_width;
        for (int j = 0; j < adj_height; j++)
        {
            // Y-axis (rows)
            const float j_src = yf + z_ratio * (x_scale * (float)j - yf);
            if (i_src >= 0.0f && i_src <= (float)width &&
                j_src >= 0.0f && j_src <= (float)height)
            {
                dst->at<U>(j, i) = get_subpixel4<T, U>(src, cv::Point2f(i_src, j_src));
            }
        }
    }
    return dst;
}

int main(int argc, char *argv[]) {
    cv::Mat src = cv::imread(argv[1], CV_LOAD_IMAGE_UNCHANGED);

    // Grab pointer to function of appropriate type
    // see: http://docs.opencv.org/modules/core/doc/basic_structures.html#vec
    cv::Mat* (*project)(cv::Mat&);
    switch (src.type()) {

        case CV_8UC4:  // 8-bit 4-channel images
            project = project_cylinder_fixw<uchar, cv::Vec4b>;
            break;

        case CV_16UC4: // 16-bit 4-channel images
            project = project_cylinder_fixw<ushort, cv::Vec4w>;
            break;

        default:       // all other image types
            std::cerr << "Unexpected image type: " << src.type() << std::endl;
            std::exit(1);
    }

    // fix jaggies on top and bottom by adding a 2-px border
    cv::copyMakeBorder(src, src, 2, 2, 0, 0, cv::BORDER_CONSTANT, cv::Scalar::all(0));

    cv::Mat *dst = project(src);
    cv::imwrite(argv[2], *dst);
    delete dst;
}

