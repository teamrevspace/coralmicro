#include <SD.h>

#include "Arduino.h"
#include "libs/tensorflow/classification.h"
#include "libs/tensorflow/utils.h"
#include "libs/tpu/edgetpu_manager.h"
#include "third_party/tflite-micro/tensorflow/lite/micro/micro_error_reporter.h"
#include "third_party/tflite-micro/tensorflow/lite/micro/micro_interpreter.h"
#include "third_party/tflite-micro/tensorflow/lite/micro/micro_mutable_op_resolver.h"

namespace {
tflite::MicroMutableOpResolver<1> resolver;
const tflite::Model* model = nullptr;
std::unique_ptr<tflite::MicroInterpreter> interpreter = nullptr;
std::vector<uint8_t> image_data;
std::shared_ptr<valiant::EdgeTpuContext> context = nullptr;

const int kTensorArenaSize = 1024 * 1024;
static uint8_t tensor_arena[kTensorArenaSize] __attribute__((aligned(16)))
__attribute__((section(".sdram_bss,\"aw\",%nobits @")));
}  // namespace

void setup() {
    Serial.begin(115200);
    SD.begin();

    Serial.println("Loading Model");
    std::vector<uint8_t> model_data;

    const char* model_name =
        "/models/mobilenet_v1_1.0_224_quant_edgetpu.tflite";
    if (!SD.exists(model_name)) {
        Serial.println("Model file not found");
        return;
    }

    SDFile model_file = SD.open(model_name);
    uint32_t model_size = model_file.size();
    model_data.resize(model_size);
    if (model_file.read(model_data.data(), model_size) != model_size) {
        Serial.print("Error loading model");
    }

    const char* image_name = "/apps/ClassifyImage/cat_224x224.rgb";
    Serial.println("Loading Input Image");
    if (!SD.exists(image_name)) {
        Serial.println("Image file not found");
        return;
    }
    SDFile image_file = SD.open(image_name);
    uint32_t image_size = image_file.size();
    image_data.resize(image_size);
    if (image_file.read(image_data.data(), image_size) != image_size) {
        Serial.print("Error loading image");
    }

    model = tflite::GetModel(model_data.data());
    context = valiant::EdgeTpuManager::GetSingleton()->OpenDevice();
    if (!context) {
        Serial.println("Failed to get EdgeTpuContext");
        return;
    }

    resolver.AddCustom(valiant::kCustomOp, valiant::RegisterCustomOp());
    tflite::MicroErrorReporter error_reporter;
    interpreter = std::unique_ptr<tflite::MicroInterpreter>(
        new tflite::MicroInterpreter(model, resolver, tensor_arena, kTensorArenaSize, &error_reporter));
    interpreter->AllocateTensors();

    if (!interpreter) {
        Serial.println("Failed to make interpreter");
        return;
    }
    if (interpreter->inputs().size() != 1) {
        Serial.println("Bad inputs size");
        return;
    }
    Serial.println("Initialized");
}

void loop() {
    delay(1000);

    if (!interpreter) {
        Serial.println("Cannot invoke because of a problem during setup!");
        return;
    }

    auto* input_tensor = interpreter->input_tensor(0);
    if (input_tensor->type != kTfLiteUInt8) {
        Serial.println("Bad input type");
        return;
    }

    if (valiant::tensorflow::ClassificationInputNeedsPreprocessing(
            *input_tensor)) {
        valiant::tensorflow::ClassificationPreprocess(input_tensor);
    }

    valiant::tensorflow::TensorSize(input_tensor);
    auto* input_tensor_data = tflite::GetTensorData<uint8_t>(input_tensor);
    memcpy(input_tensor_data, image_data.data(), input_tensor->bytes);

    if (interpreter->Invoke() != kTfLiteOk) {
        Serial.println("invoke failed");
        return;
    }

    auto results = valiant::tensorflow::GetClassificationResults(
        interpreter.get(), 0.0f, 3);
    for (auto result : results) {
        Serial.print("Label ID: ");
        Serial.print(result.id);
        Serial.print(" Score: ");
        Serial.println(result.score);
    }
}