#define SCREEN_W 40
#define SCREEN_H 30
#define GAME_SPEED 40
#define JOYSTICK_ACCURACY 300
#define MAX_SCORE 185
#define SCORE_POINTS 10

#define VSYNC_PULSE 2
#define BACK_PORCH 33
#define VISIBLE_AREA 480

int8_t PXL_DATA[SCREEN_H][SCREEN_W];
int lineCounter;
int8_t skipLine;
bool visibleArea;
int8_t dy;
int8_t enemyX;
int8_t enemyY;
int8_t player;
int score;
int* defaultPos;

ISR(TIMER1_OVF_vect)
{
    lineCounter = 0;
}

ISR(TIMER2_OVF_vect)
{
    sei();
    __asm__("sleep \n");
}

ISR(TIMER2_COMPB_vect)
{
    int8_t columnCounter;
    int8_t* rowPtr;

    if (visibleArea)
    {
        rowPtr = &(PXL_DATA[(lineCounter - (BACK_PORCH + VSYNC_PULSE)) / (VISIBLE_AREA / SCREEN_H)][0]);
        columnCounter = SCREEN_W;

        while (columnCounter--)
        {
            PORTD = (*(rowPtr++)) << PD2;
        }
        
        PORTD = 0;
    }

    lineCounter++;
    skipLine = !skipLine;

    if (skipLine && (lineCounter > (BACK_PORCH + VSYNC_PULSE)) && (lineCounter < (BACK_PORCH + VSYNC_PULSE + VISIBLE_AREA)))
    {
        visibleArea = true;
    }
    else
    {
        visibleArea = false;
    }
}

void setup()
{
    randomSeed(analogRead(0));
    pinMode(10, OUTPUT);
    pinMode(4, OUTPUT);
    pinMode(3, OUTPUT);
    pinMode(2, OUTPUT);

    pinMode(A0, INPUT);
    pinMode(12, INPUT);

    TCCR1A = 0b00100011; // PIN D10 OUTPUT, FAST PWM MODE
    TCCR1B = 0b00011101; // FAST PWM, PRESCALER OF 1024
    TIMSK1 = 0b00000001; // OVERFLOW INTERRUPT ENABLE
    TCCR2A = 0b00100011; // PIN D3 OUTPUT, FAST PWM MODE
    TCCR2B = 0b00001010; // FAST PWM, PRESCALER OF 8
    TIMSK2 = 0b00000101; // OVERFLOW INTERRUPT ENABLE
    OCR1A = 259; // SET FREQ OF T1
    OCR2A = 63;  // SET FREQ OF T2
    OCR2B = 7;   // HSYNC PULSE DURATUIN
    SMCR = bit(SE);
    enemyX = SCREEN_W / 2 - 1;
    enemyY = 4;
}

void loop()
{
    delay(GAME_SPEED);

    auto pos = analogRead(A0) / JOYSTICK_ACCURACY;
    if (defaultPos != nullptr)
    {
        auto newPos = player + pos - *defaultPos;
        player = min(max(newPos, 0), SCREEN_W - 3);
    }
    else
    {
        defaultPos = new int(pos);
        player = SCREEN_W / 2 - 2;
    }

    bool shoot = !digitalRead(12);
    bool bulletFired;
    int8_t bulletX;
    int8_t bulletY;
  
    if (shoot)
    {
        bulletFired = true;
        dy = 0.0;
        bulletX = player;

        digitalWrite(4, HIGH);
    }
    else
    {
        digitalWrite(4, LOW);
    }

    if (bulletFired && (dy <= 27))
    {
        bulletY = 27 - dy;
        dy = dy + 1;
    }

    if ((bulletY <= 0) && (dy >= 27))
    {
        bulletFired = false;
        bulletY = 31;
    }

    uint8_t scoreLvL = score / SCREEN_W;

    if ((bulletX == (enemyX - 1)) && (bulletY == enemyY))
    {
        enemyX = random(1, 36);
        enemyY = random(scoreLvL + 3, 25);
        bulletFired = false;
        dy = 27;
        score = score + SCORE_POINTS;
        if (score >= MAX_SCORE)
        {
            score = 0;
        }
    }

    for (uint8_t y = 0; y < SCREEN_H; y++)
    {
        for (uint8_t x = 0; x < SCREEN_W; x++)
        {
            if ((y >= 28) && (y < 28 + 1) && (x >= player) && (x < player + 3))
            {
                PXL_DATA[y][x] = 0; // PLAYER BOTTOM
            }
            else if ((y >= 27) && (y < 27 + 1) && (x >= player + 1) && (x < player + 2))
            {
                PXL_DATA[y][x] = 0; // PLAYER TOP
            }
            else if ((bulletFired == 1) && (y == bulletY - 1) && (x == bulletX + 1))
            {
                PXL_DATA[y][x] = 0; // BULLET
            }
            else if ((y >= enemyY) && (y < enemyY + 1) && (x >= enemyX) && (x < enemyX + 1))
            {
                PXL_DATA[y][x] = 0; // ENEMY
            }
            else if (((y < scoreLvL) && (x < score)) || ((y <= scoreLvL) && (x < score % SCREEN_W)))
            {
                PXL_DATA[y][x] = 0; // SCORE
            }
            else
            {
                PXL_DATA[y][x] = 1; // BACKGROUND
            }
        }
    }
}
