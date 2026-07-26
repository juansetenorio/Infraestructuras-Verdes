#ifndef PAYLOAD_BUILDER_H
#define PAYLOAD_BUILDER_H

#include <Arduino.h>

#include "SensorManager.h"

String construirPayload(const SensorData& data, bool valveOpen);

#endif
