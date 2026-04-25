// TP 2 - MFCC / FFT
#include <Arduino.h>
#include "arduinoMFCC.h"
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

// ==================  MFCC ================== //

#define MFCC_SIZE      11
#define DCT_MFCC_SIZE  6
#define FRAME_SIZE     256
#define FREQ_ECH       8000

// ==================  OLED ================== //

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET   -1
#define OLED_ADDR    0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

arduinoMFCC mymfcc(MFCC_SIZE, DCT_MFCC_SIZE, FRAME_SIZE, FREQ_ECH);

float frame[FRAME_SIZE];
float mfcc[MFCC_SIZE];
float dct_mfcc[DCT_MFCC_SIZE];

int currentXInit = 0;

// ==================  OLED SETUP ================== //

void setupOLED() {
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("SSD1306 allocation failed"));
    while(1) {}
  }
  display.clearDisplay();
  display.display();
}

// ==================  ADC SETUP ================== //

void setupADC() {
  PMC->PMC_PCER1 |= PMC_PCER1_PID37;
  ADC->ADC_MR = ADC_MR_TRGEN_DIS
              | ADC_MR_LOWRES_BITS_12
              | ADC_MR_PRESCAL(3)
              | ADC_MR_STARTUP_SUT64
              | ADC_MR_SETTLING_AST3
              | ADC_MR_TRACKTIM(15);
  ADC->ADC_CHDR = 0xFFFF;
  ADC->ADC_CHER = ADC_CHER_CH7;
}

// ==================  FILL BUFFER ================== //

void fillBuffer() {
  for(int i = 0; i < FRAME_SIZE; i++) {
    ADC->ADC_CR = ADC_CR_START;
    while((ADC->ADC_ISR & 0x80) == 0);
    frame[i] = ((float)ADC->ADC_CDR[7] - 2048.0f) / 2048.0f;
  }
}

// ==================  AFFICHAGE MFCC OLED ================== //

// Fonction disponible sur le Github arduinoMFCC
void displayMFCC(float* coeffs, int size) {
  display.clearDisplay();

  // Titre
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("MFCC");

  // Calcul de la valeur max pour normaliser l'affichage
  float maxVal = 0.001f;
  for(int i = 0; i < size; i++) {
    if(abs(coeffs[i]) > maxVal) maxVal = abs(coeffs[i]);
  }

  // Largeur de chaque barre
  int barWidth = SCREEN_WIDTH / size;  // 128 / 11 ≈ 11 px

  for(int i = 0; i < size; i++) {
    // Normaliser la hauteur de barre (max 44px, zone sous le titre)
    int barHeight = (int)(abs(coeffs[i]) / maxVal * 44);
    int x = i * barWidth;

    // Dessiner la barre depuis le bas de l'écran
    display.fillRect(x, 64 - barHeight, barWidth - 1, barHeight, WHITE);

    // Afficher la valeur sous forme tronquée au dessus de la barre
    display.setCursor(x, 10);
    display.setTextSize(1);
    display.print((int)coeffs[i]);
  }

  display.display();
}

// ==================  SETUP  ================== //

void setup() {
  Serial.begin(115200);
  setupOLED();
  setupADC();
  mymfcc.create_hamming_window();
  mymfcc.create_mel_filter_bank();
  mymfcc.create_dct_matrix();
  currentXInit = 0;
}

// ==================  Loop E13  ================== //
/*void loop() {
  fillBuffer();
  mymfcc.compute(frame, mfcc);

  // Affichage OLED
  displayMFCC(mfcc, MFCC_SIZE);

  // Affichage série
  Serial.print("MFCC: ");
  for(int i = 0; i < MFCC_SIZE; i++) {
    Serial.print(mfcc[i], 4);
    if(i < MFCC_SIZE - 1) Serial.print(", ");
  }
  Serial.println();
}*/

// ==================  Loop E14  ================== //
void loop() {
  fillBuffer();

  // Calcul MFCC pré-DCT
  mymfcc.compute(frame, mfcc);

  // Calcul MFCC post-DCT — passer frame[], pas mfcc[] !
  mymfcc.computeWithDCT(frame, dct_mfcc);

  // --- Affichage OLED : pre-DCT (haut) vs post-DCT (bas) ---
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);

  // Titre pre-DCT
  display.setCursor(0, 0);
  display.print("pre-DCT");

  // Barres pre-DCT (zone haute : y = 8 à 30)
  float maxPre = 0.001f;
  for(int i = 0; i < MFCC_SIZE; i++)
    if(fabs(mfcc[i]) > maxPre) maxPre = fabs(mfcc[i]);

  int barW = SCREEN_WIDTH / MFCC_SIZE;
  for(int i = 0; i < MFCC_SIZE; i++) {
    int h = (int)(fabs(mfcc[i]) / maxPre * 20);
    display.fillRect(i * barW, 30 - h, barW - 1, h, WHITE);
  }

  // Séparateur
  display.drawLine(0, 33, 127, 33, WHITE);

  // Titre post-DCT
  display.setCursor(0, 35);
  display.print("post-DCT");

  // Barres post-DCT (zone basse : y = 44 à 64)
  float maxPost = 0.001f;
  for(int i = 0; i < DCT_MFCC_SIZE; i++)
    if(fabs(dct_mfcc[i]) > maxPost) maxPost = fabs(dct_mfcc[i]);

  int barWdct = SCREEN_WIDTH / DCT_MFCC_SIZE;
  for(int i = 0; i < DCT_MFCC_SIZE; i++) {
    int h = (int)(fabs(dct_mfcc[i]) / maxPost * 20);
    display.fillRect(i * barWdct, 64 - h, barWdct - 1, h, WHITE);
  }

  display.display();

  // Affichage série comparatif
  Serial.print("pre-DCT:  ");
  for(int i = 0; i < MFCC_SIZE; i++) {
    Serial.print(mfcc[i], 4);
    if(i < MFCC_SIZE - 1) Serial.print(", ");
  }
  Serial.println();

  Serial.print("post-DCT: ");
  for(int i = 0; i < DCT_MFCC_SIZE; i++) {
    Serial.print(dct_mfcc[i], 4);
    if(i < DCT_MFCC_SIZE - 1) Serial.print(", ");
  }
  Serial.println();
  Serial.println("---");
}