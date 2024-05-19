//
// Intel 8080 CPU emulation on an Arduino Nano
//            derived from Z80PACK
// Copyright 2024, Udo Munk
//
// History:
// 19-MAY-2024 implemented configuration dialog

// read an config command line from the terminal
void get_cmdline(char *buf, int len)
{
  int i = 0;
  char c;

  while (i < len - 1) {
    while (!Serial.available())
      ;
    c = Serial.read();
    if (c != '\r') {
      buf[i] = c;
      Serial.write(c);
      i++;
    } else {
      Serial.write('\n');
      break;
    }
  }
  buf[i] = '\0';
}

// configuration dialog for the machine
void config(void)
{
  char s[10];
  int go_flag = 0;

  while (!go_flag) {
    Serial.print(F("1 - set port 255 value, currently "));
    Serial.println(fp_value, HEX);
    Serial.println(F("2 - load file"));
    Serial.println(F("3 - run machine\n"));
    Serial.print(F("Command: "));

    get_cmdline(s, 2);
    Serial.println(F("\n"));

    switch (*s) {
    case '1':
      break;

    case '2':
      Serial.print(F("Filename: "));
      get_cmdline(s, 9);
      Serial.println();
      load_file(s);
      break;

    case '3':
      go_flag = 1;
      break;

    default:
      break;
    }
  }
}
