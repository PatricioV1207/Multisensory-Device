#include "diagnostics/INMP441RawDiagnostic.h"

#include <driver/i2s.h>
#include <esp_err.h>
#include "config.h"
#include "pins.h"
#include "utils/Logger.h"

namespace {
constexpr i2s_port_t kI2sPort = I2S_NUM_0;
constexpr size_t kDmaReadWords = 256U;
}

bool INMP441RawDiagnostic::begin() {
  if (_started) {
    return true;
  }

  i2s_config_t config = {};
  config.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_RX);
  config.sample_rate = ACOUSTIC_SAMPLE_RATE_HZ;
  config.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;
  config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
  config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  config.dma_buf_count = 4;
  config.dma_buf_len = kDmaReadWords;
  config.use_apll = false;
  config.tx_desc_auto_clear = false;
  config.fixed_mclk = 0;
  config.mclk_multiple = I2S_MCLK_MULTIPLE_DEFAULT;
  config.bits_per_chan = I2S_BITS_PER_CHAN_32BIT;

  esp_err_t result = i2s_driver_install(kI2sPort, &config, 0, nullptr);
  if (result != ESP_OK) {
    Logger::error("AUDIO", "Raw I2S driver install failed error=" +
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
    Logger::error("AUDIO", "Raw I2S pin configuration failed error=" +
                               String(result));
    i2s_driver_uninstall(kI2sPort);
    _driverInstalled = false;
    return false;
  }

  i2s_zero_dma_buffer(kI2sPort);
  resetCounters();
  _started = true;
  Logger::info(
      "AUDIO",
      "Raw INMP441 diagnostic started: 32-bit stereo slots at " +
          String(ACOUSTIC_SAMPLE_RATE_HZ) +
          " Hz; values are signed 24-bit words before audio processing");
  return true;
}

void INMP441RawDiagnostic::update() {
  if (!_started) {
    return;
  }

  int32_t rawWords[kDmaReadWords] = {0};
  size_t bytesRead = 0U;
  const esp_err_t result =
      i2s_read(kI2sPort, rawWords, sizeof(rawWords), &bytesRead,
               pdMS_TO_TICKS(100U));
  if (result != ESP_OK || bytesRead == 0U) {
    ++_failedReads;
    return;
  }

  ++_successfulReads;
  _bytesRead += static_cast<uint32_t>(bytesRead);
  const size_t wordsRead = bytesRead / sizeof(int32_t);
  for (size_t index = 0U; index < wordsRead; ++index) {
    const int32_t signed24 = rawWords[index] >> 8;
    addSample(_slots[index & 1U], signed24);
  }
}

INMP441RawSnapshot INMP441RawDiagnostic::takeSnapshot() {
  INMP441RawSnapshot snapshot;
  snapshot.driverReady = _started && _driverInstalled;
  snapshot.successfulReads = _successfulReads;
  snapshot.failedReads = _failedReads;
  snapshot.bytesRead = _bytesRead;
  snapshot.slots[0] = finishSlot(_slots[0]);
  snapshot.slots[1] = finishSlot(_slots[1]);
  resetCounters();
  return snapshot;
}

void INMP441RawDiagnostic::addSample(SlotAccumulator& slot, int32_t sample) {
  ++slot.samples;
  if (sample != 0) {
    ++slot.nonzero;
  }
  if (slot.hasPrevious && sample != slot.previous) {
    ++slot.changes;
  }
  slot.minimum = sample < slot.minimum ? sample : slot.minimum;
  slot.maximum = sample > slot.maximum ? sample : slot.maximum;
  const int64_t widened = static_cast<int64_t>(sample);
  slot.absoluteSum +=
      static_cast<uint64_t>(widened < 0 ? -widened : widened);
  slot.previous = sample;
  slot.hasPrevious = true;
}

INMP441RawSlotStats INMP441RawDiagnostic::finishSlot(
    const SlotAccumulator& slot) {
  INMP441RawSlotStats result;
  result.samples = slot.samples;
  result.nonzero = slot.nonzero;
  result.changes = slot.changes;
  result.minimum = slot.samples == 0U ? 0 : slot.minimum;
  result.maximum = slot.samples == 0U ? 0 : slot.maximum;
  result.absoluteSum = slot.absoluteSum;
  return result;
}

void INMP441RawDiagnostic::resetCounters() {
  _slots[0] = SlotAccumulator{};
  _slots[1] = SlotAccumulator{};
  _successfulReads = 0U;
  _failedReads = 0U;
  _bytesRead = 0U;
}
