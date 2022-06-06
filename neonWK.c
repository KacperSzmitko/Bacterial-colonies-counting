#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <arm_neon.h>

const int BACTERIA_VALUE = 255;
const int BACKGROUND_VALUE = 0;



unsigned char** add_padding(unsigned char** img,int width, int height) {
    /*
    Adds 1 pixel padding to image and returns, padded one
    */
    unsigned char** padded_image = (unsigned char**)calloc((height + 2), sizeof(unsigned char*));
    for (int i = 0; i < height + 2; i++) {
        
        padded_image[i] = (unsigned char*)malloc((width + 2)* sizeof(unsigned char));
        for (int j = 0; j < (width+2)/16; j++) {
            uint8x16_t result = vdupq_n_u8(0);
            vst1q_u8(padded_image[i], result);
            padded_image[i] += 16;
        }
    }
    for (int i = 1; i < height +1; i++) {
        for (int j = 1; j < width +1; j++) {
            padded_image[i][j] = img[i - 1][j - 1];
        }
    }
    return padded_image;
}


unsigned char** to_grayscale(FILE* img, int width, int height, FILE* out) {
    /*
    Converts image to grayscale. Also saves modified image on disk.

    img: Image to convert
    out: File to save grayscale image
    */
    
    uint8_t *image_read = (uint8_t*)malloc(width*height * 3 *sizeof(uint8_t));
    uint8_t *image_write = (uint8_t*)malloc(width*height * sizeof(uint8_t));
    uint8_t **image_write_2d = (uint8_t**)malloc(height * sizeof(uint8_t* ));
    for (int i = 0; i < height; i++) {
        image_write_2d[i] = (uint8_t*)malloc(width * sizeof(uint8_t));
    }
    
    
    fread(image_read,1,width*height*3,img);
    
    
    uint8_t *src = &image_read[0];
    uint8_t *dest = &image_write[0];

    
    uint8x8_t r_ratio = vdup_n_u8 (77);
    uint8x8_t g_ratio = vdup_n_u8 (151);
    uint8x8_t b_ratio = vdup_n_u8 (28);
    int num = (width * height)/8;
    
    for(int i = 0 ; i< num ; i++)
    {
        uint16x8_t temp;
        uint8x8x3_t rgb = vld3_u8(src);
        uint8x8_t result;
        temp = vmull_u8 (rgb.val[0], r_ratio);
        temp = vmlal_u8 (temp, rgb.val[1], g_ratio);
        temp = vmlal_u8 (temp, rgb.val[2], b_ratio);
        result = vshrn_n_u16(temp,8);
        vst1_u8(dest, result);
        src += 8*3;
        dest += 8;
    }
 
    
    int k = 0;
    for(int i = 0; i < height; i++)
    {
        for(int j = 0; j < width; j++)
        {
            image_write_2d[i][j] = image_write[k++];
            //fputc(image_write_2d[i][j],out);
        }
    }
    return image_write_2d;
}



unsigned char** gaussian_blur_with_threshhold(unsigned char** grayscale_img, int width, int height, FILE* blured_out, FILE* thresh_out,int threshhold) {
    /*
    Applies gaussian blur and threshoold based on given value. Also saves blured image and thresholded image.

    blured_out: File to save blured image
    thresh_out: File to save thresholded image
    */
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
    
    unsigned char pcs_arr[8];
    int cnt = 0;  
    int l = 1; 
    int width_h = width +1;
    for (int i = 1; i < height + 1; i++) {
        l = 1;
        for (int j = 1; j < width + 1; j++) {
            pixel_conv_sum = 0;
            mask_index = 0;
            for (int k = i - 1; k < i + 2; k++) {
                for (int p = j - 1; p < j + 2; p++) {
                    pixel_conv_sum += mask[mask_index] * padded_image[k][p];
                    mask_index++;
                }
            }
            blured_image[i - 1][j - 1] = (unsigned char)pixel_conv_sum;
            pcs_arr[cnt] = blured_image[i - 1][j - 1]; // 0 1 2 3 4 5 6 7
            cnt++;
            
            if (cnt == 8) {
                if (width_h < 8){
                    if (pixel_conv_sum < threshhold) {
                    thresh_value = BACTERIA_VALUE;
                    }
                    else {
                     thresh_value = BACKGROUND_VALUE;
                    }
                    thresh_image[i - 1][j - 1] = thresh_value;
                    cnt = 7;
                }
                else
                {
                
                    uint8x8_t thrf = vdup_n_u8(145);
                    uint8x8_t pf = vld1_u8(pcs_arr);
                    uint8x8_t comp = vclt_u8(pf,thrf);

                    for (int k = 0 ; k < 8; k++) {
                        thresh_image[i - 1][l - 1 + k] = comp[k];
                    }
                    cnt = 0;
                    l += 8;
                    
                }
            }
            
        }
        
        width_h -= 8;
    }  
    return thresh_image;
}


int connected_components_count(unsigned char** thresh_image, int width, int height) {
    /*
    Counts number of bacterias using connected componets method.
    */
    unsigned char** labels = (unsigned char**)calloc(height, sizeof(unsigned char*));
    for (int i = 0; i < height; i++) {
        labels[i] = (unsigned char*)malloc(width* sizeof(unsigned char));
        for (int j = 0; j < width/16; j++) {
            uint8x16_t result1 = vdupq_n_u8(0);
            vst1q_u8(labels[i], result1);
            labels[i] += 16;
        }
    }
    unsigned char** padded_image = add_padding(thresh_image, width, height);
    int current_label = 1;
    int neigbour_count = 0;
    int16_t* neighbour_check = malloc((width) * (height) * sizeof(int16_t));

    for (int i = 0; i < (width * height)/8 ; i++) {
        int16x8_t result2 = vdupq_n_s16(0);
        vst1q_s16(neighbour_check, result2);
        neighbour_check += 8;
    }

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

int main(int argc, char** argv) {
	
	
	clock_t t;
	t = clock();
	
    FILE * grayscale_output, *blured_output,*thresh_output, *in;
    char* input_path = "bactery_colony_input.ppm";
    if (argc > 1) {
        input_path = argv[1];
    }
    grayscale_output = fopen("grayscale_output.ppm", "wb");

    blured_output = fopen("blured_output.ppm", "wb");

    thresh_output = fopen("thresh_output.ppm", "wb");

    in = fopen(input_path, "rb");

    int width, height, max_colour;
    char line[256];
    fgets(line, sizeof(line), in);
    // Add meta information to output files
    int i = 0;
    while (!fscanf(in, "%d %d %d", &width, &height, &max_colour) || i > 5) {
        // Dont read comments
        fgets(line, sizeof(line), in);
        i++;
    }
    fprintf(grayscale_output, "P6\n%d %d\n%d\n", width, height, 255);
    fprintf(blured_output, "P6\n%d %d\n%d\n", width, height, 255);
    fprintf(thresh_output, "P6\n%d %d\n%d\n", width, height, 255);

    unsigned char** gray_scale_image = to_grayscale(in, width, height, grayscale_output);
    fclose(in);
    fclose(grayscale_output);
    unsigned char** thresh_image = gaussian_blur_with_threshhold(gray_scale_image, width, height, blured_output, thresh_output, 145);
    fclose(blured_output);
    fclose(thresh_output);
    int number_of_bacterias = connected_components_count(thresh_image, width, height);
    printf("Number of bacterias: %d\n", number_of_bacterias);

    
    t = clock() - t;
	double time_taken = ((double)t)/CLOCKS_PER_SEC;
	printf("%f\n",time_taken);
	
    return number_of_bacterias;
}
