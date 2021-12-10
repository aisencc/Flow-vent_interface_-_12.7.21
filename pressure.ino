void pressurefunction(){
  // READ PRESSURE SENSOR
  pressure = mpr.readPressure();
  KPSI = (pressure / 6.89476); // convert from kPa to PSI? why is this labelled kPSI? the conversion value is definitely for kPA to PSI
  delay(100);
  }
