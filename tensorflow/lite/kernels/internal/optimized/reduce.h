/* Copyright 2022 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/
#ifndef TENSORFLOW_LITE_KERNELS_INTERNAL_OPTIMIZED_REDUCE_OPS_H_
#define TENSORFLOW_LITE_KERNELS_INTERNAL_OPTIMIZED_REDUCE_OPS_H_

#include <stdint.h>

#include <algorithm>
#include <limits>

#include "ruy/profiler/instrumentation.h"  // from @ruy
#include "tensorflow/lite/kernels/cpu_backend_threadpool.h"
#include "tensorflow/lite/kernels/internal/optimized/optimized_ops_utils.h"
#include "tensorflow/lite/kernels/internal/reference/reduce.h"
#include "tensorflow/lite/kernels/internal/runtime_shape.h"
#include "tensorflow/lite/kernels/internal/types.h"

namespace tflite {
namespace optimized_ops {

inline void MeanImpl(const tflite::MeanParams& op_params,
                     const RuntimeShape& input_shape, const uint8_t* input_data,
                     int32 multiplier, int32 shift, int32 bias,
                     const RuntimeShape& output_shape, uint8_t* output_data,
                     int start_depth, int end_depth) {
  ruy::profiler::ScopeLabel label("Mean4D/Uint8/MeanImpl");

  // Current implementation only supports dimension equals 4 and simultaneous
  // reduction over width and height.
  const int output_batch = output_shape.Dims(0);
  const int output_height = output_shape.Dims(2);
  const int output_width = output_shape.Dims(2);
  const int input_height = input_shape.Dims(1);
  const int input_width = input_shape.Dims(2);

  TFLITE_CHECK_EQ(op_params.axis_count, 2);
  TFLITE_CHECK((op_params.axis[0] == 1 && op_params.axis[1] == 2) ||
               (op_params.axis[0] == 2 && op_params.axis[1] == 1));
  TFLITE_CHECK_EQ(output_height, 1);
  TFLITE_CHECK_EQ(output_width, 1);

  constexpr int32_t kMinValue = std::numeric_limits<uint8_t>::min();
  constexpr int32_t kMaxValue = std::numeric_limits<uint8_t>::max();

#ifdef USE_NEON
  const int32x4_t bias_dup = vdupq_n_s32(bias);
  const int32x4_t min_dup = vdupq_n_s32(kMinValue);
  const int32x4_t max_dup = vdupq_n_s32(kMaxValue);
#endif  // USE_NEON

  for (int out_b = 0; out_b < output_batch; ++out_b) {
    int out_d = start_depth;
#ifdef USE_NEON

    for (; out_d <= end_depth - 16; out_d += 16) {
      int32x4x4_t temp_sum;
      temp_sum.val[0] = vdupq_n_s32(0);
      temp_sum.val[1] = vdupq_n_s32(0);
      temp_sum.val[2] = vdupq_n_s32(0);
      temp_sum.val[3] = vdupq_n_s32(0);
      for (int in_h = 0; in_h < input_height; ++in_h) {
        for (int in_w = 0; in_w < input_width; ++in_w) {
          const uint8_t* input_data_ptr =
              input_data + Offset(input_shape, out_b, in_h, in_w, out_d);
          uint8x16_t input_data_val = vld1q_u8(input_data_ptr);

          int16x8_t input_data_low_shift =
              vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(input_data_val)));
          int16x8_t input_data_high_shift =
              vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(input_data_val)));

          int32x4_t input_low_low =
              vmovl_s16(vget_low_s16(input_data_low_shift));
          int32x4_t input_high_low =
              vmovl_s16(vget_high_s16(input_data_low_shift));
          int32x4_t input_low_high =
              vmovl_s16(vget_low_s16(input_data_high_shift));
          int32x4_t input_high_high =
              vmovl_s16(vget_high_s16(input_data_high_shift));

          temp_sum.val[0] = vaddq_s32(temp_sum.val[0], input_low_low);
          temp_sum.val[1] = vaddq_s32(temp_sum.val[1], input_high_low);
          temp_sum.val[2] = vaddq_s32(temp_sum.val[2], input_low_high);
          temp_sum.val[3] = vaddq_s32(temp_sum.val[3], input_high_high);
        }
      }

      temp_sum =
          MultiplyByQuantizedMultiplier4Rows(temp_sum, multiplier, shift);

      temp_sum.val[0] = vaddq_s32(temp_sum.val[0], bias_dup);
      temp_sum.val[1] = vaddq_s32(temp_sum.val[1], bias_dup);
      temp_sum.val[2] = vaddq_s32(temp_sum.val[2], bias_dup);
      temp_sum.val[3] = vaddq_s32(temp_sum.val[3], bias_dup);

      temp_sum.val[0] = vminq_s32(vmaxq_s32(temp_sum.val[0], min_dup), max_dup);
      temp_sum.val[1] = vminq_s32(vmaxq_s32(temp_sum.val[1], min_dup), max_dup);
      temp_sum.val[2] = vminq_s32(vmaxq_s32(temp_sum.val[2], min_dup), max_dup);
      temp_sum.val[3] = vminq_s32(vmaxq_s32(temp_sum.val[3], min_dup), max_dup);

      uint16x4_t narrowed_low_low =
          vmovn_u32(vreinterpretq_u32_s32(temp_sum.val[0]));
      uint16x4_t narrowed_high_low =
          vmovn_u32(vreinterpretq_u32_s32(temp_sum.val[1]));
      uint16x4_t narrowed_low_high =
          vmovn_u32(vreinterpretq_u32_s32(temp_sum.val[2]));
      uint16x4_t narrowed_high_high =
          vmovn_u32(vreinterpretq_u32_s32(temp_sum.val[3]));

      uint16x8_t combined_low =
          vcombine_u16(narrowed_low_low, narrowed_high_low);
      uint16x8_t combined_high =
          vcombine_u16(narrowed_low_high, narrowed_high_high);

      uint8x8_t narrowed_low = vmovn_u16(combined_low);
      uint8x8_t narrowed_high = vmovn_u16(combined_high);

      uint8x16_t combined_output = vcombine_u8(narrowed_low, narrowed_high);

      uint8_t* output_data_ptr =
          output_data + Offset(output_shape, out_b, 0, 0, out_d);
      vst1q_u8(output_data_ptr, combined_output);
    }
#endif  // USE_NEON

    for (; out_d < end_depth; ++out_d) {
      int acc = 0;
      for (int in_h = 0; in_h < input_height; ++in_h) {
        for (int in_w = 0; in_w < input_width; ++in_w) {
          acc += input_data[Offset(input_shape, out_b, in_h, in_w, out_d)];
        }
      }

      acc = MultiplyByQuantizedMultiplier(acc, multiplier, shift);
      acc += bias;
      acc = std::min(std::max(acc, kMinValue), kMaxValue);
      output_data[Offset(output_shape, out_b, 0, 0, out_d)] =
          static_cast<uint8_t>(acc);
    }
  }
}

struct MeanWorkerTask : cpu_backend_threadpool::Task {
  MeanWorkerTask(const tflite::MeanParams& op_params,
                 const RuntimeShape& input_shape, const uint8_t* input_data,
                 int32 multiplier, int32 shift, int32 bias,
                 const RuntimeShape& output_shape, uint8_t* output_data,
                 int start_height, int end_height)
      : op_params(op_params),
        input_shape(input_shape),
        input_data(input_data),
        multiplier(multiplier),
        shift(shift),
        bias(bias),
        output_shape(output_shape),
        output_data(output_data),
        start_height(start_height),
        end_height(end_height) {}

  void Run() override {
    MeanImpl(op_params, input_shape, input_data, multiplier, shift, bias,
             output_shape, output_data, start_height, end_height);
  }

 private:
  const tflite::MeanParams& op_params;
  const RuntimeShape& input_shape;
  const uint8_t* input_data;
  int32 multiplier;
  int32 shift;
  int32 bias;
  const RuntimeShape& output_shape;
  uint8_t* output_data;
  int start_height;
  int end_height;
};

inline void Mean(const tflite::MeanParams& op_params,
                 const RuntimeShape& unextended_input_shape,
                 const uint8_t* input_data, int32 input_zero_point,
                 float input_scale, const RuntimeShape& unextended_output_shape,
                 uint8_t* output_data, int32 output_zero_point,
                 float output_scale, CpuBackendContext* cpu_backend_context) {
  ruy::profiler::ScopeLabel label("Mean4D/Uint8");
  // Current implementation only supports dimension equals 4 and simultaneous
  // reduction over width and height.
  TFLITE_CHECK_EQ(unextended_input_shape.DimensionsCount(), 4);
  TFLITE_CHECK_LE(unextended_output_shape.DimensionsCount(), 4);
  const RuntimeShape input_shape =
      RuntimeShape::ExtendedShape(4, unextended_input_shape);
  const RuntimeShape output_shape =
      RuntimeShape::ExtendedShape(4, unextended_output_shape);
  const int output_height = output_shape.Dims(1);
  const int output_width = output_shape.Dims(2);
  const int output_depth = output_shape.Dims(3);

  TFLITE_CHECK_EQ(op_params.axis_count, 2);
  TFLITE_CHECK((op_params.axis[0] == 1 && op_params.axis[1] == 2) ||
               (op_params.axis[0] == 2 && op_params.axis[1] == 1));
  TFLITE_CHECK_EQ(output_height, 1);
  TFLITE_CHECK_EQ(output_width, 1);

  const int input_height = input_shape.Dims(1);
  const int input_width = input_shape.Dims(2);
  const float num_elements_in_axis = input_width * input_height;

  float temp = input_zero_point * input_scale / output_scale;
  temp = temp > 0 ? temp + 0.5f : temp - 0.5f;
  int32_t bias = output_zero_point - static_cast<int32_t>(temp);
  float real_scale = input_scale / (num_elements_in_axis * output_scale);

  int32 multiplier, shift;
  QuantizeMultiplier(real_scale, &multiplier, &shift);

  constexpr int kMinDepthPerThread = 8;
  int thread_count = output_depth / kMinDepthPerThread;
  thread_count = thread_count > 0 ? thread_count : 1;
  const int capped_thread_count =
      std::min(thread_count, cpu_backend_context->max_num_threads());

  if (capped_thread_count == 1) {
    MeanImpl(op_params, input_shape, input_data, multiplier, shift, bias,
             output_shape, output_data, 0, output_depth);
  } else {
    // Instead parallel for batch, we loop for the output_depth since batch
    // is typical 1.
    std::vector<MeanWorkerTask> tasks;
    // TODO(b/131746020) don't create new heap allocations every time.
    // At least we make it a single heap allocation by using reserve().
    tasks.reserve(capped_thread_count);
    int depth_start = 0;
    for (int i = 0; i < capped_thread_count; ++i) {
      // Try to distribute the tasks as even as possible.
      int depth_end = depth_start +
                      (output_depth - depth_start) / (capped_thread_count - i);
      tasks.emplace_back(op_params, input_shape, input_data, multiplier, shift,
                         bias, output_shape, output_data, depth_start,
                         depth_end);
      depth_start = depth_end;
    }
    cpu_backend_threadpool::Execute(tasks.size(), tasks.data(),
                                    cpu_backend_context);
  }
}

template <typename T, typename U>
inline bool MeanGeneral(const T* input_data, const int* input_dims,
                        const int input_num_dims, T* output_data,
                        const int* output_dims, const int output_num_dims,
                        const int* axis, const int num_axis_dimensions,
                        bool keep_dims, int* temp_index, int* resolved_axis,
                        U* temp_sum) {
  return reference_ops::Mean(input_data, input_dims, input_num_dims,
                             output_data, output_dims, output_num_dims, axis,
                             num_axis_dimensions, keep_dims, temp_index,
                             resolved_axis, temp_sum);
}

template <>
inline bool MeanGeneral<float, float>(
    const float* input_data, const int* input_dims, const int input_num_dims,
    float* output_data, const int* output_dims, const int output_num_dims,
    const int* axis, const int num_axis_dimensions, bool keep_dims,
    int* temp_index, int* resolved_axis, float* temp_sum) {
  // Handle reduce_mean for the last dimensions.
  if (num_axis_dimensions == 1 && axis[0] == (input_num_dims - 1)) {
    ruy::profiler::ScopeLabel label("MeanLastDim/Float");
    int output_size = 1;
    for (int i = 0; i < input_num_dims - 1; ++i) {
      output_size *= input_dims[i];
    }
    const int last_input_dim = input_dims[axis[0]];

    // TODO(b/152563685): Consider use eigen to cover more general cases.
    const MatrixMap<const float> in_mat(input_data, last_input_dim,
                                        output_size);
    VectorMap<float> out(output_data, output_size, 1);
    out = (in_mat.array().colwise().sum()) / static_cast<float>(last_input_dim);
    return true;
  }

  return reference_ops::Mean(input_data, input_dims, input_num_dims,
                             output_data, output_dims, output_num_dims, axis,
                             num_axis_dimensions, keep_dims, temp_index,
                             resolved_axis, temp_sum);
}

}  // namespace optimized_ops
}  // namespace tflite

#endif  // TENSORFLOW_LITE_KERNELS_INTERNAL_OPTIMIZED_OPTIMIZED_OPS_H_
