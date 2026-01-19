/*
Copyright (c) 2021 Jakub Mandula

Example of using one PZEM module with Software Serial interface.
================================================================

If only RX and TX pins are passed to the constructor, software 
serial interface will be used for communication with the module.

memset(big_data, 0, sizeof(big_data)); // обнуление данных в переменной



{v:200,p:2500,a:40,ms:123123123123123,pf:50.25,kwt:8000,kwts:60000,ps:500,as:20,pfs:1000,wf:60.55}

*/

#include <PZEM004Tv30.h>
#include <HTTPClient.h>
#include <WiFi.h>


int limit_for_http = 50; // после этого цикла отправка
int loop_counter = 0; // колво циклов сейчас
int wf_attempt = 0;


char big_data[20000] = "val=";

PZEM004Tv30 pzem(Serial2, 19, 18);        // общая сеть
PZEM004Tv30 solar_pzem(Serial1, 17, 16);  // солнце


void wifi_connection(){
  wf_attempt++;

  WiFi.begin("netis_kitchen", "password"); 
  //WiFi.begin("netis_7AB60F", "password");  
  //WiFi.begin("netis_2.4G_007C9D", "d11v09n03");
  
  Serial.print("Connecting to WiFi");

  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  WiFi.setHostname("electromer");

  Serial.print("OK! IP=");
  Serial.println(WiFi.localIP());
}


void send_http(){
  Serial.println("send_http_func");
  
  // проверка статуса вайфай рестарт если что:
  if (WiFi.status() != WL_CONNECTED || limit_for_http == 100 || limit_for_http == 150){
    sprintf(big_data + strlen(big_data), "|wifi_restart_status_error:%d,limit_now:%d|", WiFi.status(), limit_for_http);
    WiFi.disconnect();
    wifi_connection();
  }

  sprintf(big_data + strlen(big_data), "|wf_stat:%d,wf_attempt:%d,before_http_ms:%d,limit_now:%d|", WiFi.status(), wf_attempt, millis(), limit_for_http);
  
  // Указываем URL и данные для POST запроса
  HTTPClient http;
  http.begin("https://fuguja.com/esp/esp_test.php");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  // Отправляем POST запрос
  float httpResponseCode = http.POST(big_data);
  String payload;
  if (httpResponseCode == 200) {    
    payload = http.getString();
    //Serial.print("HTTP Response code: ");
    //Serial.println(httpResponseCode);
    //Serial.println(payload);    
  }
    
  //Serial.println("end http: "+String(millis()));
  //Serial.println("html_result:  "+payload);
  http.end();  

  if(payload == "ok" || limit_for_http > 101){
    // перезапись переменной
    loop_counter = 0;
    limit_for_http =50;
    memset(big_data, 0, sizeof(big_data));
    strcat(big_data, "val="); // обнуляем большую строку

     if(payload == "ok"){
      sprintf(big_data + strlen(big_data), "|success_after_http_ms:%d,http_response:%s,http_reponse_code:%.2f|", millis(), payload.c_str(), httpResponseCode);
    } else {
      sprintf(big_data + strlen(big_data), "|more_then_600_error_after_http_ms:%d,http_response:%s,http_reponse_code:%.2f|", millis(), payload.c_str(), httpResponseCode);
    }    
  } else {
    // отправлено не было продолжаем запись в переменную
    limit_for_http +=50;
    sprintf(big_data + strlen(big_data), "|after_http_not_send_ms:%d,http_response:%s,http_reponse_code:%.2f|", millis(), payload.c_str(), httpResponseCode);
  }
  
}


void setup() {
  delay(4000);
  Serial.begin(115200);

  delay(1000);
  wifi_connection();  
  delay(1000);
  
  pinMode(23, OUTPUT); // Устанавливаем пин 23 как выход
  digitalWrite(23, HIGH); // инверсная логика
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
  
  float rssi = WiFi.RSSI();  // wi-fi dbi
  
  /*
  float p = 1;       // мощность квт
  float pf = 2;        // фактор
  float v = 3;     // напряжение В
  float a = 4;     // Ток А
  float kwt = 5; // сумма КВТ
  float rssi = 6;  // wi-fi dbi
  */
  
  sprintf(big_data + strlen(big_data), "{v:%.2f,p:%.2f,a:%.2f,ms:%d,pf:%.2f,kwt:%.2f,kwts:%.2f,ps:%.2f,as:%.2f,pfs:%.2f,wf:%.2f}", v, p, a, millis(), pf, kwt, kwt_solar, p_solar, a_solar, pf_solar, rssi);

  if (p_solar > 170) {
    digitalWrite(23, LOW); // Включаем пин 23 (на нем появляется 3.3В)
  } 
  if (p_solar < 120) {
    digitalWrite(23, HIGH);  // Выключаем пин 23
  }
  
  //Serial.println(big_data);
  
  if(loop_counter == limit_for_http){
    send_http();
  }
  
  delay(10000); 

}
