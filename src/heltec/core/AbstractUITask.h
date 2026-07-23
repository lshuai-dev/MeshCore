#pragma once
#include <Arduino.h>
#include <MeshCore.h>
#include <helpers/BaseSerialInterface.h>

enum class UIEventType {
  none,
  contactMessage,
  channelMessage,
  roomMessage,
  newContactMessage,
  ack
};

class AbstractUITask {
 public:
  AbstractUITask() : _board(nullptr), _serial(nullptr), _connected(false) {}
  AbstractUITask(mesh::MainBoard* board, BaseSerialInterface* serial)
      : _board(board), _serial(serial), _connected(false) {}

  virtual void setHasConnection(bool connected) { _connected = connected; }
  bool hasConnection() const { return _connected; }
  bool isSerialEnabled() const { return _serial->isEnabled(); }
  void enableSerial() { _serial->enable(); }
  void disableSerial() { _serial->disable(); }
  virtual void msgRead(int msgcount) = 0;
  virtual void newMsg(uint8_t path_len, const char* from_name, const char* text, int msgcount) = 0;
  virtual void notify(UIEventType t = UIEventType::none) = 0;
  virtual void loop() = 0;

 protected:
  mesh::MainBoard* _board;
  BaseSerialInterface* _serial;
  bool _connected;
};
