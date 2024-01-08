#include "CanvasLabel.h"
#include "AzIoTConfig.h"
#include <M5Stack.h>
#include <M5GFX.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <SHT3X.h>
#include <QMP6988.h>
#include <PubSubClient.h>

// Canvasの設定
M5GFX lcd;
M5Canvas canvas(&lcd);
CanvasLabel *_topLabel = 0;
CanvasLabel *_topSubLabel = 0;
CanvasLabel *_bottomLabel = 0;
CanvasLabel *_timerLabel = 0;

// Wi-Fiの設定
const char *ssid = CONFIG_WIFI_SSID;
const char *password = CONFIG_WIFI_PASS;
WiFiMulti _wifiMulti;

// MQTTの設定
WiFiClientSecure wifiClient;
PubSubClient mqttClient(wifiClient);
const char _mqttBroker[]      = CONFIG_MQTT_BROKER;
const char _mqttDeviceId[]  =  CONFIG_DEVICE_ID;
const int16_t _mqttPort = CONFIG_MQTT_PORT;
const char _mqttUsername[] = CONFIG_USERNAME;
const char _mqttPassword[] = CONFIG_SAS_TOKEN;
const String _mqttPubTopic = "devices/" + String(_mqttDeviceId) + "/messages/events/";
const String _mqttSubTopic = "devices/" + String(_mqttDeviceId) + "/messages/devicebound/#";

PROGMEM const char root_ca[] = R"(-----BEGIN CERTIFICATE-----
MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH
MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI
2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx
1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ
q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz
tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ
vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP
BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV
5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY
1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4
NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG
Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91
8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe
pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl
MrY=
-----END CERTIFICATE-----
)";


//ENV IIIセンサーの設定
SHT3X sht30;
QMP6988 qmp6988;

// タイマーカウント用
#define JST (3600L * 9)
const long _timerIntervalTimerLabel = 1000;
const long _timerIntervalUpdateDeviceTwin = 1000 * 60 * 5;

// function headers
void SetupWiFi(const char* ssid, const char* password);
String GetDateTimeAsString();
String GetHumidityFromSencsor();
String GetTempertureFromSencsor();

//MQTT function headers
void ConnectMQTT();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void SendD2CMessage();
void UpdateDeviceTwinReportedProperty(String batteryLevel, String temperture, String humidity);
void RespondToDirectMethod(String topic, String &reqBody);

void setup() {
  M5.begin();         // 本体初期化
  lcd.begin();        // 画面初期化
  M5.Power.begin();   // バッテリープロパティ使用
  Serial.begin(19200); // シリアル通信速度固定 https://qiita.com/Rcobb/items/20ee2a0613d081295327
  lcd.setBrightness(10);

  // 温湿度センサーを初期化
  sht30.init();
  qmp6988.init();
  // 時間取得関数初期化
  configTime(JST,0,"ntp.nict.jp", "time.google.com","ntp.jst.mfeed.ad.jp");

  // テキストラベルを配置
  _timerLabel = new CanvasLabel(lcd, "", 0,0 ,lcd.width(),16);
  _topLabel = new CanvasLabel(lcd, "システムが起動されました。", 0, 16 ,lcd.width(),16);
  _topSubLabel = new  CanvasLabel(lcd, "",0, 32, lcd.width(), lcd.height()-64-32);
  _bottomLabel = new  CanvasLabel(lcd, "",0, lcd.height()-32, lcd.width(), 32);

  // Wi-Fiに接続する
  SetupWiFi(ssid, password);

  // MQTT接続設定  
  // MQTTサーバーの設定
  mqttClient.setServer(_mqttBroker,_mqttPort);
  // コールバック関数の登録
  mqttClient.setCallback(mqttCallback);
  // ルート証明書登録
  wifiClient.setCACert(root_ca);

  // MQTT接続
  ConnectMQTT();

  delay(1000);
}


void loop() {
  M5.update();
  
  // MQTT接続を維持する
  mqttClient.loop();
  if(mqttClient.connected() == false){
    Serial.println("MQTTクライアントが切断されました。");
    delay(1000);
    ConnectMQTT();
  }

  // 現在時刻を更新
  static long nextTimeTimerLabel = 0;
  if(millis() >=  nextTimeTimerLabel){
    _timerLabel->UpdateLabel(GetDateTimeAsString());

    nextTimeTimerLabel = millis() + _timerIntervalTimerLabel;
  }

  // デバイスツインのプロパティを更新
  static long nextTimeUpdateDeviceTwin = 0;
  if(millis() >=  nextTimeUpdateDeviceTwin){

    String batteryLevel = String(M5.Power.getBatteryLevel()) + "%";
    String temperature = GetTempertureFromSencsor();
    String humidity = GetHumidityFromSencsor();
    UpdateDeviceTwinReportedProperty(batteryLevel,temperature,humidity);

    nextTimeUpdateDeviceTwin = millis() + _timerIntervalUpdateDeviceTwin;
  }

  // ボタンCが押されたとき、D2Cメッセージを送信する
  static bool BtnCPressed = false;
  if(M5.BtnC.isPressed() && BtnCPressed == false){   
    BtnCPressed = true; 

    // D2Cメッセージを送信する
    SendD2CMessage();
    _bottomLabel->UpdateLabel("D2Cメッセージが送信されました。");
  }
  if(M5.BtnC.isReleased()){
    BtnCPressed = false;
  }

  delay(100);
}

#pragma region mqtt

  // MQTTに接続します
  void ConnectMQTT(){

    // 再試行回数
    int retryCount = 3;
    do{
      _topLabel->UpdateLabel("MQTT接続中です。\n");
      _topSubLabel->UpdateLabel("MQTTに接続します。 残り" + String(retryCount) + "回");
  
      //_mqttDeviceId
      if (mqttClient.connect(_mqttDeviceId ,_mqttUsername ,_mqttPassword)) {
        Serial.println("MQTTサーバーに接続しました。");
      }else{
        // MQTT 接続エラーの場合はつながるまで 5 秒ごとに繰り返します
        Serial.print("MQTT接続に失敗しました。Error state=");
        Serial.println(mqttClient.state());
      }

      if(mqttClient.connected()) {
        _topLabel->UpdateLabel("MQTTに接続しました。");
        Serial.println("You're connected to the MQTT broker!");
        break;
      }else{
        _topLabel->UpdateLabel("MQTTに失敗しました。");
      }

      delay(3000);
    }while(retryCount--);
    
    // サブスクライブするトピックの設定
    if(mqttClient.connected() == true){
      _topSubLabel->UpdateLabel("IoT Hubからのイベントを購読します。");
      // C2Dメッセージ
      mqttClient.subscribe(_mqttSubTopic.c_str());
      // ダイレクトメソッド
      mqttClient.subscribe("$iothub/methods/POST/#");
      // DeviceTwinのReportedプロパティ変更通知(デバイス側プロパティ)
      mqttClient.subscribe("$iothub/twin/res/#");
      // DeviceTwinのDesiredプロパティ変更通知(クラウド側プロパティ)
      mqttClient.subscribe("$iothub/twin/PATCH/properties/desired/#");
   
    }

  }

  // MQTTコールバック関数
  //MQTT Brokerからメッセージを受信されたときのコールバック関数です。
  void mqttCallback(char* topic, byte* payload, unsigned int length) {

    _topLabel->UpdateLabel("IoT Hubからメッセージを取得しました。");

    // トピックを文字列に変換
    String topicName = topic;

    // ペイロードを文字列に変換
    String str = "";
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
        str += (char)payload[i];
    }
    Serial.print("\n");
    Serial.println("Received. topic=" + topicName);

    // トピックの種類ごとに分岐
    if(topicName.startsWith("$iothub/twin/res/204/")){
      // デバイス側プロパティを更新した場合

      _topSubLabel->UpdateLabel("デバイスプロパティが正常に更新されました。");
      _bottomLabel->UpdateLabel(topicName);
      return;      
    }else if(topicName.startsWith("$iothub/twin/PATCH/properties/desired/")){      
      // サービス側プロパティを更新した場合
      
      _topSubLabel->UpdateLabel("サービス側のプロパティが正常に更新されました。");
      _bottomLabel->UpdateLabel(topicName);
      return;

    }else if(topicName.startsWith("$iothub/methods/POST/")){
      // ダイレクトメソッドを受信した場合

      _topSubLabel->UpdateLabel("ダイレクトメソッドが呼び出されました。");
      _bottomLabel->UpdateLabel(topicName);

      // ここに温度などを返す
      RespondToDirectMethod(topicName, str);
      return;
    }
    
    // D2Cメッセージを取得した場合
    _topSubLabel->UpdateLabel("D2Cメッセージを取得しました。");
    _bottomLabel->UpdateLabel(str);

  }


  // IoT HubにD2Cメッセージを送信します。
  void SendD2CMessage()
  {

    DynamicJsonDocument json(200);
    char data_json[200];
    json["MQTT_TOPIC"] = "Message From M5Stack";
    size_t payloadSize = serializeJson(json, data_json);

    // D2Cメッセージをパブリッシュ
    mqttClient.publish(_mqttPubTopic.c_str(), data_json);
  }

  // デバイスツインのReporteプロパティを変更します。
  void UpdateDeviceTwinReportedProperty(String batteryLevel, String temperture, String humidity){

    // メッセージをパブリッシュ
    const char rid[] = "1";

    // プロパティを定義する。
    DynamicJsonDocument json(200);
    char data_json[200];
    json["BatteryLevel"] = batteryLevel +"%";
    json["Temperture"] = temperture;
    json["Humidity"] = humidity;        
    size_t payloadSize = serializeJson(json, data_json);

    // プロパティをパブリッシュ
    mqttClient.publish("$iothub/twin/PATCH/properties/reported/?$rid=1" , data_json);
  }

  // ダイレクトメソッドに応答します
  void RespondToDirectMethod(String topic, String &reqBody){
  
  // topic「$iothub/methods/POST/MethodName/$rid=1」からMethodNameを抜き出します。
  // MethodNameの抜き出し
  String methodName = topic;
  methodName.replace("$iothub/methods/POST/","");
  methodName = methodName.substring(0,methodName.indexOf("/"));
  Serial.println("Method Name=" + methodName);

  // ridの抜き出し
  String rid = topic.substring(topic.lastIndexOf("=")+1);
  Serial.println("rid=" + rid);

  String resTopic = "$iothub/methods/res/200/?$rid=" + rid;

  DynamicJsonDocument json(200);
  char data_json[200];  
  size_t payloadSize;

  if(methodName == "GetTelemetry"){
    String batteryLevel = String(M5.Power.getBatteryLevel());
    String temperature = GetTempertureFromSencsor();
    String humidity = GetHumidityFromSencsor();
    json["BatteryLevel"] = batteryLevel +"%";
    json["Temperture"] = temperature;
    json["Humidity"] = humidity;        
    payloadSize = serializeJson(json, data_json);
  }else{
    json["Message"] = "M5Stack is responding to direct methods.";

  }

  // 応答メッセージをメッセージをパブリッシュ
  mqttClient.publish(resTopic.c_str(), data_json);
}

#pragma endregion

#pragma region functions

  // Wi-Fiに接続します。
  void SetupWiFi(const char* ssid, const char* password){

    // WiFi接続
    _wifiMulti.addAP(ssid, password);

    int retryCount = 3;
    do{
      _topLabel->UpdateLabel("WiFi接続中です。");
      // 試行回数
      int tcount = 20;
      while (tcount-- > 0 && _wifiMulti.run() != WL_CONNECTED){
        _topLabel->AddLabelText(".");
        delay(500);
      }
      
      if(_wifiMulti.run() == WL_CONNECTED){
        _topLabel->UpdateLabel("WiFi接続が完了しました。");
        _topSubLabel->AddLabelText(WiFi.localIP().toString(),true);
        _topSubLabel->AddLabelText(WiFi.dnsIP().toString(),true);
        _topSubLabel->AddLabelText(WiFi.subnetMask().toString(),true);
        _topSubLabel->AddLabelText(WiFi.macAddress(),true);
        break;
      }

      // 後始末
      _topSubLabel->UpdateLabel("接続できませんでした。リトライします。");
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      delay(10);

    }while(retryCount--);

  }

  // 日時をテキストで取得します
  String GetDateTimeAsString(){
    struct tm tm;
    if(getLocalTime(&tm)){
      char str[100];
      sprintf(str, "%04d-%02d-%02dT%02d:%02d:%02d", 
        tm.tm_year + 1900, // 年
        tm.tm_mon + 1, // 月
        tm.tm_mday, // 日
        tm.tm_hour, // 時
        tm.tm_min, // 分
        tm.tm_sec // 秒
      );
      return String(str);
    }

    return "";
  }
  // 温度センサーのデータを返します。
  String GetTempertureFromSencsor(){

    // 温度湿度を取得
    if(sht30.get()!=0){
      return String("--℃");
    }

    float temperture = 0;
    temperture = sht30.cTemp;
    //%全体の桁数.小数点以下の桁数float
    char str[10];
    sprintf(str, "%2.0f℃" , temperture);

    return String(str);
  }

  // 温度センサーのデータを返します。
  String GetHumidityFromSencsor(){

    // 温度湿度を取得
    if(sht30.get()!=0){
      return String("--%");
    }

    float humidity = 0;
    humidity = sht30.humidity;  

    //%全体の桁数.小数点以下の桁数float
    char str[10];
    sprintf(str, "%2.0f %%" , humidity);

    return String(str);
  }



#pragma endregion