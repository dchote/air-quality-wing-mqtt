/*
 * Project: Particle Air Quality Wing over MQTT
 * Description: MQTT Sketch for the Particle Air Quality Wing
 * Author: Jared Wolff (Circuit Dojo LLC) & Daniel Chote (https://github.com/dchote)
 * Date: 10/27/2019
 * License: GNU GPLv3
 */

#include "AirQualityWing.h"
#include "MQTT.h"
#include "board.h"

char *MQTTServer = (char*)"172.18.0.5";
String DeviceName = "unknown";

// Logger
SerialLogHandler logHandler(115200, LOG_LEVEL_ERROR, {
    { "app", LOG_LEVEL_WARN }, // enable all app messages
});

// Forward declaration of event handler
void AirQualityWingEvent();

// AirQualityWing object
AirQualityWing AirQual = AirQualityWing();
AirQualityWingData_t aqData;

// MQTT object
MQTT client(MQTTServer, 1883, mqtt_callback);

// Handler is called in main loop.
// Ok to run Particle.Publish
void AirQualityWingEvent() {

  Log.trace("pub");

  if (client.isConnected()) {
    aqData = AirQual.getData();

    if (aqData.hpma115.hasData) {
      client.publish("environment/" + DeviceName + "/pm25", String::format("%d", aqData.hpma115.data.pm25));
      client.publish("environment/" + DeviceName + "/pm10", String::format("%d", aqData.hpma115.data.pm10));
    }

    if (aqData.si7021.hasData) {
      client.publish("environment/" + DeviceName + "/temperature", String::format("%.2f", aqData.si7021.data.temperature));
      client.publish("environment/" + DeviceName + "/humidity", String::format("%.2f", aqData.si7021.data.humidity));
    }

    if (aqData.ccs811.hasData) {
      client.publish("environment/" + DeviceName + "/tvoc", String::format("%d", aqData.ccs811.data.tvoc));
      client.publish("environment/" + DeviceName + "/co2", String::format("%d", aqData.ccs811.data.c02));
    }
  }

}

// Particle device name handler
void devicename_callback(const char *topic, const char *data) {
    DeviceName = String(data);
}

// MQTT recieve message
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  // dont care...
}


// setup() runs once, when the device is first turned on.
void setup() {

  // Turn off the LED
  // RGB.control(true);
  // RGB.brightness(0);

  // Set up PC based UART (for debugging)
  Serial.blockOnOverrun(false);
  Serial.begin();

  // Set up keep alive
  Particle.keepAlive(60);
  Particle.subscribe("particle/device/name", devicename_callback, MY_DEVICES);
  Particle.publish("particle/device/name", PRIVATE);

  // Set up I2C
  Wire.setSpeed(I2C_CLK_SPEED);
  Wire.begin();

  // Default settings
  AirQualityWingSettings_t defaultSettings =
  { MEASUREMENT_DELAY_MS, //Measurement Interval
    true,                 //Has HPMA115
    true,                 //Has CCS811
    true,                 //Has Si7021
    CCS811_ADDRESS,       //CCS811 address
    CCS811_INT_PIN,       //CCS811 intpin
    CCS811_RST_PIN,       //CCS811 rst pin
    CCS811_WAKE_PIN,      //CCS811 wake pin
    HPMA1150_EN_PIN       //HPMA int pin
  };

  client.connect(Particle.deviceID());

  // Setup & Begin Air Quality
  AirQual.setup(AirQualityWingEvent, defaultSettings);
  AirQual.begin();

  AirQual.setInterval(15000);

  // Startup message
  Serial.println("Air Quality Wing for Particle Mesh");
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {

  uint32_t err_code = AirQual.process();
  if( err_code != success ) {

    switch(err_code) {
      case si7021_error:
          Particle.publish("err", "si7021" , PRIVATE, NO_ACK);
          Log.error("Error si7021");
      case ccs811_error:
          Particle.publish("err", "ccs811" , PRIVATE, NO_ACK);
          Log.error("Error ccs811");
      case hpma115_error:
          Particle.publish("err", "hpma115" , PRIVATE, NO_ACK);
          Log.error("Error hpma115");
      default:
        break;
    }

  }

}