bool breathe_AC(bool breath) {
  
  pressurefunction();
   
  Serial.println("Breathing in AC MODE!");
  
  if (breath == true ) {
    Serial.println("breathing");
    // Inhale
    previous = millis (); // determine time at start of pumping air in
    digitalWrite(airvalve, HIGH);
    digitalWrite(vacvalve, LOW);
    digitalWrite(red4, HIGH);
    digitalWrite(red3, LOW);
    v_calc = 0; // start volume pumped in is 0
    time_breath = 0; // amount of time breathing is 0 so far
    int i=0;
    while (v_calc < v_set) {
      flowrate = sensor.flow()/60; // get flowrate from sensor and convert from mL/min to mL/s
      current = millis(); // get current time
      time_elapsed = current - previous; // determine time in milliseconds from beginning to pump air to now
      v_trans = flowrate * time_elapsed * 0.001; // determine volume pumped in this time, flow is in mL/s, time_elapsed is in ms, have to multiply by 0.001 to get s
      v_calc = v_calc + v_trans; // add to counter of total volume pumped
      time_breath = time_breath + time_elapsed; // add to counter of how long it took to breathe in
    }
    // Hold
    digitalWrite(airvalve, LOW);
    digitalWrite(vacvalve, LOW);
    digitalWrite(red4, LOW);
    digitalWrite(red3, LOW);
    breathlength_hold = breathlength_in - time_breath; // calculate how long left in inhalation to maintain proper IE ratio
    if (breathlength_hold < 0) {
      // if inhalation time too long to maintain IE ratio, remove hold time and provide warning.
      Serial.println("Not enough flow to reach set volume. Please increase airflow or decrease set volume.");
      breathlength_hold = 0; 
    }
    delay(breathlength_hold);
    // Exhale
    Serial.println("Exhale");
    digitalWrite(airvalve, LOW);
    digitalWrite(vacvalve, HIGH);
    digitalWrite(red4, LOW);
    digitalWrite(red3, HIGH);
    delay(breathlength_out);
  } else if (breath == false)  {
    // Waiting for breath, all pumps off
    Serial.println("wait");
    digitalWrite(airvalve, LOW);
    digitalWrite(vacvalve, LOW);
    digitalWrite(red3, LOW);
    digitalWrite(red4, LOW);
    digitalWrite(red1, LOW);
    digitalWrite(red2, LOW);
    delay(150);
  }

}
