#include <Arduino.h>
#define OLEDDISPLAY
#include <qrcodeoled.h>
#include <SSD1306.h>

SSD1306 display(0x3c, 2, 15);
QRcodeOled qrcode(&display);

bool qrCodeVisivel = true;

const int botaoAutenticacao = 17;
const int leituraswitchPin = 23;
const int estacaoSwitchPin = 13;
const int redPin = 4;
const int greenPin = 0;
const int buzzerPin = 16;
const int trigPin = 19;
const int echoPin = 18;
const int pirPin = 22;

bool patineteDestravado = false;
bool patineteCarregando = false;
bool estacaoIncorretaAtiva = false;
bool somEstacaoIncorretaAtivo = false;
bool pirDetectado = false;
bool botaoPressionado = false;
unsigned long ultimoTrigger = 0;
unsigned long ultimoTriggerPIR = 0;
unsigned long tempoEstacaoIncorreta = 0;
unsigned long tempoSomEstacaoIncorreta = 0;
const unsigned long cooldown = 5000;
const unsigned long cooldownPIR = 5000;
const unsigned long duracaoEstacaoIncorreta = 3000;
const unsigned long duracaoSomEstacaoIncorreta = 1000;

const float LAT_INICIAL = -3.742221f;
const float LNG_INICIAL = -38.535461f;
volatile float latitude = LAT_INICIAL;
volatile float longitude = LNG_INICIAL;

int distanciaReferencia = 0;

float direcaoLat = 1.0f;
float direcaoLng = 1.0f;

bool alarmeAtivo = false;
unsigned long ultimoSomAlarme = 0;
const unsigned long intervaloAlarme = 50;
unsigned long ultimoTempoAlarme = 0;
const unsigned long tempoMinimoAlarme = 5000;

unsigned long ultimoUpdateGPS = 0;
const unsigned long intervaloGPS = 5000;

void mostrarTelaInicial();
void atualizarTelaOLED(const char* mensagem = "");
void lcdLeituraValida();
void lcdLeituraInvalida();
void gerenciarAlarme();
void mostrarTelaCarregando();
void mostrarEstacaoIncorreta();
int lerDistanciaUltrassonico();
void atualizarPosicao();
void exibirCoordenadas();
void desativarAlarme();

void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(34));
  
  display.init();
  display.clear();
  display.display();
  
  qrcode.debug();
  qrcode.init();
  
  mostrarTelaInicial();
  
  pinMode(botaoAutenticacao, INPUT_PULLUP);
  pinMode(leituraswitchPin, INPUT_PULLUP);
  pinMode(estacaoSwitchPin, INPUT_PULLUP);
  pinMode(pirPin, INPUT);
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  digitalWrite(redPin, LOW);
  digitalWrite(greenPin, LOW);
  digitalWrite(trigPin, LOW);
  noTone(buzzerPin);
  
  pirDetectado = false;
  ultimoTriggerPIR = 0;
}

void loop() {
  if (estacaoIncorretaAtiva) {
    unsigned long tempoAtual = millis();
    
    if (somEstacaoIncorretaAtivo && (tempoAtual - tempoSomEstacaoIncorreta >= duracaoSomEstacaoIncorreta)) {
      digitalWrite(redPin, LOW);
      noTone(buzzerPin);
      somEstacaoIncorretaAtivo = false;
    }
    
    if (tempoAtual - tempoEstacaoIncorreta >= duracaoEstacaoIncorreta) {
      estacaoIncorretaAtiva = false;
      somEstacaoIncorretaAtivo = false;
      digitalWrite(redPin, LOW);
      noTone(buzzerPin);
      exibirCoordenadas();
      pirDetectado = false;
    } else {
      mostrarEstacaoIncorreta();
      return;
    }
  }

  if (patineteCarregando) {
    mostrarTelaCarregando();
    return;
  }

  if (patineteDestravado) {
    unsigned long tempoAtual = millis();
    
    if (alarmeAtivo) {
      int distanciaAtual = lerDistanciaUltrassonico();
      if (distanciaAtual >= 0) {
        int variacaoCM = distanciaAtual - distanciaReferencia;
        float deslocamentoMetros = abs(variacaoCM) * 2.0f;
        
        if (deslocamentoMetros <= 200.0f && (tempoAtual - ultimoTempoAlarme >= tempoMinimoAlarme)) {
          desativarAlarme();
          atualizarPosicao();
          return;
        }
      }
      
      gerenciarAlarme();
      return;
    }
    
    if (!pirDetectado && (tempoAtual - ultimoTriggerPIR > cooldownPIR)) {
      int estadoPIR = digitalRead(pirPin);
      if (estadoPIR == HIGH) {
        pirDetectado = true;
        ultimoTriggerPIR = tempoAtual;
        
        int estadoEstacao = digitalRead(estacaoSwitchPin);
        
        if (estadoEstacao == HIGH) {
          estacaoIncorretaAtiva = true;
          somEstacaoIncorretaAtivo = true;
          tempoEstacaoIncorreta = tempoAtual;
          tempoSomEstacaoIncorreta = tempoAtual;
          Serial.println("[ESTAÇÃO] Tentativa de estacionamento em ESTAÇÃO INCORRETA");
        } else {
          tone(buzzerPin, 1500);
          delay(250);
          noTone(buzzerPin);
          
          digitalWrite(redPin, LOW);
          digitalWrite(greenPin, LOW);
          
          patineteCarregando = true;
          patineteDestravado = false;
          Serial.println("[ESTAÇÃO] Patinete estacionado na ESTAÇÃO CORRETA");
          return;
        }
      }
    }
    
    atualizarPosicao();
    return;
  }

  pirDetectado = false;
  
  int estadoBotao = digitalRead(botaoAutenticacao);
  unsigned long tempoAtual = millis();

  if (estadoBotao == LOW && !botaoPressionado && (tempoAtual - ultimoTrigger > cooldown)) {
    botaoPressionado = true;
    ultimoTrigger = tempoAtual;
    
    int estadoSwitch = digitalRead(leituraswitchPin);
    
    if (estadoSwitch == LOW) {
      lcdLeituraValida();
      delay(2000);
      
      distanciaReferencia = lerDistanciaUltrassonico();
      if (distanciaReferencia < 0) distanciaReferencia = 0;
      
      latitude = LAT_INICIAL;
      longitude = LNG_INICIAL;
      alarmeAtivo = false;
      
      direcaoLat = (random(2) == 0) ? -1.0f : 1.0f;
      direcaoLng = (random(2) == 0) ? -1.0f : 1.0f;
      
      qrCodeVisivel = false;
      patineteDestravado = true;
      
      Serial.print("[GPS] Referência inicial: ");
      Serial.print(distanciaReferencia);
      Serial.println("cm");
      
      exibirCoordenadas();
    } 
    else {
      lcdLeituraInvalida();
      delay(2000);
      
      qrCodeVisivel = true;
    }
  }
  else if (estadoBotao == HIGH && botaoPressionado) {
    botaoPressionado = false;
  }
  
  if (alarmeAtivo) {
    gerenciarAlarme();
  }
  
  delay(10);
}

void mostrarTelaInicial() {
  display.clear();
  
  qrcode.create("Hello world.");
  
  display.display();
  delay(2000);
}

void atualizarTelaOLED(const char* mensagem) {
  if (qrCodeVisivel) {
  } 
  else {
    display.clear();
    display.setFont(ArialMT_Plain_16);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 25, mensagem);
  }
  
  display.display();
}

void lcdLeituraValida() {
  digitalWrite(greenPin, HIGH);
  tone(buzzerPin, 1000);
  delay(250);
  digitalWrite(greenPin, LOW);
  noTone(buzzerPin);
  delay(500);
  digitalWrite(greenPin, HIGH);
  tone(buzzerPin, 1000);
  delay(250);
  digitalWrite(greenPin, LOW);
  noTone(buzzerPin);
}

void lcdLeituraInvalida() {
  digitalWrite(redPin, HIGH);
  tone(buzzerPin, 250);
  delay(1000);
  digitalWrite(redPin, LOW);
  noTone(buzzerPin);
}

void gerenciarAlarme() {
  display.clear();
  
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 10, "ALERTA!");
  display.drawString(64, 25, "Fora da area");
  display.drawString(64, 35, "segura!");
  display.drawString(64, 50, "Retorne!");
  
  display.display();
  
  unsigned long tempoAtual = millis();
  if (tempoAtual - ultimoSomAlarme >= intervaloAlarme) {
    ultimoSomAlarme = tempoAtual;
    static bool tomAlto = true;
    tone(buzzerPin, tomAlto ? 2200 : 1200);
    digitalWrite(redPin, tomAlto ? HIGH : LOW);
    tomAlto = !tomAlto;
  }
}

void desativarAlarme() {
  alarmeAtivo = false;
  digitalWrite(redPin, LOW);
  noTone(buzzerPin);
  
  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 20, "Area segura");
  display.drawString(64, 40, "retomada!");
  display.display();
  
  delay(1500);
  Serial.println("[ALERTA] Alarme desativado - patinete voltou para area segura");
}

void mostrarTelaCarregando() {
  display.clear();
  
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 20, "Patinete");
  display.drawString(64, 40, "carregando...");
  
  display.display();
  
  digitalWrite(redPin, HIGH);
  digitalWrite(greenPin, HIGH);
  noTone(buzzerPin);
}

void mostrarEstacaoIncorreta() {
  display.clear();
  
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 20, "Estação incorreta!");
  
  display.display();
  
  if (somEstacaoIncorretaAtivo) {
    digitalWrite(redPin, HIGH);
    tone(buzzerPin, 500);
  }
}

int lerDistanciaUltrassonico() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duracao = pulseIn(echoPin, HIGH, 30000);
  if (duracao == 0) return -1;
  
  float distancia = duracao * 0.034 / 2;
  return (distancia >= 2 && distancia <= 400) ? (int)distancia : -1;
}

void atualizarPosicao() {
  unsigned long tempoAtual = millis();
  if (tempoAtual - ultimoUpdateGPS < intervaloGPS) return;
  
  ultimoUpdateGPS = tempoAtual;
  int distanciaAtual = lerDistanciaUltrassonico();
  
  if (distanciaAtual < 0) {
    Serial.println("[GPS] Leitura inválida do sensor");
    return;
  }
  
  int variacaoCM = distanciaAtual - distanciaReferencia;
  float deslocamentoMetros = abs(variacaoCM) * 2.0f;
  
  const float METRO_PARA_GRAU_LAT = 0.000008983f;
  const float METRO_PARA_GRAU_LNG = 0.000012213f;
  
  latitude = LAT_INICIAL + (deslocamentoMetros * direcaoLat * METRO_PARA_GRAU_LAT);
  longitude = LNG_INICIAL + (deslocamentoMetros * direcaoLng * METRO_PARA_GRAU_LNG);
  
  latitude = constrain(latitude, -4.0f, -3.5f);
  longitude = constrain(longitude, -38.6f, -38.4f);
  
  if (deslocamentoMetros > 200.0f && !alarmeAtivo) {
    Serial.print("\n[ALERTA] Deslocamento excedido: ");
    Serial.print(deslocamentoMetros);
    Serial.println(" metros");
    alarmeAtivo = true;
    ultimoTempoAlarme = millis();
  }
  
  if (!alarmeAtivo && !estacaoIncorretaAtiva) {
    exibirCoordenadas();
  }
  
  Serial.print("[GPS] Deslocamento: ");
  Serial.print(deslocamentoMetros, 0);
  Serial.print("m | LAT:");
  Serial.print(latitude, 6);
  Serial.print(" LNG:");
  Serial.println(longitude, 6);
}

void exibirCoordenadas() {
  display.clear();
  
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  
  char latStr[15];
  dtostrf(latitude, 9, 6, latStr);
  char latDisplay[12];
  strncpy(latDisplay, latStr, 11);
  latDisplay[11] = '\0';
  
  char lngStr[15];
  dtostrf(longitude, 10, 6, lngStr);
  char lngDisplay[12];
  strncpy(lngDisplay, lngStr, 11);
  lngDisplay[11] = '\0';
  
  display.drawString(0, 10, "LAT:");
  display.drawString(40, 10, latDisplay);
  
  display.drawString(0, 30, "LNG:");
  display.drawString(40, 30, lngDisplay);
  
  display.display();
}