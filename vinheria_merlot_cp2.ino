// Grupo: Merlot
// Giovanna Oliveira Ferreira Dias RM: 566647
// Maria Laura Pereira Druzeic RM: 566634
// Marianne Mukai Nishikawa RM: 568001

// -----------------------------------------------------------
// Projeto MERLOT - Vinharia Agnello
// ===================================================================
// SECÇÃO 1: BIBLIOTECAS, PINOS E DEFINIÇÕES GLOBAIS
// ===================================================================

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <RTClib.h>
#include <EEPROM.h>
#include <Keypad.h>

// --- Opções de Configuração ---
#define SERIAL_OPTION 1
#define UTC_OFFSET    -3
#define LOG_INTERVAL_ALARM (10 * 60 * 1000UL)
#define INTERVALO_CICLO_DISPLAY 4000 // Tempo para alternar display (4s)

// --- Definição dos Pinos ---
#define LDR_PIN       A0
#define LED_VERDE     2
#define LED_AMARELO   3
#define LED_VERMELHO  4
#define BUZZER_PIN    A1
#define DHTPIN        5
#define DHTTYPE       DHT22

// --- Enum para Menus e Ações ---
enum MenuState {
  MENU_MONITOR = 0, MENU_PRINCIPAL = 1, MENU_CONFIG = 2, MENU_LOGS = 3,
  MENU_ALARME_ATIVO = 100,
  ACAO_SET_LUZ_ALERTA = 10, ACAO_SET_LUZ_CRITICO = 11,
  ACAO_SET_TEMP_ALERTA_MIN = 12, ACAO_SET_TEMP_CRITICO_MIN = 13,
  ACAO_SET_TEMP_ALERTA_MAX = 14, ACAO_SET_TEMP_CRITICO_MAX = 15,
  ACAO_SET_UMID_ALERTA_MIN = 16, ACAO_SET_UMID_CRITICO_MIN = 17,
  ACAO_SET_UMID_ALERTA_MAX = 18, ACAO_SET_UMID_CRITICO_MAX = 19,
  ACAO_CALIBRA_LDR_MIN = 20, ACAO_CALIBRA_LDR_MAX = 21,
  ACAO_RESET_CONFIRMA = 22, ACAO_VER_LOG = 30,
  ACAO_LIMPAR_LOG_CONF = 31, ACAO_DEBUG_SERIAL = 32,
  ACAO_MOSTRAR_MSG = 200
};

// --- Configuração do Keypad ---
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'}, {'4','5','6','B'}, {'7','8','9','C'}, {'*','0','#','D'}
};
byte rowPins[ROWS] = {13, 12, 11, 10};
byte colPins[COLS] = {9, 8, 7, 6};
Keypad customKeypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// --- Endereços e Versionamento da Memória EEPROM ---
#define CONFIG_VERSION    2
#define ADDR_CFG_VERSION  1020
#define ADDR_EEPROM_INIT  1022
#define ADDR_LDR_MIN      0
#define ADDR_LDR_MAX      2
#define ADDR_LUZ_ALERTA   4
#define ADDR_LUZ_CRITICO  6
#define ADDR_TEMP_ALERTA_MIN  8
#define ADDR_TEMP_CRITICO_MIN 10
#define ADDR_TEMP_ALERTA_MAX  12
#define ADDR_TEMP_CRITICO_MAX 14
#define ADDR_UMID_ALERTA_MIN  16
#define ADDR_UMID_CRITICO_MIN 18
#define ADDR_UMID_ALERTA_MAX  20
#define ADDR_UMID_CRITICO_MAX 22
#define ADDR_LOG_START    30
#define MAX_LOG_ADDRESS   1000
int proximoEnderecoLog = ADDR_LOG_START;

// --- Estrutura do Log ---
struct LogEntry { uint32_t timestamp; float temperatura; float umidade; int16_t luz; };

// --- Objetos de Hardware ---
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);
RTC_DS3231 rtc;

// --- Variáveis Globais de Configuração ---
int16_t cfgLdrMin=50, cfgLdrMax=974, cfgLuzAlerta=20, cfgLuzCritico=40;
int16_t cfgTempAlertaMin=12, cfgTempCriticoMin=11, cfgTempAlertaMax=14, cfgTempCriticoMax=15;
int16_t cfgUmidAlertaMin=65, cfgUmidCriticoMin=60, cfgUmidAlertaMax=75, cfgUmidCriticoMax=80;

// --- Variáveis Globais de Sistema ---
#define NUM_LEITURAS 10
float leiturasLuz[NUM_LEITURAS], leiturasTemp[NUM_LEITURAS], leiturasUmid[NUM_LEITURAS];
int indiceLeitura = 0;
bool bufferCheio = false;
float gMediaLuz=0, gMediaTemp=0, gMediaUmid=0;
int gEstadoLuz=0, gEstadoTemp=0, gEstadoUmid=0, gEstadoFinal=0;
int gEstadoLuzAnterior=0, gEstadoTempAnterior=0, gEstadoUmidAnterior=0, gEstadoFinalAnterior=0;
LogEntry gLastLog;
MenuState menuatual = MENU_MONITOR;
MenuState menuAnterior = MENU_MONITOR;
int gLogCount = 0;

// --- Variáveis Globais para Input ---
char gInputBuffer[9] = ""; int gInputIndex = 0;

// --- Variáveis Globais de Timers ---
unsigned long tempoLeituraCurta = 0;
unsigned long tempoBuzzer = 0; bool estadoBuzzer = false; long intervaloBuzzer = 3000;
unsigned long gMensagemTimer = 0; char gMensagemTexto[17] = "";
unsigned long gTempoUltimoLog = 0;
unsigned long tempoCicloDisplay = 0;
bool displayMostraValores = true;

// --- Caracteres Customizados (Emojis) ---
byte uva[8]={B00000,B00100,B01110,B11111,B11111,B01110,B00100,B00000};
byte taca[8]={B11111,B10001,B11111,B11111,B01110,B00100,B01110,B00000};
byte brilho[8]={B00100,B10101,B01110,B10101,B00100,B00000,B00000,B00000};
byte emojiFeliz[8]={B00000,B01010,B00000,B10001,B01110,B00000,B00000,B00000};
byte tacaMetade[8]={B11111,B10001,B10001,B11111,B11111,B00100,B01110,B00000};
byte emojiSerio[8]={B00000,B01010,B00000,B11111,B00000,B00000,B00000,B00000};
byte tacaVazia[8]={B11111,B10001,B10001,B10001,B01110,B00100,B01110,B00000};
byte emojiTriste[8]={B00000,B01010,B00000,B01110,B10001,B00000,B00000,B00000};


// ===================================================================
// DECLARAÇÕES ANTECIPADAS (PROTÓTIPOS)
// ===================================================================
void checkKeypad(unsigned long agora);
void atualizarDisplay(unsigned long agora);
void carregarConfiguracoes();
void restaurarPadroes();
void limparLogsEEPROM();
void debugEEPROM();
float calcularMedia(float arr[], int tamanho);
int avaliarLuminosidade(int lum);
int avaliarTemperatura(float temp);
int avaliarUmidade(float umid);
void controlarAlarmes(int estadoFinal);
void salvarLog(float temp, float umid, int16_t luz);
int encontrarProximoLogAddr();
int contarLogs();
void desenharStatus(int estado);
void desenharStatusIndividual(int col, int row, int estado);
void acionarBuzzer();
void pararBuzzer();
void gerenciarBuzzer(unsigned long agora);
void executarAcao(MenuState acaoId);
void lerSensores(unsigned long agora);
void mostrarMensagem(const char* texto, int duracao);
void limparInputBuffer();
bool handleInput(char key, int16_t &configVar, int minimo, int maximo);
void lcdPrintFixed(int col, int row, const char* text);
void lcdPrintFixed(int col, int row, const __FlashStringHelper* text);
int calcularMediaAnalog(int pino, int amostras);

// ===================================================================
// SETUP
// ===================================================================
void setup() {
  #if SERIAL_OPTION
  Serial.begin(9600);
  Serial.println(F("Iniciando Sistema MERLOT"));
  #endif

  pinMode(LED_VERDE, OUTPUT); pinMode(LED_AMARELO, OUTPUT); pinMode(LED_VERMELHO, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  dht.begin(); lcd.init(); lcd.backlight();

  if (!rtc.begin()) {
    #if SERIAL_OPTION
    Serial.println(F("Erro RTC!"));
    #endif
    lcd.print(F("ERRO RELOGIO")); while (1);
  }
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  lcd.createChar(0, uva); lcd.createChar(1, taca); lcd.createChar(2, brilho);
  lcd.createChar(3, emojiFeliz); lcd.createChar(4, tacaMetade); lcd.createChar(5, emojiSerio);
  lcd.createChar(6, tacaVazia); lcd.createChar(7, emojiTriste);

  // --- Animação ---
  lcd.clear(); lcd.setCursor(0, 0); lcd.print(F("MERLOT - Monitoramento"));
  for (int i = 0; i < 7; i++) { lcd.setCursor(i, 1); lcd.write(byte(0)); delay(180); lcd.setCursor(i, 1); lcd.print(F(" ")); }
  lcd.setCursor(7, 1); lcd.write(byte(2)); delay(250); lcd.setCursor(7, 1); lcd.write(byte(1)); delay(300); lcd.setCursor(7, 1); lcd.print(F(" ")); delay(180);
  for (int i = 7; i < 16; i++) { lcd.setCursor(i, 1); lcd.write(byte(1)); delay(180); lcd.setCursor(i, 1); lcd.print(F(" ")); }

  carregarConfiguracoes();
  proximoEnderecoLog = encontrarProximoLogAddr();
  gLogCount = contarLogs();
  #if SERIAL_OPTION
  Serial.print(F("Logs:")); Serial.println(gLogCount);
  #endif
  if (gLogCount > 0) { EEPROM.get(proximoEnderecoLog - sizeof(LogEntry), gLastLog); }
  else { gLastLog.timestamp = 0; }

  // Inicializa buffers de leitura
  for (int i = 0; i < NUM_LEITURAS; ++i) {
      leiturasLuz[i] = 0.0; leiturasTemp[i] = 15.0; leiturasUmid[i] = 50.0;
  }
  gMediaTemp = dht.readTemperature(); gMediaUmid = dht.readHumidity();
  if(isnan(gMediaTemp)) gMediaTemp = 15.0; if(isnan(gMediaUmid)) gMediaUmid = 50.0;
  int vl = analogRead(LDR_PIN);
  gMediaLuz = map(constrain(vl, cfgLdrMin, cfgLdrMax), cfgLdrMin, cfgLdrMax, 0, 100);

  mostrarMensagem("Monitorando...", 2000);
  menuatual = MENU_MONITOR;
}

// ===================================================================
// LOOP PRINCIPAL
// ===================================================================
void loop() {
  unsigned long agora = millis();
  gerenciarBuzzer(agora);
  checkKeypad(agora);
  lerSensores(agora);
  atualizarDisplay(agora);
}

// ===================================================================
// FUNÇÕES DE MENU E NAVEGAÇÃO
// ===================================================================
void checkKeypad(unsigned long agora) {
  char key = customKeypad.getKey();
  if (!key) return;

  if (gEstadoFinal != 0) {
    if (menuatual != MENU_ALARME_ATIVO) menuatual = MENU_ALARME_ATIVO;
    if (key == 'D') pararBuzzer();
    return;
  }
  if (menuatual == MENU_ALARME_ATIVO && gEstadoFinal == 0) menuatual = MENU_PRINCIPAL;

  if (menuatual == ACAO_MOSTRAR_MSG) { gMensagemTimer = 0; menuatual = menuAnterior; return; }
  if (menuatual == MENU_MONITOR && key) { // Modificado para verificar 'key'
       menuatual = MENU_PRINCIPAL;
       menuAnterior = MENU_MONITOR;
       // Não retorna aqui, deixa o switch processar a tecla se for C, 1, 2, 3...
  }


  // --- Máquina de Estados ---
  switch (menuatual) {
    case MENU_PRINCIPAL:
      if (key == '1') menuatual = MENU_MONITOR;
      else if (key == '2') menuatual = MENU_CONFIG;
      else if (key == '3') menuatual = MENU_LOGS;
      break;
    case MENU_CONFIG:
      if (key == '1') { menuatual = ACAO_SET_LUZ_ALERTA; menuAnterior = MENU_CONFIG; limparInputBuffer(); }
      else if (key == '2') { menuatual = ACAO_CALIBRA_LDR_MIN; menuAnterior = MENU_CONFIG; }
      else if (key == '3') { menuatual = ACAO_RESET_CONFIRMA; menuAnterior = MENU_CONFIG; }
      else if (key == 'C') menuatual = MENU_PRINCIPAL;
      break;
    case MENU_LOGS:
      if (key == '1') { menuatual = ACAO_VER_LOG; menuAnterior = MENU_LOGS; }
      else if (key == '2') { menuatual = ACAO_LIMPAR_LOG_CONF; menuAnterior = MENU_LOGS; }
      else if (key == '3') { executarAcao(ACAO_DEBUG_SERIAL); }
      else if (key == 'C') menuatual = MENU_PRINCIPAL;
      break;

    // --- Cadeia de Ações: Configurar Limites ---
    case ACAO_SET_LUZ_ALERTA:   if (handleInput(key, cfgLuzAlerta, 0, 100))        { EEPROM.put(ADDR_LUZ_ALERTA, cfgLuzAlerta);         menuatual = ACAO_SET_LUZ_CRITICO;    limparInputBuffer(); } break;
    case ACAO_SET_LUZ_CRITICO:  if (handleInput(key, cfgLuzCritico, 0, 100))       { EEPROM.put(ADDR_LUZ_CRITICO, cfgLuzCritico);       menuatual = ACAO_SET_TEMP_ALERTA_MIN; limparInputBuffer(); } break;
    case ACAO_SET_TEMP_ALERTA_MIN: if (handleInput(key, cfgTempAlertaMin, -10, 20)) { EEPROM.put(ADDR_TEMP_ALERTA_MIN, cfgTempAlertaMin); menuatual = ACAO_SET_TEMP_CRITICO_MIN; limparInputBuffer(); } break;
    case ACAO_SET_TEMP_CRITICO_MIN: if (handleInput(key, cfgTempCriticoMin, -10, 20)){ EEPROM.put(ADDR_TEMP_CRITICO_MIN, cfgTempCriticoMin); menuatual = ACAO_SET_TEMP_ALERTA_MAX; limparInputBuffer(); } break;
    case ACAO_SET_TEMP_ALERTA_MAX: if (handleInput(key, cfgTempAlertaMax, 0, 50))   { EEPROM.put(ADDR_TEMP_ALERTA_MAX, cfgTempAlertaMax); menuatual = ACAO_SET_TEMP_CRITICO_MAX; limparInputBuffer(); } break;
    case ACAO_SET_TEMP_CRITICO_MAX: if (handleInput(key, cfgTempCriticoMax, 0, 50))  { EEPROM.put(ADDR_TEMP_CRITICO_MAX, cfgTempCriticoMax); menuatual = ACAO_SET_UMID_ALERTA_MIN; limparInputBuffer(); } break;
    case ACAO_SET_UMID_ALERTA_MIN: if (handleInput(key, cfgUmidAlertaMin, 0, 100))  { EEPROM.put(ADDR_UMID_ALERTA_MIN, cfgUmidAlertaMin); menuatual = ACAO_SET_UMID_CRITICO_MIN; limparInputBuffer(); } break;
    case ACAO_SET_UMID_CRITICO_MIN: if (handleInput(key, cfgUmidCriticoMin, 0, 100)) { EEPROM.put(ADDR_UMID_CRITICO_MIN, cfgUmidCriticoMin); menuatual = ACAO_SET_UMID_ALERTA_MAX; limparInputBuffer(); } break;
    case ACAO_SET_UMID_ALERTA_MAX: if (handleInput(key, cfgUmidAlertaMax, 0, 100))  { EEPROM.put(ADDR_UMID_ALERTA_MAX, cfgUmidAlertaMax); menuatual = ACAO_SET_UMID_CRITICO_MAX; limparInputBuffer(); } break;
    case ACAO_SET_UMID_CRITICO_MAX: if (handleInput(key, cfgUmidCriticoMax, 0, 100)) { EEPROM.put(ADDR_UMID_CRITICO_MAX, cfgUmidCriticoMax); mostrarMensagem("Limites Salvos!", 2000); menuatual = menuAnterior; limparInputBuffer(); } break;

    // --- Ações (Calibração) ---
    case ACAO_CALIBRA_LDR_MIN: if (key == 'D') { cfgLdrMin = calcularMediaAnalog(LDR_PIN, 10); EEPROM.put(ADDR_LDR_MIN, cfgLdrMin); menuatual = ACAO_CALIBRA_LDR_MAX; } else if (key == 'C') menuatual = menuAnterior; break;
    case ACAO_CALIBRA_LDR_MAX: if (key == 'D') { cfgLdrMax = calcularMediaAnalog(LDR_PIN, 10); EEPROM.put(ADDR_LDR_MAX, cfgLdrMax); mostrarMensagem("LDR Calibrado!", 2000); menuatual = menuAnterior; } else if (key == 'C') menuatual = menuAnterior; break;

    // --- Ações (Confirmação) ---
    case ACAO_RESET_CONFIRMA:  if (key == 'D') { restaurarPadroes(); mostrarMensagem("Resetado!", 2000); menuatual = menuAnterior; } else if (key == 'C') menuatual = menuAnterior; break;
    case ACAO_LIMPAR_LOG_CONF: if (key == 'D') { limparLogsEEPROM(); mostrarMensagem("Logs Limpos!", 2000); menuatual = menuAnterior; } else if (key == 'C') menuatual = menuAnterior; break;

    // --- Telas Estáticas ---
    case ACAO_VER_LOG:         if (key == 'C') menuatual = menuAnterior; break;
  }
}

void executarAcao(MenuState acaoId) {
    switch(acaoId) {
        case ACAO_DEBUG_SERIAL:
            debugEEPROM();
            mostrarMensagem("Debug no Serial", 2000);
            menuatual = MENU_LOGS;
            break;
        default:
           menuatual = MENU_PRINCIPAL;
           break;
    }
}


void atualizarDisplay(unsigned long agora) {
  static MenuState menuAnteriorDisplay = (MenuState)-1;
  static unsigned long tempoUltimoRefresh = 0;
  static bool displayNeedsClear = true; // Inicia true para limpar na primeira vez

  // --- Lógica para alternar display a cada 4 segundos ---
  if (menuatual == MENU_MONITOR && agora - tempoCicloDisplay >= INTERVALO_CICLO_DISPLAY) {
      displayMostraValores = !displayMostraValores;
      tempoCicloDisplay = agora;
      menuAnteriorDisplay = (MenuState)-1; // Força redesenho
      displayNeedsClear = true; // Marca para limpar
  }

  if (menuatual == ACAO_MOSTRAR_MSG) {
    if (agora > gMensagemTimer) { menuatual = menuAnterior; menuAnteriorDisplay = (MenuState)-1; displayNeedsClear = true;}
    if (menuAnteriorDisplay != ACAO_MOSTRAR_MSG) {
      lcd.clear();
      lcdPrintFixed(0, 0, F("!---------------!"));
      lcdPrintFixed(0, 1, gMensagemTexto);
      menuAnteriorDisplay = ACAO_MOSTRAR_MSG;
    } return;
  }

  bool forcarRefresh = (agora - tempoUltimoRefresh > 1000);
  if (menuatual >= ACAO_SET_LUZ_ALERTA && menuatual <= ACAO_SET_UMID_CRITICO_MAX) forcarRefresh = true;

  // Só redesenha se o menu mudou ou se forçou refresh
  if (menuatual == menuAnteriorDisplay && !forcarRefresh) return;

  // Limpa o LCD apenas se o menu mudou (ou se forçado pela flag)
  if (menuatual != menuAnteriorDisplay || displayNeedsClear) {
      lcd.clear();
      displayNeedsClear = false; // Reseta a flag
  }

  menuAnteriorDisplay = menuatual; tempoUltimoRefresh = agora;

  char linha1[17] = ""; char linha2[17] = ""; char bufferAux[10];

  DateTime utcTime = rtc.now();
  DateTime localTime = DateTime(utcTime.unixtime() + (UTC_OFFSET * 3600L));

  switch (menuatual) {
    case MENU_MONITOR:
      if (displayMostraValores) {
          char tempStr[6], umidStr[6];
          dtostrf(gMediaTemp, 4, 1, tempStr);
          dtostrf(gMediaUmid, 4, 1, umidStr);
          sprintf(linha1, "L:%d%% T:%sC", (int)gMediaLuz, tempStr);
          sprintf(linha2, "U:%s%%     %02d:%02d", umidStr, localTime.hour(), localTime.minute());
      } else {
          strcpy_P(linha1, PSTR("Luz:      T:"));
          strcpy_P(linha2, PSTR("U:"));
      }
      break;
    case MENU_PRINCIPAL:    strcpy_P(linha1, PSTR("Menu Principal")); strcpy_P(linha2, PSTR("1.Monitor 2.Cfg 3.Log")); break;
    case MENU_CONFIG:       strcpy_P(linha1, PSTR("Configuracoes")); strcpy_P(linha2, PSTR("1.Limites 2.LDR 3.Rst")); break;
    case MENU_LOGS:
      strcpy_P(linha1, PSTR("Logs EEPROM"));
      sprintf(linha2, "1.Ver(%d) 2.Limp 3.Dbg", gLogCount);
      break;
    case ACAO_VER_LOG:
      strcpy_P(linha1, PSTR("Ultimo Alarme:"));
      if (gLastLog.timestamp == 0) { strcpy_P(linha2, PSTR("Nenhum log salvo")); }
      else {
          char tempStrLog[6], umidStrLog[6];
          dtostrf(gLastLog.temperatura, 4, 0, tempStrLog);
          dtostrf(gLastLog.umidade, 4, 0, umidStrLog);
          sprintf(linha2, "T:%s U:%s L:%d", tempStrLog, umidStrLog, gLastLog.luz);
      }
      break;
    case MENU_ALARME_ATIVO:
      strcpy_P(linha1, PSTR("ALARME ATIVO"));
      strcpy(linha2, "");
      if(gEstadoLuz != 0) strcat_P(linha2, PSTR("Luz!"));
      if(gEstadoTemp != 0) strcat_P(linha2, PSTR("Temp!"));
      if(gEstadoUmid != 0) strcat_P(linha2, PSTR("Umid!"));
      break;
    // --- Telas de Input/Confirmação ---
    case ACAO_SET_LUZ_ALERTA:   strcpy_P(linha1, PSTR("Luz Alerta")); strcpy(linha2, gInputBuffer); break;
    case ACAO_SET_LUZ_CRITICO:  strcpy_P(linha1, PSTR("Luz Critico")); strcpy(linha2, gInputBuffer); break;
    case ACAO_SET_TEMP_ALERTA_MIN: strcpy_P(linha1, PSTR("Temp Alerta MIN")); strcpy(linha2, gInputBuffer); break;
    case ACAO_SET_TEMP_CRITICO_MIN: strcpy_P(linha1, PSTR("Temp Critico MIN")); strcpy(linha2, gInputBuffer); break;
    case ACAO_SET_TEMP_ALERTA_MAX: strcpy_P(linha1, PSTR("Temp Alerta MAX")); strcpy(linha2, gInputBuffer); break;
    case ACAO_SET_TEMP_CRITICO_MAX: strcpy_P(linha1, PSTR("Temp Critico MAX")); strcpy(linha2, gInputBuffer); break;
    case ACAO_SET_UMID_ALERTA_MIN: strcpy_P(linha1, PSTR("Umid Alerta MIN")); strcpy(linha2, gInputBuffer); break;
    case ACAO_SET_UMID_CRITICO_MIN: strcpy_P(linha1, PSTR("Umid Critico MIN")); strcpy(linha2, gInputBuffer); break;
    case ACAO_SET_UMID_ALERTA_MAX: strcpy_P(linha1, PSTR("Umid Alerta MAX")); strcpy(linha2, gInputBuffer); break;
    case ACAO_SET_UMID_CRITICO_MAX: strcpy_P(linha1, PSTR("Umid Critico MAX")); strcpy(linha2, gInputBuffer); break;
    case ACAO_CALIBRA_LDR_MIN: strcpy_P(linha1, PSTR("Apague a luz...")); strcpy_P(linha2, PSTR("Pressione D")); break;
    case ACAO_CALIBRA_LDR_MAX: strcpy_P(linha1, PSTR("Acenda a luz...")); strcpy_P(linha2, PSTR("Pressione D")); break;
    case ACAO_RESET_CONFIRMA:  strcpy_P(linha1, PSTR("Resetar? D=Sim")); strcpy_P(linha2, PSTR("(C=Nao)")); break;
    case ACAO_LIMPAR_LOG_CONF: strcpy_P(linha1, PSTR("Limpar Logs? D=Sim")); strcpy_P(linha2, PSTR("(C=Nao)")); break;
  }

  // Desenha as linhas principais
  lcdPrintFixed(0, 0, linha1);
  lcdPrintFixed(0, 1, linha2);

  // Desenha ícones se estiver no modo Monitor de Status
  if (menuatual == MENU_MONITOR && !displayMostraValores) {
      desenharStatusIndividual(4, 0, gEstadoLuz);
      desenharStatusIndividual(11, 0, gEstadoTemp);
      desenharStatusIndividual(2, 1, gEstadoUmid);
  }
}

// ... (handleInput, limparInputBuffer, mostrarMensagem) ...
bool handleInput(char key, int16_t &configVar, int minimo, int maximo) {
  if (key >= '0' && key <= '9' && gInputIndex < 8) { gInputBuffer[gInputIndex++] = key; gInputBuffer[gInputIndex] = '\0'; }
  else if (key == '*') { limparInputBuffer(); }
  else if (key == 'C') { menuatual = menuAnterior; limparInputBuffer(); }
  else if (key == 'D') {
    if (gInputIndex == 0) return false;
    int num = atoi(gInputBuffer);
    if (num >= minimo && num <= maximo) { configVar = (int16_t)num; tone(BUZZER_PIN, 1500, 100); return true; }
    else { mostrarMensagem("Valor Invalido!", 2000); menuatual = menuAnterior; limparInputBuffer(); }
  } return false;
}

void limparInputBuffer() { gInputBuffer[0] = '\0'; gInputIndex = 0; }

void mostrarMensagem(const char* texto, int duracao) {
  strncpy(gMensagemTexto, texto, 16);
  gMensagemTexto[16] = '\0';
  gMensagemTimer = millis() + duracao;
  menuAnterior = menuatual;
  menuatual = ACAO_MOSTRAR_MSG;
}

// ===================================================================
// SECÇÃO 5: FUNÇÕES DE LEITURA E ALARME
// ===================================================================
void lerSensores(unsigned long agora) {
  if (agora - tempoLeituraCurta < 1000) return;
  tempoLeituraCurta = agora;

  int valorLido = analogRead(LDR_PIN);
  float valorTemp = dht.readTemperature();
  float valorUmid = dht.readHumidity();

  // Tratamento de NaN usando valor anterior
  if (!isnan(valorTemp)) { leiturasTemp[indiceLeitura] = valorTemp; }
  else { leiturasTemp[indiceLeitura] = leiturasTemp[(indiceLeitura + NUM_LEITURAS -1) % NUM_LEITURAS]; }
  if (!isnan(valorUmid)) { leiturasUmid[indiceLeitura] = valorUmid; }
  else { leiturasUmid[indiceLeitura] = leiturasUmid[(indiceLeitura + NUM_LEITURAS -1) % NUM_LEITURAS]; }


  int valorCalibrado = constrain(valorLido, cfgLdrMin, cfgLdrMax);
  leiturasLuz[indiceLeitura] = map(valorCalibrado, cfgLdrMin, cfgLdrMax, 0, 100);
  indiceLeitura = (indiceLeitura + 1) % NUM_LEITURAS;
  if (indiceLeitura == 0 && !bufferCheio) bufferCheio = true;

  if (bufferCheio) {
    gMediaLuz = calcularMedia(leiturasLuz, NUM_LEITURAS);
    gMediaTemp = calcularMedia(leiturasTemp, NUM_LEITURAS);
    gMediaUmid = calcularMedia(leiturasUmid, NUM_LEITURAS);

    gEstadoLuzAnterior = gEstadoLuz; gEstadoTempAnterior = gEstadoTemp;
    gEstadoUmidAnterior = gEstadoUmid; gEstadoFinalAnterior = gEstadoFinal;

    gEstadoLuz = avaliarLuminosidade(gMediaLuz);
    gEstadoTemp = avaliarTemperatura(gMediaTemp);
    gEstadoUmid = avaliarUmidade(gMediaUmid);
    gEstadoFinal = max(gEstadoLuz, max(gEstadoTemp, gEstadoUmid));

    controlarAlarmes(gEstadoFinal);

    bool deveLogar = false;
    if (gEstadoFinal > gEstadoFinalAnterior) deveLogar = true;
    else if (gEstadoFinal != 0 && (agora - gTempoUltimoLog > LOG_INTERVAL_ALARM)) deveLogar = true;

    if (deveLogar) {
      salvarLog(gMediaTemp, gMediaUmid, (int16_t)gMediaLuz);
      gTempoUltimoLog = agora;
    }
  } else {
      gMediaLuz = leiturasLuz[indiceLeitura > 0 ? indiceLeitura -1 : 0];
      gMediaTemp = leiturasTemp[indiceLeitura > 0 ? indiceLeitura -1 : 0];
      gMediaUmid = leiturasUmid[indiceLeitura > 0 ? indiceLeitura -1 : 0];
      gEstadoFinal = 0;
      controlarAlarmes(gEstadoFinal);
  }
}


void controlarAlarmes(int estadoFinal) {
  digitalWrite(LED_VERDE, estadoFinal == 0);
  digitalWrite(LED_AMARELO, estadoFinal == 1);
  digitalWrite(LED_VERMELHO, estadoFinal == 2);
  if (estadoFinal == 0 && estadoBuzzer) pararBuzzer();
  else if (estadoFinal != 0 && !estadoBuzzer) acionarBuzzer();
}

const float TEMP_HISTERESE = 0.5; const float UMID_HISTERESE = 1.0; const int LUZ_HISTERESE = 2;

int avaliarLuminosidade(int lum) {
  int estadoAtual = gEstadoLuzAnterior;
  if (estadoAtual == 0 && lum > cfgLuzAlerta + LUZ_HISTERESE) estadoAtual = 1;
  else if (estadoAtual == 1 && lum > cfgLuzCritico + LUZ_HISTERESE) estadoAtual = 2;
  else if (estadoAtual == 2 && lum < cfgLuzCritico - LUZ_HISTERESE) estadoAtual = 1;
  else if (estadoAtual == 1 && lum < cfgLuzAlerta - LUZ_HISTERESE) estadoAtual = 0;
  return estadoAtual;
}
int avaliarTemperatura(float temp) {
  int estadoAtual = gEstadoTempAnterior;
  if (isnan(temp)) return estadoAtual;
  if (estadoAtual == 0 && (temp < cfgTempAlertaMin - TEMP_HISTERESE || temp > cfgTempAlertaMax + TEMP_HISTERESE)) estadoAtual = 1;
  else if (estadoAtual == 1 && (temp < cfgTempCriticoMin - TEMP_HISTERESE || temp > cfgTempCriticoMax + TEMP_HISTERESE)) estadoAtual = 2;
  else if (estadoAtual == 2 && (temp > cfgTempCriticoMin + TEMP_HISTERESE && temp < cfgTempCriticoMax - TEMP_HISTERESE)) estadoAtual = 1;
  else if (estadoAtual == 1 && (temp > cfgTempAlertaMin + TEMP_HISTERESE && temp < cfgTempAlertaMax - TEMP_HISTERESE)) estadoAtual = 0;
  return estadoAtual;
}
int avaliarUmidade(float umid) {
    int estadoAtual = gEstadoUmidAnterior;
    if (isnan(umid)) return estadoAtual;
  if (estadoAtual == 0 && (umid < cfgUmidAlertaMin - UMID_HISTERESE || umid > cfgUmidAlertaMax + UMID_HISTERESE)) estadoAtual = 1;
  else if (estadoAtual == 1 && (umid < cfgUmidCriticoMin - UMID_HISTERESE || umid > cfgUmidCriticoMax + UMID_HISTERESE)) estadoAtual = 2;
  else if (estadoAtual == 2 && (umid > cfgUmidCriticoMin + UMID_HISTERESE && umid < cfgUmidCriticoMax - UMID_HISTERESE)) estadoAtual = 1;
  else if (estadoAtual == 1 && (umid > cfgUmidAlertaMin + UMID_HISTERESE && umid < cfgUmidAlertaMax - UMID_HISTERESE)) estadoAtual = 0;
  return estadoAtual;
}

// ===================================================================
// SECÇÃO 6: FUNÇÕES DE EEPROM (Memória)
// ===================================================================
void restaurarPadroes() {
  #if SERIAL_OPTION
  Serial.println(F("Restaurando..."));
  #endif
  EEPROM.put(ADDR_LDR_MIN, (int16_t)50); EEPROM.put(ADDR_LDR_MAX, (int16_t)974);
  EEPROM.put(ADDR_LUZ_ALERTA, (int16_t)20); EEPROM.put(ADDR_LUZ_CRITICO, (int16_t)40);
  EEPROM.put(ADDR_TEMP_ALERTA_MIN, (int16_t)12); EEPROM.put(ADDR_TEMP_CRITICO_MIN, (int16_t)11);
  EEPROM.put(ADDR_TEMP_ALERTA_MAX, (int16_t)14); EEPROM.put(ADDR_TEMP_CRITICO_MAX, (int16_t)15);
  EEPROM.put(ADDR_UMID_ALERTA_MIN, (int16_t)65); EEPROM.put(ADDR_UMID_CRITICO_MIN, (int16_t)60);
  EEPROM.put(ADDR_UMID_ALERTA_MAX, (int16_t)75); EEPROM.put(ADDR_UMID_CRITICO_MAX, (int16_t)80);
  EEPROM.put(ADDR_EEPROM_INIT, (int16_t)12345);
  EEPROM.put(ADDR_CFG_VERSION, (int16_t)CONFIG_VERSION);
  carregarConfiguracoes();
}

void carregarConfiguracoes() {
  int16_t initFlag, cfgVersionEEPROM;
  EEPROM.get(ADDR_EEPROM_INIT, initFlag);
  EEPROM.get(ADDR_CFG_VERSION, cfgVersionEEPROM);
  if (initFlag != 12345 || cfgVersionEEPROM != CONFIG_VERSION) {
      restaurarPadroes();
  } else {
    #if SERIAL_OPTION
    Serial.println(F("Carregando config."));
    #endif
    EEPROM.get(ADDR_LDR_MIN, cfgLdrMin); EEPROM.get(ADDR_LDR_MAX, cfgLdrMax);
    EEPROM.get(ADDR_LUZ_ALERTA, cfgLuzAlerta); EEPROM.get(ADDR_LUZ_CRITICO, cfgLuzCritico);
    EEPROM.get(ADDR_TEMP_ALERTA_MIN, cfgTempAlertaMin); EEPROM.get(ADDR_TEMP_CRITICO_MIN, cfgTempCriticoMin);
    EEPROM.get(ADDR_TEMP_ALERTA_MAX, cfgTempAlertaMax); EEPROM.get(ADDR_TEMP_CRITICO_MAX, cfgTempCriticoMax);
    EEPROM.get(ADDR_UMID_ALERTA_MIN, cfgUmidAlertaMin); EEPROM.get(ADDR_UMID_CRITICO_MIN, cfgUmidCriticoMin);
    EEPROM.get(ADDR_UMID_ALERTA_MAX, cfgUmidAlertaMax); EEPROM.get(ADDR_UMID_CRITICO_MAX, cfgUmidCriticoMax);
  }
}

int calcularMediaAnalog(int pino, int amostras) {
  long soma = 0; for (int i = 0; i < amostras; i++) { soma += analogRead(pino); delayMicroseconds(50); }
   return (int)(soma / amostras);
}


void limparLogsEEPROM() {
  #if SERIAL_OPTION
  Serial.println(F("Limpando logs..."));
  #endif
  for (int i = ADDR_LOG_START; i < MAX_LOG_ADDRESS; i++) EEPROM.update(i, 0xFF);
  proximoEnderecoLog = ADDR_LOG_START; gLastLog.timestamp = 0; gLogCount = 0;
}

void debugEEPROM() {
  #if SERIAL_OPTION
  Serial.println(F("DEBUG"));
  Serial.print(F("Config Version:")); Serial.println(CONFIG_VERSION);
  Serial.print(F("LDR:")); Serial.print(cfgLdrMin); Serial.print(F("/")); Serial.println(cfgLdrMax);
  Serial.print(F("Luz:")); Serial.print(cfgLuzAlerta); Serial.print(F("/")); Serial.println(cfgLuzCritico);
  Serial.print(F("Temp Alerta:")); Serial.print(cfgTempAlertaMin); Serial.print(F("/")); Serial.println(cfgTempAlertaMax);
  Serial.print(F("Temp Critico:")); Serial.print(cfgTempCriticoMin); Serial.print(F("/")); Serial.println(cfgTempCriticoMax);
  Serial.print(F("Umid Alerta:")); Serial.print(cfgUmidAlertaMin); Serial.print(F("/")); Serial.println(cfgUmidAlertaMax);
  Serial.print(F("Umid Critico:")); Serial.print(cfgUmidCriticoMin); Serial.print(F("/")); Serial.println(cfgUmidCriticoMax);

  int count = contarLogs();
  Serial.print(F("\n LOGS ")); Serial.print(count); Serial.println(F(") ====="));
  if (count == 0) { Serial.println(F("Nenhum log salvo.")); return; }
  int endereco = ADDR_LOG_START; LogEntry log;
  for(int i = 0; i < count; i++) {
    EEPROM.get(endereco, log); DateTime dt(log.timestamp);
    Serial.print(F("LOG")); Serial.print(i + 1); Serial.print(F("] "));
    DateTime localLogTime = DateTime(log.timestamp + (UTC_OFFSET * 3600L));
    Serial.print(localLogTime.timestamp(DateTime::TIMESTAMP_FULL));
    Serial.print(F("L:")); Serial.print(log.luz); Serial.print(F("T:")); Serial.print(log.temperatura); Serial.print(F("U:")); Serial.println(log.umidade);
    endereco += sizeof(LogEntry);
  }
  #else
  mostrarMensagem("Use Serial Monitor", 2000);
  #endif
}

void salvarLog(float temp, float umid, int16_t luz) {
  if (isnan(temp) || isnan(umid)) {
    #if SERIAL_OPTION
    Serial.println(F("Dados invalidos, log nao salvo."));
    #endif
    return;
  }
  LogEntry log; log.timestamp = rtc.now().unixtime(); log.temperatura = temp; log.umidade = umid; log.luz = luz;
  gLastLog = log; EEPROM.put(proximoEnderecoLog, log);
  proximoEnderecoLog += sizeof(LogEntry);
  if (proximoEnderecoLog > (MAX_LOG_ADDRESS - sizeof(LogEntry))) proximoEnderecoLog = ADDR_LOG_START;
  gLogCount = contarLogs();
  #if SERIAL_OPTION
  Serial.println(F("Log salvo."));
  #endif
}

int encontrarProximoLogAddr() {
  int addr = ADDR_LOG_START; LogEntry tempLog;
  while (addr < (MAX_LOG_ADDRESS - sizeof(LogEntry))) {
      EEPROM.get(addr, tempLog); if (tempLog.timestamp == 0xFFFFFFFF || tempLog.timestamp == 0) break; addr += sizeof(LogEntry);
  }
  if (addr > (MAX_LOG_ADDRESS - sizeof(LogEntry))) return ADDR_LOG_START; return addr;
}

int contarLogs() {
    int count = 0; int addr = ADDR_LOG_START; LogEntry tempLog;
    while (addr < (MAX_LOG_ADDRESS - sizeof(LogEntry))) {
        EEPROM.get(addr, tempLog); if (tempLog.timestamp == 0xFFFFFFFF || tempLog.timestamp == 0) break; count++; addr += sizeof(LogEntry);
    } return count;
}


// ===================================================================
// SECÇÃO 7: FUNÇÕES AUXILIARES (Genéricas)
// ===================================================================
float calcularMedia(float arr[], int tamanho) {
    if (tamanho <= 0) return 0.0;
    float soma = 0.0;
    int amostrasValidas = 0;
    // Usa a flag bufferCheio ou indiceLeitura para determinar quantas amostras usar
    int limite = bufferCheio ? NUM_LEITURAS : indiceLeitura;

    if (limite == 0) { // Fallback inicial (antes da primeira leitura completa)
        // Retorna a última média calculada ou um valor inicial seguro
        if (arr == leiturasLuz) return gMediaLuz; // Usa a última média válida
        if (arr == leiturasTemp) return isnan(gMediaTemp) ? 15.0 : gMediaTemp;
        if (arr == leiturasUmid) return isnan(gMediaUmid) ? 50.0 : gMediaUmid;
        return 0.0;
    }

    for (int i = 0; i < limite; i++) {
        // Assume que NaN foi substituído por valor anterior em lerSensores
        soma += arr[i];
        amostrasValidas++;
    }

    if (amostrasValidas == 0) return 0.0; // Segurança
    return soma / (float)amostrasValidas;
}

// --- NOVO: Função auxiliar para desenhar status individual ---
void desenharStatusIndividual(int col, int row, int estado) {
  lcd.setCursor(col, row);
  if (estado == 0) { // OK
    lcd.write(byte(1)); lcd.print(F(" ")); lcd.write(byte(3));
  } else if (estado == 1) { // ALERTA
    lcd.write(byte(4)); lcd.print(F(" ")); lcd.write(byte(5));
  } else { // CRITICO
    lcd.write(byte(6)); lcd.print(F(" ")); lcd.write(byte(7));
  }
  // Limpa os próximos 2 caracteres para garantir que não fique lixo
  // lcd.print(F("  ")); // Removido pois lcdPrintFixed faz isso
}


void acionarBuzzer() { if (!estadoBuzzer) { estadoBuzzer = true; tempoBuzzer = millis(); } }
void pararBuzzer() { if (estadoBuzzer) { estadoBuzzer = false; noTone(BUZZER_PIN); } }

void gerenciarBuzzer(unsigned long agora) {
  if (!estadoBuzzer) { noTone(BUZZER_PIN); return; }
  unsigned long tempoDecorrido = agora - tempoBuzzer;
  if (tempoDecorrido < 3000) tone(BUZZER_PIN, 1000);
  else if (tempoDecorrido < 6000) noTone(BUZZER_PIN);
  else tempoBuzzer = agora;
}

// --- REFINAMENTO: Função otimizada para display suave ---
void lcdPrintFixed(int col, int row, const char* text) {
  lcd.setCursor(col, row);
  int len = strlen(text);
  // Imprime o texto
  for (int i = 0; i < len && (col + i) < 16; i++) {
      lcd.print(text[i]);
  }
  // Limpa o resto da linha
  for (int i = len; (col + i) < 16; i++) {
    lcd.print(F(" "));
  }
}
void lcdPrintFixed(int col, int row, const __FlashStringHelper* text) {
    lcd.setCursor(col, row);
    int len = strlen_P((const char*)text);
    // Imprime o texto da Flash
    for (int i = 0; i < len && (col + i) < 16; i++) {
        lcd.print((char)pgm_read_byte_near(((const char*)text) + i));
    }
    // Limpa o resto da linha
    for (int i = len; (col + i) < 16; i++) {
        lcd.print(F(" "));
    }
}