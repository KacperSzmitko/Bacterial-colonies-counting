﻿#include <stdio.h>
#include <string.h>
#include <stdlib.h>

const int BACTERIA_VALUE = 255;
const int BACKGROUND_VALUE = 0;

unsigned char** add_padding(char** img,int width, int height) {
    unsigned char** padded_image = (unsigned char**)calloc((height + 2), sizeof(unsigned char*));
    for (int i = 0; i < height + 2; i++) {
        if (padded_image) {
            padded_image[i] = (unsigned char*)calloc((width + 2), sizeof(unsigned char));
        }
    }
    for (int i = 1; i < height; i++) {
        for (int j = 1; j < width; j++) {
            padded_image[i][j] = img[i - 1][j - 1];
        }
    }
    return padded_image;
}

unsigned char** to_grayscale(FILE* img, int width, int height, FILE* out) {
    unsigned char** gray_scale_image = (unsigned char**)malloc(height * sizeof(unsigned char* ));
    for (int i = 0; i < height; i++) {
        gray_scale_image[i] = (unsigned char*)malloc(width * sizeof(unsigned char));
    }
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            unsigned char color[3];
            fread(color, 1, 3, img);
            gray_scale_image[i][j] = (int)((int)color[0] * 0.2989 + (int)color[1] * 0.5870 + (int)color[2] * 0.1140);
            fputc(gray_scale_image[i][j], out);
            fputc(gray_scale_image[i][j], out);
            fputc(gray_scale_image[i][j], out);

        }
    }
    return gray_scale_image;
}

unsigned char** gaussian_blur_with_threshhold(unsigned char** grayscale_img, int width, int height, FILE* blured_out, FILE* thresh_out,int threshhold) {
    unsigned char** blured_image = (unsigned char**)malloc((height) * sizeof(unsigned char*));
    for (int i = 0; i < height; i++) {
        if (blured_image) {
            blured_image[i] = (unsigned char*)malloc((width) * sizeof(unsigned char));
        }
    }        
    
    unsigned char** thresh_image = (unsigned char**)malloc((height) * sizeof(unsigned char*));
    for (int i = 0; i < height; i++) {
        if (thresh_image) {
            thresh_image[i] = (unsigned char*)malloc((width) * sizeof(unsigned char));
        }
    }    
    
    unsigned char** padded_image = add_padding(grayscale_img, width, height);
    float pixel_conv_sum = 0;
    int mask_index = 0;
    int thresh_value = 0;
    float mask[9] = { 0.0625, 0.125, 0.0625, 0.125, 0.25, 0.125, 0.0625, 0.125, 0.0625 };
    for (int i = 1; i < height + 1; i++) {
        for (int j = 1; j < width + 1; j++) {
            pixel_conv_sum = 0;
            mask_index = 0;
            for (int k = i - 1; k < i + 2; k++) {
                for (int p = j - 1; p < j + 2; p++) {
                    pixel_conv_sum += mask[mask_index] * padded_image[k][p];
                    mask_index++;
                }
            }
            blured_image[i - 1][j - 1] = (int)pixel_conv_sum;
            fputc(blured_image[i - 1][j - 1], blured_out);
            fputc(blured_image[i - 1][j - 1], blured_out);
            fputc(blured_image[i - 1][j - 1], blured_out);
            if (pixel_conv_sum < threshhold) {
                thresh_value = BACTERIA_VALUE;
            }
            else {
                thresh_value = BACKGROUND_VALUE;
            }
            thresh_image[i - 1][j - 1] = thresh_value;
            fputc(thresh_image[i - 1][j - 1], thresh_out);
            fputc(thresh_image[i - 1][j - 1], thresh_out);
            fputc(thresh_image[i - 1][j - 1], thresh_out);
        }
    }
    return thresh_image;
}

int connected_components_count(unsigned char** thresh_image, int width, int height) {
    unsigned char** labels = (unsigned char**)calloc(height, sizeof(unsigned char*));
    for (int i = 0; i < height; i++) {
        labels[i] = (unsigned char*)calloc(width, sizeof(unsigned char));
    }
    char** padded_image = add_padding(thresh_image, width, height);
    int current_label = 1;
    int neigbour_count = 0;
    int neighbour_check[400];
    for (int i = 1; i < height + 1; i++) {
        for (int j = 1; j < width + 1; j++) {
            if ((int)padded_image[i][j] != BACKGROUND_VALUE && labels[i - 1][j - 1] == 0){
                labels[i - 1][j - 1] = current_label;
                neighbour_check[neigbour_count] = i;
                neighbour_check[neigbour_count + 1] = j;
                neigbour_count += 2;
                while (neigbour_count > 0) {
                    int neigbour_i = neighbour_check[neigbour_count - 2];
                    int neigbour_j = neighbour_check[neigbour_count - 1];
                    neigbour_count -= 2;
                    for (int k = neigbour_i - 1; k < neigbour_i + 2; k++) {
                        for (int p = neigbour_j - 1; p < neigbour_j + 2; p++) {
                            if ((int)padded_image[k][p] != BACKGROUND_VALUE && labels[k - 1][p - 1] == 0) {
                                labels[k - 1][p - 1] = current_label;
                                neighbour_check[neigbour_count] = k;
                                neighbour_check[neigbour_count + 1] = p;
                                neigbour_count += 2;
                            }
                        }
                    }
                }
                current_label++;
            }
        }
    }
    return current_label - 1;
}

int main() {
    errno_t err;
    FILE * grayscale_output, *blured_output,*thresh_output, *in;
    err = fopen_s(&grayscale_output,"grayscale_output.ppm", "wb");
    if (err || grayscale_output == NULL) {
        printf("Cant open grayscale output image");
        return -1;
    }
    err = fopen_s(&blured_output,"blured_output.ppm", "wb");
    if (err || blured_output == NULL) {
        printf("Cant open blured output image");
        return -1;
    }
    err = fopen_s(&thresh_output,"thresh_output.ppm", "wb");
    if (err || thresh_output == NULL) {
        printf("Cant open thresh output image");
        return -1;
    }
    err = fopen_s(&in,"bactery_colony_input.ppm", "rb");
    if (err || in == NULL) {
        printf("Cant open input image");
        return -1;
    }
    int width, height, max_colour;
    fscanf_s(in, "P6\n %d %d %d", &width, &height, &max_colour);
    fprintf(grayscale_output, "P6\n%d %d\n%d\n", width, height, 255);
    fprintf(blured_output, "P6\n%d %d\n%d\n", width, height, 255);
    fprintf(thresh_output, "P6\n%d %d\n%d\n", width, height, 255);

    unsigned char** gray_scale_image = to_grayscale(in, width, height, grayscale_output);
    unsigned char** thresh_image = gaussian_blur_with_threshhold(gray_scale_image, width, height, blured_output, thresh_output, 145);
    int number_of_bacterias = connected_components_count(thresh_image, width, height);
    printf("Number of bacterias: %d", number_of_bacterias);
    free(gray_scale_image);
    free(thresh_image);
    fclose(grayscale_output);
    fclose(blured_output);
    fclose(thresh_output);
    fclose(in);
    return number_of_bacterias;
}