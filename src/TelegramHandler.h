#ifndef TELEGRAMHANDLER_H
#define TELEGRAMHANDLER_H

#include <FastBot.h>

class TelegramHandler {
private:
  FastBot& bot;

  static void newMsg(FB_msg& msg);
  // Checks for new messages every 1 second.
  const int botRequestDelay = 1000;

public:
 // Constructor
  TelegramHandler(FastBot& bot);
  void begin();
  void tick();
  void sendMessage(const String& message);
  void sendMessage(const String& message, const String&  chat_id);
  void sendMainMenu(const String& chatId);
  void handleNewMessages(FB_msg& msg);
  String unescapeUnicode(const String& input);
};

#endif