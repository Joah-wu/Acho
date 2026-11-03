/**
  ******************************************************************************
  * @file    acho_detect.c
  * @author  AST Embedded Analytics Research Platform
  * @date    2026-10-31T19:22:31+0800
  * @brief   AI Tool Automatic Code Generator for Embedded NN computing
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  ******************************************************************************
  */


#include "acho_detect.h"
#include "acho_detect_data.h"

#include "ai_platform.h"
#include "ai_platform_interface.h"
#include "ai_math_helpers.h"

#include "core_common.h"
#include "core_convert.h"

#include "layers.h"



#undef AI_NET_OBJ_INSTANCE
#define AI_NET_OBJ_INSTANCE g_acho_detect
 
#undef AI_ACHO_DETECT_MODEL_SIGNATURE
#define AI_ACHO_DETECT_MODEL_SIGNATURE     "0xb8d5c3a2a324abe92fcdebb7a5c91cbb"

#ifndef AI_TOOLS_REVISION_ID
#define AI_TOOLS_REVISION_ID     ""
#endif

#undef AI_TOOLS_DATE_TIME
#define AI_TOOLS_DATE_TIME   "2026-10-31T19:22:31+0800"

#undef AI_TOOLS_COMPILE_TIME
#define AI_TOOLS_COMPILE_TIME    __DATE__ " " __TIME__

#undef AI_ACHO_DETECT_N_BATCHES
#define AI_ACHO_DETECT_N_BATCHES         (1)

static ai_ptr g_acho_detect_activations_map[1] = AI_C_ARRAY_INIT;
static ai_ptr g_acho_detect_weights_map[1] = AI_C_ARRAY_INIT;



/**  Array declarations section  **********************************************/
/* Array#0 */
AI_ARRAY_OBJ_DECLARE(
  mfcc_output_array, AI_ARRAY_FORMAT_FLOAT|AI_FMT_FLAG_IS_IO,
  NULL, NULL, 1287, AI_STATIC)

/* Array#1 */
AI_ARRAY_OBJ_DECLARE(
  _model_features_features_0_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 528, AI_STATIC)

/* Array#2 */
AI_ARRAY_OBJ_DECLARE(
  _model_features_features_3_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 88, AI_STATIC)

/* Array#3 */
AI_ARRAY_OBJ_DECLARE(
  _model_features_features_6_Slice_3_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 16, AI_STATIC)

/* Array#4 */
AI_ARRAY_OBJ_DECLARE(
  _model_features_features_6_ReduceMean_3_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 8, AI_STATIC)

/* Array#5 */
AI_ARRAY_OBJ_DECLARE(
  _model_features_features_6_ReduceMean_3_output_0_Mul_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 8, AI_STATIC)

/* Array#6 */
AI_ARRAY_OBJ_DECLARE(
  _model_features_features_6_Slice_2_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 24, AI_STATIC)

/* Array#7 */
AI_ARRAY_OBJ_DECLARE(
  _model_features_features_6_ReduceMean_2_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 8, AI_STATIC)

/* Array#8 */
AI_ARRAY_OBJ_DECLARE(
  _model_features_features_6_ReduceMean_2_output_0_Mul_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 8, AI_STATIC)

/* Array#9 */
AI_ARRAY_OBJ_DECLARE(
  _model_features_features_6_Slice_1_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 24, AI_STATIC)

/* Array#10 */
AI_ARRAY_OBJ_DECLARE(
  _model_features_features_6_ReduceMean_1_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 8, AI_STATIC)

/* Array#11 */
AI_ARRAY_OBJ_DECLARE(
  _model_features_features_6_ReduceMean_1_output_0_Mul_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 8, AI_STATIC)

/* Array#12 */
AI_ARRAY_OBJ_DECLARE(
  _model_features_features_6_Slice_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 24, AI_STATIC)

/* Array#13 */
AI_ARRAY_OBJ_DECLARE(
  _model_features_features_6_ReduceMean_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 8, AI_STATIC)

/* Array#14 */
AI_ARRAY_OBJ_DECLARE(
  _model_features_features_6_ReduceMean_output_0_Mul_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 8, AI_STATIC)

/* Array#15 */
AI_ARRAY_OBJ_DECLARE(
  _model_features_features_6_Concat_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 32, AI_STATIC)

/* Array#16 */
AI_ARRAY_OBJ_DECLARE(
  _model_classifier_classifier_0_Flatten_output_0_to_chlast_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 32, AI_STATIC)

/* Array#17 */
AI_ARRAY_OBJ_DECLARE(
  _model_classifier_classifier_1_Gemm_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 64, AI_STATIC)

/* Array#18 */
AI_ARRAY_OBJ_DECLARE(
  _model_classifier_classifier_2_Relu_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 64, AI_STATIC)

/* Array#19 */
AI_ARRAY_OBJ_DECLARE(
  _model_classifier_classifier_3_Gemm_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 32, AI_STATIC)

/* Array#20 */
AI_ARRAY_OBJ_DECLARE(
  _model_classifier_classifier_4_Relu_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 32, AI_STATIC)

/* Array#21 */
AI_ARRAY_OBJ_DECLARE(
  _model_classifier_classifier_5_Gemm_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 1, AI_STATIC)

/* Array#22 */
AI_ARRAY_OBJ_DECLARE(
  _Sigmoid_output_0_output_array, AI_ARRAY_FORMAT_FLOAT|AI_FMT_FLAG_IS_IO,
  NULL, NULL, 1, AI_STATIC)

/* Array#23 */
AI_ARRAY_OBJ_DECLARE(
  _model_features_features_0_Conv_output_0_weights_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 36, AI_STATIC)

/* Array#24 */
AI_ARRAY_OBJ_DECLARE(
  _model_features_features_0_Conv_output_0_bias_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4, AI_STATIC)

/* Array#25 */
AI_ARRAY_OBJ_DECLARE(
  _model_features_features_3_Conv_output_0_weights_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 288, AI_STATIC)

/* Array#26 */
AI_ARRAY_OBJ_DECLARE(
  _model_features_features_3_Conv_output_0_bias_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 8, AI_STATIC)

/* Array#27 */
AI_ARRAY_OBJ_DECLARE(
  _model_features_features_6_ReduceMean_3_output_0_Mul_scale_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 8, AI_STATIC)

/* Array#28 */
AI_ARRAY_OBJ_DECLARE(
  _model_features_features_6_ReduceMean_3_output_0_Mul_bias_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 8, AI_STATIC)

/* Array#29 */
AI_ARRAY_OBJ_DECLARE(
  _model_features_features_6_ReduceMean_2_output_0_Mul_scale_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 8, AI_STATIC)

/* Array#30 */
AI_ARRAY_OBJ_DECLARE(
  _model_classifier_classifier_1_Gemm_output_0_weights_array, AI_ARRAY_FORMAT_LUT4_FLOAT,
  NULL, NULL, 2048, AI_STATIC)

/* Array#31 */
AI_ARRAY_OBJ_DECLARE(
  _model_classifier_classifier_1_Gemm_output_0_bias_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 64, AI_STATIC)

/* Array#32 */
AI_ARRAY_OBJ_DECLARE(
  _model_classifier_classifier_3_Gemm_output_0_weights_array, AI_ARRAY_FORMAT_LUT4_FLOAT,
  NULL, NULL, 2048, AI_STATIC)

/* Array#33 */
AI_ARRAY_OBJ_DECLARE(
  _model_classifier_classifier_3_Gemm_output_0_bias_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 32, AI_STATIC)

/* Array#34 */
AI_ARRAY_OBJ_DECLARE(
  _model_classifier_classifier_5_Gemm_output_0_weights_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 32, AI_STATIC)

/* Array#35 */
AI_ARRAY_OBJ_DECLARE(
  _model_classifier_classifier_5_Gemm_output_0_bias_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 1, AI_STATIC)

/* Array#36 */
AI_ARRAY_OBJ_DECLARE(
  _model_features_features_0_Conv_output_0_scratch0_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 9, AI_STATIC)

/* Array#37 */
AI_ARRAY_OBJ_DECLARE(
  _model_features_features_0_Conv_output_0_scratch1_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 792, AI_STATIC)

/* Array#38 */
AI_ARRAY_OBJ_DECLARE(
  _model_features_features_3_Conv_output_0_scratch0_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 36, AI_STATIC)

/* Array#39 */
AI_ARRAY_OBJ_DECLARE(
  _model_features_features_3_Conv_output_0_scratch1_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 528, AI_STATIC)

/**  Tensor declarations section  *********************************************/
/* Tensor #0 */
AI_TENSOR_OBJ_DECLARE(
  _Sigmoid_output_0_output, AI_STATIC,
  0, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 1, 1), AI_STRIDE_INIT(4, 4, 4, 4, 4),
  1, &_Sigmoid_output_0_output_array, NULL)

/* Tensor #1 */
AI_TENSOR_OBJ_DECLARE(
  _model_classifier_classifier_0_Flatten_output_0_to_chlast_output, AI_STATIC,
  1, 0x0,
  AI_SHAPE_INIT(4, 1, 4, 1, 8), AI_STRIDE_INIT(4, 4, 4, 16, 16),
  1, &_model_classifier_classifier_0_Flatten_output_0_to_chlast_output_array, NULL)

/* Tensor #2 */
AI_TENSOR_OBJ_DECLARE(
  _model_classifier_classifier_0_Flatten_output_0_to_chlast_output0, AI_STATIC,
  2, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 1, 1), AI_STRIDE_INIT(4, 4, 4, 128, 128),
  1, &_model_classifier_classifier_0_Flatten_output_0_to_chlast_output_array, NULL)

/* Tensor #3 */
AI_TENSOR_OBJ_DECLARE(
  _model_classifier_classifier_1_Gemm_output_0_bias, AI_STATIC,
  3, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 1, 1), AI_STRIDE_INIT(4, 4, 4, 256, 256),
  1, &_model_classifier_classifier_1_Gemm_output_0_bias_array, NULL)

/* Tensor #4 */
AI_TENSOR_OBJ_DECLARE(
  _model_classifier_classifier_1_Gemm_output_0_output, AI_STATIC,
  4, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 1, 1), AI_STRIDE_INIT(4, 4, 4, 256, 256),
  1, &_model_classifier_classifier_1_Gemm_output_0_output_array, NULL)

/* Tensor #5 */
AI_TENSOR_OBJ_DECLARE(
  _model_classifier_classifier_1_Gemm_output_0_weights, AI_STATIC,
  5, 0x0,
  AI_SHAPE_INIT(4, 32, 64, 1, 1), AI_STRIDE_INIT(4, 1, 16, 1024, 1024),
  1, &_model_classifier_classifier_1_Gemm_output_0_weights_array, NULL)

/* Tensor #6 */
AI_TENSOR_OBJ_DECLARE(
  _model_classifier_classifier_2_Relu_output_0_output, AI_STATIC,
  6, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 1, 1), AI_STRIDE_INIT(4, 4, 4, 256, 256),
  1, &_model_classifier_classifier_2_Relu_output_0_output_array, NULL)

/* Tensor #7 */
AI_TENSOR_OBJ_DECLARE(
  _model_classifier_classifier_3_Gemm_output_0_bias, AI_STATIC,
  7, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 1, 1), AI_STRIDE_INIT(4, 4, 4, 128, 128),
  1, &_model_classifier_classifier_3_Gemm_output_0_bias_array, NULL)

/* Tensor #8 */
AI_TENSOR_OBJ_DECLARE(
  _model_classifier_classifier_3_Gemm_output_0_output, AI_STATIC,
  8, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 1, 1), AI_STRIDE_INIT(4, 4, 4, 128, 128),
  1, &_model_classifier_classifier_3_Gemm_output_0_output_array, NULL)

/* Tensor #9 */
AI_TENSOR_OBJ_DECLARE(
  _model_classifier_classifier_3_Gemm_output_0_weights, AI_STATIC,
  9, 0x0,
  AI_SHAPE_INIT(4, 64, 32, 1, 1), AI_STRIDE_INIT(4, 1, 32, 1024, 1024),
  1, &_model_classifier_classifier_3_Gemm_output_0_weights_array, NULL)

/* Tensor #10 */
AI_TENSOR_OBJ_DECLARE(
  _model_classifier_classifier_4_Relu_output_0_output, AI_STATIC,
  10, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 1, 1), AI_STRIDE_INIT(4, 4, 4, 128, 128),
  1, &_model_classifier_classifier_4_Relu_output_0_output_array, NULL)

/* Tensor #11 */
AI_TENSOR_OBJ_DECLARE(
  _model_classifier_classifier_5_Gemm_output_0_bias, AI_STATIC,
  11, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 1, 1), AI_STRIDE_INIT(4, 4, 4, 4, 4),
  1, &_model_classifier_classifier_5_Gemm_output_0_bias_array, NULL)

/* Tensor #12 */
AI_TENSOR_OBJ_DECLARE(
  _model_classifier_classifier_5_Gemm_output_0_output, AI_STATIC,
  12, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 1, 1), AI_STRIDE_INIT(4, 4, 4, 4, 4),
  1, &_model_classifier_classifier_5_Gemm_output_0_output_array, NULL)

/* Tensor #13 */
AI_TENSOR_OBJ_DECLARE(
  _model_classifier_classifier_5_Gemm_output_0_weights, AI_STATIC,
  13, 0x0,
  AI_SHAPE_INIT(4, 32, 1, 1, 1), AI_STRIDE_INIT(4, 4, 128, 128, 128),
  1, &_model_classifier_classifier_5_Gemm_output_0_weights_array, NULL)

/* Tensor #14 */
AI_TENSOR_OBJ_DECLARE(
  _model_features_features_0_Conv_output_0_bias, AI_STATIC,
  14, 0x0,
  AI_SHAPE_INIT(4, 1, 4, 1, 1), AI_STRIDE_INIT(4, 4, 4, 16, 16),
  1, &_model_features_features_0_Conv_output_0_bias_array, NULL)

/* Tensor #15 */
AI_TENSOR_OBJ_DECLARE(
  _model_features_features_0_Conv_output_0_output, AI_STATIC,
  15, 0x0,
  AI_SHAPE_INIT(4, 1, 4, 33, 4), AI_STRIDE_INIT(4, 4, 4, 16, 528),
  1, &_model_features_features_0_Conv_output_0_output_array, NULL)

/* Tensor #16 */
AI_TENSOR_OBJ_DECLARE(
  _model_features_features_0_Conv_output_0_scratch0, AI_STATIC,
  16, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 3, 3), AI_STRIDE_INIT(4, 4, 4, 4, 12),
  1, &_model_features_features_0_Conv_output_0_scratch0_array, NULL)

/* Tensor #17 */
AI_TENSOR_OBJ_DECLARE(
  _model_features_features_0_Conv_output_0_scratch1, AI_STATIC,
  17, 0x0,
  AI_SHAPE_INIT(4, 1, 4, 99, 2), AI_STRIDE_INIT(4, 4, 4, 16, 1584),
  1, &_model_features_features_0_Conv_output_0_scratch1_array, NULL)

/* Tensor #18 */
AI_TENSOR_OBJ_DECLARE(
  _model_features_features_0_Conv_output_0_weights, AI_STATIC,
  18, 0x0,
  AI_SHAPE_INIT(4, 1, 3, 3, 4), AI_STRIDE_INIT(4, 4, 4, 16, 48),
  1, &_model_features_features_0_Conv_output_0_weights_array, NULL)

/* Tensor #19 */
AI_TENSOR_OBJ_DECLARE(
  _model_features_features_3_Conv_output_0_bias, AI_STATIC,
  19, 0x0,
  AI_SHAPE_INIT(4, 1, 8, 1, 1), AI_STRIDE_INIT(4, 4, 4, 32, 32),
  1, &_model_features_features_3_Conv_output_0_bias_array, NULL)

/* Tensor #20 */
AI_TENSOR_OBJ_DECLARE(
  _model_features_features_3_Conv_output_0_output, AI_STATIC,
  20, 0x0,
  AI_SHAPE_INIT(4, 1, 8, 11, 1), AI_STRIDE_INIT(4, 4, 4, 32, 352),
  1, &_model_features_features_3_Conv_output_0_output_array, NULL)

/* Tensor #21 */
AI_TENSOR_OBJ_DECLARE(
  _model_features_features_3_Conv_output_0_scratch0, AI_STATIC,
  21, 0x0,
  AI_SHAPE_INIT(4, 1, 4, 3, 3), AI_STRIDE_INIT(4, 4, 4, 16, 48),
  1, &_model_features_features_3_Conv_output_0_scratch0_array, NULL)

/* Tensor #22 */
AI_TENSOR_OBJ_DECLARE(
  _model_features_features_3_Conv_output_0_scratch1, AI_STATIC,
  22, 0x0,
  AI_SHAPE_INIT(4, 1, 8, 33, 2), AI_STRIDE_INIT(4, 4, 4, 32, 1056),
  1, &_model_features_features_3_Conv_output_0_scratch1_array, NULL)

/* Tensor #23 */
AI_TENSOR_OBJ_DECLARE(
  _model_features_features_3_Conv_output_0_weights, AI_STATIC,
  23, 0x0,
  AI_SHAPE_INIT(4, 4, 3, 3, 8), AI_STRIDE_INIT(4, 4, 16, 128, 384),
  1, &_model_features_features_3_Conv_output_0_weights_array, NULL)

/* Tensor #24 */
AI_TENSOR_OBJ_DECLARE(
  _model_features_features_6_Concat_output_0_output, AI_STATIC,
  24, 0x0,
  AI_SHAPE_INIT(4, 1, 8, 4, 1), AI_STRIDE_INIT(4, 4, 4, 32, 128),
  1, &_model_features_features_6_Concat_output_0_output_array, NULL)

/* Tensor #25 */
AI_TENSOR_OBJ_DECLARE(
  _model_features_features_6_ReduceMean_1_output_0_Mul_output, AI_STATIC,
  25, 0x0,
  AI_SHAPE_INIT(4, 1, 8, 1, 1), AI_STRIDE_INIT(4, 4, 4, 32, 32),
  1, &_model_features_features_6_ReduceMean_1_output_0_Mul_output_array, NULL)

/* Tensor #26 */
AI_TENSOR_OBJ_DECLARE(
  _model_features_features_6_ReduceMean_1_output_0_output, AI_STATIC,
  26, 0x0,
  AI_SHAPE_INIT(4, 1, 8, 1, 1), AI_STRIDE_INIT(4, 4, 4, 32, 32),
  1, &_model_features_features_6_ReduceMean_1_output_0_output_array, NULL)

/* Tensor #27 */
AI_TENSOR_OBJ_DECLARE(
  _model_features_features_6_ReduceMean_2_output_0_Mul_output, AI_STATIC,
  27, 0x0,
  AI_SHAPE_INIT(4, 1, 8, 1, 1), AI_STRIDE_INIT(4, 4, 4, 32, 32),
  1, &_model_features_features_6_ReduceMean_2_output_0_Mul_output_array, NULL)

/* Tensor #28 */
AI_TENSOR_OBJ_DECLARE(
  _model_features_features_6_ReduceMean_2_output_0_Mul_scale, AI_STATIC,
  28, 0x0,
  AI_SHAPE_INIT(4, 1, 8, 1, 1), AI_STRIDE_INIT(4, 4, 4, 32, 32),
  1, &_model_features_features_6_ReduceMean_2_output_0_Mul_scale_array, NULL)

/* Tensor #29 */
AI_TENSOR_OBJ_DECLARE(
  _model_features_features_6_ReduceMean_2_output_0_output, AI_STATIC,
  29, 0x0,
  AI_SHAPE_INIT(4, 1, 8, 1, 1), AI_STRIDE_INIT(4, 4, 4, 32, 32),
  1, &_model_features_features_6_ReduceMean_2_output_0_output_array, NULL)

/* Tensor #30 */
AI_TENSOR_OBJ_DECLARE(
  _model_features_features_6_ReduceMean_3_output_0_Mul_bias, AI_STATIC,
  30, 0x0,
  AI_SHAPE_INIT(4, 1, 8, 1, 1), AI_STRIDE_INIT(4, 4, 4, 32, 32),
  1, &_model_features_features_6_ReduceMean_3_output_0_Mul_bias_array, NULL)

/* Tensor #31 */
AI_TENSOR_OBJ_DECLARE(
  _model_features_features_6_ReduceMean_3_output_0_Mul_output, AI_STATIC,
  31, 0x0,
  AI_SHAPE_INIT(4, 1, 8, 1, 1), AI_STRIDE_INIT(4, 4, 4, 32, 32),
  1, &_model_features_features_6_ReduceMean_3_output_0_Mul_output_array, NULL)

/* Tensor #32 */
AI_TENSOR_OBJ_DECLARE(
  _model_features_features_6_ReduceMean_3_output_0_Mul_scale, AI_STATIC,
  32, 0x0,
  AI_SHAPE_INIT(4, 1, 8, 1, 1), AI_STRIDE_INIT(4, 4, 4, 32, 32),
  1, &_model_features_features_6_ReduceMean_3_output_0_Mul_scale_array, NULL)

/* Tensor #33 */
AI_TENSOR_OBJ_DECLARE(
  _model_features_features_6_ReduceMean_3_output_0_output, AI_STATIC,
  33, 0x0,
  AI_SHAPE_INIT(4, 1, 8, 1, 1), AI_STRIDE_INIT(4, 4, 4, 32, 32),
  1, &_model_features_features_6_ReduceMean_3_output_0_output_array, NULL)

/* Tensor #34 */
AI_TENSOR_OBJ_DECLARE(
  _model_features_features_6_ReduceMean_output_0_Mul_output, AI_STATIC,
  34, 0x0,
  AI_SHAPE_INIT(4, 1, 8, 1, 1), AI_STRIDE_INIT(4, 4, 4, 32, 32),
  1, &_model_features_features_6_ReduceMean_output_0_Mul_output_array, NULL)

/* Tensor #35 */
AI_TENSOR_OBJ_DECLARE(
  _model_features_features_6_ReduceMean_output_0_output, AI_STATIC,
  35, 0x0,
  AI_SHAPE_INIT(4, 1, 8, 1, 1), AI_STRIDE_INIT(4, 4, 4, 32, 32),
  1, &_model_features_features_6_ReduceMean_output_0_output_array, NULL)

/* Tensor #36 */
AI_TENSOR_OBJ_DECLARE(
  _model_features_features_6_Slice_1_output_0_output, AI_STATIC,
  36, 0x0,
  AI_SHAPE_INIT(4, 1, 8, 3, 1), AI_STRIDE_INIT(4, 4, 4, 32, 96),
  1, &_model_features_features_6_Slice_1_output_0_output_array, NULL)

/* Tensor #37 */
AI_TENSOR_OBJ_DECLARE(
  _model_features_features_6_Slice_2_output_0_output, AI_STATIC,
  37, 0x0,
  AI_SHAPE_INIT(4, 1, 8, 3, 1), AI_STRIDE_INIT(4, 4, 4, 32, 96),
  1, &_model_features_features_6_Slice_2_output_0_output_array, NULL)

/* Tensor #38 */
AI_TENSOR_OBJ_DECLARE(
  _model_features_features_6_Slice_3_output_0_output, AI_STATIC,
  38, 0x0,
  AI_SHAPE_INIT(4, 1, 8, 2, 1), AI_STRIDE_INIT(4, 4, 4, 32, 64),
  1, &_model_features_features_6_Slice_3_output_0_output_array, NULL)

/* Tensor #39 */
AI_TENSOR_OBJ_DECLARE(
  _model_features_features_6_Slice_output_0_output, AI_STATIC,
  39, 0x0,
  AI_SHAPE_INIT(4, 1, 8, 3, 1), AI_STRIDE_INIT(4, 4, 4, 32, 96),
  1, &_model_features_features_6_Slice_output_0_output_array, NULL)

/* Tensor #40 */
AI_TENSOR_OBJ_DECLARE(
  mfcc_output, AI_STATIC,
  40, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 99, 13), AI_STRIDE_INIT(4, 4, 4, 4, 396),
  1, &mfcc_output_array, NULL)



/**  Layer declarations section  **********************************************/


AI_TENSOR_CHAIN_OBJ_DECLARE(
  _Sigmoid_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_classifier_classifier_5_Gemm_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_Sigmoid_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _Sigmoid_output_0_layer, 40,
  NL_TYPE, 0x0, NULL,
  nl, forward_sigmoid,
  &_Sigmoid_output_0_chain,
  NULL, &_Sigmoid_output_0_layer, AI_STATIC, 
  .nl_params = NULL, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_classifier_classifier_5_Gemm_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_classifier_classifier_4_Relu_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_classifier_classifier_5_Gemm_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_classifier_classifier_5_Gemm_output_0_weights, &_model_classifier_classifier_5_Gemm_output_0_bias),
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_classifier_classifier_5_Gemm_output_0_layer, 37,
  DENSE_TYPE, 0x0, NULL,
  dense, forward_dense,
  &_model_classifier_classifier_5_Gemm_output_0_chain,
  NULL, &_Sigmoid_output_0_layer, AI_STATIC, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_classifier_classifier_4_Relu_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_classifier_classifier_3_Gemm_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_classifier_classifier_4_Relu_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_classifier_classifier_4_Relu_output_0_layer, 36,
  NL_TYPE, 0x0, NULL,
  nl, forward_relu,
  &_model_classifier_classifier_4_Relu_output_0_chain,
  NULL, &_model_classifier_classifier_5_Gemm_output_0_layer, AI_STATIC, 
  .nl_params = NULL, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_classifier_classifier_3_Gemm_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_classifier_classifier_2_Relu_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_classifier_classifier_3_Gemm_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_classifier_classifier_3_Gemm_output_0_weights, &_model_classifier_classifier_3_Gemm_output_0_bias),
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_classifier_classifier_3_Gemm_output_0_layer, 35,
  DENSE_TYPE, 0x0, NULL,
  dense, forward_dense,
  &_model_classifier_classifier_3_Gemm_output_0_chain,
  NULL, &_model_classifier_classifier_4_Relu_output_0_layer, AI_STATIC, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_classifier_classifier_2_Relu_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_classifier_classifier_1_Gemm_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_classifier_classifier_2_Relu_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_classifier_classifier_2_Relu_output_0_layer, 34,
  NL_TYPE, 0x0, NULL,
  nl, forward_relu,
  &_model_classifier_classifier_2_Relu_output_0_chain,
  NULL, &_model_classifier_classifier_3_Gemm_output_0_layer, AI_STATIC, 
  .nl_params = NULL, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_classifier_classifier_1_Gemm_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_classifier_classifier_0_Flatten_output_0_to_chlast_output0),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_classifier_classifier_1_Gemm_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_classifier_classifier_1_Gemm_output_0_weights, &_model_classifier_classifier_1_Gemm_output_0_bias),
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_classifier_classifier_1_Gemm_output_0_layer, 33,
  DENSE_TYPE, 0x0, NULL,
  dense, forward_dense,
  &_model_classifier_classifier_1_Gemm_output_0_chain,
  NULL, &_model_classifier_classifier_2_Relu_output_0_layer, AI_STATIC, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_classifier_classifier_0_Flatten_output_0_to_chlast_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_features_features_6_Concat_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_classifier_classifier_0_Flatten_output_0_to_chlast_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_classifier_classifier_0_Flatten_output_0_to_chlast_layer, 32,
  TRANSPOSE_TYPE, 0x0, NULL,
  transpose, forward_transpose,
  &_model_classifier_classifier_0_Flatten_output_0_to_chlast_chain,
  NULL, &_model_classifier_classifier_1_Gemm_output_0_layer, AI_STATIC, 
  .out_mapping = AI_SHAPE_INIT(6, AI_SHAPE_IN_CHANNEL, AI_SHAPE_WIDTH, AI_SHAPE_HEIGHT, AI_SHAPE_CHANNEL, AI_SHAPE_DEPTH, AI_SHAPE_EXTENSION), 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_features_features_6_Concat_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 4, &_model_features_features_6_ReduceMean_output_0_Mul_output, &_model_features_features_6_ReduceMean_1_output_0_Mul_output, &_model_features_features_6_ReduceMean_2_output_0_Mul_output, &_model_features_features_6_ReduceMean_3_output_0_Mul_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_features_features_6_Concat_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_features_features_6_Concat_output_0_layer, 31,
  CONCAT_TYPE, 0x0, NULL,
  concat, forward_concat,
  &_model_features_features_6_Concat_output_0_chain,
  NULL, &_model_classifier_classifier_0_Flatten_output_0_to_chlast_layer, AI_STATIC, 
  .axis = AI_SHAPE_WIDTH, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_features_features_6_ReduceMean_output_0_Mul_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_features_features_6_ReduceMean_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_features_features_6_ReduceMean_output_0_Mul_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_features_features_6_ReduceMean_2_output_0_Mul_scale, &_model_features_features_6_ReduceMean_3_output_0_Mul_bias),
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_features_features_6_ReduceMean_output_0_Mul_layer, 27,
  BN_TYPE, 0x0, NULL,
  bn, forward_bn,
  &_model_features_features_6_ReduceMean_output_0_Mul_chain,
  NULL, &_model_features_features_6_Concat_output_0_layer, AI_STATIC, 
)


AI_STATIC_CONST ai_float _model_features_features_6_ReduceMean_output_0_neutral_value_data[] = { 0.0f };
AI_ARRAY_OBJ_DECLARE(
    _model_features_features_6_ReduceMean_output_0_neutral_value, AI_ARRAY_FORMAT_FLOAT,
    _model_features_features_6_ReduceMean_output_0_neutral_value_data, _model_features_features_6_ReduceMean_output_0_neutral_value_data, 1, AI_STATIC_CONST)
AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_features_features_6_ReduceMean_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_features_features_6_Slice_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_features_features_6_ReduceMean_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_features_features_6_ReduceMean_output_0_layer, 27,
  REDUCE_TYPE, 0x0, NULL,
  reduce, forward_reduce,
  &_model_features_features_6_ReduceMean_output_0_chain,
  NULL, &_model_features_features_6_ReduceMean_output_0_Mul_layer, AI_STATIC, 
  .operation = ai_sum, 
  .neutral_value = &_model_features_features_6_ReduceMean_output_0_neutral_value, 
)


AI_STATIC_CONST ai_u8 _model_features_features_6_Slice_output_0_axes_data[] = { 1 };
AI_ARRAY_OBJ_DECLARE(
    _model_features_features_6_Slice_output_0_axes, AI_ARRAY_FORMAT_U8,
    _model_features_features_6_Slice_output_0_axes_data, _model_features_features_6_Slice_output_0_axes_data, 1, AI_STATIC_CONST)

AI_STATIC_CONST ai_i16 _model_features_features_6_Slice_output_0_starts_data[] = { 0 };
AI_ARRAY_OBJ_DECLARE(
    _model_features_features_6_Slice_output_0_starts, AI_ARRAY_FORMAT_S16,
    _model_features_features_6_Slice_output_0_starts_data, _model_features_features_6_Slice_output_0_starts_data, 1, AI_STATIC_CONST)

AI_STATIC_CONST ai_i16 _model_features_features_6_Slice_output_0_ends_data[] = { 3 };
AI_ARRAY_OBJ_DECLARE(
    _model_features_features_6_Slice_output_0_ends, AI_ARRAY_FORMAT_S16,
    _model_features_features_6_Slice_output_0_ends_data, _model_features_features_6_Slice_output_0_ends_data, 1, AI_STATIC_CONST)
AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_features_features_6_Slice_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_features_features_3_Conv_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_features_features_6_Slice_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_features_features_6_Slice_output_0_layer, 11,
  SLICE_TYPE, 0x0, NULL,
  slice, forward_slice,
  &_model_features_features_6_Slice_output_0_chain,
  NULL, &_model_features_features_6_ReduceMean_output_0_layer, AI_STATIC, 
  .axes = &_model_features_features_6_Slice_output_0_axes, 
  .starts = &_model_features_features_6_Slice_output_0_starts, 
  .ends = &_model_features_features_6_Slice_output_0_ends, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_features_features_6_ReduceMean_1_output_0_Mul_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_features_features_6_ReduceMean_1_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_features_features_6_ReduceMean_1_output_0_Mul_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_features_features_6_ReduceMean_2_output_0_Mul_scale, &_model_features_features_6_ReduceMean_3_output_0_Mul_bias),
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_features_features_6_ReduceMean_1_output_0_Mul_layer, 28,
  BN_TYPE, 0x0, NULL,
  bn, forward_bn,
  &_model_features_features_6_ReduceMean_1_output_0_Mul_chain,
  NULL, &_model_features_features_6_Slice_output_0_layer, AI_STATIC, 
)


AI_STATIC_CONST ai_float _model_features_features_6_ReduceMean_1_output_0_neutral_value_data[] = { 0.0f };
AI_ARRAY_OBJ_DECLARE(
    _model_features_features_6_ReduceMean_1_output_0_neutral_value, AI_ARRAY_FORMAT_FLOAT,
    _model_features_features_6_ReduceMean_1_output_0_neutral_value_data, _model_features_features_6_ReduceMean_1_output_0_neutral_value_data, 1, AI_STATIC_CONST)
AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_features_features_6_ReduceMean_1_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_features_features_6_Slice_1_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_features_features_6_ReduceMean_1_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_features_features_6_ReduceMean_1_output_0_layer, 28,
  REDUCE_TYPE, 0x0, NULL,
  reduce, forward_reduce,
  &_model_features_features_6_ReduceMean_1_output_0_chain,
  NULL, &_model_features_features_6_ReduceMean_1_output_0_Mul_layer, AI_STATIC, 
  .operation = ai_sum, 
  .neutral_value = &_model_features_features_6_ReduceMean_1_output_0_neutral_value, 
)


AI_STATIC_CONST ai_u8 _model_features_features_6_Slice_1_output_0_axes_data[] = { 1 };
AI_ARRAY_OBJ_DECLARE(
    _model_features_features_6_Slice_1_output_0_axes, AI_ARRAY_FORMAT_U8,
    _model_features_features_6_Slice_1_output_0_axes_data, _model_features_features_6_Slice_1_output_0_axes_data, 1, AI_STATIC_CONST)

AI_STATIC_CONST ai_i16 _model_features_features_6_Slice_1_output_0_starts_data[] = { 3 };
AI_ARRAY_OBJ_DECLARE(
    _model_features_features_6_Slice_1_output_0_starts, AI_ARRAY_FORMAT_S16,
    _model_features_features_6_Slice_1_output_0_starts_data, _model_features_features_6_Slice_1_output_0_starts_data, 1, AI_STATIC_CONST)

AI_STATIC_CONST ai_i16 _model_features_features_6_Slice_1_output_0_ends_data[] = { 6 };
AI_ARRAY_OBJ_DECLARE(
    _model_features_features_6_Slice_1_output_0_ends, AI_ARRAY_FORMAT_S16,
    _model_features_features_6_Slice_1_output_0_ends_data, _model_features_features_6_Slice_1_output_0_ends_data, 1, AI_STATIC_CONST)
AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_features_features_6_Slice_1_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_features_features_3_Conv_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_features_features_6_Slice_1_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_features_features_6_Slice_1_output_0_layer, 16,
  SLICE_TYPE, 0x0, NULL,
  slice, forward_slice,
  &_model_features_features_6_Slice_1_output_0_chain,
  NULL, &_model_features_features_6_ReduceMean_1_output_0_layer, AI_STATIC, 
  .axes = &_model_features_features_6_Slice_1_output_0_axes, 
  .starts = &_model_features_features_6_Slice_1_output_0_starts, 
  .ends = &_model_features_features_6_Slice_1_output_0_ends, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_features_features_6_ReduceMean_2_output_0_Mul_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_features_features_6_ReduceMean_2_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_features_features_6_ReduceMean_2_output_0_Mul_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_features_features_6_ReduceMean_2_output_0_Mul_scale, &_model_features_features_6_ReduceMean_3_output_0_Mul_bias),
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_features_features_6_ReduceMean_2_output_0_Mul_layer, 29,
  BN_TYPE, 0x0, NULL,
  bn, forward_bn,
  &_model_features_features_6_ReduceMean_2_output_0_Mul_chain,
  NULL, &_model_features_features_6_Slice_1_output_0_layer, AI_STATIC, 
)


AI_STATIC_CONST ai_float _model_features_features_6_ReduceMean_2_output_0_neutral_value_data[] = { 0.0f };
AI_ARRAY_OBJ_DECLARE(
    _model_features_features_6_ReduceMean_2_output_0_neutral_value, AI_ARRAY_FORMAT_FLOAT,
    _model_features_features_6_ReduceMean_2_output_0_neutral_value_data, _model_features_features_6_ReduceMean_2_output_0_neutral_value_data, 1, AI_STATIC_CONST)
AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_features_features_6_ReduceMean_2_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_features_features_6_Slice_2_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_features_features_6_ReduceMean_2_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_features_features_6_ReduceMean_2_output_0_layer, 29,
  REDUCE_TYPE, 0x0, NULL,
  reduce, forward_reduce,
  &_model_features_features_6_ReduceMean_2_output_0_chain,
  NULL, &_model_features_features_6_ReduceMean_2_output_0_Mul_layer, AI_STATIC, 
  .operation = ai_sum, 
  .neutral_value = &_model_features_features_6_ReduceMean_2_output_0_neutral_value, 
)


AI_STATIC_CONST ai_u8 _model_features_features_6_Slice_2_output_0_axes_data[] = { 1 };
AI_ARRAY_OBJ_DECLARE(
    _model_features_features_6_Slice_2_output_0_axes, AI_ARRAY_FORMAT_U8,
    _model_features_features_6_Slice_2_output_0_axes_data, _model_features_features_6_Slice_2_output_0_axes_data, 1, AI_STATIC_CONST)

AI_STATIC_CONST ai_i16 _model_features_features_6_Slice_2_output_0_starts_data[] = { 6 };
AI_ARRAY_OBJ_DECLARE(
    _model_features_features_6_Slice_2_output_0_starts, AI_ARRAY_FORMAT_S16,
    _model_features_features_6_Slice_2_output_0_starts_data, _model_features_features_6_Slice_2_output_0_starts_data, 1, AI_STATIC_CONST)

AI_STATIC_CONST ai_i16 _model_features_features_6_Slice_2_output_0_ends_data[] = { 9 };
AI_ARRAY_OBJ_DECLARE(
    _model_features_features_6_Slice_2_output_0_ends, AI_ARRAY_FORMAT_S16,
    _model_features_features_6_Slice_2_output_0_ends_data, _model_features_features_6_Slice_2_output_0_ends_data, 1, AI_STATIC_CONST)
AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_features_features_6_Slice_2_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_features_features_3_Conv_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_features_features_6_Slice_2_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_features_features_6_Slice_2_output_0_layer, 21,
  SLICE_TYPE, 0x0, NULL,
  slice, forward_slice,
  &_model_features_features_6_Slice_2_output_0_chain,
  NULL, &_model_features_features_6_ReduceMean_2_output_0_layer, AI_STATIC, 
  .axes = &_model_features_features_6_Slice_2_output_0_axes, 
  .starts = &_model_features_features_6_Slice_2_output_0_starts, 
  .ends = &_model_features_features_6_Slice_2_output_0_ends, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_features_features_6_ReduceMean_3_output_0_Mul_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_features_features_6_ReduceMean_3_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_features_features_6_ReduceMean_3_output_0_Mul_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_features_features_6_ReduceMean_3_output_0_Mul_scale, &_model_features_features_6_ReduceMean_3_output_0_Mul_bias),
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_features_features_6_ReduceMean_3_output_0_Mul_layer, 30,
  BN_TYPE, 0x0, NULL,
  bn, forward_bn,
  &_model_features_features_6_ReduceMean_3_output_0_Mul_chain,
  NULL, &_model_features_features_6_Slice_2_output_0_layer, AI_STATIC, 
)


AI_STATIC_CONST ai_float _model_features_features_6_ReduceMean_3_output_0_neutral_value_data[] = { 0.0f };
AI_ARRAY_OBJ_DECLARE(
    _model_features_features_6_ReduceMean_3_output_0_neutral_value, AI_ARRAY_FORMAT_FLOAT,
    _model_features_features_6_ReduceMean_3_output_0_neutral_value_data, _model_features_features_6_ReduceMean_3_output_0_neutral_value_data, 1, AI_STATIC_CONST)
AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_features_features_6_ReduceMean_3_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_features_features_6_Slice_3_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_features_features_6_ReduceMean_3_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_features_features_6_ReduceMean_3_output_0_layer, 30,
  REDUCE_TYPE, 0x0, NULL,
  reduce, forward_reduce,
  &_model_features_features_6_ReduceMean_3_output_0_chain,
  NULL, &_model_features_features_6_ReduceMean_3_output_0_Mul_layer, AI_STATIC, 
  .operation = ai_sum, 
  .neutral_value = &_model_features_features_6_ReduceMean_3_output_0_neutral_value, 
)


AI_STATIC_CONST ai_u8 _model_features_features_6_Slice_3_output_0_axes_data[] = { 1 };
AI_ARRAY_OBJ_DECLARE(
    _model_features_features_6_Slice_3_output_0_axes, AI_ARRAY_FORMAT_U8,
    _model_features_features_6_Slice_3_output_0_axes_data, _model_features_features_6_Slice_3_output_0_axes_data, 1, AI_STATIC_CONST)

AI_STATIC_CONST ai_i16 _model_features_features_6_Slice_3_output_0_starts_data[] = { 9 };
AI_ARRAY_OBJ_DECLARE(
    _model_features_features_6_Slice_3_output_0_starts, AI_ARRAY_FORMAT_S16,
    _model_features_features_6_Slice_3_output_0_starts_data, _model_features_features_6_Slice_3_output_0_starts_data, 1, AI_STATIC_CONST)

AI_STATIC_CONST ai_i16 _model_features_features_6_Slice_3_output_0_ends_data[] = { 11 };
AI_ARRAY_OBJ_DECLARE(
    _model_features_features_6_Slice_3_output_0_ends, AI_ARRAY_FORMAT_S16,
    _model_features_features_6_Slice_3_output_0_ends_data, _model_features_features_6_Slice_3_output_0_ends_data, 1, AI_STATIC_CONST)
AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_features_features_6_Slice_3_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_features_features_3_Conv_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_features_features_6_Slice_3_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_features_features_6_Slice_3_output_0_layer, 26,
  SLICE_TYPE, 0x0, NULL,
  slice, forward_slice,
  &_model_features_features_6_Slice_3_output_0_chain,
  NULL, &_model_features_features_6_ReduceMean_3_output_0_layer, AI_STATIC, 
  .axes = &_model_features_features_6_Slice_3_output_0_axes, 
  .starts = &_model_features_features_6_Slice_3_output_0_starts, 
  .ends = &_model_features_features_6_Slice_3_output_0_ends, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_features_features_3_Conv_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_features_features_0_Conv_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_features_features_3_Conv_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 3, &_model_features_features_3_Conv_output_0_weights, &_model_features_features_3_Conv_output_0_bias, NULL),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_features_features_3_Conv_output_0_scratch0, &_model_features_features_3_Conv_output_0_scratch1)
)

AI_LAYER_OBJ_DECLARE(
  _model_features_features_3_Conv_output_0_layer, 6,
  OPTIMIZED_CONV2D_TYPE, 0x0, NULL,
  conv2d_nl_pool, forward_conv2d_if32of32wf32_nl_pool,
  &_model_features_features_3_Conv_output_0_chain,
  NULL, &_model_features_features_6_Slice_3_output_0_layer, AI_STATIC, 
  .groups = 1, 
  .nl_params = NULL, 
  .nl_func = AI_HANDLE_PTR(forward_lite_nl_relu_if32of32), 
  .filter_stride = AI_SHAPE_2D_INIT(1, 1), 
  .dilation = AI_SHAPE_2D_INIT(1, 1), 
  .filter_pad = AI_SHAPE_INIT(4, 1, 1, 1, 1), 
  .pool_size = AI_SHAPE_2D_INIT(2, 2), 
  .pool_stride = AI_SHAPE_2D_INIT(3, 3), 
  .pool_pad = AI_SHAPE_INIT(4, 0, 0, 0, 0), 
  .pool_func = AI_HANDLE_PTR(pool_func_mp_array_f32), 
  .in_ch_format = AI_LAYER_FORMAT_CHANNEL_LAST_SAME, 
  .out_ch_format = AI_LAYER_FORMAT_CHANNEL_LAST_VALID, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_features_features_0_Conv_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &mfcc_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_features_features_0_Conv_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 3, &_model_features_features_0_Conv_output_0_weights, &_model_features_features_0_Conv_output_0_bias, NULL),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_features_features_0_Conv_output_0_scratch0, &_model_features_features_0_Conv_output_0_scratch1)
)

AI_LAYER_OBJ_DECLARE(
  _model_features_features_0_Conv_output_0_layer, 3,
  OPTIMIZED_CONV2D_TYPE, 0x0, NULL,
  conv2d_nl_pool, forward_conv2d_if32of32wf32_nl_pool,
  &_model_features_features_0_Conv_output_0_chain,
  NULL, &_model_features_features_3_Conv_output_0_layer, AI_STATIC, 
  .groups = 1, 
  .nl_params = NULL, 
  .nl_func = AI_HANDLE_PTR(forward_lite_nl_relu_if32of32), 
  .filter_stride = AI_SHAPE_2D_INIT(1, 1), 
  .dilation = AI_SHAPE_2D_INIT(1, 1), 
  .filter_pad = AI_SHAPE_INIT(4, 1, 1, 1, 1), 
  .pool_size = AI_SHAPE_2D_INIT(2, 2), 
  .pool_stride = AI_SHAPE_2D_INIT(3, 3), 
  .pool_pad = AI_SHAPE_INIT(4, 0, 0, 0, 0), 
  .pool_func = AI_HANDLE_PTR(pool_func_mp_array_f32), 
  .in_ch_format = AI_LAYER_FORMAT_CHANNEL_LAST_SAME, 
  .out_ch_format = AI_LAYER_FORMAT_CHANNEL_LAST_VALID, 
)


#if (AI_TOOLS_API_VERSION < AI_TOOLS_API_VERSION_1_5)

AI_NETWORK_OBJ_DECLARE(
  AI_NET_OBJ_INSTANCE, AI_STATIC,
  AI_BUFFER_INIT(AI_FLAG_NONE,  AI_BUFFER_FORMAT_U8,
    AI_BUFFER_SHAPE_INIT(AI_SHAPE_BCWH, 4, 1, 4132, 1, 1),
    4132, NULL, NULL),
  AI_BUFFER_INIT(AI_FLAG_NONE,  AI_BUFFER_FORMAT_U8,
    AI_BUFFER_SHAPE_INIT(AI_SHAPE_BCWH, 4, 1, 9588, 1, 1),
    9588, NULL, NULL),
  AI_TENSOR_LIST_IO_OBJ_INIT(AI_FLAG_NONE, AI_ACHO_DETECT_IN_NUM, &mfcc_output),
  AI_TENSOR_LIST_IO_OBJ_INIT(AI_FLAG_NONE, AI_ACHO_DETECT_OUT_NUM, &_Sigmoid_output_0_output),
  &_model_features_features_0_Conv_output_0_layer, 0xa906173f, NULL)

#else

AI_NETWORK_OBJ_DECLARE(
  AI_NET_OBJ_INSTANCE, AI_STATIC,
  AI_BUFFER_ARRAY_OBJ_INIT_STATIC(
  	AI_FLAG_NONE, 1,
    AI_BUFFER_INIT(AI_FLAG_NONE,  AI_BUFFER_FORMAT_U8,
      AI_BUFFER_SHAPE_INIT(AI_SHAPE_BCWH, 4, 1, 4132, 1, 1),
      4132, NULL, NULL)
  ),
  AI_BUFFER_ARRAY_OBJ_INIT_STATIC(
  	AI_FLAG_NONE, 1,
    AI_BUFFER_INIT(AI_FLAG_NONE,  AI_BUFFER_FORMAT_U8,
      AI_BUFFER_SHAPE_INIT(AI_SHAPE_BCWH, 4, 1, 9588, 1, 1),
      9588, NULL, NULL)
  ),
  AI_TENSOR_LIST_IO_OBJ_INIT(AI_FLAG_NONE, AI_ACHO_DETECT_IN_NUM, &mfcc_output),
  AI_TENSOR_LIST_IO_OBJ_INIT(AI_FLAG_NONE, AI_ACHO_DETECT_OUT_NUM, &_Sigmoid_output_0_output),
  &_model_features_features_0_Conv_output_0_layer, 0xa906173f, NULL)

#endif	/*(AI_TOOLS_API_VERSION < AI_TOOLS_API_VERSION_1_5)*/



/******************************************************************************/
AI_DECLARE_STATIC
ai_bool acho_detect_configure_activations(
  ai_network* net_ctx, const ai_network_params* params)
{
  AI_ASSERT(net_ctx)

  if (ai_platform_get_activations_map(g_acho_detect_activations_map, 1, params)) {
    /* Updating activations (byte) offsets */
    
    mfcc_output_array.data = AI_PTR(g_acho_detect_activations_map[0] + 1236);
    mfcc_output_array.data_start = AI_PTR(g_acho_detect_activations_map[0] + 1236);
    _model_features_features_0_Conv_output_0_scratch0_array.data = AI_PTR(g_acho_detect_activations_map[0] + 6384);
    _model_features_features_0_Conv_output_0_scratch0_array.data_start = AI_PTR(g_acho_detect_activations_map[0] + 6384);
    _model_features_features_0_Conv_output_0_scratch1_array.data = AI_PTR(g_acho_detect_activations_map[0] + 6420);
    _model_features_features_0_Conv_output_0_scratch1_array.data_start = AI_PTR(g_acho_detect_activations_map[0] + 6420);
    _model_features_features_0_Conv_output_0_output_array.data = AI_PTR(g_acho_detect_activations_map[0] + 0);
    _model_features_features_0_Conv_output_0_output_array.data_start = AI_PTR(g_acho_detect_activations_map[0] + 0);
    _model_features_features_3_Conv_output_0_scratch0_array.data = AI_PTR(g_acho_detect_activations_map[0] + 2112);
    _model_features_features_3_Conv_output_0_scratch0_array.data_start = AI_PTR(g_acho_detect_activations_map[0] + 2112);
    _model_features_features_3_Conv_output_0_scratch1_array.data = AI_PTR(g_acho_detect_activations_map[0] + 2256);
    _model_features_features_3_Conv_output_0_scratch1_array.data_start = AI_PTR(g_acho_detect_activations_map[0] + 2256);
    _model_features_features_3_Conv_output_0_output_array.data = AI_PTR(g_acho_detect_activations_map[0] + 4368);
    _model_features_features_3_Conv_output_0_output_array.data_start = AI_PTR(g_acho_detect_activations_map[0] + 4368);
    _model_features_features_6_Slice_3_output_0_output_array.data = AI_PTR(g_acho_detect_activations_map[0] + 0);
    _model_features_features_6_Slice_3_output_0_output_array.data_start = AI_PTR(g_acho_detect_activations_map[0] + 0);
    _model_features_features_6_ReduceMean_3_output_0_output_array.data = AI_PTR(g_acho_detect_activations_map[0] + 64);
    _model_features_features_6_ReduceMean_3_output_0_output_array.data_start = AI_PTR(g_acho_detect_activations_map[0] + 64);
    _model_features_features_6_ReduceMean_3_output_0_Mul_output_array.data = AI_PTR(g_acho_detect_activations_map[0] + 0);
    _model_features_features_6_ReduceMean_3_output_0_Mul_output_array.data_start = AI_PTR(g_acho_detect_activations_map[0] + 0);
    _model_features_features_6_Slice_2_output_0_output_array.data = AI_PTR(g_acho_detect_activations_map[0] + 32);
    _model_features_features_6_Slice_2_output_0_output_array.data_start = AI_PTR(g_acho_detect_activations_map[0] + 32);
    _model_features_features_6_ReduceMean_2_output_0_output_array.data = AI_PTR(g_acho_detect_activations_map[0] + 128);
    _model_features_features_6_ReduceMean_2_output_0_output_array.data_start = AI_PTR(g_acho_detect_activations_map[0] + 128);
    _model_features_features_6_ReduceMean_2_output_0_Mul_output_array.data = AI_PTR(g_acho_detect_activations_map[0] + 32);
    _model_features_features_6_ReduceMean_2_output_0_Mul_output_array.data_start = AI_PTR(g_acho_detect_activations_map[0] + 32);
    _model_features_features_6_Slice_1_output_0_output_array.data = AI_PTR(g_acho_detect_activations_map[0] + 64);
    _model_features_features_6_Slice_1_output_0_output_array.data_start = AI_PTR(g_acho_detect_activations_map[0] + 64);
    _model_features_features_6_ReduceMean_1_output_0_output_array.data = AI_PTR(g_acho_detect_activations_map[0] + 160);
    _model_features_features_6_ReduceMean_1_output_0_output_array.data_start = AI_PTR(g_acho_detect_activations_map[0] + 160);
    _model_features_features_6_ReduceMean_1_output_0_Mul_output_array.data = AI_PTR(g_acho_detect_activations_map[0] + 64);
    _model_features_features_6_ReduceMean_1_output_0_Mul_output_array.data_start = AI_PTR(g_acho_detect_activations_map[0] + 64);
    _model_features_features_6_Slice_output_0_output_array.data = AI_PTR(g_acho_detect_activations_map[0] + 96);
    _model_features_features_6_Slice_output_0_output_array.data_start = AI_PTR(g_acho_detect_activations_map[0] + 96);
    _model_features_features_6_ReduceMean_output_0_output_array.data = AI_PTR(g_acho_detect_activations_map[0] + 192);
    _model_features_features_6_ReduceMean_output_0_output_array.data_start = AI_PTR(g_acho_detect_activations_map[0] + 192);
    _model_features_features_6_ReduceMean_output_0_Mul_output_array.data = AI_PTR(g_acho_detect_activations_map[0] + 96);
    _model_features_features_6_ReduceMean_output_0_Mul_output_array.data_start = AI_PTR(g_acho_detect_activations_map[0] + 96);
    _model_features_features_6_Concat_output_0_output_array.data = AI_PTR(g_acho_detect_activations_map[0] + 128);
    _model_features_features_6_Concat_output_0_output_array.data_start = AI_PTR(g_acho_detect_activations_map[0] + 128);
    _model_classifier_classifier_0_Flatten_output_0_to_chlast_output_array.data = AI_PTR(g_acho_detect_activations_map[0] + 0);
    _model_classifier_classifier_0_Flatten_output_0_to_chlast_output_array.data_start = AI_PTR(g_acho_detect_activations_map[0] + 0);
    _model_classifier_classifier_1_Gemm_output_0_output_array.data = AI_PTR(g_acho_detect_activations_map[0] + 128);
    _model_classifier_classifier_1_Gemm_output_0_output_array.data_start = AI_PTR(g_acho_detect_activations_map[0] + 128);
    _model_classifier_classifier_2_Relu_output_0_output_array.data = AI_PTR(g_acho_detect_activations_map[0] + 384);
    _model_classifier_classifier_2_Relu_output_0_output_array.data_start = AI_PTR(g_acho_detect_activations_map[0] + 384);
    _model_classifier_classifier_3_Gemm_output_0_output_array.data = AI_PTR(g_acho_detect_activations_map[0] + 0);
    _model_classifier_classifier_3_Gemm_output_0_output_array.data_start = AI_PTR(g_acho_detect_activations_map[0] + 0);
    _model_classifier_classifier_4_Relu_output_0_output_array.data = AI_PTR(g_acho_detect_activations_map[0] + 128);
    _model_classifier_classifier_4_Relu_output_0_output_array.data_start = AI_PTR(g_acho_detect_activations_map[0] + 128);
    _model_classifier_classifier_5_Gemm_output_0_output_array.data = AI_PTR(g_acho_detect_activations_map[0] + 0);
    _model_classifier_classifier_5_Gemm_output_0_output_array.data_start = AI_PTR(g_acho_detect_activations_map[0] + 0);
    _Sigmoid_output_0_output_array.data = AI_PTR(g_acho_detect_activations_map[0] + 4);
    _Sigmoid_output_0_output_array.data_start = AI_PTR(g_acho_detect_activations_map[0] + 4);
    return true;
  }
  AI_ERROR_TRAP(net_ctx, INIT_FAILED, NETWORK_ACTIVATIONS);
  return false;
}




/******************************************************************************/
AI_DECLARE_STATIC
ai_bool acho_detect_configure_weights(
  ai_network* net_ctx, const ai_network_params* params)
{
  AI_ASSERT(net_ctx)

  if (ai_platform_get_weights_map(g_acho_detect_weights_map, 1, params)) {
    /* Updating weights (byte) offsets */
    
    _model_features_features_0_Conv_output_0_weights_array.format |= AI_FMT_FLAG_CONST;
    _model_features_features_0_Conv_output_0_weights_array.data = AI_PTR(g_acho_detect_weights_map[0] + 0);
    _model_features_features_0_Conv_output_0_weights_array.data_start = AI_PTR(g_acho_detect_weights_map[0] + 0);
    _model_features_features_0_Conv_output_0_bias_array.format |= AI_FMT_FLAG_CONST;
    _model_features_features_0_Conv_output_0_bias_array.data = AI_PTR(g_acho_detect_weights_map[0] + 144);
    _model_features_features_0_Conv_output_0_bias_array.data_start = AI_PTR(g_acho_detect_weights_map[0] + 144);
    _model_features_features_3_Conv_output_0_weights_array.format |= AI_FMT_FLAG_CONST;
    _model_features_features_3_Conv_output_0_weights_array.data = AI_PTR(g_acho_detect_weights_map[0] + 160);
    _model_features_features_3_Conv_output_0_weights_array.data_start = AI_PTR(g_acho_detect_weights_map[0] + 160);
    _model_features_features_3_Conv_output_0_bias_array.format |= AI_FMT_FLAG_CONST;
    _model_features_features_3_Conv_output_0_bias_array.data = AI_PTR(g_acho_detect_weights_map[0] + 1312);
    _model_features_features_3_Conv_output_0_bias_array.data_start = AI_PTR(g_acho_detect_weights_map[0] + 1312);
    _model_features_features_6_ReduceMean_3_output_0_Mul_scale_array.format |= AI_FMT_FLAG_CONST;
    _model_features_features_6_ReduceMean_3_output_0_Mul_scale_array.data = AI_PTR(g_acho_detect_weights_map[0] + 1344);
    _model_features_features_6_ReduceMean_3_output_0_Mul_scale_array.data_start = AI_PTR(g_acho_detect_weights_map[0] + 1344);
    _model_features_features_6_ReduceMean_3_output_0_Mul_bias_array.format |= AI_FMT_FLAG_CONST;
    _model_features_features_6_ReduceMean_3_output_0_Mul_bias_array.data = AI_PTR(g_acho_detect_weights_map[0] + 1376);
    _model_features_features_6_ReduceMean_3_output_0_Mul_bias_array.data_start = AI_PTR(g_acho_detect_weights_map[0] + 1376);
    _model_features_features_6_ReduceMean_2_output_0_Mul_scale_array.format |= AI_FMT_FLAG_CONST;
    _model_features_features_6_ReduceMean_2_output_0_Mul_scale_array.data = AI_PTR(g_acho_detect_weights_map[0] + 1408);
    _model_features_features_6_ReduceMean_2_output_0_Mul_scale_array.data_start = AI_PTR(g_acho_detect_weights_map[0] + 1408);
    _model_classifier_classifier_1_Gemm_output_0_weights_array.format |= AI_FMT_FLAG_CONST;
    _model_classifier_classifier_1_Gemm_output_0_weights_array.data = AI_PTR(g_acho_detect_weights_map[0] + 1504);
    _model_classifier_classifier_1_Gemm_output_0_weights_array.data_start = AI_PTR(g_acho_detect_weights_map[0] + 1440);
    _model_classifier_classifier_1_Gemm_output_0_bias_array.format |= AI_FMT_FLAG_CONST;
    _model_classifier_classifier_1_Gemm_output_0_bias_array.data = AI_PTR(g_acho_detect_weights_map[0] + 2528);
    _model_classifier_classifier_1_Gemm_output_0_bias_array.data_start = AI_PTR(g_acho_detect_weights_map[0] + 2528);
    _model_classifier_classifier_3_Gemm_output_0_weights_array.format |= AI_FMT_FLAG_CONST;
    _model_classifier_classifier_3_Gemm_output_0_weights_array.data = AI_PTR(g_acho_detect_weights_map[0] + 2848);
    _model_classifier_classifier_3_Gemm_output_0_weights_array.data_start = AI_PTR(g_acho_detect_weights_map[0] + 2784);
    _model_classifier_classifier_3_Gemm_output_0_bias_array.format |= AI_FMT_FLAG_CONST;
    _model_classifier_classifier_3_Gemm_output_0_bias_array.data = AI_PTR(g_acho_detect_weights_map[0] + 3872);
    _model_classifier_classifier_3_Gemm_output_0_bias_array.data_start = AI_PTR(g_acho_detect_weights_map[0] + 3872);
    _model_classifier_classifier_5_Gemm_output_0_weights_array.format |= AI_FMT_FLAG_CONST;
    _model_classifier_classifier_5_Gemm_output_0_weights_array.data = AI_PTR(g_acho_detect_weights_map[0] + 4000);
    _model_classifier_classifier_5_Gemm_output_0_weights_array.data_start = AI_PTR(g_acho_detect_weights_map[0] + 4000);
    _model_classifier_classifier_5_Gemm_output_0_bias_array.format |= AI_FMT_FLAG_CONST;
    _model_classifier_classifier_5_Gemm_output_0_bias_array.data = AI_PTR(g_acho_detect_weights_map[0] + 4128);
    _model_classifier_classifier_5_Gemm_output_0_bias_array.data_start = AI_PTR(g_acho_detect_weights_map[0] + 4128);
    return true;
  }
  AI_ERROR_TRAP(net_ctx, INIT_FAILED, NETWORK_WEIGHTS);
  return false;
}


/**  PUBLIC APIs SECTION  *****************************************************/



AI_DEPRECATED
AI_API_ENTRY
ai_bool ai_acho_detect_get_info(
  ai_handle network, ai_network_report* report)
{
  ai_network* net_ctx = AI_NETWORK_ACQUIRE_CTX(network);

  if (report && net_ctx)
  {
    ai_network_report r = {
      .model_name        = AI_ACHO_DETECT_MODEL_NAME,
      .model_signature   = AI_ACHO_DETECT_MODEL_SIGNATURE,
      .model_datetime    = AI_TOOLS_DATE_TIME,
      
      .compile_datetime  = AI_TOOLS_COMPILE_TIME,
      
      .runtime_revision  = ai_platform_runtime_get_revision(),
      .runtime_version   = ai_platform_runtime_get_version(),

      .tool_revision     = AI_TOOLS_REVISION_ID,
      .tool_version      = {AI_TOOLS_VERSION_MAJOR, AI_TOOLS_VERSION_MINOR,
                            AI_TOOLS_VERSION_MICRO, 0x0},
      .tool_api_version  = AI_STRUCT_INIT,

      .api_version            = ai_platform_api_get_version(),
      .interface_api_version  = ai_platform_interface_api_get_version(),
      
      .n_macc            = 97527,
      .n_inputs          = 0,
      .inputs            = NULL,
      .n_outputs         = 0,
      .outputs           = NULL,
      .params            = AI_STRUCT_INIT,
      .activations       = AI_STRUCT_INIT,
      .n_nodes           = 0,
      .signature         = 0xa906173f,
    };

    if (!ai_platform_api_get_network_report(network, &r)) return false;

    *report = r;
    return true;
  }
  return false;
}



AI_API_ENTRY
ai_bool ai_acho_detect_get_report(
  ai_handle network, ai_network_report* report)
{
  ai_network* net_ctx = AI_NETWORK_ACQUIRE_CTX(network);

  if (report && net_ctx)
  {
    ai_network_report r = {
      .model_name        = AI_ACHO_DETECT_MODEL_NAME,
      .model_signature   = AI_ACHO_DETECT_MODEL_SIGNATURE,
      .model_datetime    = AI_TOOLS_DATE_TIME,
      
      .compile_datetime  = AI_TOOLS_COMPILE_TIME,
      
      .runtime_revision  = ai_platform_runtime_get_revision(),
      .runtime_version   = ai_platform_runtime_get_version(),

      .tool_revision     = AI_TOOLS_REVISION_ID,
      .tool_version      = {AI_TOOLS_VERSION_MAJOR, AI_TOOLS_VERSION_MINOR,
                            AI_TOOLS_VERSION_MICRO, 0x0},
      .tool_api_version  = AI_STRUCT_INIT,

      .api_version            = ai_platform_api_get_version(),
      .interface_api_version  = ai_platform_interface_api_get_version(),
      
      .n_macc            = 97527,
      .n_inputs          = 0,
      .inputs            = NULL,
      .n_outputs         = 0,
      .outputs           = NULL,
      .map_signature     = AI_MAGIC_SIGNATURE,
      .map_weights       = AI_STRUCT_INIT,
      .map_activations   = AI_STRUCT_INIT,
      .n_nodes           = 0,
      .signature         = 0xa906173f,
    };

    if (!ai_platform_api_get_network_report(network, &r)) return false;

    *report = r;
    return true;
  }
  return false;
}


AI_API_ENTRY
ai_error ai_acho_detect_get_error(ai_handle network)
{
  return ai_platform_network_get_error(network);
}


AI_API_ENTRY
ai_error ai_acho_detect_create(
  ai_handle* network, const ai_buffer* network_config)
{
  return ai_platform_network_create(
    network, network_config, 
    AI_CONTEXT_OBJ(&AI_NET_OBJ_INSTANCE),
    AI_TOOLS_API_VERSION_MAJOR, AI_TOOLS_API_VERSION_MINOR, AI_TOOLS_API_VERSION_MICRO);
}


AI_API_ENTRY
ai_error ai_acho_detect_create_and_init(
  ai_handle* network, const ai_handle activations[], const ai_handle weights[])
{
  ai_error err;
  ai_network_params params;

  err = ai_acho_detect_create(network, AI_ACHO_DETECT_DATA_CONFIG);
  if (err.type != AI_ERROR_NONE) {
    return err;
  }
  
  if (ai_acho_detect_data_params_get(&params) != true) {
    err = ai_acho_detect_get_error(*network);
    return err;
  }
#if defined(AI_ACHO_DETECT_DATA_ACTIVATIONS_COUNT)
  /* set the addresses of the activations buffers */
  for (ai_u16 idx=0; activations && idx<params.map_activations.size; idx++) {
    AI_BUFFER_ARRAY_ITEM_SET_ADDRESS(&params.map_activations, idx, activations[idx]);
  }
#endif
#if defined(AI_ACHO_DETECT_DATA_WEIGHTS_COUNT)
  /* set the addresses of the weight buffers */
  for (ai_u16 idx=0; weights && idx<params.map_weights.size; idx++) {
    AI_BUFFER_ARRAY_ITEM_SET_ADDRESS(&params.map_weights, idx, weights[idx]);
  }
#endif
  if (ai_acho_detect_init(*network, &params) != true) {
    err = ai_acho_detect_get_error(*network);
  }
  return err;
}


AI_API_ENTRY
ai_buffer* ai_acho_detect_inputs_get(ai_handle network, ai_u16 *n_buffer)
{
  if (network == AI_HANDLE_NULL) {
    network = (ai_handle)&AI_NET_OBJ_INSTANCE;
    AI_NETWORK_OBJ(network)->magic = AI_MAGIC_CONTEXT_TOKEN;
  }
  return ai_platform_inputs_get(network, n_buffer);
}


AI_API_ENTRY
ai_buffer* ai_acho_detect_outputs_get(ai_handle network, ai_u16 *n_buffer)
{
  if (network == AI_HANDLE_NULL) {
    network = (ai_handle)&AI_NET_OBJ_INSTANCE;
    AI_NETWORK_OBJ(network)->magic = AI_MAGIC_CONTEXT_TOKEN;
  }
  return ai_platform_outputs_get(network, n_buffer);
}


AI_API_ENTRY
ai_handle ai_acho_detect_destroy(ai_handle network)
{
  return ai_platform_network_destroy(network);
}


AI_API_ENTRY
ai_bool ai_acho_detect_init(
  ai_handle network, const ai_network_params* params)
{
  ai_network* net_ctx = AI_NETWORK_OBJ(ai_platform_network_init(network, params));
  ai_bool ok = true;

  if (!net_ctx) return false;
  ok &= acho_detect_configure_weights(net_ctx, params);
  ok &= acho_detect_configure_activations(net_ctx, params);

  ok &= ai_platform_network_post_init(network);

  return ok;
}


AI_API_ENTRY
ai_i32 ai_acho_detect_run(
  ai_handle network, const ai_buffer* input, ai_buffer* output)
{
  return ai_platform_network_process(network, input, output);
}


AI_API_ENTRY
ai_i32 ai_acho_detect_forward(ai_handle network, const ai_buffer* input)
{
  return ai_platform_network_process(network, input, NULL);
}



#undef AI_ACHO_DETECT_MODEL_SIGNATURE
#undef AI_NET_OBJ_INSTANCE
#undef AI_TOOLS_DATE_TIME
#undef AI_TOOLS_COMPILE_TIME

