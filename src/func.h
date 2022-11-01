// Функция преобразования чисел с плавающей запятой в текст
// Function: convert float numbers to string
String fFTS(float x, byte precision)
{
  char tmp[50];
  dtostrf(x, 0, precision, tmp);
  return String(tmp);
}

float kalmanFilter(float val)
{
  Pc1 = P1 + Pr1;
  G1 = Pc1 / (Pc1 + Kl1);
  P1 = (1 - G1) * Pc1;
  Xp1 = Xe1;
  Zp1 = Xp1;
  Xe1 = G1 * (val - Zp1) + Xp1; // "фильтрованное" значение - Filtered value
  return (Xe1);
}

float unitsToRaw(float units)
{
  return units * scale.get_scale() + scale.get_offset();
}

float rawToUnits(float raw)
{
  return (raw - scale.get_offset()) / scale.get_scale();
}

// Функции для работы с весами
float readScales(int times)
{
  float value1 = scale.get_units(times / 2);
  delay(20);
  float value2 = scale.get_units(times / 2);
  return (fabs(value1 - value2) > 0.01) ? NAN : (value1 + value2) / 2;
}

float readScalesWithCheck(int times)
{
  while (true)
  {
    float result = readScales(times);
    if (!isnan(result))
    {
      return result;
    }
  }
}

void tareScalesWithCheck(int times)
{
  scale.set_offset(unitsToRaw(readScalesWithCheck(times)));
}

// Функции помп
// Pump function
void PumpStart(int npump, int npumpr)
{
  mcp.begin();
  mcp.pinMode(npump, OUTPUT);
  mcp.pinMode(npumpr, OUTPUT);
  mcp.digitalWrite(npump, HIGH);
  mcp.digitalWrite(npumpr, LOW);
}

void PumpStop(int npump, int npumpr)
{
  mcp.begin();
  mcp.pinMode(npump, OUTPUT);
  mcp.pinMode(npumpr, OUTPUT);
  mcp.digitalWrite(npump, LOW);
  mcp.digitalWrite(npumpr, LOW);
}

void PumpReverse(int npump, int npumpr)
{
  mcp.begin();
  mcp.pinMode(npump, OUTPUT);
  mcp.pinMode(npumpr, OUTPUT);
  mcp.digitalWrite(npump, LOW);
  mcp.digitalWrite(npumpr, HIGH);
}

// Функция налива
// Function: pour solution
float pumping(float wt, int npump, int npumpr, String nm, int preload)
{
  wstatus = "Worked...";
  wpomp = nm;

  server.handleClient();

  if (wt != 0 and wt < 400)
  {
    // Продувка
    // Mix the solution
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(nm);
    lcd.print(" Reverse...");
    PumpReverse(npump, npumpr);
    delay(preload);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(nm);
    lcd.setCursor(0, 1);
    lcd.print(" Preload=");
    lcd.print(preload);
    lcd.print("ms");
    tareScalesWithCheck(255);
    mcp.begin();

    PumpStart(npump, npumpr);

    delay(preload);

    server.handleClient();
    ArduinoOTA.handle();
    PumpStop(npump, npumpr);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(nm);
    lcd.print(":");
    lcd.print(wt);

    lcd.setCursor(10, 0);
    lcd.print("RUNING");

    float value = readScalesWithCheck(128);
    float pvalue, sk;

    while (value < wt - 0.01)
    {
      lcd.setCursor(0, 1);
      lcd.print(value, 2);
      lcd.print(" (");
      lcd.print(100 - (wt - value) / wt * 100, 0);
      lcd.print("%) ");
      lcd.print(sk, 0);
      lcd.print("ms     ");
      // mcp.pinMode(npump, OUTPUT);
      // mcp.digitalWrite(npump, HIGH);
      PumpStart(npump, npumpr);
      if (value < (wt - 1.5))
      {

        if (wt - value > 20)
        {
          delay(10000);
        }
        else
        {
          delay(1000);
        }
        pvalue = value;
        sk = 80;
      }
      else
      {
        lcd.setCursor(10, 0);
        lcd.print("PCIS ");

        if (value - pvalue < 0.01)
        {
          if (sk < 80)
          {
            sk = sk + 2;
          }
        }
        if (value - pvalue > 0.01)
        {
          if (sk > 2)
          {
            sk = sk - 2;
          }
        }
        if (value - pvalue > 0.1)
        {
          sk = 0;
        }

        pvalue = value;
        delay(sk);
      }
      // mcp.digitalWrite(npump, LOW);
      PumpStop(npump, npumpr);

      server.handleClient();
      ArduinoOTA.handle();

      delay(100);

      value = readScalesWithCheck(128);
      Xe1 = value;
      curvol = value;
    }
    PumpStop(npump, npumpr);
    lcd.setCursor(0, 1);
    lcd.print(value, 2);
    lcd.print(" (");
    lcd.print(100 - (wt - value) / wt * 100, 2);
    lcd.print("%)      ");
    // server.handleClient();
    PumpReverse(npump, npumpr);
    delay(preload * 2);
    PumpStop(npump, npumpr);

    return value;
  }
  else
  {
    lcd.setCursor(0, 0);
    lcd.print(nm);
    lcd.print(":");
    lcd.print(wt);
    lcd.setCursor(10, 0);
    lcd.print("SKIP..   ");
    delay(1000);
    server.handleClient();
  }
}

// Функция налива 2
float pumping2(float wt, int npump, int npumpr, String nm, int preload)
{
  // Инерция кальмана, чем меньше тем выше инерционность (труднее изменить значение)
  float Preload_Pr1 = 0.0000001; // Для прелоада
  float Speed_Pr1 = 0.005;       // Для быстрого налива
  float SpeedWeight = 3;         // При быстром наливе не доливать заданное число грамм (чтобы не промахнуться).
  int SpeedMid = 4;              // Число усреднений ацп при быстром наливе, чем выше тем меньше шума но есть шанс перелива

  float Drops_Pr1 = 0.0001; // Для капельного налива
  float psize = 0.02;       // Примерный вес капли в граммах для адаптивного режима (зависит от выхода трубки)
  float atime = 5;          // Значение в мс плюс и минус для адаптивного режима
  float pgramm = 0.5;       // Значение в граммах при меньше которого не используется адаптивный прелоад

  wstatus = "Worked...";
  wpomp = nm;
  float value = 0;
  tareScalesWithCheck(255);
  curvol = value;
  server.handleClient();
  if (wt > 0)
  {

    // Прелоад
    wstatus = "Worked, preload...";
    float TimePreload = millis();

    if (wt > pgramm)
    {
      // Если  да то адаптивный, если нет то статический прелоад

      PumpReverse(npump, npumpr);
      delay(4);
      PumpStart(npump, npumpr);
      Pr1 = Preload_Pr1;
      while (value < 0.3)
      {
        server.handleClient();
        curvol = value;
        value = scale.get_units(8);
        lcd.setCursor(0, 0);
        lcd.print(nm);
        lcd.print(" Preload...");
        lcd.setCursor(0, 1);
        lcd.print(value, 1);
        lcd.print("g      ");
        lcd.print(millis() - TimePreload, 0);
        lcd.print("ms ");
      }
      TimePreload = millis() - TimePreload;
    }
    else // если налить мало, то делаем сперва реверс а затем прелоад статический.
    {
      lcd.setCursor(0, 0);
      lcd.print(nm);
      lcd.print(" SPreload...");
      PumpReverse(npump, npumpr);
      delay(preload);
      PumpStart(npump, npumpr);
      delay(preload);
    }

    // Быстрый налив
    Pr1 = Speed_Pr1;
    PumpStart(npump, npumpr);
    while (value < wt - SpeedWeight)
    {
      value = kalmanFilter(scale.get_units(SpeedMid));
      curvol = value;
      server.handleClient();
      lcd.setCursor(0, 0);
      lcd.print(nm);
      lcd.print(" Speed pump...");
      lcd.setCursor(0, 1);
      lcd.print(value, 2);
      lcd.print("g ");
      if (value > wt)
      {
        PumpStop(npump, npumpr);
        value = scale.get_units(255);
        Xe1 = value;
        PumpStart(npump, npumpr);
      }
    }

    PumpStop(npump, npumpr);
    delay(100);
    curvol = value;
    server.handleClient();
    value = scale.get_units(255);
    lcd.setCursor(0, 1);
    lcd.print(value, 2);
    lcd.print("g ");

    // Капельный

    Pr1 = Drops_Pr1; // Инерция кальмана, чем меньше тем медленнее
    int ws = 8;      // Усреднение измерений при быстром наливе, чем больше тем точнее но медленнее (можно пролить)

    float ms = 0;
    Xe1 = value;

    while (value < wt - psize)
    {
      curvol = value;
      server.handleClient();

      PumpStart(npump, npumpr);
      delay(ms);
      PumpStop(npump, npumpr);

      value = scale.get_units(ws);
      float v0 = value;
      lcd.setCursor(0, 0);
      lcd.print(nm);
      lcd.print(" Drops pump...");
      lcd.setCursor(0, 1);
      lcd.print(value, 2);
      lcd.print("g ");
      lcd.print(ms, 0);
      lcd.print("ms         ");

      PumpReverse(npump, npumpr);
      delay(4);
      PumpStart(npump, npumpr);
      delay(ms);
      PumpStop(npump, npumpr);

      value = scale.get_units(ws);
      float v1 = value;

      lcd.setCursor(0, 1);
      lcd.print(value, 2);
      lcd.print("g ");
      lcd.print(ms, 0);
      lcd.print("ms         ");

      if (abs((v1 - v0) / v0) > 0.01)
      {
        value = scale.get_units(254);
        Xe1 = value;
      }

      if (v1 - v0 < 0.02)
      {
        ms = ms + atime;
      }
      else
      {
        ms = ms - atime;
      }
      if (ms < atime)
      {
        ms = atime;
      }

      if (wt - value < 0.2)
      {
        ws = 80;
      }

      // Если между двумя замерами налилось больше 3-х капель сбросить скорость на начальную
      if (v1 - v0 > psize * 3)
      {
        ms = atime;
      }

      if (value - psize >= wt)
      {
        value = scale.get_units(254);
        Xe1 = value;
      }
    }

    PumpStop(npump, npumpr);
    delay(100);
    curvol = value;
    server.handleClient();
    value = scale.get_units(255);
    lcd.setCursor(0, 1);
    lcd.print(value, 2);
    lcd.print("g ");

    // Реверс-продувка

    float stime = millis();
    PumpReverse(npump, npumpr);
    while (millis() - stime < preload)
    {
      server.handleClient();
      lcd.setCursor(0, 0);
      lcd.print(nm);
      lcd.print(" Reverse...");
      lcd.setCursor(0, 1);
      lcd.print(preload - (millis() - stime), 0);
      lcd.print(" ms        ");
    }
    PumpStop(npump, npumpr);

    delay(3000);

    lcd.setCursor(0, 0);
    lcd.print(nm);
    lcd.print(" End pump...");
    lcd.setCursor(0, 1);
    lcd.print(value, 2);
    lcd.print("g ");
    lcd.setCursor(0, 1);
    lcd.print(value, 2);
    lcd.print(" (");
    lcd.print((value - wt) / wt * 100, 2);
    lcd.print("%)");
  }
  return value;
}

void test()
{
  float dl = 30000;
  server.send(200, "text/html", "testing pump...");
  lcd.home();
  lcd.print("Pump 1 Start");
  PumpStart(pump1, pump1r);
  delay(dl);
  lcd.home();
  lcd.print("Pump 1 Revers       ");
  PumpReverse(pump1, pump1r);
  delay(dl);
  lcd.home();
  lcd.print("Pump 1 Stop      ");
  delay(1000);
  PumpStop(pump1, pump1r);
  lcd.home();
  lcd.print("Pump 2 Start");
  PumpStart(pump2, pump2r);
  delay(dl);
  lcd.home();
  lcd.print("Pump 2 Revers       ");
  PumpReverse(pump2, pump2r);
  delay(dl);
  lcd.home();
  lcd.print("Pump 2 Stop      ");
  delay(1000);
  PumpStop(pump2, pump2r);
  lcd.home();
  lcd.print("Pump 3 Start");
  PumpStart(pump3, pump3r);
  delay(dl);
  lcd.home();
  lcd.print("Pump 3 Revers       ");
  PumpReverse(pump3, pump3r);
  delay(dl);
  lcd.home();
  lcd.print("Pump 3 Stop      ");
  delay(1000);
  PumpStop(pump3, pump3r);
  lcd.home();
  lcd.print("Pump 4 Start");
  PumpStart(pump4, pump4r);
  delay(dl);
  lcd.home();
  lcd.print("Pump 4 Revers       ");
  PumpReverse(pump4, pump4r);
  delay(dl);
  lcd.home();
  lcd.print("Pump 4 Stop      ");
  delay(1000);
  PumpStop(pump4, pump4r);
  lcd.home();
  lcd.print("Pump 5 Start");
  PumpStart(pump5, pump5r);
  delay(dl);
  lcd.home();
  lcd.print("Pump 5 Revers       ");
  PumpReverse(pump5, pump5r);
  delay(dl);
  lcd.home();
  lcd.print("Pump 5 Stop      ");
  delay(1000);
  PumpStop(pump5, pump5r);
  lcd.home();
  lcd.print("Pump 6 Start");
  PumpStart(pump6, pump6r);
  delay(dl);
  lcd.home();
  lcd.print("Pump 6 Revers       ");
  PumpReverse(pump6, pump6r);
  delay(dl);
  lcd.home();
  lcd.print("Pump 6 Stop      ");
  delay(1000);
  PumpStop(pump6, pump6r);
  lcd.home();
  lcd.print("Pump 7 Start");
  PumpStart(pump7, pump7r);
  delay(dl);
  lcd.home();
  lcd.print("Pump 7 Revers       ");
  PumpReverse(pump7, pump7r);
  delay(dl);
  lcd.home();
  lcd.print("Pump 7 Stop      ");
  delay(1000);
  PumpStop(pump7, pump7r);
  lcd.home();
  lcd.print("Pump 8 Start");
  PumpStart(pump8, pump8r);
  delay(dl);
  lcd.home();
  lcd.print("Pump 8 Revers       ");
  PumpReverse(pump8, pump8r);
  delay(dl);
  lcd.home();
  lcd.print("Pump 8 Stop      ");
  delay(1000);
  PumpStop(pump8, pump8r);
}

void st()
{
  mTimes = millis();

  float v1 = server.arg("p1").toFloat();
  float v2 = server.arg("p2").toFloat();
  float v3 = server.arg("p3").toFloat();
  float v4 = server.arg("p4").toFloat();
  float v5 = server.arg("p5").toFloat();
  float v6 = server.arg("p6").toFloat();
  float v7 = server.arg("p7").toFloat();
  float v8 = server.arg("p8").toFloat();

  String message = "Plant<br>P1:";
  message += fFTS(v1, 2);
  message += "<br>P2:";
  message += fFTS(v2, 2);
  message += "<br>P3:";
  message += fFTS(v3, 2);
  message += "<br>P4:";
  message += fFTS(v4, 2);
  message += "<br>P5:";
  message += fFTS(v5, 2);
  message += "<br>P6:";
  message += fFTS(v6, 2);
  message += "<br>P7:";
  message += fFTS(v7, 2);
  message += "<br>P8:";
  message += fFTS(v8, 2);

  message += "<meta http-equiv='refresh' content=0;URL=../";
  message += "?p1=" + server.arg("p1");
  message += "&p2=" + server.arg("p2");
  message += "&p3=" + server.arg("p3");
  message += "&p4=" + server.arg("p4");
  message += "&p5=" + server.arg("p5");
  message += "&p6=" + server.arg("p6");
  message += "&p7=" + server.arg("p7");
  message += "&p8=" + server.arg("p8");
  message += ">";

  server.send(200, "text/html", message);
  // A (1-3)
  scale.set_scale(scale_calibration_A); // A side
  RawStartA = unitsToRaw(readScalesWithCheck(255));

  p1 = pumping2(v1, pump1, pump1r, pump1n, pump1p);
  p2 = pumping2(v2, pump2, pump2r, pump2n, pump2p);
  p3 = pumping2(v3, pump3, pump3r, pump3n, pump3p);

  RawEndA = unitsToRaw(readScalesWithCheck(255));

  // B (4-8)
  scale.set_scale(scale_calibration_B); // B side
  RawStartB = unitsToRaw(readScalesWithCheck(255));

  p4 = pumping2(v4, pump4, pump4r, pump4n, pump4p);
  p5 = pumping2(v5, pump5, pump5r, pump5n, pump5p);
  p6 = pumping2(v6, pump6, pump6r, pump6n, pump6p);
  p7 = pumping2(v7, pump7, pump7r, pump7n, pump7p);
  p8 = pumping2(v8, pump8, pump8r, pump8n, pump8p);

  RawEndB = unitsToRaw(readScalesWithCheck(255));

  wstatus = "Ready";
  eTimes = millis() - mTimes;

  WiFiClient client;
  HTTPClient http;
  String httpstr = WegaApiUrl;
  httpstr += "?p1=" + fFTS(p1, 3);
  httpstr += "&p2=" + fFTS(p2, 3);
  httpstr += "&p3=" + fFTS(p3, 3);
  httpstr += "&p4=" + fFTS(p4, 3);
  httpstr += "&p5=" + fFTS(p5, 3);
  httpstr += "&p6=" + fFTS(p6, 3);
  httpstr += "&p7=" + fFTS(p7, 3);
  httpstr += "&p8=" + fFTS(p8, 3);

  httpstr += "&v1=" + fFTS(v1, 3);
  httpstr += "&v2=" + fFTS(v2, 3);
  httpstr += "&v3=" + fFTS(v3, 3);
  httpstr += "&v4=" + fFTS(v4, 3);
  httpstr += "&v5=" + fFTS(v5, 3);
  httpstr += "&v6=" + fFTS(v6, 3);
  httpstr += "&v7=" + fFTS(v7, 3);
  httpstr += "&v8=" + fFTS(v8, 3);

  http.begin(client, httpstr);
  http.GET();
  http.end();

  delay(10000);
  lcd.clear();
}