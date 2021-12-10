void flowratefunction() {
  // READ FLOWRATE SENSOR
  // the sensor returns 0 when new data is ready
  if ( sensor.readSensor() == 0 ) {
    normalize = sensor.flow() * -1;
    Serial.print( "Flow rate: " );
    Serial.print( (sensor.flow() + normalize) * 3700);
    Serial.println( " [SCCM]" ); // standard cubic centimeters per minute, equivalent to mL/min
  }
  delay(100); // Slow down sampling to 10 Hz. This is just a test.
}
