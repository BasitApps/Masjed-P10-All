"""Build-local ESP8266 core 3.1.2 multipart fixes; never edit the shared SDK."""
from hashlib import sha256
from pathlib import Path

PARSER_SHA256 = "f8fe756b04222f20c49813ea63f0c8a90a99034255324de84349e32755afe818"


def harden(source):
    if sha256(source.encode()).hexdigest() != PARSER_SHA256:
        raise RuntimeError("ESP8266 parser changed: review the local safety patch before building")

    def replace(old, new):
        nonlocal source
        if source.count(old) != 1:
            raise RuntimeError("ESP8266 parser patch anchor changed")
        source = source.replace(old, new, 1)

    replace('#include <Arduino.h>', '#include <Arduino.h>\n#include <new>')
    replace('  // Read the first line of HTTP request', '''  _currentArgsHavePlain = 0;
  // Read the first line of HTTP request''')
    replace("    while(!client.available() && client.connected())\n      yield();", """    const uint32_t started = millis();
    while (!client.available() && client.connected()) {
      if (millis() - started >= HTTP_MAX_POST_WAIT) return -1;
      yield();
    }""")
    # Bounds apply before allocating a plain body, not only in the API handler.
    replace("    String plainBuf;", """    if (!isForm && contentLength >= 512) return CLIENT_MUST_STOP;
    String plainBuf;""")
    replace("  _currentArgs = new RequestArgument[_currentArgCount + 1];", """  _currentArgs = new (std::nothrow) RequestArgument[_currentArgCount + 1];
  if (!_currentArgs) { _currentArgCount = 0; return; }""")
    start = source.index("bool ESP8266WebServerTemplate<ServerType>::_parseForm(")
    end = source.index("template <typename ServerType>\nString", start)
    form = source[start:end]
    form = form.replace("  String line;", """  const uint32_t started = millis();
  _currentArgsHavePlain = 0;
  _currentUpload.reset();
  if (boundary.length() == 0 || boundary.length() > 70) return false;
  size_t retainedBytes = 0;
  bool readOk = true;
  auto readUntil = [&](char delimiter) {
    String result;
    for (;;) {
      if (millis() - started >= 120000UL) { readOk = false; return result; }
      const int c = _uploadReadByte(client);
      if (c < 0) { readOk = false; return result; }
      if (c == delimiter) return result;
      if (result.length() >= 1024) { readOk = false; return result; }
      result += char(c);
    }
  };
  uint8_t parts = 0;
  String line;""", 1)
    form = form.replace("client.readStringUntil", "readUntil")
    # Every consumed header/value/trailer pair checks the bounded read result.
    form = form.replace("readUntil('\\n');", "readUntil('\\n');\n      if (!readOk) return _parseFormUploadAborted();")
    form = form.replace("    _postArgs = new RequestArgument[WEBSERVER_MAX_POST_ARGS];", """    _postArgs = new (std::nothrow) RequestArgument[WEBSERVER_MAX_POST_ARGS];
    if (!_postArgs) return _parseFormUploadAborted();""", 1)
    form = form.replace("    while(1){", """    while(1){
      if (++parts > WEBSERVER_MAX_POST_ARGS || millis() - started >= 120000UL)
        return _parseFormUploadAborted();""", 1)
    form = form.replace("              if (line.startsWith(\"--\"+boundary)) break;", """              if (line == ("--"+boundary) || line == ("--"+boundary+"--")) break;
              if (retainedBytes + argName.length() + argValue.length() + line.length() + 1 >= 512)
                return _parseFormUploadAborted();""", 1)
    form = form.replace("            RequestArgument& arg = _postArgs[_postArgsLen++];", """            if (_postArgsLen >= WEBSERVER_MAX_POST_ARGS) return _parseFormUploadAborted();
            retainedBytes += argName.length() + argValue.length();
            RequestArgument& arg = _postArgs[_postArgsLen++];""", 1)
    form = form.replace("            _currentUpload.reset(new HTTPUpload());", """            _currentUpload.reset(new (std::nothrow) HTTPUpload());
            if (!_currentUpload) return _parseFormUploadAborted();""", 1)
    form = form.replace("            while ( true ) {", """            while ( true ) {
                if (millis() - started >= 120000UL)
                    return _parseFormUploadAborted();""", 1)
    form = form.replace("            continue;", """            if (line.length() != 0) return _parseFormUploadAborted();
            continue;""", 1)
    form = form.replace("      }\n    }\n\n    int iarg;", """      } else return _parseFormUploadAborted();
    }

    int iarg;""", 1)
    form = form.replace("    _currentArgs = new RequestArgument[_postArgsLen];", """    _currentArgCount = 0;
    _currentArgs = new (std::nothrow) RequestArgument[_postArgsLen];
    if (!_currentArgs && _postArgsLen) return _parseFormUploadAborted();""", 1)
    source = source[:start] + form + source[end:]
    replace("""  _currentUpload->status = UPLOAD_FILE_ABORTED;
  if(_currentHandler && _currentHandler->canUpload(_currentUri))
    _currentHandler->upload(*this, _currentUri, *_currentUpload);
  return false;""", """  if (_currentUpload) {
    _currentUpload->status = UPLOAD_FILE_ABORTED;
    if(_currentHandler && _currentHandler->canUpload(_currentUri))
      _currentHandler->upload(*this, _currentUri, *_currentUpload);
  }
  delete[] _postArgs;
  _postArgs = nullptr;
  _postArgsLen = 0;
  return false;""")
    return source


def configure(env):
    sdk = Path(env.PioPlatform().get_package_dir("framework-arduinoespressif8266")) / "libraries/ESP8266WebServer/src"
    overlay = Path(env.subst("$PROJECT_BUILD_DIR")) / "basit-webserver"
    overlay.mkdir(parents=True, exist_ok=True)
    files = {
        "BasitWebServer.h": "#ifdef ESP8266WEBSERVER_H\n#error BasitWebServer must be included first\n#endif\n#define BASIT_WEBSERVER_HARDENED 1\n" + (sdk / "ESP8266WebServer.h").read_text().replace('#include "Parsing-impl.h"', '#include "BasitParsing-impl.h"'),
        "BasitParsing-impl.h": harden((sdk / "Parsing-impl.h").read_text()),
    }
    for name, data in files.items():
        output = overlay / name
        if not output.exists() or output.read_text() != data:
            output.write_text(data, encoding="utf-8")
    env.Prepend(CPPPATH=[str(overlay)])


if "Import" in globals():
    Import("env")
    configure(env)
