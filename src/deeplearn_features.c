/*
  libdeep - a library for deep learning
  Copyright (C) 2013-2015  Bob Mottram <bob@robotics.uk.to>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  1. Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
  3. Neither the name of the University nor the names of its contributors
  may be used to endorse or promote products derived from this software
  without specific prior written permission.
  .
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE HOLDERS OR
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "deeplearn_features.h"

/**
 * @brief Scans an image patch and transfers the values to an autocoder
 * @param img_width Width of the image
 * @param img_depth Depth of the image
 * @param tx Top left coordinate of the patch
 * @param ty Top coordinate of the patch
 * @param bx Bottom right coordinate of the patch
 * @param by Bottom coordinate of the patch
 * @param feature_autocoder Autocoder object
 * @return zero on success
 */
static int scan_img_patch(unsigned char img[],
                          int img_width, int img_depth,
                          int tx, int ty, int bx, int by,
                          bp * feature_autocoder)
{
    int index_feature_input = 0;
    for (int y = ty; y < by; y++) {
        for (int x = tx; x < bx; x++) {
            int index_img =
                ((y*img_width) + x) * img_depth;
            for (int d = 0; d < img_depth;
                 d++, index_feature_input++) {
                /* convert from 8 bit to a neuron value */
                float v =
                    0.25f + (img[index_img+d]/(2*255.0f));
                bp_set_input(feature_autocoder,
                             index_feature_input, v);
            }
        }
    }

    /* check that the patch size is the same as the autocoder inputs */
    if (index_feature_input != feature_autocoder->NoOfInputs) {
        return -1;
    }
    return 0;
}

/**
 * @brief Scans a patch within a 2D array of floats and transfers the values
 *        to an autocoder
 * @param inputs_width Width of the floats array
 * @param inputs_depth Depth of the floats array
 * @param tx Top left coordinate of the patch
 * @param ty Top coordinate of the patch
 * @param bx Bottom right coordinate of the patch
 * @param by Bottom coordinate of the patch
 * @param feature_autocoder Autocoder object
 * @return zero on success
 */
static int scan_floats_patch(float inputs_floats[],
                             int inputs_width, int inputs_depth,
                             int tx, int ty, int bx, int by,
                             bp * feature_autocoder)
{
    int index_feature_input = 0;
    for (int y = ty; y < by; y++) {
        for (int x = tx; x < bx; x++) {
            int index_inputs =
                ((y*inputs_width) + x) * inputs_depth;
            for (int d = 0; d < inputs_depth;
                 d++, index_feature_input++) {
                /* set the inputs of the autocoder */
                bp_set_input(feature_autocoder,
                             index_feature_input,
                             inputs_floats[index_inputs+d]);
            }
        }
    }

    /* check that the patch size is the same as the autocoder inputs */
    if (index_feature_input != feature_autocoder->NoOfInputs) {
        return -1;
    }
    return 0;
}

/**
 * @brief Returns the input patch bounding box for an x,y coordinate
 *        within the second layer
 * @param x Position across within the second layer
 * @param y Position down within the second layer
 * @param samples_across The number of units across in the second layer
 * @param samples_down The number of units down in the second layer
 * @param patch_radius The radius of the patch within the input layer
 * @param width Width of the input layer
 * @param height Height of the input layer
 * @param tx Returned top left coordinate
 * @param ty Returned top coordinate
 * @param bx Returned bottom right coordinate
 * @param by Returned bottom coordinate
 * @return zero if the patch does not exceed the limits of the area
 */
int features_patch_coords(int x, int y,
                          int samples_across,
                          int samples_down,
                          int patch_radius,
                          int width, int height,
                          int * tx, int * ty, int * bx, int * by)
{
    int cy = y * height / samples_down;
    int cx = x * width / samples_across;

    *ty = cy - patch_radius;
    *by = cy + patch_radius;
    if (*ty < 0) return -1;
    if (*by >= height) return -2;

    *tx = cx - patch_radius;
    *bx = cx + patch_radius;
    if (*tx < 0) return -3;
    if (*bx >= width) return -4;
    return 0;
}

/**
 * @brief Learn a feature set between an input image and a neuron layer
 * @param samples_across The number of units across in the second layer
 * @param samples_down The number of units down in the second layer
 * @param patch_radius The radius of the patch within the image
 * @param img_width Width of the image
 * @param img_height Height of the image
 * @param img_depth Depth of the image (mono=1, RGB=3)
 * @param img Image buffer
 * @param layer0_units Number of units in the neuron layer
 * @param feature_autocoder An autocoder used for feature learning
 * @param BPerror Returned total learning error
 * @returns zero on success
 */
int features_learn_from_img(int samples_across,
                            int samples_down,
                            int patch_radius,
                            int img_width,
                            int img_height,
                            int img_depth,
                            unsigned char img[],
                            int layer0_units,
                            bp * feature_autocoder,
                            float * BPerror)
{
    int no_of_learned_features = feature_autocoder->NoOfHiddens;
    *BPerror = 0;

    if (samples_across * samples_down * no_of_learned_features !=
        layer0_units) {
        /* across*down doesn't equal the second layer units */
        return -1;
    }

    if (feature_autocoder->NoOfInputs !=
        patch_radius*patch_radius*4*img_depth) {
        /* the patch size doesn't match the feature
           learner inputs */
        printf("NoOfInputs %d\n",feature_autocoder->NoOfInputs);
        printf("patch_radius %d\n",patch_radius);
        printf("img_depth %d\n",img_depth);
        return -2;
    }

    if (feature_autocoder->NoOfInputs !=
        feature_autocoder->NoOfOutputs) {
        /* same number of inputs and outputs */
        return -3;
    }

    for (int fy = 0; fy < samples_down; fy++) {
        for (int fx = 0; fx < samples_across; fx++) {
            int tx=0, ty=0, bx=0, by=0;
            if (features_patch_coords(fx, fy,
                                      samples_across, samples_down,
                                      patch_radius,
                                      img_width, img_height,
                                      &tx, &ty, &bx, &by) != 0) {
                continue;
            }

            if (scan_img_patch(img, img_width, img_depth,
                               tx, ty, bx, by,
                               feature_autocoder) != 0) {
                return -4;
            }

            bp_update_autocoder(feature_autocoder);
            *BPerror = *BPerror + feature_autocoder->BPerror;
        }
    }

    /* calculate the average error */
    *BPerror = *BPerror / (samples_across*samples_down);
    return 0;
}

/**
 * @brief Learn a feature set between an array of floats and a neuron layer
 *        Inputs are expected to have values in the range 0.25->0.75
 * @param samples_across The number of units across in the second layer
 * @param samples_down The number of units down in the second layer
 * @param patch_radius The radius of the patch within the inputs
 * @param inputs_width Width of the inputs array
 * @param inputs_height Height of the inputs array
 * @param inputs_depth Depth of the inputs array
 * @param inputs_floats Inputs buffer of floats
 * @param layer0_units Number of units in the neuron layer
 * @param feature_autocoder An autocoder used for feature learning
 * @param BPerror Returned total backprop error
 * @returns zero on success
 */
int features_learn_from_flt(int samples_across,
                            int samples_down,
                            int patch_radius,
                            int inputs_width,
                            int inputs_height,
                            int inputs_depth,
                            float inputs_floats[],
                            int layer0_units,
                            bp * feature_autocoder,
                            float * BPerror)
{
    int no_of_learned_features = feature_autocoder->NoOfHiddens;
    *BPerror = 0;

    if (samples_across * samples_down * no_of_learned_features !=
        layer0_units) {
        /* across*down doesn't equal the second layer units */
        return -1;
    }

    if (feature_autocoder->NoOfInputs !=
        patch_radius*patch_radius*4*inputs_depth) {
        /* the patch size doesn't match the feature
           learner inputs */
        return -2;
    }

    if (feature_autocoder->NoOfInputs !=
        feature_autocoder->NoOfOutputs) {
        /* same number of inputs and outputs */
        return -3;
    }

    for (int fy = 0; fy < samples_down; fy++) {
        for (int fx = 0; fx < samples_across; fx++) {
            int tx=0, ty=0, bx=0, by=0;
            if (features_patch_coords(fx, fy, samples_across,
                                      samples_down,
                                      patch_radius,
                                      inputs_width, inputs_height,
                                      &tx, &ty, &bx, &by) != 0) {
                continue;
            }

            if (scan_floats_patch(inputs_floats,
                                  inputs_width, inputs_depth,
                                  tx, ty, bx, by,
                                  feature_autocoder) != 0) {
                return -4;
            }

            bp_update_autocoder(feature_autocoder);
            *BPerror = *BPerror + feature_autocoder->BPerror;
        }
    }

    /* calculate the average error */
    *BPerror = *BPerror / (samples_across*samples_down);
    return 0;
}

/**
 * @brief Convolve an image with learned features and output
 *        the results to the input layer of a neural net
 * @param samples_across The number of units across in the input layer
 *        (sampling grid resolution)
 * @param samples_down The number of units down in the input layer
 *        (sampling grid resolution)
 * @param patch_radius The radius of the patch within the image
 * @param img_width Width of the image
 * @param img_height Height of the image
 * @param img_depth Depth of the image (mono=1, RGB=3)
 * @param img Image buffer
 * @param layer0 Neural net
 * @param feature_autocoder An autocoder containing learned features
 * @returns zero on success
 */
int features_conv_img_to_neurons(int samples_across,
                                 int samples_down,
                                 int patch_radius,
                                 int img_width,
                                 int img_height,
                                 int img_depth,
                                 unsigned char img[],
                                 bp * net,
                                 bp * feature_autocoder)
{
    int no_of_learned_features = feature_autocoder->NoOfHiddens;

    if (samples_across * samples_down * no_of_learned_features !=
        net->NoOfInputs) {
        /* across*down doesn't equal the second layer units */
        return -1;
    }

    if (feature_autocoder->NoOfInputs !=
        patch_radius*patch_radius*4*img_depth) {
        /* the patch size doesn't match the feature
           learner inputs */
        return -2;
    }

    if (feature_autocoder->NoOfInputs !=
        feature_autocoder->NoOfOutputs) {
        /* same number of inputs and outputs */
        return -3;
    }

    for (int fy = 0; fy < samples_down; fy++) {
        for (int fx = 0; fx < samples_across; fx++) {
            int tx=0, ty=0, bx=0, by=0;
            if (features_patch_coords(fx, fy,
                                      samples_across, samples_down,
                                      patch_radius,
                                      img_width, img_height,
                                      &tx, &ty, &bx, &by) != 0) {
                continue;
            }

            if (scan_img_patch(img,
                               img_width, img_depth,
                               tx, ty, bx, by,
                               feature_autocoder) != 0) {
                return -4;
            }

            bp_feed_forward_layers(feature_autocoder, 0);
            int index_input_layer =
                (fy * samples_across + fx) *
                no_of_learned_features;
            for (int f = 0; f < no_of_learned_features; f++) {
                bp_set_input(net, index_input_layer+f,
                             bp_get_hidden(feature_autocoder, 0, f));
            }
        }
    }
    return 0;
}

/**
 * @brief Convolve an image with learned features and output
 *        the results to an array of floats
 * @param samples_across The number of units across in the array of floats
 *        (sampling grid resolution)
 * @param samples_down The number of units down in the array of floats
 *        (sampling grid resolution)
 * @param patch_radius The radius of the patch within the float array
 * @param img_width Width of the image
 * @param img_height Height of the image
 * @param img_depth Depth of the image (mono=1, RGB=3)
 * @param img Image buffer
 * @param layer0_units Number of units in the float array
 * @param layer0 float array
 * @param feature_autocoder An autocoder containing learned features
 * @returns zero on success
 */
int features_conv_img_to_flt(int samples_across,
                             int samples_down,
                             int patch_radius,
                             int img_width,
                             int img_height,
                             int img_depth,
                             unsigned char img[],
                             int layer0_units,
                             float layer0[],
                             bp * feature_autocoder)
{
    int no_of_learned_features = feature_autocoder->NoOfHiddens;

    if (samples_across * samples_down * no_of_learned_features !=
        layer0_units) {
        /* across*down doesn't equal the second layer units */
        return -1;
    }

    if (feature_autocoder->NoOfInputs !=
        patch_radius*patch_radius*4*img_depth) {
        /* the patch size doesn't match the feature
           learner inputs */
        return -2;
    }

    if (feature_autocoder->NoOfInputs !=
        feature_autocoder->NoOfOutputs) {
        /* same number of inputs and outputs */
        return -3;
    }

    for (int fy = 0; fy < samples_down; fy++) {
        for (int fx = 0; fx < samples_across; fx++) {
            int index_layer0 =
                (fy * samples_across + fx) *
                no_of_learned_features;

            int tx=0, ty=0, bx=0, by=0;
            if (features_patch_coords(fx, fy,
                                      samples_across, samples_down,
                                      patch_radius,
                                      img_width, img_height,
                                      &tx, &ty, &bx, &by) != 0) {
				for (int f = 0; f < no_of_learned_features; f++) {
					layer0[index_layer0+f] = 0;
				}
                continue;
            }

            if (scan_img_patch(img, img_width, img_depth,
                               tx, ty, bx, by,
                               feature_autocoder) != 0) {
                return -4;
            }

            bp_feed_forward_layers(feature_autocoder, 0);
            for (int f = 0; f < no_of_learned_features; f++) {
                layer0[index_layer0+f] =
                    bp_get_hidden(feature_autocoder, 0, f);
            }
        }
    }
    return 0;
}

/**
 * @brief Convolve a first array of floats to a second one
 * @param samples_across The number of units across in the second array of floats (sampling grid resolution)
 * @param samples_down The number of units down in the second array of floats (sampling grid resolution)
 * @param patch_radius The radius of the patch within the first float array
 * @param floats_width Width of the image
 * @param floats_height Height of the image
 * @param floats_depth Depth of the image (mono=1, RGB=3)
 * @param layer0 First array of floats
 * @param layer1_units Number of units in the second float array
 * @param layer1 Second float array
 * @param feature_autocoder An autocoder containing learned features
 * @returns zero on success
 */
int features_conv_flt_to_flt(int samples_across,
                             int samples_down,
                             int patch_radius,
                             int floats_width,
                             int floats_height,
                             int floats_depth,
                             float layer0[],
                             int layer1_units,
                             float layer1[],
                             bp * feature_autocoder)
{
    int no_of_learned_features = feature_autocoder->NoOfHiddens;

    if (samples_across * samples_down * no_of_learned_features !=
        layer1_units) {
        /* across*down doesn't equal the second layer units */
        return -1;
    }

    if (feature_autocoder->NoOfInputs !=
        patch_radius*patch_radius*4*floats_depth) {
        /* the patch size doesn't match the feature
           learner inputs */
        return -2;
    }

    if (feature_autocoder->NoOfInputs !=
        feature_autocoder->NoOfOutputs) {
        /* same number of inputs and outputs */
        return -3;
    }

    for (int fy = 0; fy < samples_down; fy++) {
        for (int fx = 0; fx < samples_across; fx++) {
            int index_layer1 =
                (fy * samples_across + fx) *
                no_of_learned_features;
            int tx=0, ty=0, bx=0, by=0;
            if (features_patch_coords(fx, fy,
                                      samples_across, samples_down,
                                      patch_radius,
                                      floats_width, floats_height,
                                      &tx, &ty, &bx, &by) != 0) {
				for (int f = 0; f < no_of_learned_features; f++) {
					layer1[index_layer1+f] = 0;
				}
                continue;
            }

            if (scan_floats_patch(layer0,
                                  floats_width, floats_depth,
                                  tx, ty, bx, by,
                                  feature_autocoder) != 0) {
                return -4;
            }

            bp_feed_forward_layers(feature_autocoder, 0);
            for (int f = 0; f < no_of_learned_features; f++) {
                layer1[index_layer1+f] =
                    bp_get_hidden(feature_autocoder, 0, f);
            }
        }
    }
    return 0;
}

/**
 * @brief Convolve an array of floats to the input layer of a neural net
 * @param samples_across The number of units across in the layer of neurons
 *        (sampling grid resolution)
 * @param samples_down The number of units down in the layer of neurons
 *        (sampling grid resolution)
 * @param patch_radius The radius of the patch within the float array
 * @param floats_width Width of the image
 * @param floats_height Height of the image
 * @param floats_depth Depth of the image (mono=1, RGB=3)
 * @param layer0 Array of floats
 * @param net Neural net to set the inputs for
 * @param feature_autocoder An autocoder containing learned features
 * @returns zero on success
 */
int features_conv_floats_to_neurons(int samples_across,
                                    int samples_down,
                                    int patch_radius,
                                    int floats_width,
                                    int floats_height,
                                    int floats_depth,
                                    float layer0[],
                                    bp * net,
                                    bp * feature_autocoder)
{
    int no_of_learned_features = feature_autocoder->NoOfHiddens;

    if (samples_across * samples_down * no_of_learned_features !=
        net->NoOfInputs) {
        /* across*down doesn't equal the second layer units */
        return -1;
    }

    if (feature_autocoder->NoOfInputs !=
        patch_radius*patch_radius*4*floats_depth) {
        /* the patch size doesn't match the feature
           learner inputs */
        return -2;
    }

    if (feature_autocoder->NoOfInputs !=
        feature_autocoder->NoOfOutputs) {
        /* same number of inputs and outputs */
        return -3;
    }

    for (int fy = 0; fy < samples_down; fy++) {
        for (int fx = 0; fx < samples_across; fx++) {
            int tx=0, ty=0, bx=0, by=0;
            if (features_patch_coords(fx, fy,
                                      samples_across, samples_down,
                                      patch_radius,
                                      floats_width, floats_height,
                                      &tx, &ty, &bx, &by) != 0) {
                continue;
            }

            if (scan_floats_patch(layer0,
                                  floats_width, floats_depth,
                                  tx, ty, bx, by,
                                  feature_autocoder) != 0) {
                return -4;
            }

            bp_feed_forward_layers(feature_autocoder, 0);
            int index_net_inputs =
                (fy * samples_across + fx) *
                no_of_learned_features;
            for (int f = 0; f < no_of_learned_features; f++) {
                bp_set_input(net, index_net_inputs+f,
                             bp_get_hidden(feature_autocoder, 0, f));
            }
        }
    }
    return 0;
}

/**
 * @brief Plots the learned feature vectors within an image
 * @param net The feature learning neural net
 * @param filename The filename to save as
 * @param input_img_width The depth of the input image used
 *        during training
 * @param img_width Width of the image to be saved
 * @param img_height Height of the image to be saved
 * @return zero on success
 */
int features_plot_weights(bp * net,
                          char * filename,
                          int input_img_depth,
                          int img_width, int img_height)
{
    int no_of_features = net->NoOfHiddens;
    int features_across = (int)sqrt(no_of_features);
    int features_down = no_of_features / features_across;
    int patch_diameter = (int)sqrt(net->NoOfInputs/input_img_depth);
    unsigned char * img;
    float * min_weight, * max_weight, * weight_range;
    const int layer_index = 0;
    const int test_pattern = 0;

    /* allocate memory for the image */
    img = (unsigned char*)malloc(img_width*img_height*3);
    if (!img) {
        return -1;
    }

    /* clear the img with a white background */
    memset((void*)img,'\255',img_width*img_height*3);

    if (test_pattern != 0) {
        for (int i = 0; i < net->NoOfHiddens; i++) {
            bp_weights_test_pattern(net->hiddens[layer_index][i],
                                    input_img_depth);
        }
    }

    /* get the min and max weight values, so that we can
       scale accordingly */
    min_weight = (float*)malloc(no_of_features*sizeof(float));
    if (!min_weight) return -2;
    memset((void*)min_weight,'\0',no_of_features*sizeof(float));
    max_weight = (float*)malloc(no_of_features*sizeof(float));
    if (!max_weight) return -2;
    memset((void*)max_weight,'\0',no_of_features*sizeof(float));
    weight_range = (float*)malloc(no_of_features*sizeof(float));
    if (!weight_range) return -3;
    memset((void*)weight_range,'\0',no_of_features*sizeof(float));
    for (int i = 0; i < no_of_features; i++) {
        bp_neuron * nrn = net->hiddens[layer_index][i];
        min_weight[i] = nrn->weights[0];
        max_weight[i] = nrn->weights[0];
        for (int j = 1; j < net->NoOfInputs; j++) {
            if (nrn->weights[j] < min_weight[i]) {
                min_weight[i] = nrn->weights[j];
            }
            if (nrn->weights[j] > max_weight[i]) {
                max_weight[i] = nrn->weights[j];
            }
        }
        weight_range[i] = max_weight[i] - min_weight[i];
    }

    for (int y = 0; y < img_height; y++) {
        int fy = y * features_down / img_height;
        /* array y position within the patch */
        int py =
            (int)((((float)y * features_down / (float)img_height) - fy)*
                  patch_diameter);
        for (int x = 0; x < img_width; x++) {
            int fx = x * features_across / img_width;
            /* array x position within the patch */
            int px =
                (int)((((float)x * features_across /
                        (float)img_width) - fx) *
                      patch_diameter);
            /* position within the patch */
            int pn = (py*patch_diameter + px)*input_img_depth;
            /* array index within the output image */
            int in = (y*img_width + x)*3;
            /* array index of the feature (hidden unit) */
            int hidden_index = fy*features_across + fx;
            if (hidden_index < net->NoOfHiddens) {
                if (max_weight[hidden_index] > min_weight[hidden_index]) {
                    bp_neuron * n = net->hiddens[layer_index][hidden_index];

                    if (input_img_depth == 3) {
                        for (int c = 0; c < 3; c++) {
                            /* convert weight to an unsigned char */
                            img[in++] =
                                (unsigned char)((n->weights[pn+c] -
                                                 min_weight[hidden_index]) * 255 /
                                                weight_range[hidden_index]);
                        }
                    }

                    if (input_img_depth == 1) {
                        /* convert weight to an unsigned char */
                        img[in] =
                            (unsigned char)((n->weights[pn] -
                                             min_weight[hidden_index]) * 255 /
                                            weight_range[hidden_index]);
                        img[in+1] = img[in];
                        img[in+2] = img[in];
                    }

                }
            }
        }
    }

    deeplearn_write_png_file(filename,
                             (unsigned int)img_width,
                             (unsigned int)img_height,
                             24, img);

    /* deallocate memory */
    free(img);
    free(min_weight);
    free(max_weight);
    free(weight_range);
    return 0;
}
