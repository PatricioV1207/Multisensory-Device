#include "sensors/INMP441Microphone.h"

#include <driver/i2s.h>
#include <esp_err.h>
#include <cmath>
#include "config.h"
#include "pins.h"
#include "utils/Logger.h"

namespace {
constexpr i2s_port_t kI2sPort = I2S_NUM_0;
constexpr size_t kDmaReadSamples = 256U;
}

bool INMP441Microphone::begin() {
  _lastBeginAttemptMs = millis();
  if (_started) {
    return true;
  }
  if (ACOUSTIC_FRAME_SAMPLES > AudioFeatureExtractor::MAX_SAMPLES) {
    Logger::error("AUDIO", "Configured frame exceeds extractor capacity");
    return false;
  }

  i2s_config_t config = {};
  config.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_RX);
  config.sample_rate = ACOUSTIC_SAMPLE_RATE_HZ;
  config.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;
  config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
  config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  config.dma_buf_count = 4;
  config.dma_buf_len = kDmaReadSamples;
  config.use_apll = false;
  config.tx_desc_auto_clear = false;
  config.fixed_mclk = 0;
  config.mclk_multiple = I2S_MCLK_MULTIPLE_DEFAULT;
  config.bits_per_chan = I2S_BITS_PER_CHAN_32BIT;

  esp_err_t result = i2s_driver_install(kI2sPort, &config, 0, nullptr);
  if (result != ESP_OK) {
    Logger::error("AUDIO", "I2S driver install failed error=" +
                               String(result));
    return false;
  }
  _driverInstalled = true;

  i2s_pin_config_t pins = {};
  pins.mck_io_num = I2S_PIN_NO_CHANGE;
  pins.bck_io_num = PIN_INMP441_BCLK;
  pins.ws_io_num = PIN_INMP441_WS;
  pins.data_out_num = I2S_PIN_NO_CHANGE;
  pins.data_in_num = PIN_INMP441_DATA;
  result = i2s_set_pin(kI2sPort, &pins);
  if (result != ESP_OK) {
    Logger::error("AUDIO", "I2S pin configuration failed error=" +
                               String(result));
    i2s_driver_uninstall(kI2sPort);
    _driverInstalled = false;
    return false;
  }
  i2s_zero_dma_buffer(kI2sPort);

  const BaseType_t created = xTaskCreatePinnedToCore(
      taskEntry, "vs_audio", 8192U, this, 1U, &_taskHandle, 0);
  if (created != pdPASS) {
    Logger::error("AUDIO", "Could not create bounded audio task");
    i2s_driver_uninstall(kI2sPort);
    _driverInstalled = false;
    _taskHandle = nullptr;
    return false;
  }
  _started = true;
  Logger::info("AUDIO", "INMP441 I2S started at " +
                            String(ACOUSTIC_SAMPLE_RATE_HZ) +
                            " Hz; output is relative dBFS, not dB SPL");
  return true;
}

void INMP441Microphone::update(uint32_t nowMs) {
  if (!_started) {
    if (static_cast<uint32_t>(nowMs - _lastBeginAttemptMs) >=
        SENSOR_RETRY_INTERVAL_MS) {
      begin();
    }
    return;
  }
  portENTER_CRITICAL(&_dataMux);
  if (_data.updatedAtMs != 0U &&
      static_cast<uint32_t>(nowMs - _data.updatedAtMs) > ACOUSTIC_STALE_MS) {
    _data.microphoneValid = false;
    _data.analysisValid = false;
    _data.category = "unknown";
    _data.confidence = 0.0F;
  }
  portEXIT_CRITICAL(&_dataMux);
}

void INMP441Microphone::taskEntry(void* context) {
  static_cast<INMP441Microphone*>(context)->run();
}

void INMP441Microphone::run() {
  int32_t rawSamples[kDmaReadSamples] = {0};
  uint32_t consecutiveReadFailures = 0U;
  while (true) {
    size_t bytesRead = 0U;
    const esp_err_t result = i2s_read(
        kI2sPort, rawSamples, sizeof(rawSamples), &bytesRead,
        pdMS_TO_TICKS(250U));
    if (result != ESP_OK || bytesRead == 0U) {
      ++consecutiveReadFailures;
      if (consecutiveReadFailures >= 4U) {
        publishInvalid(millis());
        consecutiveReadFailures = 0U;
      }
      continue;
    }
    consecutiveReadFailures = 0U;
    const size_t samplesRead = bytesRead / sizeof(int32_t);
    for (size_t index = 0U; index < samplesRead; ++index) {
      const int32_t signed24 = rawSamples[index] >> 8;
      _normalizedSamples[_normalizedCount++] =
          static_cast<float>(signed24) / 8388608.0F;
      if (_normalizedCount < ACOUSTIC_FRAME_SAMPLES) {
        continue;
      }
      AudioFeatures frame;
      if (!_extractor.analyze(_normalizedSamples, ACOUSTIC_FRAME_SAMPLES,
                              ACOUSTIC_SAMPLE_RATE_HZ, frame)) {
        publishInvalid(millis());
        _normalizedCount = 0U;
        continue;
      }
      AudioFeatures aggregate;
      if (_accumulator.add(frame, aggregate)) {
        publishAggregate(aggregate, millis());
      }
      _normalizedCount = 0U;
    }
  }
}

void INMP441Microphone::publishAggregate(const AudioFeatures& features,
                                         uint32_t nowMs) {
  const AcousticClassification classification =
      AcousticClassifier::classify(features);
  AcousticData next;
  next.microphoneValid = features.valid && features.signalResponsive;
  next.analysisValid = next.microphoneValid;
  next.relativeLevelDbfs = features.relativeLevelDbfs;
  next.peakDbfs = features.peakDbfs;
  next.clippingRatio = features.clippingRatio;
  next.clipping = features.clippingRatio >=
                  ACOUSTIC_CLIPPING_RATIO_THRESHOLD;
  next.crestFactor = features.crestFactor;
  next.zeroCrossingRate = features.zeroCrossingRate;
  next.spectralCentroidHz = features.spectralCentroidHz;
  next.spectralFlatness = features.spectralFlatness;
  next.spectralRolloffHz = features.spectralRolloffHz;
  next.bands = features.bands;
  next.sampleRateHz = features.sampleRateHz;
  next.windowDurationMs = features.windowDurationMs;
  next.category = classification.category;
  next.confidence = classification.confidence;
  next.classifierVersion = classification.classifierVersion;
  next.updatedAtMs = nowMs;
  portENTER_CRITICAL(&_dataMux);
  _data = next;
  portEXIT_CRITICAL(&_dataMux);
}

void INMP441Microphone::publishInvalid(uint32_t nowMs) {
  AcousticData next;
  next.updatedAtMs = nowMs;
  portENTER_CRITICAL(&_dataMux);
  _data = next;
  portEXIT_CRITICAL(&_dataMux);
}

bool INMP441Microphone::isValid() const {
  return getData().microphoneValid;
}

AcousticData INMP441Microphone::getData() const {
  AcousticData snapshot;
  portENTER_CRITICAL(&_dataMux);
  snapshot = _data;
  portEXIT_CRITICAL(&_dataMux);
  return snapshot;
}
