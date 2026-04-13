
#include <PZEM004Tv30.h>
#include <HTTPClient.h>
#include <WiFi.h>


int limit_for_http = 50; // после этого цикла отправка
int loop_counter = 0; // колво циклов сейчас


int stage_now = 90; // 90 80 70 60 50 40
int old_stage = 90;

int burden_switch_counter = 0;

bool burden_corrector = false;


char big_data[40000] = "val=";

PZEM004Tv30 pzem(Serial2, 19, 18);        // (rxPin, txPin) общая сеть
PZEM004Tv30 solar_pzem(Serial1, 17, 16);  // (rxPin, txPin) солнце



void wifi_connection(){
  delay(1000);

  WiFi.mode(WIFI_STA);
  WiFi.setHostname("electromer");
  
  WiFi.begin("netis_kitchen", "password"); 
  //WiFi.begin("netis_7AB60F", "password"); 


  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  
}


void send_http(){
  
  // проверка статуса вайфай рестарт если что:
  if (WiFi.status() != WL_CONNECTED){
    WiFi.disconnect();
    wifi_connection();
  }

  
  // Указываем URL и данные для POST запроса
  HTTPClient http;
  http.begin("https://fuguja.com/esp/esp_test.php");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  // Отправляем POST запрос
  int httpResponseCode = http.POST(big_data);
  String payload;
  if (httpResponseCode == 200) {    
    payload = http.getString();
  }    

  http.end();  

  if(payload == "ok" || limit_for_http > 303){
    // перезапись переменной
    loop_counter = 0;
    limit_for_http =50;
    memset(big_data, 0, sizeof(big_data));
    strcat(big_data, "val="); // обнуляем большую строку     
  } else {
    // отправлено не было продолжаем запись в переменную
    limit_for_http +=50;
  }
  
}


void setup() {
  delay(1000);
  Serial.begin(115200);

  delay(1000);
  wifi_connection();  
  delay(1000);
  
  pinMode(23, OUTPUT); // Устанавливаем пин 23 как выход
  digitalWrite(23, LOW);

  pinMode(32, OUTPUT); // 80
  pinMode(33, OUTPUT); // 70
  pinMode(25, OUTPUT); // 60
  pinMode(26, OUTPUT); // 50
  pinMode(27, OUTPUT); // 40

  digitalWrite(32, LOW); // 80
  digitalWrite(33, LOW); // 70
  digitalWrite(25, LOW); // 60
  digitalWrite(26, LOW); // 50
  digitalWrite(27, LOW); // 40
}

void set_stage(int stage){
  
  digitalWrite(32, LOW); // 80
  digitalWrite(33, LOW); // 70
  digitalWrite(25, LOW); // 60
  digitalWrite(26, LOW); // 50
  digitalWrite(27, LOW); // 40
  
       if(stage == 80){ digitalWrite(32, HIGH); }
  else if(stage == 70){ digitalWrite(33, HIGH); }
  else if(stage == 60){ digitalWrite(25, HIGH); }
  else if(stage == 50){ digitalWrite(26, HIGH); }
  else if(stage == 40){ digitalWrite(27, HIGH); }
  
  }

void loop() {

  loop_counter++;  
  Serial.println("------------: "+String(loop_counter));  
  
  float p = pzem.power();       // мощность квт
  float pf = pzem.pf();        // фактор
  float v = pzem.voltage();     // напряжение В
  float a = pzem.current();     // Ток А
  float kwt = pzem.energy(); // сумма КВТ

  float p_solar = solar_pzem.power();       // мощность квт
  float a_solar = solar_pzem.current();       // 
  float pf_solar = solar_pzem.pf();       // 
  float kwt_solar = solar_pzem.energy();    // сумма КВТ

  if (!isfinite(p)) p = 0;
  if (!isfinite(pf)) pf = 0;
  if (!isfinite(v)) v = 0;
  if (!isfinite(a)) a = 0;
  if (!isfinite(kwt)) kwt = 0;
  
  if (!isfinite(p_solar)) p_solar = 0;
  if (!isfinite(a_solar)) a_solar = 0;
  if (!isfinite(pf_solar)) pf_solar = 0;
  if (!isfinite(kwt_solar)) kwt_solar = 0;
  
  
  sprintf(big_data + strlen(big_data), "{%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%d,%d,%d,%lu}", v, p, a, pf, kwt, kwt_solar, p_solar, a_solar, pf_solar, WiFi.RSSI(), stage_now, burden_switch_counter, millis());

  if (p_solar > 170) {
    digitalWrite(23, HIGH); // Включаем пин 23 (на нем появляется 3.3В)
  } 
  if (p_solar < 120) {
    digitalWrite(23, LOW);  // Выключаем пин 23
  }



if(v > 0 && kwt_solar > 0){

    if(!burden_corrector && p < 550 && p_solar > p+10){
      // потребление меньше 600 и генерация превысила потребление включаем режим коррекции
      burden_corrector = true;
      }
  
    if (burden_corrector && p < 500 && p_solar > 0 && p > p_solar * 1.3) {
      // режим коррекции был включен но генерация все равно не дотягивает смылса в бурдене нет
        burden_corrector = false;
        old_stage = 90;
        stage_now = 90;
        set_stage(stage_now);      
      }
  
    if(burden_corrector){
      int pf_int_now = (int)(pf * 100.0f + 0.5f);
      if (pf_int_now > 0 && pf_int_now <= 100) {
          if      (pf_int_now >= 85){ stage_now = 90; }
          else if (pf_int_now >= 75){ stage_now = 80; }
          else if (pf_int_now >= 65){ stage_now = 70; }
          else if (pf_int_now >= 55){ stage_now = 60; }
          else if (pf_int_now >= 45){ stage_now = 50; }
          else                      { stage_now = 40; }
      
          if (stage_now != old_stage) {
              set_stage(stage_now);
              old_stage = stage_now;
              burden_switch_counter++;
          }
      }
    }

}


 
  
  if(loop_counter == limit_for_http){
    send_http();
  }
  
  delay(10000); 

}
