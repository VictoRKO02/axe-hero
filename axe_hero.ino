#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_NeoPixel.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

const int interrupcaoOR = 2;
const int botao1 = 3;
const int botao2 = 4;
const int botao3 = 5;
const int botao4 = 9;
const int chaveLiga = 6;
const int ledVerde = 7;
const int ledVermelho = 8;
const int buzzer = A0;

#define LEDS_POR_FITA 8
#define PIN_FITA1 10
#define PIN_FITA2 11
#define PIN_FITA3 12
#define PIN_FITA4 13

Adafruit_NeoPixel fita1(LEDS_POR_FITA, PIN_FITA1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel fita2(LEDS_POR_FITA, PIN_FITA2, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel fita3(LEDS_POR_FITA, PIN_FITA3, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel fita4(LEDS_POR_FITA, PIN_FITA4, NEO_GRB + NEO_KHZ800);

#define REST 0
#define NOTE_C4 262
#define NOTE_D4 294
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_G4 392
#define NOTE_A4 440
#define NOTE_B4 494
#define NOTE_C5 523
#define NOTE_D5 587
#define NOTE_E5 659
#define NOTE_F5 698
#define NOTE_G5 784
#define NOTE_A5 880

struct NotaMusical {
  byte trilha;
  int freq;
  byte div;
};

struct NotaAtiva {
  bool ativa;
  byte trilha;
  int posicao;
  int freq;
};

#define MAX_NOTAS_ATIVAS 10
NotaAtiva notasAtivas[MAX_NOTAS_ATIVAS];

NotaMusical musicaFarao[] = {
  {1, NOTE_C4, 8}, {2, NOTE_E4, 8}, {1, NOTE_C4, 8}, {3, NOTE_G4, 8},
  {2, NOTE_E4, 8}, {4, NOTE_C5, 8}, {3, NOTE_G4, 8}, {2, NOTE_E4, 8},
  {1, NOTE_C4, 8}, {2, NOTE_E4, 8}, {3, NOTE_G4, 8}, {4, NOTE_C5, 8},
  {4, NOTE_C5, 8}, {3, NOTE_G4, 8}, {2, NOTE_E4, 8}, {1, NOTE_C4, 8}
};

NotaMusical musicaBaianidade[] = {
  {2, NOTE_E4, 8}, {3, NOTE_G4, 8}, {2, NOTE_E4, 8}, {1, NOTE_C4, 8},
  {4, NOTE_C5, 8}, {3, NOTE_G4, 8}, {2, NOTE_E4, 8}, {1, NOTE_C4, 8}
};

NotaMusical musicaSelvaBranca[] = {
  {3, NOTE_G4, 8}, {2, NOTE_E4, 8}, {1, NOTE_C4, 8}, {2, NOTE_E4, 8},
  {4, NOTE_C5, 8}, {3, NOTE_G4, 8}, {2, NOTE_E4, 8}, {1, NOTE_C4, 8}
};

volatile bool eventoBotao = false;

bool sistemaLigado = false;
bool jogoAtivo = false;
bool musicaSelecionada = false;
bool dificuldadeSelecionada = false;

int estadoAnteriorChave = HIGH;
int musicaEscolhida = 0;
String nomeMusica = "";
String dificuldade = "Facil";

int pontuacao = 0;
int vidas = 3;
int recorde = 0;
int comboAtual = 0;

int indiceProximaNota = 0;
int totalNotasMusica = 0;
int tempoMusica = 75;
int wholenote = 0;

unsigned long velocidadeQueda = 420;
unsigned long proximoSpawn = 0;
unsigned long ultimoMovimento = 0;
unsigned long ultimoLCD = 0;
unsigned long ultimoDano = 0;

const unsigned long intervaloProtecaoDano = 900;
bool fimSequencia = false;

const int HIT_MIN = LEDS_POR_FITA - 2;
const int HIT_MAX = LEDS_POR_FITA - 1;

void setup() {
  pinMode(interrupcaoOR, INPUT);
  pinMode(chaveLiga, INPUT_PULLUP);
  pinMode(ledVerde, OUTPUT);
  pinMode(ledVermelho, OUTPUT);
  pinMode(botao1, INPUT);
  pinMode(botao2, INPUT);
  pinMode(botao3, INPUT);
  pinMode(botao4, INPUT);
  pinMode(buzzer, OUTPUT);

  fita1.begin();
  fita2.begin();
  fita3.begin();
  fita4.begin();

  apagarLedsNotas();

  attachInterrupt(digitalPinToInterrupt(interrupcaoOR), interrupcaoBotoes, RISING);

  lcd.init();
  lcd.backlight();

  estadoInicialDesligado();
}

void loop() {
  int estadoChave = digitalRead(chaveLiga);

  if (estadoChave == LOW && estadoAnteriorChave == HIGH) ligarSistema();
  if (estadoChave == HIGH && estadoAnteriorChave == LOW) desligarSistema();

  estadoAnteriorChave = estadoChave;

  if (estadoChave == HIGH) return;

  if (eventoBotao) {
    eventoBotao = false;
    delay(25);

    byte botoes = lerBotoesPressionados();
    int botaoMenu = primeiroBotaoPressionado(botoes);

    if (botoes != 0) {
      if (!musicaSelecionada) selecionarMusica(botaoMenu);
      else if (!dificuldadeSelecionada) selecionarDificuldade(botaoMenu);
      else if (jogoAtivo) verificarJogadasSimultaneas(botoes);
    }
  }

  if (jogoAtivo) atualizarJogoPianoTiles();
}

void interrupcaoBotoes() {
  eventoBotao = true;
}

byte lerBotoesPressionados() {
  byte botoes = 0;
  if (digitalRead(botao1) == HIGH) botoes |= 1;
  if (digitalRead(botao2) == HIGH) botoes |= 2;
  if (digitalRead(botao3) == HIGH) botoes |= 4;
  if (digitalRead(botao4) == HIGH) botoes |= 8;
  return botoes;
}

int primeiroBotaoPressionado(byte botoes) {
  if (botoes & 1) return 1;
  if (botoes & 2) return 2;
  if (botoes & 4) return 3;
  if (botoes & 8) return 4;
  return 0;
}