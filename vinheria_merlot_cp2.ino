// -----------------------------------------------------------
// Projeto MERLOT - Vinharia Agnello
// GRUPO: Merlot
// Equipe:
// Maria Laura Pereira Druzeic RM: 566634
// Giovanna Oliveira Ferreira Dias RM: 566647
// Marianne Mukai Nishikawa RM: 568001
// -----------------------------------------------------------

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
#define UTC_OFFSET    0
#define LOG_INTERVAL_ALARM (10 * 60 * 1000UL)
#define INTERVALO_CICLO_DISPLAY 4000 // Tempo para alternar display (4s)
#define LCD_COLS 20 // Define colunas
#define LCD_ROWS 4  // Define linhas

// --- Definição dos Pinos ---
#define LDR_PIN       A0
#define LED_VERDE     2
#define LED_AMARELO   3
#define LED_VERMELHO  4
#define BUZZER_PIN    A1 // Pino A1 para o Buzzer
#define DHTPIN        5  // Pino 5 para o DHT
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
// ATUALIZADO: Inicializa o LCD como 20x4
LiquidCrystal_I2C lcd(0x27, LCD_COLS, LCD_ROWS);
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
char gInputBuffer[10] = ""; // Aumentado ligeiramente
int gInputIndex = 0;

// --- Variáveis Globais de Timers ---
unsigned long tempoLeituraCurta = 0;
unsigned long tempoBuzzer = 0; bool estadoBuzzer = false; long intervaloBuzzer = 3000;
unsigned long gMensagemTimer = 0; char gMensagemTexto[21] = ""; // Aumentado para 20 chars
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
void mostrarMensagem(const char* texto, int duracao, int linha = 1); // Adiciona linha
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

  // --- Animação (Adaptada para 20x4) ---
  lcd.clear(); lcd.setCursor(7, 0); lcd.print(F("MERLOT"));
  for (int i = 0; i < 9; i++) { lcd.setCursor(i, 2); lcd.write(byte(0)); delay(180); lcd.setCursor(i, 2); lcd.print(F(" ")); }
  lcd.setCursor(9, 2); lcd.write(byte(2)); delay(250);
  lcd.setCursor(9, 2); lcd.write(byte(1)); delay(300);
  lcd.setCursor(9, 2); lcd.print(F(" ")); delay(180);
  for (int i = 9; i < LCD_COLS; i++) { lcd.setCursor(i, 2); lcd.write(byte(1)); delay(180); lcd.setCursor(i, 2); lcd.print(F(" ")); }

  // Limpa a linha da animação anterior
  lcdPrintFixed(0, 2, ""); 

  for (int i = 0; i < 9; i++) { // Para 1 passo antes de colidirem
    int posEsq = i; // Posição da taça esquerda (0 -> 8)
    int posDir = (LCD_COLS - 1) - i; // Posição da taça direita (19 -> 11)

    lcd.setCursor(posEsq, 3); lcd.write(byte(1)); // Desenha taça esquerda
    lcd.setCursor(posDir, 3); lcd.write(byte(1)); // Desenha taça direita
    
    delay(100); // Velocidade do movimento
    
    // Apaga as taças para o próximo frame
    lcd.setCursor(posEsq, 3); lcd.print(F(" ")); 
    lcd.setCursor(posDir, 3); lcd.print(F(" "));
  }
  
  // Desenha as taças na posição final (coluna 9 e 10)
  lcd.setCursor(9, 3); lcd.write(byte(1));
  lcd.setCursor(10, 3); lcd.write(byte(1));
  
  // Desenha o "Brinde" (brilho) acima delas, na linha 2
  lcd.setCursor(9, 2); lcd.write(byte(2));
  tone(BUZZER_PIN, 2000, 100); // Som de "brinde"
  
  delay(600); // Pausa para ver o brinde
  
  // Limpa as linhas da animação
  lcdPrintFixed(0, 2, "");
  lcdPrintFixed(0, 3, "");
  delay(200);


  carregarConfiguracoes();
  proximoEnderecoLog = encontrarProximoLogAddr();
  gLogCount = contarLogs();
  #if SERIAL_OPTION
  Serial.print(F("Logs:")); Serial.println(gLogCount);
  #endif
  if (gLogCount > 0) {
      int ultimoLogAddr = (proximoEnderecoLog == ADDR_LOG_START) ?
                          (ADDR_LOG_START + (gLogCount -1) * sizeof(LogEntry)) : // Correção aqui
                          (proximoEnderecoLog - sizeof(LogEntry));
       if (ultimoLogAddr >= ADDR_LOG_START) {
          EEPROM.get(ultimoLogAddr, gLastLog);
       } else {
          gLastLog.timestamp = 0;
       }
  } else {
       gLastLog.timestamp = 0;
  }

  // Inicializa buffers de leitura
  for (int i = 0; i < NUM_LEITURAS; ++i) {
      leiturasLuz[i] = 0.0; leiturasTemp[i] = 15.0; leiturasUmid[i] = 50.0;
  }
  gMediaTemp = dht.readTemperature(); gMediaUmid = dht.readHumidity();
  if(isnan(gMediaTemp)) gMediaTemp = 15.0; if(isnan(gMediaUmid)) gMediaUmid = 50.0;
  int vl = analogRead(LDR_PIN);
  gMediaLuz = map(constrain(vl, cfgLdrMin, cfgLdrMax), cfgLdrMin, cfgLdrMax, 0, 100);

  mostrarMensagem("Monitorando...", 2000, 1);
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
  if (menuatual == MENU_MONITOR && key) {
       menuatual = MENU_PRINCIPAL;
       menuAnterior = MENU_MONITOR;
  }

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
    case ACAO_SET_UMID_CRITICO_MAX: if (handleInput(key, cfgUmidCriticoMax, 0, 100)) { EEPROM.put(ADDR_UMID_CRITICO_MAX, cfgUmidCriticoMax); mostrarMensagem("Limites Salvos!", 2000, 2); menuatual = menuAnterior; limparInputBuffer(); } break;

    // --- Ações (Calibração) ---
    case ACAO_CALIBRA_LDR_MIN: if (key == 'D') { cfgLdrMin = calcularMediaAnalog(LDR_PIN, 10); EEPROM.put(ADDR_LDR_MIN, cfgLdrMin); menuatual = ACAO_CALIBRA_LDR_MAX; } else if (key == 'C') menuatual = menuAnterior; break;
    case ACAO_CALIBRA_LDR_MAX: if (key == 'D') { cfgLdrMax = calcularMediaAnalog(LDR_PIN, 10); EEPROM.put(ADDR_LDR_MAX, cfgLdrMax); mostrarMensagem("LDR Calibrado!", 2000, 2); menuatual = menuAnterior; } else if (key == 'C') menuatual = menuAnterior; break;

    // --- Ações (Confirmação) ---
    case ACAO_RESET_CONFIRMA:  if (key == 'D') { restaurarPadroes(); mostrarMensagem("Resetado!", 2000, 2); menuatual = menuAnterior; } else if (key == 'C') menuatual = menuAnterior; break;
    case ACAO_LIMPAR_LOG_CONF: if (key == 'D') { limparLogsEEPROM(); mostrarMensagem("Logs Limpos!", 2000, 2); menuatual = menuAnterior; } else if (key == 'C') menuatual = menuAnterior; break;

    // --- Telas Estáticas ---
    case ACAO_VER_LOG:         if (key == 'C') menuatual = menuAnterior; break;
  }
}

void executarAcao(MenuState acaoId) {
    switch(acaoId) {
        case ACAO_DEBUG_SERIAL:
            debugEEPROM();
            mostrarMensagem("Debug no Serial", 2000, 2);
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
  static bool displayNeedsClear = true;

  // --- Lógica para alternar display a cada INTERVALO_CICLO_DISPLAY ---
  if (menuatual == MENU_MONITOR && agora - tempoCicloDisplay >= INTERVALO_CICLO_DISPLAY) {
      displayMostraValores = !displayMostraValores;
      tempoCicloDisplay = agora;
      menuAnteriorDisplay = (MenuState)-1;
      displayNeedsClear = true;
  }


  if (menuatual == ACAO_MOSTRAR_MSG) {
    if (agora > gMensagemTimer) { menuatual = menuAnterior; menuAnteriorDisplay = (MenuState)-1; displayNeedsClear = true;}
    if (menuAnteriorDisplay != ACAO_MOSTRAR_MSG) {
      lcd.clear();
      // Linha 0 e 3 com borda, 1 e 2 com mensagem
      lcdPrintFixed(0, 0, F("********************"));
      lcdPrintFixed(0, 1, gMensagemTexto);
      lcdPrintFixed(0, 2, F("")); // Limpa linha 2
      lcdPrintFixed(0, 3, F("********************"));
      menuAnteriorDisplay = ACAO_MOSTRAR_MSG;
    } return;
  }

  bool forcarRefresh = (agora - tempoUltimoRefresh > 1000);
  if (menuatual >= ACAO_SET_LUZ_ALERTA && menuatual <= ACAO_SET_UMID_CRITICO_MAX) forcarRefresh = true;
  if (menuatual == MENU_MONITOR) forcarRefresh = true;

  if (menuatual == menuAnteriorDisplay && !forcarRefresh) return;

  if (menuatual != menuAnteriorDisplay || displayNeedsClear) {
      if(!(menuatual == MENU_MONITOR && menuAnteriorDisplay == MENU_MONITOR)) {
           lcd.clear();
      }
      displayNeedsClear = false;
  }

  menuAnteriorDisplay = menuatual; tempoUltimoRefresh = agora;

  char linha1[21] = ""; char linha2[21] = ""; char linha3[21] = ""; char linha4[21] = "";

  DateTime utcTime = rtc.now();
  DateTime localTime = DateTime(utcTime.unixtime() + (UTC_OFFSET * 3600L));

  switch (menuatual) {
    case MENU_MONITOR:
        if (displayMostraValores) {
            // --- Ecrã 1: Valores (Layout 20x4) ---
            char tempStr[6], umidStr[6];
            dtostrf(gMediaTemp, 4, 1, tempStr);
            dtostrf(gMediaUmid, 4, 1, umidStr);
            sprintf(linha1, "Luz: %d%%", (int)gMediaLuz);
            sprintf(linha2, "Temp: %s C", tempStr);
            sprintf(linha3, "Umid: %s %%", umidStr);
            sprintf(linha4, "%02d/%02d/%04d   %02d:%02d", localTime.day(), localTime.month(), localTime.year(), localTime.hour(), localTime.minute());
        } else {
            // --- Ecrã 2: Status (Layout 20x4) ---
            strcpy_P(linha1, PSTR("Status Sensores"));
            strcpy_P(linha2, PSTR("Luz:"));
            strcpy_P(linha3, PSTR("Temp:"));
            strcpy_P(linha4, PSTR("Umid:"));
        }
        break;

    case MENU_PRINCIPAL:
      strcpy_P(linha1, PSTR("Menu Principal"));
      strcpy_P(linha2, PSTR("1.Monitoramento"));
      strcpy_P(linha3, PSTR("2.Configuracoes"));
      strcpy_P(linha4, PSTR("3.Logs EEPROM"));
      break;
      
    case MENU_CONFIG:
      strcpy_P(linha1, PSTR("Configuracoes"));
      strcpy_P(linha2, PSTR("1.Definir Limites"));
      strcpy_P(linha3, PSTR("2.Calibrar LDR"));
      strcpy_P(linha4, PSTR("3.Reset Fabrica"));
      break;

    case MENU_LOGS:
      strcpy_P(linha1, PSTR("Logs EEPROM"));
      sprintf(linha2, "1.Ver Ultimo Log (%d)", gLogCount);
      strcpy_P(linha3, PSTR("2.Limpar Logs"));
      strcpy_P(linha4, PSTR("3.Debug Serial"));
      break;

    case ACAO_VER_LOG:
      strcpy_P(linha1, PSTR("Ultimo Alarme"));
      if (gLastLog.timestamp == 0) {
          strcpy_P(linha2, PSTR("Nenhum log salvo"));
      } else {
          DateTime logTime(gLastLog.timestamp + (UTC_OFFSET * 3600L));
          sprintf(linha2, "%02d/%02d/%02d   %02d:%02d", logTime.day(), logTime.month(), logTime.year() % 100, logTime.hour(), logTime.minute());
          
          char tempStrLog[6], umidStrLog[6];
          dtostrf(gLastLog.temperatura, 4, 0, tempStrLog);
          dtostrf(gLastLog.umidade, 4, 0, umidStrLog);
          sprintf(linha3, "T:%sC U:%s%% L:%d%%", tempStrLog, umidStrLog, gLastLog.luz);
          strcpy_P(linha4, PSTR("(C=Voltar)"));
      }
      break;
      
    case MENU_ALARME_ATIVO:
      strcpy_P(linha1, PSTR("********************"));
      strcpy_P(linha2, PSTR("ALARME ATIVO!"));
      // Mostra quais sensores estão em alarme
      if(gEstadoLuz != 0) strcat_P(linha3, PSTR("Luz!"));
      if(gEstadoTemp != 0) strcat_P(linha3, PSTR("Temp!"));
      if(gEstadoUmid != 0) strcat_P(linha3, PSTR("Umid!"));
      strcpy_P(linha4, PSTR("Pressione D p/Silenciar"));
      break;
      
    // --- Telas de Input ---
    // (Todas agora usam 4 linhas para mais clareza)
    case ACAO_SET_LUZ_ALERTA:   
      strcpy_P(linha1, PSTR("Definir Limite(Luz)"));
      strcpy_P(linha2, PSTR("Luz Alerta (0-100):"));
      strcpy(linha3, gInputBuffer); // Linha de input
      strcpy_P(linha4, PSTR("(*=Limpa C=Sai D=OK)"));
      break;
    case ACAO_SET_LUZ_CRITICO:  
      strcpy_P(linha1, PSTR("Definir Limite(Luz)"));
      strcpy_P(linha2, PSTR("Luz Critico (0-100):"));
      strcpy(linha3, gInputBuffer);
      strcpy_P(linha4, PSTR("(*=Limpa C=Sai D=OK)"));
      break;
    case ACAO_SET_TEMP_ALERTA_MIN: 
      strcpy_P(linha1, PSTR("Definir Limite(Temp)"));
      strcpy_P(linha2, PSTR("Alerta MIN (-10~20):"));
      strcpy(linha3, gInputBuffer);
      strcpy_P(linha4, PSTR("(*=Limpa C=Sai D=OK)"));
      break;
    case ACAO_SET_TEMP_CRITICO_MIN: 
      strcpy_P(linha1, PSTR("Definir Limite(Temp)"));
      strcpy_P(linha2, PSTR("Critico MIN (-10~20):"));
      strcpy(linha3, gInputBuffer);
      strcpy_P(linha4, PSTR("(*=Limpa C=Sai D=OK)"));
      break;
    case ACAO_SET_TEMP_ALERTA_MAX: 
      strcpy_P(linha1, PSTR("Definir Limite(Temp)"));
      strcpy_P(linha2, PSTR("Alerta MAX (0~50):"));
      strcpy(linha3, gInputBuffer);
      strcpy_P(linha4, PSTR("(*=Limpa C=Sai D=OK)"));
      break;
    case ACAO_SET_TEMP_CRITICO_MAX: 
      strcpy_P(linha1, PSTR("Definir Limite(Temp)"));
      strcpy_P(linha2, PSTR("Critico MAX (0~50):"));
      strcpy(linha3, gInputBuffer);
      strcpy_P(linha4, PSTR("(*=Limpa C=Sai D=OK)"));
      break;
    case ACAO_SET_UMID_ALERTA_MIN: 
      strcpy_P(linha1, PSTR("Definir Limite (Umid)"));
      strcpy_P(linha2, PSTR("Alerta MIN (0~100):"));
      strcpy(linha3, gInputBuffer);
      strcpy_P(linha4, PSTR("(*=Limpa C=Sai D=OK)"));
      break;
    case ACAO_SET_UMID_CRITICO_MIN: 
      strcpy_P(linha1, PSTR("Definir Limite(Umid)"));
      strcpy_P(linha2, PSTR("Critico MIN (0~100):"));
      strcpy(linha3, gInputBuffer);
      strcpy_P(linha4, PSTR("(*=Limpa C=Sai D=OK)"));
      break;
    case ACAO_SET_UMID_ALERTA_MAX: 
      strcpy_P(linha1, PSTR("Definir Limite(Umid)"));
      strcpy_P(linha2, PSTR("Alerta MAX (0~100):"));
      strcpy(linha3, gInputBuffer);
      strcpy_P(linha4, PSTR("(*=Limpa C=Sai D=OK)"));
      break;
    case ACAO_SET_UMID_CRITICO_MAX: 
      strcpy_P(linha1, PSTR("Definir Limite(Umid)"));
      strcpy_P(linha2, PSTR("Critico MAX (0~100):"));
      strcpy(linha3, gInputBuffer);
      strcpy_P(linha4, PSTR("(*=Limpa C=Sai D=OK)"));
      break;
      
    case ACAO_CALIBRA_LDR_MIN: 
      strcpy_P(linha1, PSTR("Calibrar LDR"));
      strcpy_P(linha2, PSTR("Apague a luz..."));
      strcpy_P(linha3, PSTR("Pressione D p/gravar"));
      strcpy_P(linha4, PSTR("(C=Cancelar)"));
      break;
    case ACAO_CALIBRA_LDR_MAX: 
      strcpy_P(linha1, PSTR("Calibrar LDR"));
      strcpy_P(linha2, PSTR("Acenda a luz..."));
      strcpy_P(linha3, PSTR("Pressione D p/gravar"));
      strcpy_P(linha4, PSTR("(C=Cancelar)"));
      break;
      
    case ACAO_RESET_CONFIRMA:  
      strcpy_P(linha1, PSTR("Confirmacao"));
      strcpy_P(linha2, PSTR("Resetar p/ Fabrica?"));
      strcpy_P(linha3, PSTR(""));
      strcpy_P(linha4, PSTR("(D = Sim) (C = Nao)"));
      break;
    case ACAO_LIMPAR_LOG_CONF: 
      strcpy_P(linha1, PSTR("Confirmacao"));
      strcpy_P(linha2, PSTR("Limpar todos os Logs?"));
      strcpy_P(linha3, PSTR(""));
      strcpy_P(linha4, PSTR("(D = Sim) (C = Nao)"));
      break;
  }

  // Desenha as linhas (exceto para o modo Monitor de Status)
  if (!(menuatual == MENU_MONITOR && !displayMostraValores)) {
      lcdPrintFixed(0, 0, linha1);
      lcdPrintFixed(0, 1, linha2);
      lcdPrintFixed(0, 2, linha3);
      lcdPrintFixed(0, 3, linha4);
  } else {
      // Desenha ícones (modo Monitor de Status)
      lcdPrintFixed(0, 0, linha1); // Título
      desenharStatusIndividual(0, 1, gEstadoLuz); // Linha 1
      lcdPrintFixed(5, 1, F("Luz"));
      desenharStatusIndividual(0, 2, gEstadoTemp); // Linha 2
      lcdPrintFixed(5, 2, F("Temperatura"));
      desenharStatusIndividual(0, 3, gEstadoUmid); // Linha 3
      lcdPrintFixed(5, 3, F("Umidade"));
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
    else { mostrarMensagem("Valor Invalido!", 2000, 2); menuatual = menuAnterior; limparInputBuffer(); }
  } return false;
}

void limparInputBuffer() { gInputBuffer[0] = '\0'; gInputIndex = 0; }

void mostrarMensagem(const char* texto, int duracao, int linha = 1) { // linha 1 ou 2
  strncpy(gMensagemTexto, texto, 20); // Aumentado para 20
  gMensagemTexto[20] = '\0';
  gMensagemTimer = millis() + duracao;
  menuAnterior = menuatual;
  menuatual = ACAO_MOSTRAR_MSG;
  // (A lógica de desenhar a mensagem foi movida para atualizarDisplay)
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
  Serial.println(F("Restaurando padroes EEPROM..."));
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
  Serial.println(F("===== DEBUG EEPROM ====="));
  Serial.print(F("Config Version: ")); Serial.println(CONFIG_VERSION);
  Serial.print(F("LDR Min/Max: ")); Serial.print(cfgLdrMin); Serial.print(F("/")); Serial.println(cfgLdrMax);
  Serial.print(F("Luz Alerta/Critico: ")); Serial.print(cfgLuzAlerta); Serial.print(F("/")); Serial.println(cfgLuzCritico);
  Serial.print(F("Temp Alerta Min/Max: ")); Serial.print(cfgTempAlertaMin); Serial.print(F("/")); Serial.println(cfgTempAlertaMax);
  Serial.print(F("Temp Critico Min/Max: ")); Serial.print(cfgTempCriticoMin); Serial.print(F("/")); Serial.println(cfgTempCriticoMax);
  Serial.print(F("Umid Alerta Min/Max: ")); Serial.print(cfgUmidAlertaMin); Serial.print(F("/")); Serial.println(cfgUmidAlertaMax);
  Serial.print(F("Umid Critico Min/Max: ")); Serial.print(cfgUmidCriticoMin); Serial.print(F("/")); Serial.println(cfgUmidCriticoMax);

  int count = contarLogs();
  Serial.print(F("\ LOGS (")); Serial.print(count); Serial.println(F(") ====="));
  if (count == 0) { Serial.println(F("Nenhum log salvo.")); return; }
  int endereco = ADDR_LOG_START; LogEntry log;
  for(int i = 0; i < count; i++) {
    EEPROM.get(endereco, log); DateTime dt(log.timestamp);
    Serial.print(F("[LOG ")); Serial.print(i + 1); Serial.print(F("] "));
    DateTime localLogTime = DateTime(log.timestamp + (UTC_OFFSET * 3600L));
    Serial.print(localLogTime.timestamp(DateTime::TIMESTAMP_FULL));
    Serial.print(F(" L:")); Serial.print(log.luz); Serial.print(F(" T:")); Serial.print(log.temperatura); Serial.print(F(" U:")); Serial.println(log.umidade);
    endereco += sizeof(LogEntry);
  }
  #else
  mostrarMensagem("Use Serial Monitor", 2000, 2);
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
    int limite = bufferCheio ? NUM_LEITURAS : indiceLeitura;

    if (limite == 0) {
        if (arr == leiturasLuz) return gMediaLuz;
        if (arr == leiturasTemp) return isnan(gMediaTemp) ? 15.0 : gMediaTemp;
        if (arr == leiturasUmid) return isnan(gMediaUmid) ? 50.0 : gMediaUmid;
        return 0.0;
    }

    for (int i = 0; i < limite; i++) {
        if (!isnan(arr[i])) {
            soma += arr[i];
            amostrasValidas++;
        }
    }

    if (amostrasValidas == 0) {
       if (arr == leiturasLuz) return gMediaLuz;
       if (arr == leiturasTemp) return isnan(gMediaTemp) ? 15.0 : gMediaTemp;
       if (arr == leiturasUmid) return isnan(gMediaUmid) ? 50.0 : gMediaUmid;
       return 0.0;
    }
    return soma / (float)amostrasValidas;
}


void desenharStatusIndividual(int col, int row, int estado) {
  lcd.setCursor(col, row);
  if (estado == 0) { // OK
    lcd.write(byte(1)); lcd.print(F(" ")); lcd.write(byte(3));
  } else if (estado == 1) { // ALERTA
    lcd.write(byte(4)); lcd.print(F(" ")); lcd.write(byte(5));
  } else { // CRITICO
    lcd.write(byte(6)); lcd.print(F(" ")); lcd.write(byte(7));
  }
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

void lcdPrintFixed(int col, int row, const char* text) {
  lcd.setCursor(col, row);
  int len = strlen(text);
  for (int i = 0; i < len && (col + i) < LCD_COLS; i++) lcd.print(text[i]);
  for (int i = len; (col + i) < LCD_COLS; i++) lcd.print(F(" "));
}
void lcdPrintFixed(int col, int row, const __FlashStringHelper* text) {
    lcd.setCursor(col, row);
    int len = strlen_P((const char*)text);
    for (int i = 0; i < len && (col + i) < LCD_COLS; i++) lcd.print((char)pgm_read_byte_near(((const char*)text) + i));
    for (int i = len; (col + i) < LCD_COLS; i++) lcd.print(F(" "));
}


