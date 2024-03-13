#include "esp_camera.h"
#include "Arduino.h"
#include "FS.h"
#include <WiFi.h>
#include "ESP32_MailClient.h"
#include "SPIFFS.h"
#include "driver/rtc_io.h"
#include <EEPROM.h>

// Credenciais de rede
char ssid[] = "NOME_DA_REDE_WIFI";
char password[] = "SENHA_DA_REDE_WIFI";

// Para enviar e-mail usando o Gmail, use a porta 465 (SSL) e o servidor SMTP smtp.gmail.com
// VOCÊ DEVE ATIVAR a opção de aplicativos menos seguros em https://myaccount.google.com/lesssecureapps?pli=1
#define emailSenderAccount "EMAIL_DE_ENVIO"
#define emailSenderPassword "SENHA_DO_EMAIL_DE_ENVIO"
#define emailRecipient "EMAIL_DE_DESTINO"
#define smtpServer "smtp.gmail.com"
#define smtpServerPort 465
#define emailSubject "Foto Capturada pelo ESP32-CAM"
SMTPData smtpData;

// Definição dos pinos para CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22
#define TIME_TO_SLEEP 300           // tempo em que o ESP32 irá dormir (em segundos)
#define uS_TO_S_FACTOR 1000000ULL  // fator de conversão de microssegundos para segundos
int pictureNumber = 0;

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  // desabilita o detector de queda de tensão

  Serial.begin(115200);
  Serial.println();

  Serial.print("Conectando");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(200);
  }

  Serial.println();
  Serial.println("WiFi conectado.");
  Serial.println();
  Serial.println("Preparando para enviar e-mail");
  Serial.println();
  delay(500);

  if (!SPIFFS.begin(true)) {
    Serial.println("Ocorreu um erro ao montar o SPIFFS");
    ESP.restart();
  } else {
    Serial.println("SPIFFS montado com sucesso");
  }
  SPIFFS.format();

  EEPROM.begin(400);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;

  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Inicializa a câmera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Falha na inicialização da câmera com erro 0x%x", err);
    return;
  }

  capturePhoto();
  sendEmail();

  // Limpa todos os dados do objeto de e-mail para liberar memória
  smtpData.empty();
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

void loop() {
}

void capturePhoto() {
  camera_fb_t *fb = NULL;

  // Tira uma foto com a câmera
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Falha na captura da câmera");
    return;
  }

  // Caminho onde a nova imagem será salva no cartão SD
  String path = "/Sight.jpg";
  File file = SPIFFS.open(path, FILE_WRITE);


  if (!file) {
    Serial.println("Falha ao abrir o arquivo em modo de escrita");

  } else {
    file.write(fb->buf, fb->len);
    Serial.printf("Arquivo salvo no caminho: %s\n", path);
  }

  file.close();
  esp_camera_fb_return(fb);

  // Desliga o LED branco embarcado do ESP32-CAM (flash) conectado ao GPIO 4
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);
  rtc_gpio_hold_en(GPIO_NUM_4);
}
void sendEmail() {

  // Define o host do servidor de e-mail SMTP, porta, conta e senha
  smtpData.setLogin(smtpServer, smtpServerPort, emailSenderAccount, emailSenderPassword);

  // Define o nome do remetente e o e-mail
  smtpData.setSender("Foto", emailSenderAccount);

  // Define a prioridade do e-mail como Alta, Normal, Baixa ou de 1 a 5 (1 é a mais alta)
  smtpData.setPriority("Alta");

  // Define o assunto
  smtpData.setSubject(emailSubject);

  // Define a mensagem com formato HTML
  smtpData.setMessage("<div style=\"color:#2f4468;\"><h1>Encontre uma coisa!</h1><p>- Enviado a partir da placa ESP32</p></div>", true);

  smtpData.addRecipient(emailRecipient);
  smtpData.setFileStorageType(MailClientStorageType::SPIFFS);
  smtpData.addAttachFile("/Sight.jpg");
  smtpData.setSendCallback(sendCallback);

  // Inicia o envio do e-mail, pode ser definida uma função de retorno para acompanhar o status
  if (!MailClient.sendMail(smtpData)) {
    Serial.println("Erro ao enviar e-mail, " + MailClient.smtpErrorReason());
    }
}


void sendCallback(SendStatus msg) {
  // Imprime o status atual
  Serial.println("Status do envio de e-mail:");
  Serial.println(msg.info());

  // Verifica se houve sucesso no envio
  if (msg.success()) {
    Serial.println("O e-mail foi enviado com sucesso!");
  } else {
    // Imprime o motivo do erro
    Serial.println("Erro ao enviar e-mail:");
  }

  Serial.println("----------------");
}