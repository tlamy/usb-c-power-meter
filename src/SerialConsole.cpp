//
// Copyright (c) 2025 Thomas Lamy
// SPDX-License-Identifier: MIT
//
#include "SerialConsole.h"

SerialConsole::SerialConsole(Stream& serial) : _serial(serial) {}

void SerialConsole::registerCommand(const String& name, const String& description, CommandHandler handler) {
  _commands[name] = {description, std::move(handler)};
}

void SerialConsole::poll() {
  while (_serial.available()) {
    const char c = static_cast<char>(_serial.read());
    if (c == '\r') {
      continue;
    }
    if (c == '\n') {
      if (_active) {
        _serial.println();
      }
      _lineBuffer.trim();
      if (_lineBuffer.length() > 0) {
        processLine(_lineBuffer);
      }
      _lineBuffer = "";
      return;  // one line per poll to avoid blocking loop()
    }
    if (_active) {
      _serial.print(c);
    }
    _lineBuffer += c;
  }
}

void SerialConsole::processLine(const String& line) {
  if (!_active) {
    if (line == "diag" || line == "d") {
      _active = true;
      _serial.println("\n--- Command mode active (type 'help', 'exit' to leave) ---");
      printPrompt();
    }
    return;
  }

  const Args args = splitArgs(line);
  if (args.empty()) {
    printPrompt();
    return;
  }

  String cmd = args[0];
  cmd.toLowerCase();

  if (cmd == "exit" || cmd == "quit" || cmd == "q") {
    _active = false;
    _serial.println("Exiting command mode.");
    return;
  }

  if (cmd == "help" || cmd == "h" || cmd == "?") {
    printHelp();
    printPrompt();
    return;
  }

  const auto it = _commands.find(cmd);
  if (it != _commands.end()) {
    it->second.handler(args);
  } else {
    _serial.printf("Unknown command '%s' (type 'help')\n", cmd.c_str());
  }
  printPrompt();
}

void SerialConsole::printPrompt() const { _serial.print("> "); }

void SerialConsole::printHelp() const {
  _serial.println("Commands:");
  _serial.println("  help / h / ?     show this help");
  _serial.println("  exit / quit / q  leave command mode");
  for (const auto& entry : _commands) {
    _serial.printf("  %-16s %s\n", entry.first.c_str(), entry.second.description.c_str());
  }
}

SerialConsole::Args SerialConsole::splitArgs(const String& line) {
  Args args;
  int start = 0;
  const int len = static_cast<int>(line.length());
  for (int i = 0; i <= len; i++) {
    if (i == len || line[i] == ' ') {
      if (i > start) {
        args.push_back(line.substring(start, i));
      }
      start = i + 1;
    }
  }
  return args;
}
