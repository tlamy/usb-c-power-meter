//
// Copyright (c) 2025 Thomas Lamy
// SPDX-License-Identifier: MIT
//
#pragma once

#include <Arduino.h>

#include <functional>
#include <map>
#include <vector>

class SerialConsole {
public:
  using Args = std::vector<String>;
  using CommandHandler = std::function<void(const Args&)>;

  explicit SerialConsole(Stream& serial);

  // Register a command. Aliases can be registered separately with the same handler.
  void registerCommand(const String& name, const String& description, CommandHandler handler);

  // Call from loop(). Non-blocking.
  void poll();

  bool isActive() const { return _active; }

private:
  Stream& _serial;
  bool _active = false;
  String _lineBuffer;

  struct Command {
    String description;
    CommandHandler handler;
  };
  std::map<String, Command> _commands;

  void processLine(const String& line);
  void printPrompt() const;
  void printHelp() const;
  static Args splitArgs(const String& line);
};
