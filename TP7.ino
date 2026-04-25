// TP 2 - MFCC / FFT 
// FFT + Affichage OLED

#include <Arduino.h>
#include <arduinoFFT.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

// ==================  FFT  ==================

#define BUFFER_SIZE     256        // Nombre d'échantillons (puissance de 2)
#define SAMPLING_FREQ   8000       // Fréquence d'échantillonnage en Hz
#define FFT_SIZE        (BUFFER_SIZE / 2)  // Bins utiles après FFT (symétrie)

double vReal[BUFFER_SIZE];
double vImag[BUFFER_SIZE];

// Indice de l'échantillon courant (rempli par l'ISR)
volatile int sampleIndex = 0;
volatile bool bufferReady = false;

// ==================  OLED ==================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ==================  ISR TC0  ==================

void TC0_Handler() {
  // Lire le registre de statut pour acquitter l'interruption
  TC0->TC_CHANNEL[0].TC_SR;

  if (!bufferReady && sampleIndex < BUFFER_SIZE) {
    // Lancer une conversion ADC
    ADC->ADC_CR = ADC_CR_START;
    // Attendre la fin de conversion
    while ((ADC->ADC_ISR & ADC_ISR_EOC7) == 0);
    // Lire la valeur et centrer autour de 0 (valeur 12 bits : 0-4095, centre = 2048)
    vReal[sampleIndex] = (double)(ADC->ADC_CDR[7]) - 2048.0;
    vImag[sampleIndex] = 0.0;
    sampleIndex++;

    if (sampleIndex >= BUFFER_SIZE) {
      bufferReady = true;
      sampleIndex = 0;
    }
  }
}

// ==================  SETUP OLED  ==================

void setupOLED() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("SSD1306 allocation failed, check screen address or connection"));
    while (1) {}
  }
  display.clearDisplay();
  display.display();
}

// ==================  SETUP ADC  ==================

void setupADC() {
  // 1 - Activer le périphérique ADC (PID37 dans PMC_PCER1)
  PMC->PMC_PCER1 |= PMC_PCER1_PID37;

  // 2 - Configurer l'ADC dans ADC_MR
  ADC->ADC_MR = ADC_MR_TRGEN_DIS         // Déclenchement logiciel
              | ADC_MR_LOWRES_BITS_12     // Résolution 12 bits
              | ADC_MR_PRESCAL(3)         // Prescaler = 3
              | ADC_MR_STARTUP_SUT64      // Startup = 64 périodes ADC_CLK
              | ADC_MR_SETTLING_AST3      // Stabilisation = 17 périodes ADC_CLK
              | ADC_MR_TRACKTIM(15);      // Suivi = 16 périodes ADC_CLK

  // 3 - Sélectionner uniquement le canal 7 (broche A0)
  ADC->ADC_CHDR = 0xFFFF;
  ADC->ADC_CHER = ADC_CHER_CH7;

  // 4 - Activer le périphérique TC0 (PID27 dans PMC_PCER0)
  PMC->PMC_PCER0 |= PMC_PCER0_PID27;

  // 5 - Configurer TC0 Channel 0 en mode waveform, reset sur RC
  TC0->TC_CHANNEL[0].TC_CMR = TC_CMR_TCCLKS_TIMER_CLOCK4  // Horloge = MCK/128
                             | TC_CMR_CPCTRG;               // Reset automatique sur RC

  // 6 - RC pour SAMPLING_FREQ Hz : RC = MCK / (128 * SAMPLING_FREQ)
  // Ex : 84 000 000 / (128 * 8000) = 82
  TC0->TC_CHANNEL[0].TC_RC = VARIANT_MCK / (128 * SAMPLING_FREQ);

  // 7 - Activer l'interruption sur comparaison RC (CPCS)
  TC0->TC_CHANNEL[0].TC_IER = TC_IER_CPCS;
  TC0->TC_CHANNEL[0].TC_IDR = ~TC_IER_CPCS;

  // 8 - Activer TC0_IRQn dans le NVIC
  NVIC_EnableIRQ(TC0_IRQn);

  // 9 - Démarrer le timer
  TC0->TC_CHANNEL[0].TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG;
}

// ==================  FILL BUFFER  ==================

void fillBuffer() {
  // Réinitialiser et attendre que l'ISR remplisse le buffer
  sampleIndex = 0;
  bufferReady = false;

  // Attente active jusqu'à ce que le buffer soit plein
  while (!bufferReady);
}

// ==================  AFFICHAGE  ==================

int barLength(double d) {
  float fy;
  int y;
  fy = 10.0 * (log10(d) + 1.1);
  y = fy;
  y = constrain(y, 0, 56 + 30) - 30;
  return y;
}

void showSpectrum() {
  double maxValue = 0;
  int peak_freq = 0;

  int displayFFTvalue[FFT_SIZE];

  for (int i = 1; i < FFT_SIZE; i++) {   // i=0 ignoré (composante DC)
    if (vReal[i] > maxValue) {
      maxValue = vReal[i];
      // Fréquence du bin i : f = i * SAMPLING_FREQ / BUFFER_SIZE
      peak_freq = (int)(i * (double)SAMPLING_FREQ / BUFFER_SIZE);
    }
  }

  for (int i = 0; i < FFT_SIZE; i++) {
    displayFFTvalue[i] = barLength(vReal[i]);
  }

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("PEAK: ");
  display.print(peak_freq);
  display.print(" Hz");

  // Affichage du spectre : chaque bin occupe (128 / FFT_SIZE) pixels en largeur
  float barWidth = (float)SCREEN_WIDTH / FFT_SIZE;
  for (int i = 0; i < FFT_SIZE; i++) {
    int x = (int)(i * barWidth);
    int w = max(1, (int)barWidth);
    display.fillRect(x, SCREEN_HEIGHT - displayFFTvalue[i],
                     w, displayFFTvalue[i], WHITE);
  }
}

// ==================  SETUP & LOOP  ==================

void setup() {
  Serial.begin(9600);
  setupOLED();
  setupADC();
}

void loop() {
  fillBuffer();

  ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, BUFFER_SIZE, SAMPLING_FREQ);
  FFT.dcRemoval();
  FFT.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.compute(vReal, vImag, BUFFER_SIZE, FFT_FORWARD);
  FFT.complexToMagnitude(vReal, vImag, BUFFER_SIZE);

  display.clearDisplay();
  showSpectrum();
  display.display();
}