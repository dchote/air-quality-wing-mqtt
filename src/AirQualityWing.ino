/*
 * Project Air Quality Wing Library Example
 * Description: Basic example using the Air Quality Wing for Particle Mesh
 * Author: Jared Wolff (Circuit Dojo LLC)
 * Date: 10/27/2019
 * License: GNU GPLv3
 */

#include "AirQualityWing.h"
#include "MQTT.h"
#include "board.h"

// Logger
SerialLogHandler logHandler(115200, LOG_LEVEL_ERROR, {
    { "app", LOG_LEVEL_WARN }, // enable all app messages
});

// Forward declaration of event handler
void AirQualityWingEvent();

// AirQualityWing object
AirQualityWing AirQual = AirQualityWing();

// MQTT object
MQTT client("172.18.0.5", 1883, mqtt_callback);

// Handler is called in main loop.
// Ok to run Particle.Publish
void AirQualityWingEvent() {

  Log.trace("pub");

  if (client.isConnected()) {
    String myId = System.deviceID();
    AirQualityWingData_t aqData = AirQual.getData();

    if (aqData.hpma115.hasData) {
      client.publish("environment/" + myId + "/pm25", String::format("%d", aqData.hpma115.data.pm25));
      client.publish("environment/" + myId + "/pm10", String::format("%d", aqData.hpma115.data.pm10));
    }

    if (aqData.si7021.hasData) {
      client.publish("environment/" + myId + "/temperature", String::format("%.2f", aqData.si7021.data.temperature));
      client.publish("environment/" + myId + "/humidity", String::format("%.2f", aqData.si7021.data.humidity));
    }

    if (aqData.ccs811.hasData) {
      client.publish("environment/" + myId + "/tvoc", String::format("%d", aqData.ccs811.data.tvoc));
      client.publish("environment/" + myId + "/co2", String::format("%d", aqData.ccs811.data.c02));
    }
  }

}

// MQTT ecieve message
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    char p[length + 1];
    memcpy(p, payload, length);
    p[length] = NULL;

    Serial.print( "Got a message : " );
    Serial.print( p );
}


// setup() runs once, when the device is first turned on.
void setup() {

  // Turn off the LED
  // RGB.control(true);
  // RGB.brightness(0);

  // Set up PC based UART (for debugging)
  Serial.blockOnOverrun(false);
  Serial.begin();

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

  client.connect("airqualitywing");

  // Setup & Begin Air Quality
  AirQual.setup(AirQualityWingEvent, defaultSettings);
  AirQual.begin();

  AirQual.setInterval(10000);

  // Set up keep alive
  Particle.keepAlive(60);

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