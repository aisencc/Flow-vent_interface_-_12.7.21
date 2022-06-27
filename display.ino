void screen() {


// REFRESH LCD SCREEN
  lcd.clear();
  digitalWrite(green, HIGH);

  // NOTE: should this section go after the pots are read? when this first starts, potIE, potVol, and potBPM haven't been initialized yet. It's not a problem because this loops but just seems odd
  lcd.setCursor(0, 0);
  lcd.print("P:");
  lcd.print(pressure);
  lcd.setCursor(9, 0);
  lcd.print("I/E:");
  lcd.print(Ex);
  lcd.setCursor(0, 2);
  lcd.print("Vol:");
  lcd.print(Vol);
  lcd.setCursor(9, 2);
  lcd.print("BPM:");
  lcd.print(BPM);


}
