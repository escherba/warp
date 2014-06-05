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

    for (int i = 0; i < width; i++)
    {   // X-axis (columns)
        float zb = r * (cos((half_width - (float)i) / r) - cos(half_width / r));
        for (int j = 0; j < height; j++)
        {   // Y-axis (rows)
            int i_new = (int)round((xf + zf * (r * sin((half_width - (float)i) / r))/(zf + zb)));
            int j_new = (int)round((yf + zf * ((float)j - yf)/(zf + zb)));
            float new_val = (float)in.at<float>(j, i);
            float old_val = (float)out.at<float>(j_new, i_new);
            if (old_val > 0.0f) {
                new_val = (new_val + old_val) * 0.50f;
            }
            out.at<float>(j_new, i_new) = new_val;
        }
    }
}

int main() {
    cv::Mat in = cv::imread("Lenna_grid.jpg",CV_LOAD_IMAGE_GRAYSCALE);
    in.convertTo(in, CV_32FC1);
    cv::Mat out = cv::Mat(in.rows, in.cols, in.type());

    wrap_cylinder(out, in);

    normalize(out, out,0,255,cv::NORM_MINMAX,CV_8UC1);

    cv::imshow("output", out);
    cv::waitKey(0);
}
