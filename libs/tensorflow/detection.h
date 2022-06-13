#ifndef LIBS_TENSORFLOW_DETECTION_H_
#define LIBS_TENSORFLOW_DETECTION_H_

#include <limits>
#include <vector>

#include "third_party/tflite-micro/tensorflow/lite/micro/micro_interpreter.h"

namespace coral::micro::tensorflow {

// Represents the bounding box of a detected object.
template <typename T>
struct BBox {
    // The box y-minimum (top-most) point.
    T ymin;
    // The box x-minimum (left-most) point.
    T xmin;
    // The box y-maximum (bottom-most) point.
    T ymax;
    // The box x-maximum (right-most) point.
    T xmax;
};

// Represents a detected object.
struct Object {
    // The class label id.
    int id;
    // The prediction score.
    float score;
    // The bounding-box (ymin,xmin,ymax,xmax).
    BBox<float> bbox;
};

// Converts detection output tensors into a vector of Objects.
//
// @param bboxes The output tensor for all detected bounding boxes in
//  box-corner encoding, for example:
//  (ymin1,xmin1,ymax1,xmax1,ymin2,xmin2,...).
// @param ids The output tensor for all label IDs.
// @param scores The output tensor for all scores.
// @param count The number of detected objects (all tensors defined above
//   have valid data for this number of objects).
// @param threshold The score threshold for results. All returned results have
//   a score greater-than-or-equal-to this value.
// @param top_k The maximum number of predictions to return.
// @returns The top_k object predictions (id, score, BBox), ordered by score
// (first element has the highest score).
std::vector<Object> GetDetectionResults(
    const float* bboxes, const float* ids, const float* scores, size_t count,
    float threshold = -std::numeric_limits<float>::infinity(),
    size_t top_k = std::numeric_limits<size_t>::max());

// Gets results from a detection model as a vector of Objects.
//
// @param interpreter The already-invoked interpreter for your detection model.
// @param threshold The score threshold for results. All returned results have
//   a score greater-than-or-equal-to this value.
// @param top_k The maximum number of predictions to return.
// @returns The top_k object predictions (id, score, BBox), ordered by score
// (first element has the highest score).
std::vector<Object> GetDetectionResults(
    tflite::MicroInterpreter* interpreter,
    float threshold = -std::numeric_limits<float>::infinity(),
    size_t top_k = std::numeric_limits<size_t>::max());

}  // namespace coral::micro

#endif  // LIBS_TENSORFLOW_DETECTION_H_
