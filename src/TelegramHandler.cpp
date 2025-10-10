#include "TelegramHandler.h"
#include <config.h>
//#include <json.h>

// Message handler wrapper (needed because FastBot uses function pointers)
TelegramHandler* botInstance = nullptr;
void messageHandlerWrapper(FB_msg& msg) {
    if (botInstance != nullptr) {
        botInstance->handleNewMessages(msg);
    }
}

// Constructor
TelegramHandler::TelegramHandler(FastBot &bot) : bot(bot) {
botInstance = this;
}
// Initialize the bot
void TelegramHandler::begin()
{
  bot.attach(messageHandlerWrapper);
  // Set the Telegram bot properies
  bot.setPeriod(botRequestDelay);
  bot.showMenu("Welcome! Use /start to begin.");
}

// Handle incoming messages
void TelegramHandler::tick()
{
  bot.tick();
}

void TelegramHandler::sendMessage(const String &message)
{
  bot.sendMessage(message);
}

void TelegramHandler::sendMessage(const String &message, const String &chat_id)
{
  bot.sendMessage(message, chat_id);
}

void TelegramHandler::sendMainMenu(const String& chat_id)
{
  String menu = "🏠 *Котел* \n\n";
      menu += "Текущее состояние:\n";
      menu += "• Температура: " + String(roomTemperature, 1) + "°C\n";
      menu += "• Уставка: " +  String(sliderValue) + "°C\n";
      menu += "• Отопление: " +  String(button1State ? "ВКЛ" : "ВЫКЛ") + "\n";
      menu += "• Вода: " +  String(button2State ? "ВКЛ" : "ВЫКЛ") + "\n";
      menu += "• Режим: " +  String(button3State ? "АВТО" : "РУЧ") + "\n";
      menu += "• Пламя: " +  String(flameState ? "ВКЛ" : "ВЫКЛ") + "\n";
      
      //String buttons = "[\"📊 Статус\", \"🌡️ Уставка\"],";
      //buttons += "[\"🔥 Включить\", \"❄️ Выключить\"],";
      //buttons += "[\"🔄 Авто\", \"✋ Ручной\"]";
      //String buttons = F("Menu 1 \t Menu 2 \t Menu 3 \n Back");
      String buttons = F("📊 Статус \t 🌡️ Уставка \n");
      buttons += F("🔥 Включить \t ❄️ Выключить \n");
      buttons += F("🔄 Авто \t ✋ Ручной");
      
      bot.inlineMenu(menu, buttons, chat_id);
}

String TelegramHandler::unescapeUnicode(const String& input) {
    String result;
    int length = input.length();
    
    for (int i = 0; i < length; i++) {
        if (i + 5 < length && 
            input[i] == '\\' && 
            input[i + 1] == 'u') {
            
            // Извлекаем hex код
            String hexCode = input.substring(i + 2, i + 6);
            uint32_t unicodeValue = strtoul(hexCode.c_str(), NULL, 16);
            
            // Конвертируем Unicode code point в UTF-8
            if (unicodeValue <= 0x7F) {
                // 1 байт: 0xxxxxxx
                result += (char)unicodeValue;
            } else if (unicodeValue <= 0x7FF) {
                // 2 байта: 110xxxxx 10xxxxxx
                result += (char)(0xC0 | (unicodeValue >> 6));
                result += (char)(0x80 | (unicodeValue & 0x3F));
            } else if (unicodeValue <= 0xFFFF) {
                // 3 байта: 1110xxxx 10xxxxxx 10xxxxxx
                result += (char)(0xE0 | (unicodeValue >> 12));
                result += (char)(0x80 | ((unicodeValue >> 6) & 0x3F));
                result += (char)(0x80 | (unicodeValue & 0x3F));
            } else if (unicodeValue <= 0x10FFFF) {
                // 4 байта: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
                result += (char)(0xF0 | (unicodeValue >> 18));
                result += (char)(0x80 | ((unicodeValue >> 12) & 0x3F));
                result += (char)(0x80 | ((unicodeValue >> 6) & 0x3F));
                result += (char)(0x80 | (unicodeValue & 0x3F));
            }
            
            i += 5; // Пропускаем обработанные символы
        } else {
            result += input[i];
        }
    }
    
    return result;
}

void TelegramHandler::handleNewMessages(FB_msg &msg)
{
  Serial.println("ReceivedNewMessages");
  // Chat id of the requester
  String chat_id = msg.chatID;
  Serial.println("chat ID" + chat_id);
  // Print the received message
  String text = msg.data;
   Serial.println(text);
   Serial.println(unescapeUnicode(msg.data));

  String from_name = msg.first_name;

  if (chat_id == CHAT_ID)
  {
    // update firmware

    if (msg.OTA)
    {
      Serial.println(msg.fileName);
      if (msg.fileName == "spiffs.bin")
      {
        Serial.println("Received Spiffs");
        bot.updateFS(); // update spiffs
      }
      else
      {
        Serial.println("Received OTA");
        bot.update(); // update OTA
      }
    }

    // download file to FS
    /*if (msg.isFile) {                     // это файл
    Serial.print("Downloading ");
    Serial.println(msg.fileName);

    String path = '/' + msg.fileName;   // путь вида /filename.xxx
    //File f = SPIFFS.open("/freq.json", "w");
    File f = LittleFS.open(path, "w");
    //File f = SPIFFS.open(path, "w");  // открываем для записи
    //bool status = bot.downloadFile(f, msg.fileUrl);  // загружаем
    Serial.println(status ? "OK" : "Error");    // статус
  }
*/
    if (msg.text == "/start" || msg.data == "\\ud83d\\udcca \\u0421\\u0442\\u0430\\u0442\\u0443\\u0441")  //📊 Статус
    {
      sendMainMenu(CHAT_ID);
    }
    //Serial.println("🔥Включить");
    //Serial.println(unescapeUnicode(msg.data));
    if (msg.data == "\\ud83d\\udd25 \\u0412\\u043a\\u043b\\u044e\\u0447\\u0438\\u0442\\u044c") //"🔥Включить")
    //if (unescapeUnicode(msg.data) == "🔥 Включить")
    {
      Serial.print("Start boiler from bot");
      button1State = true;
      enableCentralHeating = true;
      bot.sendMessage("✅ Обогрев включен", CHAT_ID);
      sendMainMenu(CHAT_ID);
    
      
    }
    else if (msg.data == "\\u2744\\ufe0f \\u0412\\u044b\\u043a\\u043b\\u044e\\u0447\\u0438\\u0442\\u044c")  //"❄️ Выключить")
    {
      Serial.print("Stop boiler from bot");
      button1State = false;
      enableCentralHeating = false;
      bot.sendMessage("✅ Обогрев выключен", CHAT_ID);
      sendMainMenu(CHAT_ID);
    }
    else if (msg.data == "\\ud83d\\udd04 \\u0410\\u0432\\u0442\\u043e")  //"🔄 Авто 
    {
      Serial.print("Auto mode from bot");
      button3State = true;
      bot.sendMessage("✅ Режим Авто", CHAT_ID);
      sendMainMenu(CHAT_ID);
    }
    else if (msg.data == "\\u270b \\u0420\\u0443\\u0447\\u043d\\u043e\\u0439")  //"✋ Ручной"
    {
      Serial.print("Manual mode from bot");
      button3State = false;
      bot.sendMessage("✅ Режим Ручной", CHAT_ID);
      sendMainMenu(CHAT_ID);
    }
    else if (text.substring(0, 3) == "@st")
    {
      String conf = "Config:\n";
      conf += "Room Temp: " + String(roomTemperature, 3) + "\n";
      conf += "Water Temp:" + String(waterTemperature, 3) + "\n";
    
/*    Serial.print(", Water Temp: ");
    Serial.print(waterTemperature);
    Serial.print("°C, Steam Temp: ");
    Serial.print(steamTemperature);
    Serial.print("°C, Slider: ");
    Serial.print(sliderValue);
    Serial.print(", Btn1: ");
    Serial.print(button1State ? "ON" : "OFF");
    Serial.print(", Btn2: ");
    Serial.print(button2State ? "ON" : "OFF");
    Serial.print(", Btn3: ");
    Serial.print(button3State ? "AUTO" : "MAN");
    Serial.print(", Flame: ");
    Serial.println(flameState ? "ON" : "OFF");
  */    
      bot.sendMessage(conf, CHAT_ID);
    }
  }
}