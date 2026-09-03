#include <Wire.h>
#include <RTClib.h>
#include <avr/sleep.h>

RTC_DS3231 rtc;

const int VALVULA_PIN = 4; // Pino ligado ao Gate do IRLZ44N
const int WAKEUP_PIN = 2;  // OBRIGATÓRIO: Pino ligado ao SQW do DS3231

void setup() {
  // 1. Segurança Máxima na Inicialização
  digitalWrite(VALVULA_PIN, LOW); // Força 0V no Gate
  pinMode(VALVULA_PIN, OUTPUT);   // Define como saída só depois de garantir o estado baixo

  // 2. Inicialização do RTC
  if (!rtc.begin()) {
    // Se o RTC falhar ou soltar um fio, o sistema trava aqui.
    // É melhor não irrigar do que irrigar infinitamente e secar o reservatório.
    while (1); 
  }

  // Se o RTC perder energia (bateria moeda fraca), ajusta a hora na hora da compilação.
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // 3. Configuração dos Alarmes de Hardware (O Segredo da Bateria)
  rtc.disable32K(); // Desliga saída 32kHz (economiza bateria do módulo)
  rtc.writeSqwPinMode(DS3231_OFF); // Fundamental para usar o pino SQW como interruptor de alarme

  rtc.clearAlarm(1);
  rtc.clearAlarm(2);

  // Define os alarmes. DS3231 possui 2 alarmes embutidos.
  // Alarme 1: Aciona às 10:00:00 (Frequência: disparar quando Horas, Minutos e Segundos baterem)
  rtc.setAlarm1(DateTime(0, 0, 0, 10, 0, 0), DS3231_A1_Hour);
  
  // Alarme 2: Aciona às 16:00:00 (Frequência: disparar quando Horas e Minutos baterem)
  rtc.setAlarm2(DateTime(0, 0, 0, 16, 0, 0), DS3231_A2_Hour);
}

void loop() {
  // O loop começa sempre verificando se o alarme do relógio disparou.
  // Se o Arduino acordou do Deep Sleep, ele vai checar qual alarme foi.
  if (rtc.alarmFired(1) || rtc.alarmFired(2)) {
    
    // Liga a Válvula
    digitalWrite(VALVULA_PIN, HIGH);
    
    // Mantém ligada por exatos 3 minutos (180.000 milissegundos)
    delay(180000); 
    
    // Desliga a Válvula
    digitalWrite(VALVULA_PIN, LOW);

    // Limpa a flag dos alarmes no chip DS3231 para permitir que disparem amanhã
    rtc.clearAlarm(1);
    rtc.clearAlarm(2);
  }

  // Se a rotina de irrigação terminou, ou se o Nano acordou por ruído elétrico,
  // manda ele de volta para o coma imediatamente.
  dormirProfundamente();
}

// ------------------------------------------------------------------
// Função que desliga a CPU e os periféricos do Arduino (Deep Sleep)
// ------------------------------------------------------------------
void dormirProfundamente() {
  sleep_enable(); // Habilita a flag de dormida
  
  // Define o pino 2 como gatilho. Quando o SQW do DS3231 for para GND (FALLING), a CPU acorda.
  attachInterrupt(digitalPinToInterrupt(WAKEUP_PIN), acordar, FALLING); 
  
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // Desliga os clocks internos e CPU (maior economia possível)
  
  // A execução do código PARA NESTA LINHA
  sleep_cpu(); 
  
  // ----------------------------------------------------------------
  // A CPU foi acordada pelo relógio e a execução RECOMEÇA DESTA LINHA
  // ----------------------------------------------------------------
  sleep_disable(); // Desabilita a dormida
  detachInterrupt(digitalPinToInterrupt(WAKEUP_PIN)); // Desliga a interrupção para evitar disparos em cascata
}

// Rotina de Serviço de Interrupção (ISR)
// Deve ser mantida o mais rápida e vazia possível. Serve apenas para tirar a CPU do estado sleep_cpu().
void acordar() {
  // Não colocar absolutamente nada aqui. Nem Serial.print, nem delay().
}
