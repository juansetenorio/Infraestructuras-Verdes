#ifndef TOKEN_MANAGER_H
#define TOKEN_MANAGER_H

#include <Arduino.h>

class TokenManager {
 private:
  String token;
  unsigned long tokenExpiry = 0;

  String urlEncode(const String& value);

 public:
  bool requestToken();
  bool ensureValidToken();

  String getToken() const { return token; }
  bool hasToken() const;
  void clear();
};

extern TokenManager tokenManager;

#endif
