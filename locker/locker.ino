#include <LiquidCrystal.h>
#include <Servo.h>
Servo servo;

LiquidCrystal lcd_1(12, 11, 5, 4, 8, 2);

char senha_correta[] = {'4', '4', '4', '4'};
const int TAMANHO_SENHA = 4;
char senha_digitada[4] = {'*', '*', '*', '*'};
int indice_senha = 0;
const int BOTAO_DIGITO = 7;
const int BUZZER = 9;
const int BOTAO_RESET = 3;
bool estado = false;
int tentativa = 1;
bool alarme = false;
volatile bool reset_solicitado = false;
const int SERVO = 10;
const int LDR = A1;
bool alerta_ldr = false;
bool perigo_ldr = false;

// usado para detectar o momento exato em que o botao e pressionado
bool botao_digito_anterior = HIGH;

// controla se o potenciometro ja foi movimentado
bool potenciometro_mexido = false;
int valor_inicial_pot;

void cofre_fechado(){
  servo.write(0);
  lcd_1.clear();
  lcd_1.setCursor(0, 0);
  lcd_1.print("Estado: FECHADO");
  lcd_1.setCursor(0, 1);
}

void cofre_aberto(){
  lcd_1.clear();
  lcd_1.setCursor(0, 0);
  lcd_1.print("Estado: ABERTO");
  servo.write(90); // servo "abre"
}

void cofre_carregando(){
  lcd_1.clear();
  lcd_1.setCursor(0, 0);
  lcd_1.print("Estado: ...");
}

// mostra a senha digitada e o digito atual selecionado
void mostrar_tentativa(char digito_atual){ 
  lcd_1.setCursor(0, 1);
  lcd_1.print("Senha: ");
  for (int i = 0; i < TAMANHO_SENHA; i++){
    if(i == indice_senha && potenciometro_mexido){
      lcd_1.print(digito_atual);
    }
    else{
      lcd_1.print(senha_digitada[i]);
    }
    lcd_1.print(" ");
  }
  lcd_1.print("    ");
}

// reinicia a tentativa atual da senha
void resetar_senha(){
  indice_senha = 0;

  for(int i = 0; i < TAMANHO_SENHA; i++){
    senha_digitada[i] = '*';
  }

  potenciometro_mexido = false;
  valor_inicial_pot = analogRead(A0);
}

// converte o valor analogico do potenciometro em um digito
char ler_digito(){

  int potenciometro = analogRead(A0);

  if(potenciometro < 150) return '0';
  else if(potenciometro < 350) return '1';
  else if(potenciometro < 550) return '2';
  else if(potenciometro < 750) return '3';

  return '4';
}

// verifica se a senha digitada corresponde a senha correta
bool verificar_senha(){
  if(indice_senha != TAMANHO_SENHA){
    return false;
  }

  for(int i = 0; i < TAMANHO_SENHA; i++){
    if(senha_digitada[i] != senha_correta[i]){
      return false;
    }
  }

  return true;
}

void senha_correta_msg(){
  lcd_1.clear();
  lcd_1.setCursor(0, 0);
  lcd_1.print("Senha correta!");
  estado = true;
  tentativa = 1;
}

void senha_incorreta_msg(){
  lcd_1.clear();
  lcd_1.setCursor(0, 0);
  lcd_1.print("Senha incorreta!");

  if(tentativa == 3) return; // 0 tentativas restantes

  lcd_1.setCursor(0, 1);
  if(tentativa == 2){
    lcd_1.print("Resta ");
    lcd_1.print(3 - tentativa);
    lcd_1.print(" chance");
  }
  else{
    lcd_1.print("Restam ");
    lcd_1.print(3 - tentativa);
    lcd_1.print(" chances");
  }
}

bool tresTentativas(){
  if(tentativa > 3){

    if(!alarme){ // se tiver alarme, a tela n pisca por causa do .clear()
      lcd_1.clear();
      lcd_1.setCursor(0, 0);
      lcd_1.print("Limite");
      lcd_1.setCursor(0, 1);
      lcd_1.print("Atingido!");

      alarme = true;
    }

  tone(BUZZER, 880);
  delay(100);
  noTone(BUZZER);
  delay(50);
  tone(BUZZER, 660);
  delay(100);
  noTone(BUZZER);
  delay(50);

    return true;
  }

  return false;
}

void isr_reset(){ // funcao para resetar do jeito certo com a porta 3
  reset_solicitado = true;
}

void resetar_cofre(){
  lcd_1.clear();
  lcd_1.setCursor(0, 0);
  lcd_1.print("Reset concluido");
  delay(1000);

  estado = false;
  tentativa = 1;
  alarme = false;
  servo.write(0);
  alerta_ldr = false;
  perigo_ldr = false;

  noTone(BUZZER);

  resetar_senha();
  cofre_fechado();
}

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

void verificar_ldr(){
  int luz = analogRead(LDR);
  
  if(!estado && luz > 150 && luz <= 300){ // zona de alerta
    if(perigo_ldr){ // voltou do perigo pro alerta
      perigo_ldr = false;
      noTone(BUZZER);
      lcd_1.clear();
      lcd_1.setCursor(0, 0);
      lcd_1.print("Alerta!");
    }
    if(!alerta_ldr){ // normal para o alerta
      alerta_ldr = true;
      lcd_1.clear();
      lcd_1.setCursor(0, 0);
      lcd_1.print("Alerta!");
    }
  }
  
  if(!estado && luz > 300){ // zona de perigo
    if(!perigo_ldr){
      perigo_ldr = true;
      alerta_ldr = true;
      lcd_1.clear();
      lcd_1.setCursor(0, 0);
      lcd_1.print("PERIGO!");
    }
    tone(BUZZER, 880);
    delay(100);
    noTone(BUZZER);
    delay(100);
  }

  if(luz <= 150){ // luz voltou ao normal
    if(alerta_ldr || perigo_ldr){
      alerta_ldr = false;
      perigo_ldr = false;
      noTone(BUZZER);
      lcd_1.clear();
      lcd_1.setCursor(0, 0);
      lcd_1.print("Estado: FECHADO");
      lcd_1.setCursor(0, 1);
    }
  }
}

void setup(){
  delay(500);
  lcd_1.begin(16, 2);
  valor_inicial_pot = analogRead(A0);
  pinMode(BOTAO_DIGITO, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);
  pinMode(BOTAO_RESET, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BOTAO_RESET), isr_reset, FALLING);
  servo.attach(SERVO);
  servo.write(0); // comeca fechado
  cofre_fechado();
}

void loop(){

  if(reset_solicitado){
    reset_solicitado = false;
    resetar_cofre();
    delay(300);
  }

  if(tresTentativas()){
    return;
  }

  verificar_ldr();

  int potenciometro = analogRead(A0);
  char digito_potenciometro = ler_digito();

  if(!alerta_ldr){
    mostrar_tentativa(digito_potenciometro); // evitar conflito de mensagens
  }

  bool botao_digito_atual = digitalRead(BOTAO_DIGITO);

  // verifica oscilacao do potenciometro para definir o valor inicial da senha, se o valor estiver absoluto (abs) 20 de diferenca é pq foi mexido
  if(abs(potenciometro - valor_inicial_pot) > 20){
    potenciometro_mexido = true;
  }

  if(botao_digito_anterior == HIGH && botao_digito_atual == LOW){ // detecta apenas o momento do clique do botao
    if(indice_senha < TAMANHO_SENHA){
      senha_digitada[indice_senha] = digito_potenciometro;
      indice_senha++;
      potenciometro_mexido = false;
      valor_inicial_pot = analogRead(A0); // novo valor inicial do potenciometro é lido novamente
      if(indice_senha == TAMANHO_SENHA){ // verifica se todos os 4 digitos foram escritos, ai valida automaticamente

        // pra mostrar o ultimo digito antes de mostrar a mensagem de senha correta/incorreta
        mostrar_tentativa(digito_potenciometro); 
        delay(500);

        cofre_carregando();
        delay(800);

        if(verificar_senha()){
          senha_correta_msg();
          bip_sucesso();
          delay(1100);
          cofre_aberto();
        }
        else{
          bip_erro();
          senha_incorreta_msg();
          delay(1500);
          cofre_fechado();
          tentativa++;
        }
        resetar_senha();
      }
    }
  }
  botao_digito_anterior = botao_digito_atual;
}