#include <LiquidCrystal.h>

LiquidCrystal lcd_1(12, 11, 5, 4, 3, 2);

void setup(){
  lcd_1.begin(16, 2);
}

void loop(){
  int potenciometro = analogRead(A0);
  int digito;
  
  // transforma o valor do potenciometro em digitos de 0 a 3
  if (potenciometro < 150) digito = 0;
  else if (potenciometro < 350) digito = 1;
  else if (potenciometro < 550) digito = 2;
  else if (potenciometro < 750) digito = 3;
  else digito = 4;

  // Linha 0: Faixa
  lcd_1.setCursor(0, 0);
  lcd_1.print("Digito: ");
  lcd_1.print(digito);
  lcd_1.print("   "); // apaga resíduo

}