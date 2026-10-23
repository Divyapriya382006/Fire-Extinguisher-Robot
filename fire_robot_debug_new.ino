// ===== Fire Robot: noise-proof version (voltage guard + stopped verification + spray lockout) =====
const int PWMA = 5, AIN1 = 7, AIN2 = 8;
const int PWMB = 6, BIN1 = 9, BIN2 = 10;
const int STBY = 4;
const int FLAME_L = 2, FLAME_C = 3, FLAME_R = 11;
const int TRIG = 12, ECHO = 13;
const int BUZZER = A4, PUMP = A0;

// ---- patrol settings ----
const int  PATROL_PATTERN    = 2;      // 0 = squares, 1 = circles, 2 = alternate each run
const long PATROL_RUN_MS     = 15000;
const long REST_MS           = 5000;
const int  SQUARE_SIDE_SEGS  = 3;
const int  SQUARE_TURN_MS    = 380;
const int  CIRCLE_INNER      = 110;

// ---- speed / timing ----
const int  SPEED         = 255;
const int  TURN          = 255;
const int  SEG_MS        = 450;
const int  SETTLE_MS     = 150;        // wait after every move before reading flames (was 40)
const int  KICK_MS       = 30;
const int  PING_GAP_MS   = 20;

// ---- noise protection ----
const int  VCC_MIN_MV    = 4600;       // ignore flame readings while the 5V rail is below this
const int  VERIFY_SETTLE = 250;        // stopped wait before verifying a flame
const int  VERIFY_READS  = 3;          // fire must show on all of these reads
const int  VERIFY_GAP_MS = 80;
const long VERIFIED_HOLD = 4000;       // a verified flame is trusted this long while chasing
const long SPRAY_LOCKOUT = 4000;       // minimum time between sprays

// ---- obstacle ----
const int  OBSTACLE_CM   = 20;
const int  HAND_CM       = 15;
const int  BACK_CM       = 30;
const int  MS_PER_CM     = 25;
const int  AVOID_TURN_MS = 450;
const int  AVOID_GROW_MS = 200;
const int  AVOID_MAX_MS  = 1100;

// ---- fire ----
const int  FLAME_ACTIVE  = LOW;
const bool SWAP_SIDES    = false;
const int  TURN_MIN_MS   = 120;
const int  TURN_GROW_MS  = 50;
const int  TURN_MAX_MS   = 450;
const int  SPRAY_MS      = 2500;
const int  SIDE_TIMEOUT  = 14;
const int  SEARCH_PULSES = 8;
const bool LEFT_INVERT   = true;
const bool RIGHT_INVERT  = true;
const bool DEBUG         = true;

int  sidePulses = 0, lastDir = 0, searchLeft = 0, avoidStreak = 0;
int  pulseMs = TURN_MIN_MS;
bool turnRightNext = true;
bool resting = false;
unsigned long patrolStart = 0, restStart = 0, lastPrint = 0, verifiedUntil = 0, lastSpray = 0;
int  patternNow = 0;
int  squareSegsDone = 0;

long readVcc() {                       // Arduino 5V rail in mV
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(3);
  ADCSRA |= _BV(ADSC);
  while (bit_is_set(ADCSRA, ADSC));
  return 1125300L / ADC;
}

void setMotor(int in1, int in2, int pwm, int speed, bool invert) {
  if (invert) speed = -speed;
  if (speed > 0)      { digitalWrite(in1, HIGH); digitalWrite(in2, LOW);  analogWrite(pwm, speed); }
  else if (speed < 0) { digitalWrite(in1, LOW);  digitalWrite(in2, HIGH); analogWrite(pwm, -speed); }
  else                { digitalWrite(in1, LOW);  digitalWrite(in2, LOW);  analogWrite(pwm, 0); }
}
void drive(int l, int r) {
  setMotor(AIN1, AIN2, PWMA, l, LEFT_INVERT);
  setMotor(BIN1, BIN2, PWMB, r, RIGHT_INVERT);
}
void stopMotors() { drive(0, 0); }
int  sgn(int v) { return v > 0 ? 1 : (v < 0 ? -1 : 0); }

long readOnce() {
  digitalWrite(TRIG, LOW);  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  long d = pulseIn(ECHO, HIGH, 15000);
  return d == 0 ? 999 : d / 58;
}
long getDistance() {
  long a = readOnce();
  if (a >= OBSTACLE_CM + 10) return (a >= 999) ? -1 : a;
  long b = readOnce(), c = readOnce();
  long med = max(min(a, b), min(max(a, b), c));
  return (med >= 999) ? -1 : med;
}

bool runFor(int l, int r, int ms, bool watch) {
  bool straightish = (l == r) || (l == -r);
  if (straightish && l != 0) { drive(255 * sgn(l), 255 * sgn(r)); delay(KICK_MS); }
  drive(l, r);
  unsigned long t0 = millis();
  int hits = 0;
  while (millis() - t0 < (unsigned long)ms) {
    if (watch) {
      long d = readOnce();
      if (d > 0 && d < OBSTACLE_CM) { if (++hits >= 2) { stopMotors(); return true; } }
      else hits = 0;
    }
    delay(PING_GAP_MS);
  }
  stopMotors();
  delay(SETTLE_MS);
  return false;
}
void pivot(int dir, int ms) { runFor(dir * TURN, -dir * TURN, ms, false); }

// reads the 3 flame sensors; returns all-false while the supply is sagging
void readFlames(bool &L, bool &C, bool &R) {
  L = C = R = false;
  if (readVcc() < VCC_MIN_MV) {        // give the supply a moment to recover, then check again
    delay(150);
    if (readVcc() < VCC_MIN_MV) return;
  }
  int nl = 0, nc = 0, nr = 0;
  for (int i = 0; i < 6; i++) {
    if (digitalRead(FLAME_L) == FLAME_ACTIVE) nl++;
    if (digitalRead(FLAME_C) == FLAME_ACTIVE) nc++;
    if (digitalRead(FLAME_R) == FLAME_ACTIVE) nr++;
    delay(1);
  }
  L = nl >= 5; C = nc >= 5; R = nr >= 5;
  if (SWAP_SIDES) { bool t = L; L = R; R = t; }
}

// stop, wait, then fire must show on every read, or it is noise
bool verifyFire(bool &L, bool &C, bool &R) {
  stopMotors();
  delay(VERIFY_SETTLE);
  for (int i = 0; i < VERIFY_READS; i++) {
    readFlames(L, C, R);
    if (!(L || C || R)) return false;
    delay(VERIFY_GAP_MS);
  }
  return true;
}

void extinguish() {
  stopMotors();
  if (DEBUG) Serial.println(">>> SPRAYING");
  digitalWrite(BUZZER, HIGH);
  digitalWrite(PUMP, HIGH);
  delay(SPRAY_MS);
  digitalWrite(PUMP, LOW);
  digitalWrite(BUZZER, LOW);
  lastSpray = millis();
  verifiedUntil = 0;                   // after a spray, the flame must be verified again
  delay(600);                          // let the supply recover
}

void avoidObstacle() {
  digitalWrite(BUZZER, LOW);
  avoidStreak++;
  int turnMs = min(AVOID_TURN_MS + (avoidStreak - 1) * AVOID_GROW_MS, AVOID_MAX_MS);
  int dir = turnRightNext ? +1 : -1;
  if (DEBUG) {
    Serial.print(">>> AVOID #"); Serial.print(avoidStreak);
    Serial.print(" turn "); Serial.print(dir > 0 ? "RIGHT " : "LEFT "); Serial.print(turnMs); Serial.println("ms");
  }
  runFor(-SPEED, -SPEED, BACK_CM * MS_PER_CM, false);
  pivot(dir, turnMs);
  turnRightNext = !turnRightNext;
  squareSegsDone = 0;
}

void setup() {
  pinMode(PUMP, OUTPUT);   digitalWrite(PUMP, LOW);
  pinMode(BUZZER, OUTPUT); digitalWrite(BUZZER, LOW);
  Serial.begin(9600);
  pinMode(STBY, OUTPUT); digitalWrite(STBY, HIGH);
  int outs[] = {PWMA, AIN1, AIN2, PWMB, BIN1, BIN2, TRIG};
  for (int i = 0; i < 7; i++) { pinMode(outs[i], OUTPUT); digitalWrite(outs[i], LOW); }
  pinMode(FLAME_L, INPUT); pinMode(FLAME_C, INPUT); pinMode(FLAME_R, INPUT);
  pinMode(ECHO, INPUT);
  patternNow = (PATROL_PATTERN == 1) ? 1 : 0;
  Serial.print("Fire robot ready (noise-proof), Vcc="); Serial.print(readVcc()); Serial.println("mV");
  delay(1500);
  patrolStart = millis();
}

void loop() {
  bool L, C, R;
  readFlames(L, C, R);                 // motors are stopped and settled

  // ---------- FIRE SEEN ----------
  if (L || C || R) {
    if (millis() > verifiedUntil) {    // not yet trusted: verify with the motors stopped
      if (!verifyFire(L, C, R)) {
        if (DEBUG) { Serial.print("FALSE ALARM (noise), Vcc="); Serial.println(readVcc()); }
        return;
      }
      verifiedUntil = millis() + VERIFIED_HOLD;
      if (DEBUG) Serial.println("FIRE VERIFIED");
    }
    resting = false; patrolStart = millis();
    avoidStreak = 0;
    searchLeft = SEARCH_PULSES;
    digitalWrite(BUZZER, HIGH);        // beep only for a verified flame

    if (DEBUG) {
      Serial.print("FIRE "); Serial.print(L); Serial.print(C); Serial.print(R);
      Serial.print(" pulseMs="); Serial.println(pulseMs);
    }

    if (C) {                           // aimed: spray (with lockout)
      lastDir = 0; sidePulses = 0; pulseMs = TURN_MIN_MS;
      if (millis() - lastSpray > (unsigned long)SPRAY_LOCKOUT) extinguish();
      else delay(100);
      return;
    }
    if (L && R) {                      // both sides, not center: close in a little
      lastDir = 0; pulseMs = TURN_MIN_MS;
      runFor(SPEED, SPEED, SEG_MS / 2, false);
      return;
    }
    int dir = L ? -1 : +1;             // one side: pivot toward it
    if (dir != lastDir) pulseMs = TURN_MIN_MS;
    else                pulseMs = min(pulseMs + TURN_GROW_MS, TURN_MAX_MS);
    lastDir = dir;
    sidePulses++;
    if (sidePulses >= SIDE_TIMEOUT) {
      if (DEBUG) Serial.println("!!! center never lit, spraying from the side");
      if (millis() - lastSpray > (unsigned long)SPRAY_LOCKOUT) extinguish();
      sidePulses = 0; pulseMs = TURN_MIN_MS;
      return;
    }
    pivot(dir, pulseMs);
    return;
  }

  // ---------- NO FIRE ----------
  sidePulses = 0;
  digitalWrite(BUZZER, LOW);
  digitalWrite(PUMP, LOW);

  if (searchLeft > 0) {                // just lost it: pivot where it was
    searchLeft--;
    if (lastDir != 0) pivot(lastDir, TURN_MIN_MS * 2);
    else              delay(60);
    return;
  }
  pulseMs = TURN_MIN_MS;

  // ---------- REST BREAK ----------
  unsigned long now = millis();
  if (resting) {
    if (now - restStart >= (unsigned long)REST_MS) {
      resting = false; patrolStart = now; squareSegsDone = 0;
      if (DEBUG) Serial.println("--- rest over, patrolling again");
    } else {
      if (DEBUG && now - lastPrint > 1000) {
        lastPrint = now;
        Serial.print("RESTING, "); Serial.print((REST_MS - (now - restStart)) / 1000); Serial.println("s left");
      }
      long hd = getDistance();
      if (hd > 0 && hd < HAND_CM) { avoidObstacle(); return; }
      delay(30);
      return;
    }
  } else if (now - patrolStart >= (unsigned long)PATROL_RUN_MS) {
    resting = true; restStart = now;
    stopMotors();
    if (PATROL_PATTERN == 2) patternNow = 1 - patternNow;
    if (DEBUG) Serial.println("--- REST: standing still");
    return;
  }

  // ---------- PATROL ----------
  if (DEBUG && now - lastPrint > 800) {
    lastPrint = now;
    Serial.print(patternNow == 0 ? "PATROL square" : "PATROL circle");
    Serial.print("  Vcc="); Serial.println(readVcc());
  }
  bool hit;
  if (patternNow == 0) {
    hit = runFor(SPEED, SPEED, SEG_MS, true);
    if (!hit && ++squareSegsDone >= SQUARE_SIDE_SEGS) {
      pivot(+1, SQUARE_TURN_MS);
      squareSegsDone = 0;
    }
  } else {
    hit = runFor(SPEED, CIRCLE_INNER, SEG_MS, true);
  }
  if (hit) avoidObstacle();
  else     avoidStreak = 0;
}