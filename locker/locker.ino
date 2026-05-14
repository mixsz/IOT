#include <LiquidCrystal.h>
#include <Servo.h>

enum Estado{
  TRANCADO,
  SENHA,
  ABERTO,
  ALERTA
};

Estado estado_atual = TRANCADO;

void entrar_estado(Estado novo);

// hardware
Servo servo;
LiquidCrystal lcd_1(12, 11, 5, 4, 8, 2);

const int BOTAO_DIGITO = 7;
const int BUZZER = 9;
const int BOTAO_RESET = 3;
const int SERVO_PIN = 10;
const int LDR = A1;

// variaveis de controle
char senha_correta[] = {'4', '4', '4', '4'};
const int TAMANHO_SENHA = 4;
char senha_digitada[4] = {'*', '*', '*', '*'};
int  indice_senha = 0;
int  tentativa = 1;
bool alarme_exibido = false;
bool arrombado = false;

bool botao_digito_anterior = HIGH;
bool potenciometro_mexido = false;
int  valor_inicial_pot = 0;

volatile bool reset_solicitado = false;

void isr_reset(){ // funcao de interrupcao do botao reset
  reset_solicitado = true;
}

// funcoes para manipular a senha
char ler_digito(){
  int v = analogRead(A0);
  if(v < 150) return '0';
  if(v < 350) return '1';
  if(v < 550) return '2';
  if(v < 750) return '3';
  return '4';
}

bool verificar_senha(){
  if(indice_senha != TAMANHO_SENHA) return false;
  for(int i = 0; i < TAMANHO_SENHA; i++){
    if(senha_digitada[i] != senha_correta[i]) return false;
  }
  return true;
}

void resetar_senha(){
  indice_senha = 0;
  for(int i = 0; i < TAMANHO_SENHA; i++) senha_digitada[i] = '*';
  potenciometro_mexido = false;
  valor_inicial_pot = analogRead(A0);
}

void mostrar_tentativa(char digito_atual){
  lcd_1.setCursor(0, 1);
  lcd_1.print("Senha: ");
  for(int i = 0; i < TAMANHO_SENHA; i++){
    if(i == indice_senha && potenciometro_mexido)
      lcd_1.print(digito_atual);
    else
      lcd_1.print(senha_digitada[i]);
    lcd_1.print(" ");
  }
  lcd_1.print("    ");
}

// sons
void bip_sucesso(){
  tone(BUZZER, 330, 150);
  delay(180);
  tone(BUZZER, 392, 150);
  delay(180);
  tone(BUZZER, 523, 400);
}

void bip_erro(){
  tone(BUZZER, 294, 300);
  delay(350);
  tone(BUZZER, 262, 500);
}

void bip_alerta(){
  tone(BUZZER, 880);
  delay(100);
  noTone(BUZZER);
  delay(50);
  tone(BUZZER, 660);
  delay(100);
  noTone(BUZZER);
  delay(50);
}

void bip_perigo(){
  tone(BUZZER, 880);
  delay(100);
  noTone(BUZZER);
  delay(100);
}

// acao de entrada pra cada estado
void entrar_estado(Estado novo){
  estado_atual = novo;

  switch(novo){

    case TRANCADO:
      servo.write(0);
      lcd_1.clear();
      lcd_1.setCursor(0, 0);
      lcd_1.print("Estado: FECHADO");
      resetar_senha();
      break;

    case SENHA:
      break;

    case ABERTO:
      lcd_1.clear();
      lcd_1.setCursor(0, 0);
      lcd_1.print("Estado: ABERTO");
      servo.write(90);
      break;

    case ALERTA:
      alarme_exibido = false;
      break;
  }
}

void processar_trancado_senha(){
  int pot = analogRead(A0);
  char digito = ler_digito();
  bool botao_atual = digitalRead(BOTAO_DIGITO);

  // verifica oscilacao do potenciometro para definir o valor inicial da senha, se o valor estiver absoluto (abs) 20 de diferenca é pq foi mexido
  if(abs(pot - valor_inicial_pot) > 20)  
    potenciometro_mexido = true;

  mostrar_tentativa(digito);

  // incrementa um digito na senha
  if(botao_digito_anterior == HIGH && botao_atual == LOW){
    senha_digitada[indice_senha] = digito;
    indice_senha++;
    potenciometro_mexido = false;
    valor_inicial_pot = analogRead(A0); // valor inicial do potenciometro é lido novamente

    if(indice_senha == TAMANHO_SENHA){

      // pra mostrar o ultimo digito antes de mostrar a mensagem de senha correta/incorreta
      mostrar_tentativa(digito);
      delay(500);

      // validacao apos ser digitado os 4 digitos
      lcd_1.clear();
      lcd_1.setCursor(0, 0);
      lcd_1.print("Verificando...");
      delay(800);

      if(verificar_senha()){
        lcd_1.clear();
        lcd_1.setCursor(0, 0);
        lcd_1.print("Senha correta!");
        tentativa = 1;
        bip_sucesso();
        delay(1100);
        entrar_estado(ABERTO);

      }
      else{
        bip_erro();
        lcd_1.clear();
        lcd_1.setCursor(0, 0);
        lcd_1.print("Senha incorreta!");
        tentativa++;

        if(tentativa > 3){
          delay(1500);
          entrar_estado(ALERTA);
        }
        else{
          lcd_1.setCursor(0, 1);
          int restantes = 3 - (tentativa - 1);
          if(restantes == 1)
            lcd_1.print("Resta 1 chance");
          else{
            lcd_1.print("Restam ");
            lcd_1.print(restantes);
            lcd_1.print(" chances");
          }
          delay(1500);
          entrar_estado(TRANCADO);
        }
      }
    }
    else{
      entrar_estado(SENHA);
    }
  }

  botao_digito_anterior = botao_atual;
}

void processar_aberto(){
  botao_digito_anterior = digitalRead(BOTAO_DIGITO); // sem transicao de saida, so com reset
}

void processar_alerta(){
  if(!alarme_exibido){ // se o alarme n tiver sido exibido ainda, ele verifica se o perigo foi por arrombamento ou tentativa
    lcd_1.clear();
    if(arrombado){
      lcd_1.setCursor(0, 0); lcd_1.print("COFRE");
      lcd_1.setCursor(0, 1); lcd_1.print("ARROMBADO!");
    }
    else{
      lcd_1.setCursor(0, 0); lcd_1.print("Limite");
      lcd_1.setCursor(0, 1); lcd_1.print("Atingido!");
    }
    alarme_exibido = true;
  }
  if(arrombado) bip_perigo();
  else bip_alerta();
  // so sai por reset
}

void verificar_ldr(){
  if(estado_atual == TRANCADO || estado_atual == SENHA){
    int luz = analogRead(LDR);
    if(luz > 290){
      arrombado = true;
      entrar_estado(ALERTA);
    }
  }
}

void processar_reset(){ // volta p origem
  noTone(BUZZER);
  tentativa = 1;
  alarme_exibido = false;
  arrombado = false;

  lcd_1.clear();
  lcd_1.setCursor(0, 0);
  lcd_1.print("Reset concluido");
  delay(1000);

  entrar_estado(TRANCADO);
}

void setup(){
  delay(500);
  lcd_1.begin(16, 2);
  valor_inicial_pot = analogRead(A0);

  pinMode(BOTAO_DIGITO, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);
  pinMode(BOTAO_RESET, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(BOTAO_RESET), isr_reset, FALLING);

  servo.attach(SERVO_PIN);
  entrar_estado(TRANCADO); // comeca trancado
}

void loop(){

  // reset prioridade maxima
  if(reset_solicitado){
    reset_solicitado = false;
    processar_reset();
    return;
  }

  // verifica arrombamento
  verificar_ldr();

  // verifica o estado se nao estiver arrombado (trancado, senha, aberto, alerta)
  switch(estado_atual){
    case TRANCADO:
    case SENHA:
      processar_trancado_senha();
      break;

    case ABERTO:
      processar_aberto();
      break;

    case ALERTA:
      processar_alerta();
      break;
  }
}