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

void identity(cv::Mat& out, cv::Mat& in) {
    int height = in.rows;
    int width = in.cols;

    for (int j = 0; j < height; j++){
        int j_new = j;
        for (int i = 0; i < width; i++){
            int i_new = i;
            out.at<float>(j_new, i_new) = (float)in.at<float>(j, i);
        }
    }
}

void project_flat(cv::Mat& out, cv::Mat& in) {
    int height = in.rows;
    int width = in.cols;

    float jf = (float)height * 0.75f; // distance from camera to ceiling
    float zf = (float)height * 2.0f;  // distance to camera to projection plane
    float zb = (float)height * 0.5f;  // distance between projection and back planes

    for (int j = 0; j < height; j++){
        int j_new = (int)(jf + (zf * ((float)j - jf) / (zb + zf)));
        for (int i = 0; i < width; i++){
            int i_new = i;
            out.at<float>(j_new, i_new) = (float)in.at<float>(j, i);
        }
    }
}

void wrap_cylinder(cv::Mat& out, cv::Mat& in) {
    int height = in.rows;
    int width = in.cols;

    float jf = (float)height * 0.75f; // distance from camera to ceiling
    float zf = (float)height * 2.0f;  // distance to camera to projection plane

    float half_width = (float)width * 0.50f;
    float xf = half_width;
    float r = (float)width * 0.50f;
    float yf = (float)height * 0.75f;

    int channels = out.channels();

    cv::Mat counter = cv::Mat::zeros(out.rows, out.cols, CV_8UC1);

    for (int i = 0; i < width; i++)
    {
        // X-axis (columns)
        float zf_ratio = zf / (zf + r * (cos((half_width - (float)i) / r) - cos(half_width / r)));
        int i_new = (int)round(xf + zf_ratio * (r * sin(((float)i - half_width) / r)));
        for (int j = 0; j < height; j++)
        {
            // Y-axis (rows)
            int j_new = (int)round(yf + zf_ratio * ((float)j - yf));
            int count = (int)counter.at<unsigned char>(j_new, i_new);
            cv::Vec3b new_val = in.at<cv::Vec3b>(j, i);
            cv::Vec3b old_val = out.at<cv::Vec3b>(j, i);
            for (int k = 0; k < channels; k++)
            {
                // for each channel
                if (count > 0) {
                    // calculate weighted value
                    new_val.val[k] = (unsigned char)((float)new_val.val[k] + (float)count * (float)old_val.val[k]) / (1.0f + (float)count);
                }
            }
            out.at<cv::Vec3b>(j_new, i_new) = new_val;
            counter.at<unsigned char>(j_new, i_new) = (unsigned char)(count + 1);
        }
    }
}

int main() {
    cv::Mat in = cv::imread("Lenna_grid.jpg", CV_LOAD_IMAGE_COLOR);
    in.convertTo(in, CV_8UC3);
    cv::Mat out = cv::Mat::zeros(in.rows, in.cols, in.type());

    wrap_cylinder(out, in);

    //normalize(out, out, 0, 255, cv::NORM_MINMAX, CV_8UC3);
    cv::imshow("output", out);
    cv::waitKey(0);
}
