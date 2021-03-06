/*
 libdeep - a library for deep learning
 Copyright (C) 2015  Bob Mottram <bob@robotics.uk.to>

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

#ifndef DEEPLEARN_CONVNET_H
#define DEEPLEARN_CONVNET_H

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <omp.h>
#include "globals.h"
#include "deeplearn_random.h"
#include "backprop_neuron.h"
#include "encoding.h"
#include "backprop.h"
#include "deeplearn.h"
#include "deeplearn_features.h"
#include "deeplearn_pooling.h"
#include "deeplearn_conv.h"

typedef struct {
    deeplearn_conv *convolution;
    deeplearn * learner;

    deeplearndata * data;
    int data_samples;
    deeplearndata_meta * training_data;
    int training_data_samples;
    deeplearndata_meta * training_data_labeled;
    int training_data_labeled_samples;
    deeplearndata_meta * test_data;
    int test_data_samples;

    float * input_range_min;
    float * input_range_max;
    float * output_range_min;
    float * output_range_max;

    unsigned int training_ctr;
    unsigned int history_plot_interval;
    char history_plot_filename[256];
    char history_plot_title[256];

    float history[DEEPLEARN_HISTORY_SIZE];
    int history_index, history_ctr, history_step;
} deepconvnet;

int deepconvnet_init(int no_of_convolutions,
                     int no_of_deep_layers,
                     int inputs_across,
                     int inputs_down,
                     int inputs_depth,
                     int max_features,
                     int reduction_factor,
                     int no_of_outputs,
                     deepconvnet * convnet,
                     float error_threshold[],
                     unsigned int * random_seed);
void deepconvnet_free(deepconvnet * convnet);
int deepconvnet_save(FILE * fp, deepconvnet * convnet);
int deepconvnet_load(FILE * fp, deepconvnet * convnet,
                     unsigned int * random_seed);
int deepconvnet_update_img(deepconvnet * convnet, unsigned char img[]);

#endif
